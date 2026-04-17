#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#define SAMPLE_PERIOD_MS 100
#define REPORT_PERIOD_MS 1000
#define BLE_PERIOD_MS 1000
#define LED_BLINK_MS 50

#define AVERAGE_WINDOW_SECONDS 60
#define WARNING_THRESHOLD_DEFAULT_CENTI 2800
#define WARNING_THRESHOLD_STEP_CENTI 50
#define WARNING_THRESHOLD_MIN_CENTI 1500
#define WARNING_THRESHOLD_MAX_CENTI 4500

#define SENSOR_MIN_VALID_MV 2000
#define SENSOR_MAX_VALID_MV 3300

#define DRIFT_THRESHOLD_CENTI 200
#define DRIFT_CLEAR_THRESHOLD_CENTI 150
#define DRIFT_CONTROLLED_VARIANCE_CENTI2 900
#define DRIFT_REQUIRED_MINUTES 1440
#define DRIFT_DEMO_REQUIRED_MINUTES 3

#define SAMPLE_QUEUE_LEN 16
#define CALIBRATION_QUEUE_LEN 8

#define ADC_THREAD_STACK_SIZE 1536
#define LOGIC_THREAD_STACK_SIZE 2560
#define REPORT_THREAD_STACK_SIZE 2048
#define COMM_THREAD_STACK_SIZE 2048
#define CAL_THREAD_STACK_SIZE 1536

#define ADC_THREAD_PRIORITY 0
#define LOGIC_THREAD_PRIORITY 1
#define CAL_THREAD_PRIORITY 2
#define REPORT_THREAD_PRIORITY 3
#define COMM_THREAD_PRIORITY 3

#define WINDOW_SAMPLE_COUNT ((AVERAGE_WINDOW_SECONDS * 1000) / SAMPLE_PERIOD_MS)
#define JITTER_REPORT_INTERVALS WINDOW_SAMPLE_COUNT
#define JITTER_BAD_TOLERANCE_MS 2

#define LED0_NODE DT_ALIAS(led0)
#define BUTTON0_NODE DT_ALIAS(sw0)
#define BUTTON1_NODE DT_ALIAS(sw1)
#define BUTTON2_NODE DT_ALIAS(sw2)
#define BUTTON3_NODE DT_ALIAS(sw3)

#if !DT_NODE_HAS_STATUS(LED0_NODE, okay)
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

#if !DT_NODE_HAS_STATUS(BUTTON0_NODE, okay) || !DT_NODE_HAS_STATUS(BUTTON1_NODE, okay) || \
	!DT_NODE_HAS_STATUS(BUTTON2_NODE, okay) || !DT_NODE_HAS_STATUS(BUTTON3_NODE, okay)
#error "Unsupported board: sw0-sw3 devicetree aliases are required"
#endif

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
#define COMPANY_ID 0x0059

enum system_state {
	SYSTEM_STATE_NORMAL = 0,
	SYSTEM_STATE_WARNING = 1,
	SYSTEM_STATE_FAULT = 2,
	SYSTEM_STATE_DRIFT = 3,
};

enum calibration_command {
	CAL_CMD_THRESHOLD_DOWN = 0,
	CAL_CMD_THRESHOLD_UP = 1,
	CAL_CMD_THRESHOLD_RESET = 2,
	CAL_CMD_TOGGLE_DRIFT_DEMO = 3,
};

struct sensor_sample {
	int16_t raw_adc;
	int32_t millivolts;
	int16_t temp_centi;
	int64_t timestamp_ms;
	uint32_t sequence;
	bool valid;
};

struct system_snapshot {
	struct sensor_sample latest;
	int16_t average_temp_centi;
	int16_t threshold_centi;
	int16_t drift_baseline_centi;
	int16_t last_minute_avg_centi;
	uint16_t samples_in_window;
	uint16_t drift_controlled_minutes;
	uint16_t drift_required_minutes;
	enum system_state state;
	bool led_asserted;
	bool drift_demo_mode;
	bool drift_baseline_ready;
	bool controlled_environment;
	uint32_t adc_error_count;
	uint32_t queue_overrun_count;
};

struct adv_payload {
	uint16_t company_id;
	int16_t average_temp_centi_be;
	uint8_t state;
} __packed;

struct welford_stats {
	uint32_t count;
	int64_t mean_q16;
	int64_t m2_q16;
};

struct jitter_summary {
	uint32_t sequence;
	uint32_t intervals;
	uint32_t bad_intervals;
	int64_t min_delta_ms;
	int64_t max_delta_ms;
	int64_t total_delta_ms;
};

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec buttons[] = {
	GPIO_DT_SPEC_GET(BUTTON0_NODE, gpios),
	GPIO_DT_SPEC_GET(BUTTON1_NODE, gpios),
	GPIO_DT_SPEC_GET(BUTTON2_NODE, gpios),
	GPIO_DT_SPEC_GET(BUTTON3_NODE, gpios),
};
static const struct adc_dt_spec adc_channel =
	ADC_DT_SPEC_GET(DT_PATH(zephyr_user));

static struct adv_payload adv_payload = {
	.company_id = COMPANY_ID,
	.average_temp_centi_be = 0,
	.state = SYSTEM_STATE_NORMAL,
};

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
	BT_DATA(BT_DATA_MANUFACTURER_DATA, (uint8_t *)&adv_payload,
		sizeof(adv_payload)),
};

#define BT_ADV_INTERVAL 0x00A0
static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
	BT_LE_ADV_OPT_NONE,
	BT_ADV_INTERVAL,
	BT_ADV_INTERVAL,
	NULL
);

K_MSGQ_DEFINE(sample_msgq, sizeof(struct sensor_sample), SAMPLE_QUEUE_LEN, 4);
K_MSGQ_DEFINE(calibration_msgq, sizeof(uint8_t), CALIBRATION_QUEUE_LEN, 1);
K_SEM_DEFINE(sample_tick_sem, 0, 32);

static K_MUTEX_DEFINE(snapshot_lock);
static K_MUTEX_DEFINE(jitter_lock);

static struct k_timer sample_timer;
static struct k_timer led_off_timer;

static struct gpio_callback button_callbacks[ARRAY_SIZE(buttons)];

static int16_t adc_buf;
static bool adc_setup_done;
static bool ble_ready;
static bool advertising_active;

static atomic_t adc_error_count;
static atomic_t queue_overrun_count;
static atomic_t warning_threshold_centi;
static atomic_t drift_demo_mode;
static atomic_t drift_reset_requested;

static uint32_t sample_sequence;

static int16_t temperature_window[WINDOW_SAMPLE_COUNT];
static int32_t window_sum;
static uint16_t window_count;
static uint16_t window_head;

static struct welford_stats minute_stats;
static struct welford_stats drift_baseline_builder;
static uint16_t minute_slot_count;
static uint16_t minute_valid_count;
static uint16_t drift_controlled_minutes;
static int16_t drift_baseline_centi;
static int16_t last_minute_avg_centi;
static bool drift_baseline_ready;
static bool drift_active;
static bool controlled_environment;

static struct system_snapshot latest_snapshot;
static struct jitter_summary latest_jitter_summary;

static int64_t jitter_last_tick_ms;
static int64_t jitter_min_delta_ms;
static int64_t jitter_max_delta_ms;
static int64_t jitter_total_delta_ms;
static uint32_t jitter_interval_count;
static uint32_t jitter_bad_interval_count;
static uint32_t jitter_summary_sequence;
static bool jitter_last_tick_valid;

static K_THREAD_STACK_DEFINE(adc_thread_stack, ADC_THREAD_STACK_SIZE);
static K_THREAD_STACK_DEFINE(logic_thread_stack, LOGIC_THREAD_STACK_SIZE);
static K_THREAD_STACK_DEFINE(report_thread_stack, REPORT_THREAD_STACK_SIZE);
static K_THREAD_STACK_DEFINE(comm_thread_stack, COMM_THREAD_STACK_SIZE);
static K_THREAD_STACK_DEFINE(cal_thread_stack, CAL_THREAD_STACK_SIZE);

static struct k_thread adc_thread_data;
static struct k_thread logic_thread_data;
static struct k_thread report_thread_data;
static struct k_thread comm_thread_data;
static struct k_thread cal_thread_data;

static int32_t abs32(int32_t value)
{
	return (value < 0) ? -value : value;
}

static void welford_reset(struct welford_stats *stats)
{
	stats->count = 0U;
	stats->mean_q16 = 0;
	stats->m2_q16 = 0;
}

static void welford_update(struct welford_stats *stats, int32_t sample_centi)
{
	int64_t x_q16 = ((int64_t)sample_centi) << 16;

	stats->count++;
	if (stats->count == 1U) {
		stats->mean_q16 = x_q16;
		stats->m2_q16 = 0;
		return;
	}

	int64_t delta_q16 = x_q16 - stats->mean_q16;

	stats->mean_q16 += delta_q16 / (int64_t)stats->count;

	int64_t delta2_q16 = x_q16 - stats->mean_q16;

	stats->m2_q16 += (delta_q16 * delta2_q16) >> 16;
}

static int32_t welford_mean_centi(const struct welford_stats *stats)
{
	if (stats->count == 0U) {
		return 0;
	}

	return (int32_t)(stats->mean_q16 >> 16);
}

static int32_t welford_variance_centi2(const struct welford_stats *stats)
{
	if (stats->count < 2U) {
		return 0;
	}

	return (int32_t)((stats->m2_q16 / (int64_t)(stats->count - 1U)) >> 16);
}

static uint16_t get_drift_required_minutes(void)
{
	return atomic_get(&drift_demo_mode) ? DRIFT_DEMO_REQUIRED_MINUTES :
					      DRIFT_REQUIRED_MINUTES;
}

static void reset_drift_tracking(void)
{
	welford_reset(&minute_stats);
	welford_reset(&drift_baseline_builder);
	minute_slot_count = 0U;
	minute_valid_count = 0U;
	drift_controlled_minutes = 0U;
	drift_baseline_centi = 0;
	last_minute_avg_centi = 0;
	drift_baseline_ready = false;
	drift_active = false;
	controlled_environment = false;
}

static const char *state_to_string(enum system_state state)
{
	switch (state) {
	case SYSTEM_STATE_NORMAL:
		return "NORMAL";
	case SYSTEM_STATE_WARNING:
		return "WARNING";
	case SYSTEM_STATE_FAULT:
		return "FAULT";
	case SYSTEM_STATE_DRIFT:
		return "DRIFT";
	default:
		return "UNKNOWN";
	}
}

static const char *led_state_to_string(enum system_state state)
{
	switch (state) {
	case SYSTEM_STATE_FAULT:
		return "SOLID";
	case SYSTEM_STATE_WARNING:
	case SYSTEM_STATE_DRIFT:
		return "BLINKING";
	case SYSTEM_STATE_NORMAL:
	default:
		return "OFF";
	}
}

static int init_led(void)
{
	if (!gpio_is_ready_dt(&led0)) {
		printk("LED device is not ready\n");
		return -ENODEV;
	}

	return gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
}

static void enqueue_calibration_command(uint8_t command)
{
	int err = k_msgq_put(&calibration_msgq, &command, K_NO_WAIT);

	if (err < 0) {
		atomic_inc(&queue_overrun_count);
	}
}

static void button0_pressed(const struct device *dev, struct gpio_callback *cb,
			    uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
	enqueue_calibration_command(CAL_CMD_THRESHOLD_DOWN);
}

static void button1_pressed(const struct device *dev, struct gpio_callback *cb,
			    uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
	enqueue_calibration_command(CAL_CMD_THRESHOLD_UP);
}

static void button2_pressed(const struct device *dev, struct gpio_callback *cb,
			    uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
	enqueue_calibration_command(CAL_CMD_THRESHOLD_RESET);
}

static void button3_pressed(const struct device *dev, struct gpio_callback *cb,
			    uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
	enqueue_calibration_command(CAL_CMD_TOGGLE_DRIFT_DEMO);
}

typedef void (*button_handler_t)(const struct device *dev,
				 struct gpio_callback *cb, uint32_t pins);

static const button_handler_t button_handlers[] = {
	button0_pressed,
	button1_pressed,
	button2_pressed,
	button3_pressed,
};

static int init_buttons(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(buttons); i++) {
		int err;
		gpio_port_pins_t pin_mask = BIT(buttons[i].pin);

		if (!gpio_is_ready_dt(&buttons[i])) {
			printk("Button %d device is not ready\n", (int)i);
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&buttons[i], GPIO_INPUT);
		if (err < 0) {
			printk("Button %d configure failed (err=%d)\n", (int)i, err);
			return err;
		}

		err = gpio_pin_interrupt_configure_dt(&buttons[i], GPIO_INT_EDGE_TO_ACTIVE);
		if (err < 0) {
			printk("Button %d interrupt configure failed (err=%d)\n", (int)i, err);
			return err;
		}

		gpio_init_callback(&button_callbacks[i], button_handlers[i], pin_mask);
		err = gpio_add_callback(buttons[i].port, &button_callbacks[i]);
		if (err < 0) {
			printk("Button %d callback add failed (err=%d)\n", (int)i, err);
			return err;
		}
	}

	return 0;
}

static int init_adc_once(void)
{
	int err;

	if (!adc_is_ready_dt(&adc_channel)) {
		printk("ADC %s is not ready\n", adc_channel.dev->name);
		return -ENODEV;
	}

	if (adc_setup_done) {
		return 0;
	}

	err = adc_channel_setup_dt(&adc_channel);
	if (err < 0) {
		printk("ADC channel setup failed (err=%d)\n", err);
		return err;
	}

	adc_setup_done = true;
	return 0;
}

static void sample_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	k_sem_give(&sample_tick_sem);
}

static void led_off_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	(void)gpio_pin_set_dt(&led0, 0);

	k_mutex_lock(&snapshot_lock, K_FOREVER);
	if (latest_snapshot.state != SYSTEM_STATE_FAULT) {
		latest_snapshot.led_asserted = false;
	}
	k_mutex_unlock(&snapshot_lock);
}

static int acquire_sensor_sample(struct sensor_sample *sample)
{
	int err;
	struct adc_sequence sequence = {
		.buffer = &adc_buf,
		.buffer_size = sizeof(adc_buf),
	};
	int32_t mv;

	if (sample == NULL) {
		return -EINVAL;
	}

	err = init_adc_once();
	if (err < 0) {
		return err;
	}

	err = adc_sequence_init_dt(&adc_channel, &sequence);
	if (err < 0) {
		printk("ADC sequence init failed (err=%d)\n", err);
		return err;
	}

	err = adc_read(adc_channel.dev, &sequence);
	if (err < 0) {
		printk("ADC read failed (err=%d)\n", err);
		return err;
	}

	mv = adc_buf;
	err = adc_raw_to_millivolts_dt(&adc_channel, &mv);
	if (err < 0) {
		printk("ADC mv conversion failed (err=%d)\n", err);
		return err;
	}

	sample->raw_adc = adc_buf;
	sample->millivolts = mv;
	sample->temp_centi = (int16_t)(mv * 10 - 27315);
	sample->timestamp_ms = k_uptime_get();
	sample->sequence = sample_sequence++;
	sample->valid = (mv >= SENSOR_MIN_VALID_MV) && (mv <= SENSOR_MAX_VALID_MV);
	if (!sample->valid) {
		atomic_inc(&adc_error_count);
	}

	return 0;
}

static void set_fault_led(void)
{
	k_timer_stop(&led_off_timer);
	(void)gpio_pin_set_dt(&led0, 1);
}

static void pulse_alert_led(void)
{
	(void)gpio_pin_set_dt(&led0, 1);
	k_timer_start(&led_off_timer, K_MSEC(LED_BLINK_MS), K_NO_WAIT);
}

static void update_shared_snapshot(const struct system_snapshot *snapshot)
{
	k_mutex_lock(&snapshot_lock, K_FOREVER);
	latest_snapshot = *snapshot;
	k_mutex_unlock(&snapshot_lock);
}

static int16_t update_average_window(int16_t latest_temp_centi,
				     uint16_t *samples_in_window)
{
	if (window_count < WINDOW_SAMPLE_COUNT) {
		temperature_window[window_head] = latest_temp_centi;
		window_sum += latest_temp_centi;
		window_count++;
	} else {
		window_sum -= temperature_window[window_head];
		temperature_window[window_head] = latest_temp_centi;
		window_sum += latest_temp_centi;
	}

	window_head = (window_head + 1U) % WINDOW_SAMPLE_COUNT;
	*samples_in_window = window_count;

	return (int16_t)(window_sum / window_count);
}

static void reset_jitter_window(void)
{
	jitter_min_delta_ms = 0;
	jitter_max_delta_ms = 0;
	jitter_total_delta_ms = 0;
	jitter_interval_count = 0U;
	jitter_bad_interval_count = 0U;
}

static void update_jitter_stats(int64_t tick_ms)
{
	int64_t delta_ms;

	if (!jitter_last_tick_valid) {
		jitter_last_tick_ms = tick_ms;
		jitter_last_tick_valid = true;
		return;
	}

	delta_ms = tick_ms - jitter_last_tick_ms;
	jitter_last_tick_ms = tick_ms;

	if (jitter_interval_count == 0U) {
		jitter_min_delta_ms = delta_ms;
		jitter_max_delta_ms = delta_ms;
	} else {
		if (delta_ms < jitter_min_delta_ms) {
			jitter_min_delta_ms = delta_ms;
		}
		if (delta_ms > jitter_max_delta_ms) {
			jitter_max_delta_ms = delta_ms;
		}
	}

	jitter_total_delta_ms += delta_ms;
	jitter_interval_count++;

	if (delta_ms < (SAMPLE_PERIOD_MS - JITTER_BAD_TOLERANCE_MS) ||
	    delta_ms > (SAMPLE_PERIOD_MS + JITTER_BAD_TOLERANCE_MS)) {
		jitter_bad_interval_count++;
	}

	if (jitter_interval_count >= JITTER_REPORT_INTERVALS) {
		struct jitter_summary summary = {
			.sequence = ++jitter_summary_sequence,
			.intervals = jitter_interval_count,
			.bad_intervals = jitter_bad_interval_count,
			.min_delta_ms = jitter_min_delta_ms,
			.max_delta_ms = jitter_max_delta_ms,
			.total_delta_ms = jitter_total_delta_ms,
		};

		k_mutex_lock(&jitter_lock, K_FOREVER);
		latest_jitter_summary = summary;
		k_mutex_unlock(&jitter_lock);

		reset_jitter_window();
	}
}

static bool get_new_jitter_summary(uint32_t *last_sequence,
				   struct jitter_summary *summary)
{
	bool has_new_summary = false;

	k_mutex_lock(&jitter_lock, K_FOREVER);
	if (latest_jitter_summary.sequence != 0U &&
	    latest_jitter_summary.sequence != *last_sequence) {
		*summary = latest_jitter_summary;
		*last_sequence = latest_jitter_summary.sequence;
		has_new_summary = true;
	}
	k_mutex_unlock(&jitter_lock);

	return has_new_summary;
}

static void finalize_minute_tracking(void)
{
	int32_t minute_avg_centi = 0;
	int32_t minute_variance_centi2 = 0;
	uint16_t required_minutes = get_drift_required_minutes();

	if (minute_valid_count == WINDOW_SAMPLE_COUNT && minute_stats.count > 0U) {
		minute_avg_centi = welford_mean_centi(&minute_stats);
		minute_variance_centi2 = welford_variance_centi2(&minute_stats);
		last_minute_avg_centi = (int16_t)minute_avg_centi;
		controlled_environment =
			(minute_variance_centi2 <= DRIFT_CONTROLLED_VARIANCE_CENTI2);
	} else {
		last_minute_avg_centi = 0;
		controlled_environment = false;
	}

	if (controlled_environment) {
		if (!drift_baseline_ready) {
			welford_update(&drift_baseline_builder, minute_avg_centi);
			drift_controlled_minutes++;

			if (drift_controlled_minutes >= required_minutes) {
				drift_baseline_centi =
					(int16_t)welford_mean_centi(&drift_baseline_builder);
				drift_baseline_ready = true;
				printk("Drift baseline locked at %d.%02d C after %u controlled minute(s) [%s]\n",
				       drift_baseline_centi / 100,
				       abs32(drift_baseline_centi % 100),
				       drift_controlled_minutes,
				       atomic_get(&drift_demo_mode) ? "demo" : "24h");
			}
		} else {
			int32_t drift_delta_centi =
				abs32(minute_avg_centi - drift_baseline_centi);

			if (drift_delta_centi >= DRIFT_THRESHOLD_CENTI) {
				drift_active = true;
			} else if (drift_delta_centi <= DRIFT_CLEAR_THRESHOLD_CENTI) {
				drift_active = false;
			}
		}
	}

	minute_slot_count = 0U;
	minute_valid_count = 0U;
	welford_reset(&minute_stats);
}

static void adc_thread_entry(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (1) {
		struct sensor_sample sample;
		int64_t tick_ms;
		int err;

		k_sem_take(&sample_tick_sem, K_FOREVER);
		tick_ms = k_uptime_get();
		update_jitter_stats(tick_ms);

		err = acquire_sensor_sample(&sample);
		if (err < 0) {
			atomic_inc(&adc_error_count);
			sample.raw_adc = 0;
			sample.millivolts = 0;
			sample.temp_centi = 0;
			sample.timestamp_ms = k_uptime_get();
			sample.sequence = sample_sequence++;
			sample.valid = false;
		}

		err = k_msgq_put(&sample_msgq, &sample, K_NO_WAIT);
		if (err < 0) {
			struct sensor_sample dropped;

			if (k_msgq_get(&sample_msgq, &dropped, K_NO_WAIT) == 0 &&
			    k_msgq_put(&sample_msgq, &sample, K_NO_WAIT) == 0) {
				atomic_inc(&queue_overrun_count);
			}
		}
	}
}

static void logic_thread_entry(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	reset_drift_tracking();

	while (1) {
		struct sensor_sample sample;
		struct system_snapshot snapshot;

		k_msgq_get(&sample_msgq, &sample, K_FOREVER);

		if (atomic_cas(&drift_reset_requested, 1, 0)) {
			reset_drift_tracking();
			printk("Drift tracking reset [%s]\n",
			       atomic_get(&drift_demo_mode) ? "demo" : "24h");
		}

		snapshot.latest = sample;
		snapshot.adc_error_count = (uint32_t)atomic_get(&adc_error_count);
		snapshot.queue_overrun_count = (uint32_t)atomic_get(&queue_overrun_count);
		snapshot.threshold_centi = (int16_t)atomic_get(&warning_threshold_centi);

		minute_slot_count++;
		if (sample.valid) {
			minute_valid_count++;
			welford_update(&minute_stats, sample.temp_centi);
		}
		if (minute_slot_count >= WINDOW_SAMPLE_COUNT) {
			finalize_minute_tracking();
		}

		if (!sample.valid) {
			snapshot.average_temp_centi = 0;
			snapshot.samples_in_window = window_count;
			snapshot.state = SYSTEM_STATE_FAULT;
			snapshot.led_asserted = true;
			set_fault_led();
		} else {
			snapshot.average_temp_centi =
				update_average_window(sample.temp_centi, &snapshot.samples_in_window);

			if (drift_baseline_ready && drift_active) {
				snapshot.state = SYSTEM_STATE_DRIFT;
			} else if (snapshot.average_temp_centi > snapshot.threshold_centi) {
				snapshot.state = SYSTEM_STATE_WARNING;
			} else {
				snapshot.state = SYSTEM_STATE_NORMAL;
			}

			if (snapshot.state == SYSTEM_STATE_WARNING ||
			    snapshot.state == SYSTEM_STATE_DRIFT) {
				snapshot.led_asserted = true;
				pulse_alert_led();
			} else {
				k_timer_stop(&led_off_timer);
				(void)gpio_pin_set_dt(&led0, 0);
				snapshot.led_asserted = false;
			}
		}

		snapshot.drift_demo_mode = atomic_get(&drift_demo_mode);
		snapshot.drift_baseline_ready = drift_baseline_ready;
		snapshot.drift_baseline_centi = drift_baseline_centi;
		snapshot.last_minute_avg_centi = last_minute_avg_centi;
		snapshot.drift_controlled_minutes = drift_controlled_minutes;
		snapshot.drift_required_minutes = get_drift_required_minutes();
		snapshot.controlled_environment = controlled_environment;

		update_shared_snapshot(&snapshot);
	}
}

static void calibration_thread_entry(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (1) {
		uint8_t command;
		int32_t threshold;

		k_msgq_get(&calibration_msgq, &command, K_FOREVER);

		threshold = atomic_get(&warning_threshold_centi);
		switch ((enum calibration_command)command) {
		case CAL_CMD_THRESHOLD_DOWN:
			threshold -= WARNING_THRESHOLD_STEP_CENTI;
			if (threshold < WARNING_THRESHOLD_MIN_CENTI) {
				threshold = WARNING_THRESHOLD_MIN_CENTI;
			}
			atomic_set(&warning_threshold_centi, threshold);
			printk("Calibration: threshold -> %d.%02d C\n",
			       threshold / 100, abs32(threshold % 100));
			break;
		case CAL_CMD_THRESHOLD_UP:
			threshold += WARNING_THRESHOLD_STEP_CENTI;
			if (threshold > WARNING_THRESHOLD_MAX_CENTI) {
				threshold = WARNING_THRESHOLD_MAX_CENTI;
			}
			atomic_set(&warning_threshold_centi, threshold);
			printk("Calibration: threshold -> %d.%02d C\n",
			       threshold / 100, abs32(threshold % 100));
			break;
		case CAL_CMD_THRESHOLD_RESET:
			atomic_set(&warning_threshold_centi,
				   WARNING_THRESHOLD_DEFAULT_CENTI);
			atomic_set(&drift_reset_requested, 1);
			printk("Calibration: threshold reset to %d.%02d C\n",
			       WARNING_THRESHOLD_DEFAULT_CENTI / 100,
			       WARNING_THRESHOLD_DEFAULT_CENTI % 100);
			break;
		case CAL_CMD_TOGGLE_DRIFT_DEMO:
			atomic_set(&drift_demo_mode, !atomic_get(&drift_demo_mode));
			atomic_set(&drift_reset_requested, 1);
			printk("Drift mode -> %s (%u controlled minute(s) required)\n",
			       atomic_get(&drift_demo_mode) ? "demo" : "24h",
			       get_drift_required_minutes());
			break;
		default:
			break;
		}
	}
}

static void reporting_thread_entry(void *arg1, void *arg2, void *arg3)
{
	uint32_t last_jitter_sequence = 0U;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (1) {
		struct system_snapshot snapshot;
		struct jitter_summary jitter;

		k_sleep(K_MSEC(REPORT_PERIOD_MS));

		k_mutex_lock(&snapshot_lock, K_FOREVER);
		snapshot = latest_snapshot;
		k_mutex_unlock(&snapshot_lock);

		if (snapshot.state == SYSTEM_STATE_FAULT) {
			printk("[%lld ms] Avg: --.- C | Raw: %d.%03d V | MV: %d | Thr: %d.%02d C | "
			       "Mode: %s | LED: %s | DriftRef: %s | Ctrl: %u/%u | ADC_ERR: %u | Q_OVR: %u\n",
			       (long long)snapshot.latest.timestamp_ms,
			       snapshot.latest.millivolts / 1000,
			       abs32(snapshot.latest.millivolts % 1000),
			       snapshot.latest.millivolts,
			       snapshot.threshold_centi / 100,
			       abs32(snapshot.threshold_centi % 100),
			       state_to_string(snapshot.state),
			       led_state_to_string(snapshot.state),
			       snapshot.drift_baseline_ready ? "SET" : "LEARN",
			       snapshot.drift_controlled_minutes,
			       snapshot.drift_required_minutes,
			       snapshot.adc_error_count,
			       snapshot.queue_overrun_count);
		} else {
			printk("[%lld ms] Avg: %d.%02d C | Raw: %d.%02d C | MV: %d | Thr: %d.%02d C | "
			       "Mode: %s | LED: %s | DriftRef: %s%s%d.%02d C | LastMin: %d.%02d C | "
			       "Ctrl: %u/%u | Env: %s | ADC_ERR: %u | Q_OVR: %u\n",
			       (long long)snapshot.latest.timestamp_ms,
			       snapshot.average_temp_centi / 100,
			       abs32(snapshot.average_temp_centi % 100),
			       snapshot.latest.temp_centi / 100,
			       abs32(snapshot.latest.temp_centi % 100),
			       snapshot.latest.millivolts,
			       snapshot.threshold_centi / 100,
			       abs32(snapshot.threshold_centi % 100),
			       state_to_string(snapshot.state),
			       led_state_to_string(snapshot.state),
			       snapshot.drift_demo_mode ? "D:" : "",
			       snapshot.drift_baseline_ready ? "" : "*",
			       snapshot.drift_baseline_centi / 100,
			       abs32(snapshot.drift_baseline_centi % 100),
			       snapshot.last_minute_avg_centi / 100,
			       abs32(snapshot.last_minute_avg_centi % 100),
			       snapshot.drift_controlled_minutes,
			       snapshot.drift_required_minutes,
			       snapshot.controlled_environment ? "CTRL" : "TRANS",
			       snapshot.adc_error_count,
			       snapshot.queue_overrun_count);
		}

		if (get_new_jitter_summary(&last_jitter_sequence, &jitter)) {
			int64_t avg_delta_x100 =
				(jitter.total_delta_ms * 100) / (int64_t)jitter.intervals;

			printk("JITTER: n=%u | min=%lld ms | max=%lld ms | avg=%lld.%02lld ms | "
			       "bad=%u | tolerance=+/- %d ms\n",
			       jitter.intervals,
			       (long long)jitter.min_delta_ms,
			       (long long)jitter.max_delta_ms,
			       (long long)(avg_delta_x100 / 100),
			       (long long)(avg_delta_x100 % 100),
			       jitter.bad_intervals,
			       JITTER_BAD_TOLERANCE_MS);
		}
	}
}

static int ble_start_advertising(void)
{
	int err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0);

	if (err == 0) {
		advertising_active = true;
	}

	return err;
}

static void ble_stop_advertising(void)
{
	if (!advertising_active) {
		return;
	}

	if (bt_le_adv_stop() == 0) {
		advertising_active = false;
	}
}

static void communication_thread_entry(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (1) {
		struct system_snapshot snapshot;
		int err;

		k_sleep(K_MSEC(BLE_PERIOD_MS));

		if (!ble_ready) {
			continue;
		}

		k_mutex_lock(&snapshot_lock, K_FOREVER);
		snapshot = latest_snapshot;
		k_mutex_unlock(&snapshot_lock);

		if (snapshot.state == SYSTEM_STATE_FAULT) {
			ble_stop_advertising();
			continue;
		}

		adv_payload.average_temp_centi_be =
			sys_cpu_to_be16(snapshot.average_temp_centi);
		adv_payload.state = (uint8_t)snapshot.state;

		if (!advertising_active) {
			err = ble_start_advertising();
			if (err < 0) {
				printk("BLE advertising start failed (err=%d)\n", err);
			}
			continue;
		}

		err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
		if (err < 0) {
			printk("BLE advertising update failed (err=%d)\n", err);
		}
	}
}

int main(void)
{
	int err;

	atomic_set(&warning_threshold_centi, WARNING_THRESHOLD_DEFAULT_CENTI);
	atomic_set(&drift_demo_mode, 0);
	atomic_set(&drift_reset_requested, 0);

	printk("Design Challenge 1: High-Reliability Thermal Monitor and Controller\n");
	printk("Sampling=%d ms, average window=%d s, threshold=%d.%02d C\n",
	       SAMPLE_PERIOD_MS, AVERAGE_WINDOW_SECONDS,
	       WARNING_THRESHOLD_DEFAULT_CENTI / 100,
	       WARNING_THRESHOLD_DEFAULT_CENTI % 100);
	printk("Buttons: SW0 threshold- | SW1 threshold+ | SW2 threshold reset | SW3 drift mode toggle\n");

	err = init_led();
	if (err < 0) {
		printk("LED init failed (err=%d)\n", err);
		return 0;
	}

	err = init_buttons();
	if (err < 0) {
		printk("Button init failed (err=%d)\n", err);
		return 0;
	}

	err = init_adc_once();
	if (err < 0) {
		printk("ADC init failed (err=%d)\n", err);
		return 0;
	}

	k_timer_init(&sample_timer, sample_timer_handler, NULL);
	k_timer_init(&led_off_timer, led_off_timer_handler, NULL);

	k_thread_create(&adc_thread_data, adc_thread_stack,
			K_THREAD_STACK_SIZEOF(adc_thread_stack),
			adc_thread_entry, NULL, NULL, NULL,
			ADC_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&adc_thread_data, "adc_sampling");

	k_thread_create(&logic_thread_data, logic_thread_stack,
			K_THREAD_STACK_SIZEOF(logic_thread_stack),
			logic_thread_entry, NULL, NULL, NULL,
			LOGIC_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&logic_thread_data, "logic_engine");

	k_thread_create(&cal_thread_data, cal_thread_stack,
			K_THREAD_STACK_SIZEOF(cal_thread_stack),
			calibration_thread_entry, NULL, NULL, NULL,
			CAL_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&cal_thread_data, "calibration");

	k_thread_create(&report_thread_data, report_thread_stack,
			K_THREAD_STACK_SIZEOF(report_thread_stack),
			reporting_thread_entry, NULL, NULL, NULL,
			REPORT_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&report_thread_data, "reporting");

	k_thread_create(&comm_thread_data, comm_thread_stack,
			K_THREAD_STACK_SIZEOF(comm_thread_stack),
			communication_thread_entry, NULL, NULL, NULL,
			COMM_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&comm_thread_data, "communication");

	err = bt_enable(NULL);
	if (err < 0) {
		printk("Bluetooth init failed (err=%d)\n", err);
	} else {
		ble_ready = true;
		err = ble_start_advertising();
		if (err < 0) {
			printk("BLE advertising start failed (err=%d)\n", err);
		}
	}

	k_timer_start(&sample_timer, K_NO_WAIT, K_MSEC(SAMPLE_PERIOD_MS));

	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}

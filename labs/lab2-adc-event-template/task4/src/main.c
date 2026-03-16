/* main.c - Lab Activity 2 - Task 4 Template (Extra)
 *
 * Task 4: Interrupt/event-driven ADC sampling (Extra Task)
 *
 * Approach:
 *   Use Zephyr ADC async API (adc_read_async) + k_poll_signal
 *   - This uses driver-managed interrupt/event internally
 *   - We do NOT write a custom ISR
 *
 * IMPORTANT:
 *   If adc_read_async is unavailable in the build, ensure:
 *     CONFIG_ADC_ASYNC=y
 *     CONFIG_POLL=y
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>

#define SAMPLE_PERIOD_MS 500
#define TEMP_THRESHOLD_C 30.0f

/* LED */
#define LED0_NODE DT_ALIAS(led0)
#if !DT_NODE_HAS_STATUS(LED0_NODE, okay)
#error "Unsupported board: led0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* ADC channel */
static const struct adc_dt_spec adc_channel =
	ADC_DT_SPEC_GET(DT_PATH(zephyr_user));

static int16_t adc_buf;
static bool adc_setup_done;

/* Async wait objects */
static struct k_poll_signal adc_sig;
static struct k_poll_event  adc_evt = K_POLL_EVENT_INITIALIZER(
	K_POLL_TYPE_SIGNAL,
	K_POLL_MODE_NOTIFY_ONLY,
	&adc_sig
);

/* One async sample -> returns raw in adc_buf */
static int adc_sample_async(int16_t *out_raw)
{
	int err;

	if (!out_raw) {
		return -EINVAL;
	}

	if (!adc_is_ready_dt(&adc_channel)) {
		printk("ADC %s not ready\n", adc_channel.dev->name);
		return -EIO;
	}

	/* Setup once */
	if (!adc_setup_done) {
		err = adc_channel_setup_dt(&adc_channel);
		if (err < 0) {
			printk("ADC setup failed (err=%d)\n", err);
			return err;
		}
		adc_setup_done = true;
	}

	struct adc_sequence seq = {
		.buffer = &adc_buf,
		.buffer_size = sizeof(adc_buf),
	};

	err = adc_sequence_init_dt(&adc_channel, &seq);
	if (err < 0) {
		printk("ADC seq init failed (err=%d)\n", err);
		return err;
	}

	/* Start async conversion */
	k_poll_signal_reset(&adc_sig);

	/* TODO: students call adc_read_async(adc_channel.dev, &seq, &adc_sig); */
	/* err = adc_read_async(...); */
	/* if (err < 0) return err; */

	/* TODO: students wait for completion using k_poll(&adc_evt, 1, timeout) */
	/* err = k_poll(&adc_evt, 1, K_MSEC(1000)); */
	/* if (err < 0) return err; */

	/* After completion, adc_buf has the sample */
	*out_raw = adc_buf;
	return 0;
}

int main(void)
{
	int err;

	printk("Lab Activity 2 - Task 4 Template: ADC async (interrupt/event-driven)\n");

	/* LED init */
	if (!gpio_is_ready_dt(&led0)) {
		printk("LED not ready\n");
		return 0;
	}
	err = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		printk("LED configure failed (err=%d)\n", err);
		return 0;
	}

	/* Init async signal */
	k_poll_signal_init(&adc_sig);

	while (1) {
		int16_t raw = 0;

		/* 1) Trigger async ADC sampling */
		err = adc_sample_async(&raw);
		if (err == 0) {
			/* 2) Convert raw -> mV (reuse Task 1 knowledge) */
			int32_t mv = 0;      /* TODO */
			float temp_c = 0.0f; /* TODO */

			/* TODO:
			 *   mv = raw;
			 *   adc_raw_to_millivolts_dt(&adc_channel, &mv);
			 *   temp_c = (mv/10) - 273.15;  (LM335)
			 */

			/* 3) Optional threshold LED */
			bool led_on = (temp_c > TEMP_THRESHOLD_C);
			(void)gpio_pin_set_dt(&led0, led_on ? 1 : 0);

			printk("raw=%d, mv=%d (TODO), temp=%.2f (TODO), LED=%s\n",
			       raw, mv, (double)temp_c, led_on ? "ON" : "OFF");
		} else {
			printk("adc_sample_async failed (err=%d)\n", err);
		}

		k_sleep(K_MSEC(SAMPLE_PERIOD_MS));
	}
}

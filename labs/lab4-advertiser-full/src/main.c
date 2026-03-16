/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2025-2026 University of Bristol
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/******************************************************************************/
#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
/******************* ADC Configuration and Data Structures ********************/
/**
 * Use this block to configure the ADC / sensor reading worker
 * - Define the sampling period (30 sec)
 * - Define the ADC channel from device tree
 * - Define the sensor reading worker
 * - Define any other ADC-/Sensor-related variables that may be needed
 */
#define ADC_SAMPLING_PERIOD_MS        5000

/* ADC channel from devicetree node */
static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));

static int16_t adc_buf;
static bool adc_setup_done;

/* ADC peridic sampling worker*/
static struct k_timer sample_timer;
static struct k_work sample_work;
/******************* LED Configuration and Data Structures ********************/
/**
 * Use this block to configure the LED
 * - Define the LED Node alias
 * - Define the gpio device tree spec
 */
#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
/******************* BLE Configuration and Data Structures ********************/
/**
 * Use this block to define BLE functionality
 * - Define the device name, configurable from prj.conf
 * - Define the COMPANY ID for the Manufacturer Specific Data element
 * - Define the format of the MSD payload, and a variable to hold it
 * - Define the format of your payload
 * - Define the advertising interval and other BLE parameters
 */
/* BLE device name, configuirable in prj.conf */
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

/* This is the company ID that will be in the Manufacturer Specific Data */
#define COMPANY_ID 0x0059 /* Nordic Semiconductor ASA */
#define GROUP_ID 0xFF

/* Define the format of the Manufacturer Specific advertisement element */
struct adv_mfg_data {
	uint16_t company_id;
	uint8_t group_id;
	int16_t temperature;
	int32_t voltage;
} __packed;
typedef struct adv_mfg_data adv_mfg_data_t;

/* Initialise the Manufacturer Specific Data advertisement element */
static adv_mfg_data_t adv_mfg_data = {
	.company_id = COMPANY_ID,
	.group_id = GROUP_ID,
	.temperature = 0,
	.voltage = 0,
};

/**
 * Our advertisement data structure.
 * 
 * - The first element (AD0) is of type 'Complete Local Name' (x09)
 * - The second element (AD1) is Manufacturer Specific Data (0xFF)
 *   The sub-format of this field is specified in struct adv_mfg_data
 */
static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
	BT_DATA(BT_DATA_MANUFACTURER_DATA, (uint8_t *)&adv_mfg_data, sizeof(adv_mfg_data)),
};

/**
 * Define advertising interval (in units of 0.625 ms)
 * 
 * For example, 0x1F40 = 8000 * 0.625 ms = 5000 ms (5s)
 */
#define BT_ADV_INTERVAL 0x1F40

/* BLE advertisement parameters */
static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
	BT_LE_ADV_OPT_NONE,
	BT_ADV_INTERVAL,
	BT_ADV_INTERVAL,
	NULL
);
/******************************************************************************/
/**
 * Use this block to define the functions needed to read the sensor
 * - A function that reads the sensor
 *   The function must turn the LED on at the start, and turn it back off if
 *   it ends successfully. In this scenario, the user will see a quick blink.
 *   If an error occurs, the LED will remain on.
 * 
 * - The sensor reading worker.
 *   When the worker has read the sensor successfully, it must update the
 *   correct part of the BLE advertisement payload.
 */
/* 
 * Read ADC once, convert to temperature and populate the arguments
 */
static int read_sensor(float *out_temp_c, int32_t *out_voltage)
{
	int err;

	/*
	 * Blink LED while taking a reading. If this function returns and the
	 * LED is still on then the function returned prematurely with an error
	 */
	int led_err;
	led_err = gpio_pin_set_dt(&led0, 1);
	if (led_err < 0) {
		printk("LED set failed (err=%d)\n", led_err);
	}

	if (!adc_is_ready_dt(&adc_channel)) {
		printk("ADC %s is not ready\n", adc_channel.dev->name);
		return -EIO;
	}

	/* Do ADC channel setup once */
	if (!adc_setup_done) {
		err = adc_channel_setup_dt(&adc_channel);
		if (err < 0) {
			printk("ADC channel setup failed (err=%d)\n", err);
			return err;
		}
		adc_setup_done = true;

		printk("ADC ready: dev=%s, channel_id=%d\n",
		       adc_channel.dev->name, adc_channel.channel_id);
	}

	struct adc_sequence sequence = {
		.buffer = &adc_buf,
		.buffer_size = sizeof(adc_buf),
	};

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

	const int16_t raw = adc_buf;
	int32_t mv = adc_buf;

	err = adc_raw_to_millivolts_dt(&adc_channel, &mv);
	if (err < 0) {
		printk("raw=%d (mv conversion failed err=%d)\n", raw, err);
		return err;
	}

	/* LM335: 10 mV/K => T(K)=mV/10 => T(C)=mV/10 - 273.15 */
	float temp_c = (mv / 10.0f) - 273.15f;

	/* Explicit promotion to double to silence compiler warning */
	printk("ADC: raw=%d, voltage=%d mV, Temp=%.2f C\n", raw, mv, (double)temp_c);

	if (out_temp_c) {
		*out_temp_c = temp_c;
	}

	if (out_voltage) {
		*out_voltage = mv;
	}

	led_err = gpio_pin_set_dt(&led0, 0);
	if (led_err < 0) {
		printk("LED set failed (err=%d)\n", led_err);
	}

	return 0;
}
/* Runs in system workqueue thread context (safe for adc_read) */
static void sample_work_handler(struct k_work *work)
{
	int err = 0;
	float temperature;
	int32_t voltage;

	ARG_UNUSED(work);

	/* Update the ADC reading. If successful, update our BLE payload */
	if(read_sensor(&temperature, &voltage) == 0) {
		/* 
		 * Our temperature is a float.
		 * The BLE packet expects temperature as a 2-byte signed integer in big-endian order
		 * The BLE packet expects voltage as a 4-byte signed integer in big-endian order
		 */
		adv_mfg_data.temperature = sys_cpu_to_be16((int16_t)temperature);
		adv_mfg_data.voltage = sys_cpu_to_be32(voltage);

		printk("Update BLE payload: C=%u, T=0x%04x (%d), V=0x%08x (%d)\n", adv_mfg_data.company_id, 
			adv_mfg_data.temperature, (int16_t)temperature, adv_mfg_data.voltage, voltage);

		err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
		if (err) {
			printk("Failed to update advertising data (err %d)\n", err);
		}
	}
}
/* Runs in timer context: do NOT call adc_read here */
static void sample_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	(void)k_work_submit(&sample_work);
}
/******************************************************************************/
int main(void)
{
	int err;

	printk("Starting LED/ADC/BLE Advertiser Demo\n");

	/* Initialize LED0 */
	if (!gpio_is_ready_dt(&led0)) {
		printk("LED device not ready\n");
		return 0;
	}

	err = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		printk("LED configure failed (err=%d)\n", err);
		return 0;
	}

	/* Start the ADC periodic sampling worker */
	printk("Initialising ADC sampling, period=%d ms\n", ADC_SAMPLING_PERIOD_MS);
	k_work_init(&sample_work, sample_work_handler);
	k_timer_init(&sample_timer, sample_timer_handler, NULL);
	k_timer_start(&sample_timer, K_MSEC(ADC_SAMPLING_PERIOD_MS), K_MSEC(ADC_SAMPLING_PERIOD_MS));

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");
	printk("Element 0: T=0x%02x, L=0x%02x, V='%s'\n", ad[0].type, ad[0].data_len, ad[0].data);
	printk("Element 1: T=0x%02x, L=0x%02x, V='C=0x%04x, T=%d'\n", ad[1].type, ad[1].data_len,
		((adv_mfg_data_t *)ad[1].data)->company_id, ((adv_mfg_data_t *)ad[1].data)->temperature);

	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err)
	{
		printk("Advertising failed to start (err %d)\n", err);
		return 0;
	}

	printk("BLE Advertiser Ready\n");

	return 0;
}

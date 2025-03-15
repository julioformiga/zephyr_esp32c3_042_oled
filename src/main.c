#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/sensor/vl53l0x.h>
#include <lvgl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

#define LED0_NODE DT_ALIAS(led0)
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#define MEASUREMENTS_COUNT 3
#define MAX_DISTANCE_DIFF 500
#define ALPHA 0.15f  // Smoothing factor (0.0 - 1.0). Lower value = smoother output

static int32_t measurements[MEASUREMENTS_COUNT] = {0};
static uint8_t measurement_index = 0;
static float smoothed_distance = 5;  // Smoothed value
static bool first_measurement = true;

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static int32_t calculate_filtered_average(void) {
    // Sort values to help detect spikes
    int32_t sorted[MEASUREMENTS_COUNT];
    for (int i = 0; i < MEASUREMENTS_COUNT; i++) {
        sorted[i] = measurements[i];
    }

    // Simplified bubble sort (efficient enough for just 3 elements)
    for (int i = 0; i < MEASUREMENTS_COUNT - 1; i++) {
        for (int j = 0; j < MEASUREMENTS_COUNT - i - 1; j++) {
            if (sorted[j] > sorted[j + 1]) {
                int32_t temp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = temp;
            }
        }
    }

    // Check for outlier values
    int32_t median = sorted[1];
    int valid_count = 0;
    int32_t sum = 0;

    for (int i = 0; i < MEASUREMENTS_COUNT; i++) {
        if (abs(sorted[i] - median) <= MAX_DISTANCE_DIFF) {
            sum += sorted[i];
            valid_count++;
        }
    }

    return valid_count > 0 ? sum / valid_count : median;
}

int main(void)
{
	int retled, retdis;

	const struct device *const dev = DEVICE_DT_GET_ONE(st_vl53l0x);
	struct sensor_value value;
	if (!device_is_ready(dev)) {
		printk("sensor: device not ready.\n");
		return 0;
	}

	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("LED device %s is not ready", led.port->name);
		return 0;
	}
	retled = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (retled < 0)
		return 0;

    lv_obj_clean(lv_scr_act());

	lv_obj_t * bar = lv_bar_create(lv_scr_act());
	lv_obj_set_size(bar, 60, 10);
	lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 5);
	lv_bar_set_range(bar, 0, 2000);

	lv_obj_remove_style_all(bar);
	static lv_style_t style_indic;
	lv_style_init(&style_indic);
	lv_style_set_bg_opa(&style_indic, LV_OPA_COVER);
	lv_style_set_bg_color(&style_indic, lv_color_black());
	lv_obj_add_style(bar, &style_indic, LV_PART_INDICATOR);

	lv_obj_t * label = lv_label_create(lv_scr_act());
	lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 20);
	lv_label_set_text(label, "Init...");
	lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);

	while (1) {
		lv_timer_handler();
		retdis = sensor_sample_fetch(dev);
		if (retdis) {
			printk("sensor_sample_fetch failed retdistance %d\n", retdis);
			continue;
		}

		retdis = sensor_channel_get(dev, SENSOR_CHAN_VL53L0X_RANGE_STATUS, &value);
		bool valid_reading = (value.val1 == VL53L0X_RANGE_STATUS_RANGE_VALID);

		retdis = sensor_channel_get(dev, SENSOR_CHAN_DISTANCE, &value);
		int32_t current_distance = sensor_value_to_milli(&value);

		int32_t distance_mm = (int32_t)smoothed_distance;

		if (valid_reading && current_distance <= 1000) {
			measurements[measurement_index] = current_distance;
			measurement_index = (measurement_index + 1) % MEASUREMENTS_COUNT;

			int32_t avg_distance = calculate_filtered_average();

			if (first_measurement) {
				smoothed_distance = (float)avg_distance;
				first_measurement = false;
			} else {
				smoothed_distance = (ALPHA * (float)avg_distance) + ((1.0f - ALPHA) * smoothed_distance);
			}

			lv_bar_set_value(bar, distance_mm, LV_ANIM_ON);
			lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 20);

			static char buf[32];
			snprintf(buf, sizeof(buf), "%d mm", (int)distance_mm);
			lv_label_set_text(label, buf);

			if (distance_mm > 30 && distance_mm < 300) {
				gpio_pin_set_dt(&led, 0);
				int blink_period = (distance_mm - 30) * 900 / 270 + 100;
				static int64_t last_toggle;
				int64_t now = k_uptime_get();

				if (now - last_toggle >= blink_period) {
					static bool led_state;
					led_state = !led_state;
					gpio_pin_set_dt(&led, led_state);
					last_toggle = now;
				}
			}
			else if (distance_mm <= 30) {
				gpio_pin_set_dt(&led, 1);
				lv_bar_set_value(bar, 0, LV_ANIM_OFF);
				lv_label_set_text(label, "");
			}
			else
				gpio_pin_set_dt(&led, 0);
		} else {
			lv_bar_set_value(bar, 0, LV_ANIM_OFF);
			lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 2);
			lv_label_set_text(label, "Out of\nrange!");
			gpio_pin_set_dt(&led, 1);
		}

		k_sleep(K_MSEC(50));
    }

    return 0;
}

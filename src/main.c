#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/zbus/zbus.h>

#include "messages.h"
#include "gps_sensor.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define USART_NODE DT_NODELABEL(usart1)
#if !DT_NODE_HAS_STATUS_OKAY(USART_NODE)
#error "Unsupported board: usart1 devicetree node label is not defined"
#endif
#define FF_INT DT_ALIAS(ff0)
#if !DT_NODE_HAS_STATUS_OKAY(FF_INT)
#error "Unsupported board: ffpin devicetree alias is not defined"
#endif
#define SW0_NODE DT_ALIAS(sw1)
#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#error "Unsupported board: sw1 devicetree alias is not defined"
#endif
#define SWSOS_NODE DT_ALIAS(sw2)
#if !DT_NODE_HAS_STATUS_OKAY(SWSOS_NODE)
#error "Unsupported board: sw2 devicetree alias is not defined"
#endif

#define PERIODIC_SENSOR_TIME    K_SECONDS(30)
#define LONG_PRESS_TIME    K_MSEC(2000)
#define SOS_LONG_PRESS_TIME    K_MSEC(1000)

ZBUS_CHAN_DECLARE(events_chan);
ZBUS_CHAN_DECLARE(sensor_data_chan);

static const struct device *usart_dev = DEVICE_DT_GET(USART_NODE);
static const struct gpio_dt_spec freefall = GPIO_DT_SPEC_GET_OR(FF_INT, gpios, {0});
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static const struct gpio_dt_spec sos_button = GPIO_DT_SPEC_GET_OR(SWSOS_NODE, gpios, {0});
static struct gpio_callback freefall_cb_data;
static struct gpio_callback button_cb_data;
static struct gpio_callback sos_button_cb_data;

static uint8_t button_pressed = 0;
static uint8_t sos_button_pressed = 0;

struct events_wq_info {
	struct k_work work;
	const struct zbus_channel *chan;
	uint8_t handle;
};

static struct events_wq_info wq_button_event_handler = {.chan = &events_chan, .handle = 1};
static struct events_wq_info wq_sos_button_event_handler = {.chan = &events_chan, .handle = 2};
static struct events_wq_info wq_freefall_event_handler = {.chan = &events_chan, .handle = 3};
static struct events_wq_info wq_timer_event_handler = {.chan = &events_chan, .handle = 4};

void freefall_detect(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    k_work_submit(&wq_freefall_event_handler.work);
}

void sos_button_timeout_handler(struct k_timer *timer_id) {
    if (sos_button_pressed) {
        sos_button_pressed = 0;
        k_work_submit(&wq_sos_button_event_handler.work);
    }
}

K_TIMER_DEFINE(sos_button_timer, sos_button_timeout_handler, NULL);

void sos_button_pressed_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    int val = gpio_pin_get(dev, sos_button.pin);

    if (val == 0) { // Button released
        sos_button_pressed = 0;
    } else { //Button Pressed
        sos_button_pressed = 1;
        k_timer_start(&sos_button_timer, SOS_LONG_PRESS_TIME, K_FOREVER);
    }
}

void button_timeout_handler(struct k_timer *timer_id) {
    if (button_pressed) {
        button_pressed = 0;
        k_work_submit(&wq_button_event_handler.work);
    }
} 

K_TIMER_DEFINE(button_timer, button_timeout_handler, NULL);

void button_pressed_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    int val = gpio_pin_get(dev, button.pin);

    if (val == 0) { // Button released
        button_pressed = 0;
    } else { //Button Pressed
        button_pressed = 1;
        k_timer_start(&button_timer, LONG_PRESS_TIME, K_FOREVER);
    }
}

static void wq_eh_cb(struct k_work *item)
{
	struct events_wq_info *sens = CONTAINER_OF(item, struct events_wq_info, work);
    uint8_t msg = sens->handle;
    zbus_chan_pub(&events_chan, &msg, K_MSEC(250));

	LOG_INF("Event_handler %u", sens->handle);
}

static void timer_cb (struct k_timer *timer_id) {
    k_work_submit(&wq_timer_event_handler.work);
}

K_TIMER_DEFINE(event_timer, timer_cb, NULL);

int main(void)
{

    if (!gpio_is_ready_dt(&button))
        LOG_ERR("Error button device is not ready");
    if (gpio_pin_configure_dt(&button, GPIO_INPUT))
        LOG_ERR("Error failed to configure pin");
    if (gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH)) 
        LOG_ERR("Error failed to configure interrupt on pin");
    
    gpio_init_callback(&button_cb_data, button_pressed_handler, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    LOG_INF("Set up %s pin %d as Button Interuption Pin Succesfully", button.port->name, button.pin);

    if (!gpio_is_ready_dt(&sos_button))
        LOG_ERR("Error sos_button device is not ready");
    if (gpio_pin_configure_dt(&sos_button, GPIO_INPUT))
        LOG_ERR("Error failed to configure pin");
    if (gpio_pin_interrupt_configure_dt(&sos_button, GPIO_INT_EDGE_FALLING)) 
        LOG_ERR("Error failed to configure interrupt on pin");
    
    gpio_init_callback(&sos_button_cb_data, sos_button_pressed_handler, BIT(sos_button.pin));
    gpio_add_callback(sos_button.port, &sos_button_cb_data);

    LOG_INF("Set up %s pin %d as SOS Button Interuption Pin Succesfully", sos_button.port->name, sos_button.pin);

    if (!gpio_is_ready_dt(&freefall))
        LOG_ERR("Error freefall pin %s is not ready", freefall.port->name);
    if (gpio_pin_configure_dt(&freefall, GPIO_INPUT))
        LOG_ERR("Error failed to configure FREEFALL pin");
    if (gpio_pin_interrupt_configure_dt(&freefall, GPIO_INT_EDGE_FALLING))
        LOG_ERR("Error failed to configure interrupt on FREEFALL pin");

    gpio_init_callback(&freefall_cb_data, freefall_detect, BIT(freefall.pin));
    gpio_add_callback(freefall.port, &freefall_cb_data);

    LOG_INF("Set up %s pin %d as FreeFall Interruption Pin Succesfully", freefall.port->name, freefall.pin);

    usart1_capture(usart_dev);
    
    k_work_init(&wq_timer_event_handler.work, wq_eh_cb);
    k_work_init(&wq_freefall_event_handler.work, wq_eh_cb);
    k_work_init(&wq_button_event_handler.work, wq_eh_cb);
    k_work_init(&wq_sos_button_event_handler.work, wq_eh_cb);
    k_timer_start(&event_timer, PERIODIC_SENSOR_TIME, PERIODIC_SENSOR_TIME);

    return 0;
    
}

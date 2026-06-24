/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2026
 */

#include "status_led.h"

#include "FreeRTOS.h"
#include "task.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

#include "probe_config.h"
#include "status_led.pio.h"

#ifdef PROBE_WS2812_STATUS_LED
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} status_led_color_t;

enum {
    STATUS_LED_EVENT_TX = 1u << 0,
    STATUS_LED_EVENT_RX = 1u << 1,
};

static PIO status_led_pio = pio1;
static uint status_led_sm;
static bool status_led_ready;
static bool usb_connected;
static bool usb_ready;
static bool usb_suspended;
static uint32_t active_events;
static TickType_t pulse_expires_at;
static status_led_color_t rendered_color = {0xff, 0xff, 0xff};

static const status_led_color_t color_off = {0, 0, 0};
static const status_led_color_t color_boot = {0, 0, 2};
static const status_led_color_t color_connected = {0, 0, 1};
static const status_led_color_t color_ready = {0, 2, 0};
static const status_led_color_t color_suspended = {2, 1, 0};
static const status_led_color_t color_tx = {3, 0, 3};
static const status_led_color_t color_rx = {0, 3, 3};
static const status_led_color_t color_tx_rx = {2, 2, 2};

#define STATUS_LED_PULSE_TICKS pdMS_TO_TICKS(45)

static inline void status_led_put_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    uint32_t grb = ((uint32_t)green << 24) | ((uint32_t)red << 16) | ((uint32_t)blue << 8);
    pio_sm_put_blocking(status_led_pio, status_led_sm, grb);
}

static bool status_led_color_equal(status_led_color_t left, status_led_color_t right)
{
    return left.red == right.red && left.green == right.green && left.blue == right.blue;
}

static void status_led_render(status_led_color_t color)
{
    if (!status_led_ready || status_led_color_equal(color, rendered_color))
        return;

    rendered_color = color;
    status_led_put_rgb(color.red, color.green, color.blue);
}

static status_led_color_t status_led_base_color(void)
{
    if (usb_suspended)
        return color_suspended;
    if (usb_ready)
        return color_ready;
    if (usb_connected)
        return color_connected;
    return color_off;
}

static status_led_color_t status_led_activity_color(uint32_t events)
{
    if ((events & (STATUS_LED_EVENT_TX | STATUS_LED_EVENT_RX)) ==
        (STATUS_LED_EVENT_TX | STATUS_LED_EVENT_RX)) {
        return color_tx_rx;
    }
    if (events & STATUS_LED_EVENT_TX)
        return color_tx;
    return color_rx;
}

static void status_led_update(void)
{
    TickType_t now = xTaskGetTickCount();
    if (active_events && (int32_t)(pulse_expires_at - now) <= 0)
        active_events = 0;

    if (active_events)
        status_led_render(status_led_activity_color(active_events));
    else
        status_led_render(status_led_base_color());
}

static void status_led_note_activity(uint32_t event)
{
    if (!status_led_ready)
        return;

    active_events |= event;
    pulse_expires_at = xTaskGetTickCount() + STATUS_LED_PULSE_TICKS;
    status_led_update();
}
#endif

void status_led_init(void)
{
#ifdef PROBE_WS2812_STATUS_LED
    uint offset = pio_add_program(status_led_pio, &status_led_program);
    status_led_sm = pio_claim_unused_sm(status_led_pio, true);
    status_led_program_init(status_led_pio, status_led_sm, offset, PROBE_WS2812_STATUS_LED, 800000);
    status_led_ready = true;
    status_led_render(color_boot);
#endif
}

void status_led_set_usb_state(bool connected, bool ready, bool suspended)
{
#ifdef PROBE_WS2812_STATUS_LED
    usb_connected = connected;
    usb_ready = ready;
    usb_suspended = suspended;
    status_led_update();
#else
    (void)connected;
    (void)ready;
    (void)suspended;
#endif
}

void status_led_note_uart_tx(void)
{
#ifdef PROBE_WS2812_STATUS_LED
    status_led_note_activity(STATUS_LED_EVENT_TX);
#endif
}

void status_led_note_uart_rx(void)
{
#ifdef PROBE_WS2812_STATUS_LED
    status_led_note_activity(STATUS_LED_EVENT_RX);
#endif
}

void status_led_tick(void)
{
#ifdef PROBE_WS2812_STATUS_LED
    status_led_update();
#endif
}

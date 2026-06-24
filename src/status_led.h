#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <stdbool.h>

void status_led_init(void);
void status_led_set_usb_state(bool connected, bool ready, bool suspended);
void status_led_note_uart_tx(void);
void status_led_note_uart_rx(void);
void status_led_tick(void);

#endif

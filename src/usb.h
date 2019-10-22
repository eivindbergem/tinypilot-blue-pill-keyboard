#ifndef _USB_H_
#define _USB_H_

#include <stdint.h>

typedef struct {
    uint8_t data[8];
    uint8_t pos;
} usb_kbd_packet_t;

void usb_init(void);
void usb_write_key(usb_kbd_packet_t *packet);

#endif

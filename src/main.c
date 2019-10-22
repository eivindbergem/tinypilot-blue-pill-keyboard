#include <libopencm3/stm32/rcc.h>

#include "uart.h"
#include "usb.h"

int main(void) {
    rcc_clock_setup_in_hse_8mhz_out_72mhz();
    uart_init();
    usb_init();

    for (;;) {
    }
}

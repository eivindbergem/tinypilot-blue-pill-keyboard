#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>

#include "usb.h"

static usb_kbd_packet_t buf;

void usart1_isr(void) {
    if (usart_get_flag(USART1, USART_SR_RXNE)) {
        buf.data[buf.pos++] = usart_recv(USART1);

        if (buf.pos == sizeof(buf.data)) {
            usb_write_key(&buf);
            buf.pos = 0;
        }
    }
}

void uart_init(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USART1);

    gpio_set_mode(GPIO_BANK_USART1_RX, GPIO_MODE_INPUT,
		      GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RX);

    usart_enable_rx_interrupt(USART1);

    usart_set_baudrate(USART1, 115200);
    usart_set_databits(USART1, 8);
    usart_set_stopbits(USART1, USART_STOPBITS_1);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
    usart_set_mode(USART1, USART_MODE_RX);
    usart_enable(USART1);

    nvic_set_priority(NVIC_USART1_IRQ, 5);
    nvic_enable_irq(NVIC_USART1_IRQ);
}

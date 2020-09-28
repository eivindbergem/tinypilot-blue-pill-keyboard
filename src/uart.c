#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>

#include "usb.h"

#define BUFFERS         128

#define TIMEOUT        10000

static usb_kbd_packet_t buffers[128];
uint8_t pos;
static usb_kbd_packet_t *packet;

static void next_packet(void) {
    pos = (pos + 1) % BUFFERS;

    packet = &buffers[pos];
    packet->pos = 0;
}

void usart1_isr(void) {
    if (usart_get_flag(USART1, USART_SR_RXNE)) {
        timer_set_counter(TIM2, 0);
        packet->data[packet->pos++] = usart_recv(USART1);

        if (packet->pos == sizeof(packet->data)) {
            usb_write_key(packet);
            next_packet();
        }
    }
}


void tim2_isr(void) {
    timer_clear_flag(TIM2, TIM_SR_UIF);
    next_packet();
}

void uart_init(void) {
    packet = &buffers[0];

    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USART1);
    rcc_periph_clock_enable(RCC_TIM2);

    timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT,
               TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_period(TIM2, TIMEOUT);
    timer_enable_counter(TIM2);
    timer_enable_irq(TIM2, TIM_DIER_UIE);

    nvic_enable_irq(NVIC_TIM2_IRQ);
    nvic_set_priority(NVIC_TIM2_IRQ, 1);

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

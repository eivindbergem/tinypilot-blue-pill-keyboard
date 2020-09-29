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
        packet->data[packet->pos++] = usart_recv(USART1);

        if (packet->pos == sizeof(packet->data)) {
            timer_disable_counter(TIM2);
            timer_set_counter(TIM2, 0);

            usb_write_key(packet);
            next_packet();
        } else {
            timer_set_counter(TIM2, 0);
            timer_enable_counter(TIM2);
        }
    }
}

void tim2_isr(void) {
    timer_clear_flag(TIM2, TIM_SR_UIF);
    timer_disable_counter(TIM2);
    timer_set_counter(TIM2, 0);
    next_packet();
}

void uart_init(void) {
    packet = &buffers[0];

    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USART1);
    rcc_periph_clock_enable(RCC_TIM2);

    /* Because we are just receiving a stream of characters, we don't
     * really know the borders of each HID report. But, we can
     * identify each report based on time. Each 8 byte report is
     * written in one go and there is likely to be delays between
     * reports. Using a timer, we reset the packet if too much time
     * passes between bytes. */
    timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT,
               TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_period(TIM2, TIMEOUT);
    timer_enable_irq(TIM2, TIM_DIER_UIE);

    nvic_enable_irq(NVIC_TIM2_IRQ);
    nvic_set_priority(NVIC_TIM2_IRQ, 3);

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

    nvic_set_priority(NVIC_USART1_IRQ, 2);
    nvic_enable_irq(NVIC_USART1_IRQ);
}

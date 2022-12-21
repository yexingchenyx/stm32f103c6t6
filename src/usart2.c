/*
LED负极接C13，正极接电源
串口RX接A9(TX)，TX接A10(RX)
串口接收'LED_ON'LED亮， 接收'LED_OFF'LED灭， 命令以@包围
*/

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <string.h>

static void led_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
    gpio_set(GPIOC, GPIO13); // LED灭
}

static void serial_setup(void) {
    rcc_clock_setup_in_hse_8mhz_out_72mhz();

    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USART1);

    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX); // TX为输出模式

    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO_USART1_RX); // RX为输入模式
    gpio_set(GPIOA, GPIO_USART1_RX); // 上拉，串口空闲时是高电平

    usart_set_mode(USART1, USART_MODE_TX_RX);
    usart_set_baudrate(USART1, 9600);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_stopbits(USART1, USART_STOPBITS_1);
    usart_set_databits(USART1, 8);

    rcc_periph_clock_enable(RCC_AFIO);
    nvic_enable_irq(NVIC_USART1_IRQ); // 开启USART1的中断
    usart_enable_rx_interrupt(USART1); // 开启接收中断

    usart_enable(USART1);
}

static void serial_send_byte(uint8_t data) {
    usart_send(USART1, data);
    while (!usart_get_flag(USART1, USART_FLAG_TXE));
}

static void serial_send_string(const char* data) {
    for (int i = 0; data[i] != '\0'; ++i) {
        serial_send_byte(data[i]);
    }
}

#define MESSAGE_MAX_LENGTH 100
char message[MESSAGE_MAX_LENGTH + 1];
bool message_receiving = false;
int message_len = 0;

void usart1_isr(void) {
    if (usart_get_flag(USART1, USART_FLAG_RXNE)) { // 判断是RXNE引起的USART1中断
        char a_char = usart_recv(USART1); // 读取后自动清RXNE
        if (a_char == '\n')
            return;
        if (!message_receiving && a_char == '@') {
            message_receiving = true;
            message_len = 0;
        } else if (message_receiving && a_char != '@') {
            if (message_len >= MESSAGE_MAX_LENGTH) {
                message_receiving = false;
                message_len = 0;
                serial_send_string("command too long, input:\n");
            } else {
                message[message_len++] = a_char;
            }
        } else if (message_receiving && a_char == '@') {
            message[message_len] = '\0';
            message[MESSAGE_MAX_LENGTH] = '\0';
            message_receiving = false;
            message_len = 0;
            if (strcmp(message, "LED_ON") == 0) {
                gpio_clear(GPIOC, GPIO13);
                serial_send_string("led on, input:\n");
            } else if (strcmp(message, "LED_OFF") == 0) {
                gpio_set(GPIOC, GPIO13);
                serial_send_string("led off, input:\n");
            } else {
                char str[MESSAGE_MAX_LENGTH * 2] = "unknown command [";
                strcat(str, message);
                strcat(str, "], input:\n");
                serial_send_string(str);
            }
        } else {

        }
    }
}

int main(void) {
    serial_setup();

    led_setup();

    for (int i = 0; i < 10000000; ++i)
        __asm__("nop");
    
    serial_send_string("input:\n");

    while (1)
    {
    }
}
 






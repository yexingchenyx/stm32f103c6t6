/*
LED负极接C13，正极接电源
轻触开关一脚接B9，一脚接地
串口RX接A9(TX)，TX接A10(RX)
按键串口发送'x'
串口接收'x'LED亮灭
*/

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

static void led_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
    gpio_set(GPIOC, GPIO13); // LED灭
}

static void button_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOB);
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO9);
    gpio_set(GPIOB, GPIO9); // 上拉，按键抬起输出1, 按键按下输出0
}

static void serial_setup(void) {
    //rcc_clock_setup_in_hse_8mhz_out_72mhz();
    rcc_clock_setup_in_hsi_out_64mhz();
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USART1);

    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO9); // TX为输出模式

    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO10); // RX为输入模式
    gpio_set(GPIOA, GPIO10); // 上拉，串口空闲时是高电平

    usart_set_baudrate(USART1, 9600);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
    usart_set_mode(USART1, USART_MODE_TX_RX);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_stopbits(USART1, USART_STOPBITS_1);
    usart_set_databits(USART1, 8);
    usart_enable(USART1);
}

int main(void) {
    led_setup();

    button_setup();

    serial_setup();

    while (1)
    {
        if (usart_get_flag(USART1, USART_FLAG_RXNE)) // RX not empty 接收到数据
        {
            uint16_t data = usart_recv(USART1); // 读取后自动清RXNE
            if (data == 'x') { // 此处处理过久，会导致接收的数据异常，因而判断字节为'x'时才控制LED亮灭
                usart_send(USART1, data);
                while (!usart_get_flag(USART1, USART_FLAG_TXE)); // 等待发送完成
                usart_send(USART1, '\n');
                while (!usart_get_flag(USART1, USART_FLAG_TXE)); // 等待发送完成
                gpio_toggle(GPIOC, GPIO13);
            }
        }

        if (gpio_get(GPIOB, GPIO9) == 0) // 按键被按下
        {
            for (int i = 0; i < 1000000; ++i) // 等待一下
                __asm__("nop");
            while (gpio_get(GPIOB, GPIO9) == 0) {} // 等待按键被抬起
            usart_send(USART1, 'x');
            while (!usart_get_flag(USART1, USART_FLAG_TXE)); // 等待发送完成
            usart_send(USART1, '\n');
            while (!usart_get_flag(USART1, USART_FLAG_TXE)); // 等待发送完成
        }
    }
}
 






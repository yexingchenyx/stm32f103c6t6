/*
LED负极接C13，正极接电源
轻触开关一脚接B9，一脚接地
按键切换LED亮灭
*/

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

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

int main(void) {
    led_setup();

    button_setup();

    while (1)
    {
        if (gpio_get(GPIOB, GPIO9) == 0) { // 按键被按下
            for (int i = 0; i < 1000000; ++i) // 等待一下
                __asm__("nop");
            while (gpio_get(GPIOB, GPIO9) == 0) {} // 等待按键被抬起
            gpio_toggle(GPIOC, GPIO13); // 开关LED
        }
    }
    
}

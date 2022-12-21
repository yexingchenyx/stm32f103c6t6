/*
LED负极接C13，正极接电源
轻触开关一脚接B0，一脚接地
按住开关LED亮，放开灭(boot0=0,boot1=0时有效)
*/

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>

static void led_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
    gpio_set(GPIOC, GPIO13); // LED灭
}

static void button_setup(void) {
    rcc_clock_setup_in_hse_8mhz_out_72mhz();

    rcc_periph_clock_enable(RCC_GPIOB);
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO0);
    gpio_set(GPIOB, GPIO0); // 上拉，按键抬起输出1, 按键按下输出0

    rcc_periph_clock_enable(RCC_AFIO); // 开启AFIO时钟
    nvic_enable_irq(NVIC_EXTI0_IRQ); // 开启EXTI0中断
    exti_select_source(EXTI0, GPIOB); // 选择EXTI源
    exti_set_trigger(EXTI0, EXTI_TRIGGER_BOTH); // 设置触发模式
    exti_enable_request(EXTI0); // 开启EXTI
}

void exti0_isr(void) {
    if (!gpio_get(GPIOB, GPIO0)) // 按键按下
        gpio_clear(GPIOC, GPIO13); // 亮
    else
        gpio_set(GPIOC, GPIO13); // 灭

    exti_reset_request(EXTI0);
}

int main(void) {

    led_setup();

    button_setup();

    while (1)
    {
    }
}
 



/*
LED负极接C13，正极接电源
LED闪烁
*/

#include <libopencm3/stm32/rcc.h> // Reset Clock Control
#include <libopencm3/stm32/gpio.h> // General Purpose Input Output

static void led_setup(void) {
    /* Enable GPIOC clock. */
    rcc_periph_clock_enable(RCC_GPIOC);

    /* Set GPIO13 (in GPIO port C) to 'output push-pull' */
    gpio_set_mode(
        GPIOC, /* GPIO port */
        GPIO_MODE_OUTPUT_2_MHZ, /* GPIO mode */
        GPIO_CNF_OUTPUT_PUSHPULL, /* GPIO I/O configuration */
        GPIO13 /* GPIO pin */
        );
}

int main(void) {
    int i;

    led_setup();

    for (;;) {
        gpio_clear(GPIOC, GPIO13); /* LED on */
        for (i = 0; i < 1500000; i++) /* Wait a bit. */
            __asm__("nop");
        
        gpio_set(GPIOC, GPIO13); /* LED off */
        for (i = 0; i < 1500000; i++) /* Wait a bit. */
            __asm__("nop");
    }
}
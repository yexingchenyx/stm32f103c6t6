/*
28BYJ-48，五线四相步进电机，ULN2003驱动
*/

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

static uint8_t step_8A(uint8_t step, bool inverse, uint32_t delay) { // 

    gpio_clear(GPIOA, GPIO1 | GPIO2 | GPIO3 | GPIO4);

    switch (step)
    {
    case 0:
        gpio_set(GPIOA, GPIO1);
        break;
    case 1:
        gpio_set(GPIOA, GPIO1 | GPIO2);
        break;
    case 2:
        gpio_set(GPIOA, GPIO2);
        break;
    case 3:
        gpio_set(GPIOA, GPIO2 | GPIO3);
        break;
    case 4:
        gpio_set(GPIOA, GPIO3);
        break;
    case 5:
        gpio_set(GPIOA, GPIO3 | GPIO4);
        break;
    case 6:
        gpio_set(GPIOA, GPIO4);
        break;
    case 7:
        gpio_set(GPIOA, GPIO4 | GPIO1);
        break;
    default:
        break;
    }

    for (uint32_t i = 0; i < delay; ++i) // 延时
        __asm__("nop");

    gpio_clear(GPIOA, GPIO1 | GPIO2 | GPIO3 | GPIO4); // 断电防过热

    if (!inverse) { //正转
        if (step == 7)
            return 0;
        else
            return step + 1;
    } else { // 反转
        if (step == 0)
            return 7;
        else
            return step - 1;
    }
}

int main(void) {

    rcc_clock_setup_in_hse_8mhz_out_72mhz();

    rcc_periph_clock_enable(RCC_GPIOA);

    gpio_set_mode(GPIOA, 
    GPIO_MODE_OUTPUT_50_MHZ, 
    GPIO_CNF_OUTPUT_PUSHPULL,
    GPIO1 | GPIO2 | GPIO3 | GPIO4);

    gpio_clear(GPIOA, GPIO1 | GPIO2 | GPIO3 | GPIO4);

    while (true) {
        uint32_t delay = 10000;
        uint8_t step = 0;

        for (uint32_t i = 0; i < 4076; ++i)
            step = step_8A(step, false, delay);
        
        for (uint32_t i = 0; i < 10000000; ++i)
            __asm__("nop");

        for (uint32_t i = 0; i < 4076; ++i)
            step = step_8A(step, true, delay);

        for (uint32_t i = 0; i < 10000000; ++i)
            __asm__("nop");
    }
}


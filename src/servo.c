/*
舵机控制
橙色线接A0, 红色线接5V, 棕色线接地
*/

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>

/*
f_counter = f_timer / (PSC + 1)
f_overflow = f_counter / (ARR + 1) = f_timer / (PSC + 1) / (ARR + 1)
f_pwm = f_overflow
duty_pwm = CCR / (ARR + 1)
resolution_pwm = 1 / (ARR + 1)
*/


int main(void) {
    rcc_clock_setup_in_hse_8mhz_out_72mhz();

    rcc_periph_clock_enable(RCC_TIM2);

    rcc_periph_clock_enable(RCC_GPIOA);

    gpio_set_mode(
        GPIOA,
        GPIO_MODE_OUTPUT_50_MHZ,
        GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
        GPIO0
    );

    rcc_periph_reset_pulse(RST_TIM2);

	timer_set_mode(
		TIM2, 
		TIM_CR1_CKD_CK_INT,
		TIM_CR1_CMS_EDGE,
		TIM_CR1_DIR_UP
    );

    timer_disable_preload(TIM2);

    timer_continuous_mode(TIM2);

    double t_pwm = 20e-3;
    double h_pwm = 0.5e-3;
    double delta_h_pwm = 0.5e-3;
    double resolution_pwm = 1. / 20e3;
    double f_pwm = 1 / t_pwm;
    double duty_pwm = h_pwm / t_pwm;
    double f_overflow = f_pwm;
    double ARR = 1. / resolution_pwm - 1;
    double f_timer = rcc_apb1_frequency * 2;
    double f_counter = f_overflow * (ARR + 1);
    double PSC = f_timer / f_counter - 1;
    double CRR = duty_pwm * (ARR + 1);

	timer_set_prescaler(TIM2, (uint32_t)PSC);
	timer_set_period(TIM2, (uint32_t)ARR);

    timer_set_oc_value(TIM2, TIM_OC1, (uint32_t)(CRR));

    timer_set_oc_mode(TIM2, TIM_OC1, TIM_OCM_PWM1);

    timer_enable_oc_output(TIM2, TIM_OC1);

    timer_enable_counter(TIM2);

    delta_h_pwm = 0.5e-3;
    h_pwm = 0.5e-3 - delta_h_pwm;
    while(1) {
        h_pwm += delta_h_pwm;

        if (h_pwm > 2.5e-3) {
            delta_h_pwm *= -1;
            h_pwm = 2.5e-3 + delta_h_pwm;
        }
        if (h_pwm < 0.5e-3) {
            delta_h_pwm *= -1;
            h_pwm = 0.5e-3 + delta_h_pwm;
        }

        double duty_pwm = h_pwm / t_pwm;
        double ARR = 1. / resolution_pwm - 1;
        double CRR = duty_pwm * (ARR + 1);

        timer_set_oc_value(TIM2, TIM_OC1, (uint32_t)(CRR));

        for (int i = 0; i < 10000000; ++i)
            __asm__("nop");
   }

   return 0;
}

/*
led呼吸灯
led负极接A0，正极接电源
*/

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>

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

    double f_pwm = 1000;
    double duty_pwm = 1.;
    double resolution_pwm = 0.01;
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

    double delta_duty_pwm = 0.01;
    duty_pwm = 0 - delta_duty_pwm;
    while (true) {
        duty_pwm += delta_duty_pwm;

        if (duty_pwm > 1.0) {
            delta_duty_pwm *= -1;
            duty_pwm = 1 + delta_duty_pwm;
        }
        if (duty_pwm < 0) {
            delta_duty_pwm *= -1;
            duty_pwm = 0 + delta_duty_pwm;
        }

        CRR = duty_pwm * (ARR + 1);
        timer_set_oc_value(TIM2, TIM_OC1, (uint32_t)(CRR));

        for (uint32_t i = 0; i < 100000; ++i)
            __asm__("nop");
    }

}

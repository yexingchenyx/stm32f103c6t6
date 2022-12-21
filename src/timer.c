/*
oled定时显示计数
接线同oled.c
*/

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>
#include <u8x8.h>
#include <stdarg.h>
#include <stdio.h>

/* Initialize I2C1 interface */
static void i2c_setup(void) {
	/* Enable GPIOB and I2C1 clocks */
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_AFIO);
	rcc_periph_clock_enable(RCC_I2C1);

	/* Set alternate functions for SCL and SDA pins of I2C1 */
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
				  GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN,
				  GPIO_I2C1_SCL | GPIO_I2C1_SDA);

	/* Disable the I2C peripheral before configuration */
	i2c_peripheral_disable(I2C1);

	/* APB1 running at 36MHz */
	i2c_set_clock_frequency(I2C1, I2C_CR2_FREQ_36MHZ);

	/* 400kHz - I2C fast mode */
	i2c_set_fast_mode(I2C1);
	i2c_set_ccr(I2C1, 0x1e);
	i2c_set_trise(I2C1, 0x0b);

	/* And go */
	i2c_peripheral_enable(I2C1);
}

static uint8_t u8x8_gpio_and_delay_cm3(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
	switch(msg) {
	case U8X8_MSG_GPIO_AND_DELAY_INIT:
		i2c_setup();  /* Init I2C communication */
		break;

	default:
		u8x8_SetGPIOResult(u8x8, 1);
		break;
	}

	return 1;
}

/* I2C hardware transfer based on u8x8_byte.c implementation */
static uint8_t u8x8_byte_hw_i2c_cm3(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
	static uint8_t buffer[32];   /* u8g2/u8x8 will never send more than 32 bytes */
	static uint8_t buf_idx;
	uint8_t *data;

	switch(msg) {
	case U8X8_MSG_BYTE_SEND:
		data = (uint8_t *)arg_ptr;
		while(arg_int > 0) {
			buffer[buf_idx++] = *data;
			data++;
			arg_int--;
		}
		break;
	case U8X8_MSG_BYTE_INIT:
		break;
	case U8X8_MSG_BYTE_SET_DC:
		break;
	case U8X8_MSG_BYTE_START_TRANSFER:
		buf_idx = 0;
		break;
	case U8X8_MSG_BYTE_END_TRANSFER:
		i2c_transfer7(I2C1, 0x3C, buffer, buf_idx, NULL, 0);
		//i2c_transfer7(I2C1, 0x3D, buffer, buf_idx, NULL, 0);
		break;
	default:
		return 0;
	}
	return 1;
}

u8x8_t u8x8_i;
static void oled_init(void) {
	u8x8_t *u8x8 = &u8x8_i;

	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	// 灯
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);

	// oled res 复位，开始低电平，等待，高电平
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO15);
	gpio_clear(GPIOC, GPIO15);
	for(int i = 0; i < 1000000; i++)
		__asm__("nop");
	gpio_set(GPIOC, GPIO15);

	u8x8_Setup(u8x8, u8x8_d_ssd1306_128x64_noname, u8x8_cad_ssd13xx_fast_i2c, u8x8_byte_hw_i2c_cm3, u8x8_gpio_and_delay_cm3);

	u8x8_InitDisplay(u8x8);
	u8x8_SetPowerSave(u8x8,0);
	u8x8_SetFont(u8x8, u8x8_font_7x14B_1x2_f);

	u8x8_ClearDisplay(u8x8);
	u8x8_DrawString(u8x8, 1,1, "Hello world!");
}

static void oled_print(int x, int y, const char* format, ...) {
    char buf[100];

    va_list vl;
    va_start(vl, format);

    vsnprintf(buf, sizeof( buf), format, vl);

    va_end(vl);

	u8x8_ClearDisplay(&u8x8_i);
	u8x8_DrawString(&u8x8_i, x, y, buf);
    gpio_toggle(GPIOC, GPIO13);
}

int main(void) {

	rcc_clock_setup_in_hse_8mhz_out_72mhz();

    oled_init();

	rcc_periph_clock_enable(RCC_TIM2);
	rcc_periph_reset_pulse(RST_TIM2);

	timer_set_mode(
		TIM2, 
		TIM_CR1_CKD_CK_INT, // no divider
		TIM_CR1_CMS_EDGE, // alignment edge
		TIM_CR1_DIR_UP); // direction up
	
	timer_disable_preload(TIM2);

	timer_continuous_mode(TIM2);

	double f_timer = rcc_apb1_frequency * 2; // 72 MHz
	double f_overflow = 1; // 1 Hz
	double f_counter = 7200; // // 7200 Hz
	double PSC = f_timer / f_counter - 1; // 10000 - 1
	double ARR = f_counter / f_overflow - 1; // 7200 - 1
	
	/*
	预分频系数PSC, 计数器频率为输入频率 / (PSC + 1)
 	自动重装寄存器ARR，计数器从0开始计数，等于此值overflow，e.g. ARR=2时，计数为0,1,2(overflow),0,1,2(overflow),0,...
	f_counter = f_timer / (PSC + 1)
	f_overflow = f_counter / (ARR + 1) = f_timer / (PSC + 1) / (ARR + 1)
	*/
	timer_set_prescaler(TIM2, (uint32_t)PSC);
	timer_set_period(TIM2, (uint32_t)ARR);

	timer_enable_irq(TIM2, TIM_DIER_UIE);
	nvic_enable_irq(NVIC_TIM2_IRQ);

	timer_enable_counter(TIM2);

    while (1)
    {
    }
    
    return 0;
}

int n = 0;
void tim2_isr(void)
{
  if (timer_get_flag(TIM2, TIM_SR_UIF)) // 更新中断标志位
  {
    timer_clear_flag(TIM2, TIM_SR_UIF); // 清空标志位
	oled_print(1, 1, "%d", n++);
  }
}

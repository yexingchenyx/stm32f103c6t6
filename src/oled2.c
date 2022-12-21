/*
7pin SPI/I2C ssd1306 128x64 oled屏显示，I2C模式需将R3电阻移到R2，R8短接
LED负极接C13，正极接电源
oled GND接地，VCC接3.3V，DO(SCL)接B6，D1(SDA)接B7，RES接C15，DC接地(接地从机地址0x3C，接高电平从机地址0x3D)，CS接地
https://github.com/StanislavLakhtin/ssd1306_libopencm3/blob/master/src/ssd1306_i2c.c
https://controllerstech.com/stm32-i2c-configuration-using-registers/
*/
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <memory.h>

static void delay(int n) {
	for(int i = 0; i < n; i++)
		__asm__("nop");
}

static void send_command(uint8_t data) {
    i2c_send_start(I2C1); // start condition
    while ((I2C_SR1(I2C1) & I2C_SR1_SB) == 0); // 1: start condition is generated

    i2c_send_7bit_address(I2C1, 0x3C, I2C_WRITE);
    while((I2C_SR1(I2C1) & I2C_SR1_ADDR) == 0); // 1: end of address transmission
    I2C_SR2(I2C1); // clear the ADDR bit

    i2c_send_data(I2C1, 0b00000000); // specify a command
    while((I2C_SR1(I2C1) & I2C_SR1_TxE) == 0); // 1: DR(Data Register) is empty

    i2c_send_data(I2C1, data); // command data
    while((I2C_SR1(I2C1) & I2C_SR1_TxE) == 0); // 1: DR(Data Register) is empty

    i2c_send_stop(I2C1); // stop 
    while((I2C_SR1(I2C1) & I2C_SR1_BTF) == 0); // 1: end of last data transmission
}

int main(void) {
    rcc_clock_setup_in_hse_8mhz_out_72mhz();

    rcc_periph_clock_enable(RCC_GPIOC); // led & oled RES
	rcc_periph_clock_enable(RCC_GPIOB); // GPIOB for I2C1
    rcc_periph_clock_enable(RCC_I2C1); // oled SCL, SDA

    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13); // led
    gpio_set(GPIOC, GPIO13); // led off

    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO15); // oled RES
    gpio_clear(GPIOC, GPIO15); // 0
	for(int i = 0; i < 10000; i++) __asm__("nop"); // delay
    gpio_set(GPIOC, GPIO15); // 1, reset oled

    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN, GPIO_I2C1_SCL | GPIO_I2C1_SDA); // oled SCL, SDA
    gpio_set(GPIOB, GPIO_I2C1_SCL | GPIO_I2C1_SDA); // 1, need pull-up resistor

    i2c_peripheral_disable(I2C1); // disable i2c before configuration

    i2c_reset(I2C1);

    i2c_set_clock_frequency(I2C1, I2C_CR2_FREQ_36MHZ);

    i2c_set_fast_mode(I2C2);

	i2c_set_ccr(I2C1, 0x1e);

	i2c_set_trise(I2C1, 0x0b);

    i2c_peripheral_enable(I2C1); // enable i2c

    // switch off
    send_command(0xAE);

    // oscillator frequency
    send_command(0xd5);
    send_command(0x80);

    // multiple ratio: height - 1
    send_command(0xa8);
    send_command(64 - 1);
    
    // inverse: false
    send_command(0xA6);

    // charge pump
    send_command(0x8D);
    send_command(0x14);

    // contrast
    send_command(0x81);
    send_command(0x3F);

    // precharge
    send_command(0xd9);
    send_command(0x22);

    // COM pins hardware configuration
    send_command(0xda);
    send_command(0b00110010 & 0x02);

    // adjust Vcom deselect level
    send_command(0xdb);
    send_command(0x20);
    
    // display on
    send_command(0xA4);

    // switch on
    send_command(0xAF);

    // memory addressing mode: horizontal
    send_command(0x20);
    send_command(0b00);

    // column address scope
    send_command(0x21);
    send_command(0);
    send_command(128 - 1); // width - 1

    // page address scope
    send_command(0x22);
    send_command(0);
    send_command(64 / 8 - 1); // height / 8 - 1

    i2c_send_start(I2C1); // start condition
    while ((I2C_SR1(I2C1) & I2C_SR1_SB) == 0); // 1: start condition is generated

    i2c_send_7bit_address(I2C1, 0x3C, I2C_WRITE);
    while((I2C_SR1(I2C1) & I2C_SR1_ADDR) == 0); // 1: end of address transmission
    I2C_SR2(I2C1); // clear the ADDR bit

    i2c_send_data(I2C1, 0b01000000); // specify data
    while((I2C_SR1(I2C1) & I2C_SR1_TxE) == 0); // 1: DR(Data Register) is empty

    uint8_t data[128 * 64 / 8]; // width * height / sizeof(uint8_t) bits
    for (uint16_t i = 0; i < sizeof(data); ++i)
        data[i] = 0x00;
    data[0] = 0b11111111;
    data[1] = 0b11111110;
    data[2] = 0b11111100;
    data[3] = 0b11111000;
    data[4] = 0b11110000;
    data[5] = 0b11100000;
    data[6] = 0b11000000;
    data[7] = 0b10000000;

    for (uint16_t i = 0; i < sizeof(data); ++i) {
        i2c_send_data(I2C1, data[i]); // command data
        while((I2C_SR1(I2C1) & I2C_SR1_TxE) == 0); // 1: DR(Data Register) is empty
    }

    i2c_send_stop(I2C1); // stop 
    while((I2C_SR1(I2C1) & I2C_SR1_BTF) == 0); // 1: end of last data transmission

	while(true) {
		gpio_toggle(GPIOC, GPIO13);

		delay(8000000);
	}

    return 0;
}

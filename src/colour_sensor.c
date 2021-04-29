#include "header.h"

// I2C

void i2c_init (void) {
	// initializes I2C in Master mode w/ 100kHz clock

	TRISC3 = 1; /* SDA and SCL as input pin */
	TRISC4 = 1; /* these pins can be configured either i/p or o/p */
	SSPSTAT |= 0x80; /* Slew rate disabled */
	SSPCON1 = 0x28; /* SSPEN = 1, I2C Master mode, clock = FOSC/(4 * (SSPADD + 1)) */
	SSPADD = (_XTAL_FREQ / (4 * 100000)) - 1; /* 100kHz */
}

void i2c_start () {
	// sends start condition

	SEN = 1; /* Start condition enabled */
	while (SEN); /* automatically cleared by hardware */
	/* wait for start condition to finish */
}

void i2c_stop () {
	// sends stop condition

	PEN = 1; /* Stop condition enabled */
	while (PEN); /* Wait for stop condition to finish */
	/* PEN automatically cleared by hardware */
}

void i2c_restart () {
	// sends restart condiiton

	RSEN = 1; /* Repeated start enabled */
	while (RSEN); /* wait for condition to finish */
}

void i2c_ack () {
	// sends ACK acknowledge bit

	ACKDT = 0; /* Acknowledge data bit, 0 = ACK */
	ACKEN = 1; /* Ack data enabled */
	while (ACKEN); /* wait for ack data to send on bus */
}

void i2c_nack () {
	// sends NACK acknowledge bit

	ACKDT = 1; /* Acknowledge data bit, 1 = NAK */
	ACKEN = 1; /* Ack data enabled */
	while (ACKEN); /* wait for ack data to send on bus */
}

void i2c_wait () {
	// waits for transfers to clear

	while ((SSPCON2 & 0x1F) | (SSPSTAT & 0x04)) {
	}
	/* wait for any pending transfer */
}

void i2c_send (unsigned char dat) {
	// sends  on i2c

	SSPBUF = dat; /* Move data to SSPBUF */
	while (BF); /* wait till complete data is sent from buffer */
	i2c_wait(); /* wait for any pending transfer */
}

unsigned char i2c_read (void) {
	// reads data from i2c
	if (((SSPCON1 & 0x0F) == 0x08) || ((SSPCON1 & 0x0F) == 0x0B)) //master mode only
		SSPCON2bits.RCEN = 1; // enable master for 1 byte reception
	while (!BF); // wait until byte received 
	unsigned char data = SSPBUF;
	return data; // return with read byte 
}


// colour sensor

void colour_sensor_write (unsigned char reg, unsigned char val) {
	// writes  to  in the colour sensor

	i2c_start(); // start
	__delay_ms(COLOUR_SENSOR_DELAY);
	i2c_send(0x29 << 1); // address
	__delay_ms(COLOUR_SENSOR_DELAY);
	i2c_send(0x80 | reg); // command
	__delay_ms(COLOUR_SENSOR_DELAY);
	i2c_send(val); // data
	__delay_ms(COLOUR_SENSOR_DELAY);
	i2c_stop(); // stop
	__delay_ms(COLOUR_SENSOR_DELAY);
}

unsigned char colour_sensor_read (unsigned char reg) {
	// reads value in  in colour sensor

	i2c_start(); // start
	__delay_ms(COLOUR_SENSOR_DELAY);
	i2c_send(0x29 << 1); // address
	__delay_ms(COLOUR_SENSOR_DELAY);
	i2c_send(0x80 | reg); // command
	__delay_ms(COLOUR_SENSOR_DELAY);
	i2c_restart();
	__delay_ms(COLOUR_SENSOR_DELAY);
	i2c_send((0x29 << 1) | 1); // address
	__delay_ms(COLOUR_SENSOR_DELAY);
	unsigned char ret = i2c_read(); // read byte
	__delay_ms(COLOUR_SENSOR_DELAY);
	i2c_stop(); // stop
	__delay_ms(100);
	return ret;
}

unsigned int colour_sensor_read_16 (unsigned char reg) {
	// reads a 16-bit value beginning at  in the colour sensor

	unsigned char l = colour_sensor_read(reg);
	unsigned char h = colour_sensor_read(reg + 1);
	return ((unsigned int) h << 8) | l;
}

void colour_sensor_data (unsigned int * c, unsigned int * r, unsigned int * g, unsigned int * b) {
	// sets the raw data for the c, r, g, b channels

	*c = colour_sensor_read_16(0x14);
	*r = colour_sensor_read_16(0x16);
	*g = colour_sensor_read_16(0x18);
	*b = colour_sensor_read_16(0x1A);
}

void colour_sensor_enable () {
	// enables the colour sensor (PON and AEN)

	colour_sensor_write(0, 1);
	__delay_ms(3);
	colour_sensor_write(0, 3);
	__delay_ms(COLOUR_SENSOR_DELAY);
	colour_sensor_write(1, 0x00); // integration time 700ms
	__delay_ms(COLOUR_SENSOR_DELAY);
	colour_sensor_write(0xF, 0x00); // gain 1x
	__delay_ms(COLOUR_SENSOR_DELAY);
}

void colour_sensor_disable () {
	// disables the colour sensor (!PON and !AEN)

	colour_sensor_write(0, 0);
	__delay_ms(COLOUR_SENSOR_DELAY);
}

unsigned int colour_temperature (unsigned int r, unsigned int g, unsigned int b) {
	// calculates the colour temperature [K] using r, g, b values

	float X, Y, Z; /* RGB to XYZ correlation      */
	float xc, yc; /* Chromaticity co-ordinates   */
	float n; /* McCamy's formula            */
	float cct;

	/* 1. Map RGB values to their XYZ counterparts.    */
	/* Based on 6500K fluorescent, 3000K fluorescent   */
	/* and 60W incandescent values for a wide range.   */
	/* Note: Y = Illuminance or lux                    */
	X = (-0.14282F * r) + (1.54924F * g) + (-0.95641F * b);
	Y = (-0.32466F * r) + (1.57837F * g) + (-0.73191F * b);
	Z = (-0.68202F * r) + (0.77073F * g) + (0.56332F * b);

	/* 2. Calculate the chromaticity co-ordinates      */
	xc = (X) / (X + Y + Z);
	yc = (Y) / (X + Y + Z);

	/* 3. Use McCamy's formula to determine the CCT    */
	n = (xc - 0.3320F) / (0.1858F - yc);

	/* Calculate the final CCT */
	cct = (449.0F * pow(n, 3)) + (3525.0F * pow(n, 2)) + (6823.3F * n) + 5520.33F;

	/* Return the results in degrees Kelvin */
	return (unsigned int) cct;

// Rapsberry Pi: I2C in C - Versione 0.61 - Luglio 2013
// Copyright (c) 2013, Vincenzo Villa (http://www.vincenzov.net)
// Creative Commons | Attribuzione-Condividi allo stesso modo 3.0 Unported.
// Creative Commons | Attribution-Share Alike 3.0 Unported
// http://www.vincenzov.net/tutorial/elettronica-di-base/RaspberryPi/i2c-c.htm

// Compile:  gcc LM92.c -std=c99 -o LM92
// Run as user with R&W right on /dev/i2c-* (NOT ROOT!)
// vv@vvrpi ~ $ ./LM92 
// Using LM92 - I2C 

#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <linux/types.h>
#include <linux/i2c-dev.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>

#include "tad.h"

static const char *device = "/dev/i2c-1";	// I2C bus
static int initialized = 0;

extern int usleep (__useconds_t __useconds);

struct Coefficients
{
	int C1, C2, C3, C4, C5, C6, C7;
	int A, B, C, D;
};

static void exit_on_error (const char *s)	// Exit and print error code
{
 	perror(s);
  	abort();
} 

void msleep(int msec)
{
usleep(msec*1000); return;
	unsigned long currentTimems; 
	currentTimems=millis();
	while (millis()-currentTimems<msec)
	{
	}
}

void TranslateSensorReading( struct Coefficients *coefficients, int  data1, int  data2, double *temperature, double *pressure )
{
	double dUT, OFF, SENS, X;

	////////////////////////////////////////////////////////////////
	// Calculate temperature and pressure based on calibration data
	////////////////////////////////////////////////////////////////

	// Step 0. Reverse data bytes.

	//data1 = data1 << 8 | (data1 >> 8 & 0xFF);
	//data2 = data2 << 8 | (data2 >> 8 & 0xFF);

	// Step 1. Get temperature value.

	// data2 >= C5 dUT= data2-C5 - ((data2-C5)/2^7) * ((data2-C5)/2^7) * A / 2^C
	if (data2 >= coefficients->C5)
	{
		dUT = data2 - coefficients->C5 - ((data2 - coefficients->C5) / 128.0 * ((data2 - coefficients->C5) / 128.0) * coefficients->A / (1 << coefficients->C));
	}
	// data2 <  C5 dUT= data2-C5 - ((data2-C5)/2^7) * ((data2-C5)/2^7) * B / 2^C
	else
	{
		dUT = data2 - coefficients->C5 - ((data2 - coefficients->C5) / 128.0 * ((data2 - coefficients->C5) / 128.0) * coefficients->B / (1 << coefficients->C));
	}

	// Step 2. Calculate offset, sensitivity and final pressure value.

	// OFF=(C2+(C4-1024)*dUT/2^14)*4
	OFF = (coefficients->C2 + (coefficients->C4-1024) * dUT / 16384) * 4;
	// SENS = C1+ C3*dUT/2^10
	SENS = coefficients->C1 + coefficients->C3 * dUT / 1024;
	// X= SENS * (D1-7168)/2^14 - OFF
	X = SENS * (data1 - 7168) / 16384 - OFF;
	// P=X*10/2^5+C7
	*pressure = (X * 10.0 / 32 + coefficients->C7) / 10.0;

	// Step 3. Calculate temperature

	// T = 250 + dUT * C6 / 2 ^ 16-dUT/2^D
	*temperature = (250 + dUT * coefficients->C6 / 65536 - dUT / (1 << coefficients->D)) / 10;
}

int ReadSensorCalibration(struct Coefficients *coefficients)
{
	int fd, err;
	__u8 block[18];

       	// Open I2C device
       	if ((fd = open(device, O_RDWR)) < 0) exit_on_error ("Can't open I2C device");

	// Set I2C slave address
	if (ioctl(fd, I2C_SLAVE, EEPROM_ADDR) < 0) exit_on_error ("Can't talk to slave");
        
	digitalWrite(XCLR_PIN,0);

	err = i2c_smbus_read_i2c_block_data (fd, COEFF_REG, 18, block);

	if(err < 0) {
		perror ("Can't read calibration data");
		return 0;
	} else {
		coefficients->C1 = (block[0] << 8) | block[1];
		coefficients->C2 = (block[2] << 8) | block[3];
		coefficients->C3 = (block[4] << 8) | block[5];
		coefficients->C4 = (block[6] << 8) | block[7];
		coefficients->C5 = (block[8] << 8) | block[9];
		coefficients->C6 = (block[10] << 8) | block[11];
		coefficients->C7 = (block[12] << 8) | block[13];
		coefficients->A = block[14];
		coefficients->B = block[15];
		coefficients->C = block[16];
		coefficients->D = block[17];
	}

        close(fd);

	printf("C1:	%d\n", 	coefficients->C1  );
	printf("C2:	%d\n", 	coefficients->C2 );
	printf("C3:	%d\n", 	coefficients->C3 );
	printf("C4:	%d\n", 	coefficients->C4 );
	printf("C5:	%d\n", 	coefficients->C5 );
	printf("C6:	%d\n", 	coefficients->C6 );
	printf("C7:	%d\n", 	coefficients->C7 );
	printf("A :	%d\n", 	coefficients->A );
	printf("B :	%d\n", 	coefficients->B );
	printf("C :	%d\n", 	coefficients->C );
	printf("D :	%d\n", 	coefficients->D );

	return 1;
}

static struct Coefficients coefficients;
static int baroFd;

void initBarometer()
{
	/* set up communication with the barometer
	if (wiringPiSetupGpio () == -1)
	{
		perror( "Can't initialise wiringPi" );
		return -2;
	} */

	pinMode(XCLR_PIN, OUTPUT);

	if(!ReadSensorCalibration( &coefficients )) {
		perror("Calibration failed");
		return;
	}

       	// Open I2C device
       	if ((baroFd = open(device, O_RDWR)) < 0) exit_on_error ("Can't open I2C device");

	// Set I2C slave address
	if (ioctl(baroFd, I2C_SLAVE, ADC_ADDRESS) < 0) exit_on_error ("Can't talk to slave");

	initialized = 1;
}

int readBarometer(struct sensorGrid *grid)
{
	int err; 
	
        int  data1, data2;
        double   temperature, pressure;
        
	digitalWrite(XCLR_PIN,1);
	err = i2c_smbus_write_byte_data(baroFd, 0xFF, 0xF0) ;
	msleep(120);
	data1 = i2c_smbus_read_word_data(baroFd, 0xFD) ;
	err = i2c_smbus_write_byte_data(baroFd, 0xFF, 0xE8) ;
	msleep(120);
	data2 = i2c_smbus_read_word_data(baroFd, 0xFD) ;
	digitalWrite(XCLR_PIN,0);

	data1 = (data1 & 0xFF) << 8 | ((data1 >> 8) & 0xFF);
	data2 = (data2 & 0xFF) << 8 | ((data2 >> 8) & 0xFF);
	TranslateSensorReading(&coefficients, data1, data2, &temperature, &pressure );
	grid->temperature = temperature;
	grid->pressure = pressure;

//	printf("D1:\t %d, D2:\t%d, P: %f\tT: %f\n", data1, data2, pressure, temperature );
	fflush(stdout);

	return (0);
}

void *readBarometerThread(void *args)
{
	if(!initialized) return;
	while(1) {
		readBarometer(args);
		msleep(750);
	}
}

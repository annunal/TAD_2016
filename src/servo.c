#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <wiringPi.h>
#include <pthread.h>
#include <ctype.h>
#include <fcntl.h>
#include "tad.h"

static int servoPin = 0;
static FILE *ttyFP = NULL;
static char command[32];

void initServo(int pin, FILE * inputFp)
{
	servoPin = pin;
	ttyFP = inputFp;
	pinMode(7, OUTPUT);
	pinMode(8, OUTPUT);
}

void turnServoOn()
{
fprintf(stderr, "TURNING ON SERVO\n");
	digitalWrite(servoPin,0);
}

void turnServoOff()
{
fprintf(stderr, "TURNING OFF SERVO\n");
	digitalWrite(servoPin,1);
}

void *readSerialThread(void *args)
{
	while(1) {
		int p = 0;
		char c = fgetc(ttyFP);
		switch(c)
		{
			case '0':
				digitalWrite( 7, 0);
			break;
			case '1':
				digitalWrite( 7, 1);
			break;
			case '2':
				digitalWrite( 8, 0);
			break;
			case '3':
				digitalWrite( 8, 1);
			break;
			case 10:
			case 13:
			break;
			default:
				sleep(1);
			break;
		}
	}
}

void startServo()
{
	pthread_t serialThread;
        int s = 0;
        pthread_attr_t attr;

	if( !servoPin || !ttyFP ) return;

        s = pthread_attr_init(&attr);
        if (s) perror("pthread_attr_init");

        if(pthread_create(&serialThread, NULL, readSerialThread, NULL))
                perror("Unable to start serial thread");

        /* Destroy the thread attributes object, since it is no longer needed */
        s = pthread_attr_destroy(&attr);
}


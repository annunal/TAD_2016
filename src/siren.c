#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <wiringPi.h>
#include <pthread.h>
#include "tad.h"

#define SIREN_PROP 9

static int sirenPin;

static char *sirenStatus[] = {
                "OFF","ON", NULL
        };

void initSiren(int pin)
{
	sirenPin = pin;
	pinMode(pin, OUTPUT);
}

void turnSirenOn()
{
	digitalWrite(sirenPin,0);
	status[SIREN_PROP] = sirenStatus[1];
}

void turnSirenOff()
{
	digitalWrite(sirenPin,1);
	status[SIREN_PROP] = sirenStatus[0];
}

void *delayedTurnSirenOff(void * arg)
{
	int delay = (int)arg;
	sleep(delay);
	turnSirenOff();
	writeStatus();

	return NULL;
}

/* Call this function with three possible parameters: on, off or an integer number of seconds */
void siren(char *ptr)
{
	if(!strcasecmp("on", ptr)) {
		turnSirenOn();
	} else
	if(!strcasecmp("off", ptr)) {
		turnSirenOff();
	} else {
		pthread_t thread;
		int s = 0, delay = 0;
	        pthread_attr_t attr;

		s = pthread_attr_init(&attr);
		if (s) perror("pthread_attr_init");

		while(*ptr) delay = 10 * delay + *ptr++ - '0';
		/* turn on the siren, but schedule a delayed turn off */
       		if(pthread_create(&thread, NULL, delayedTurnSirenOff, (void*)delay))
			turnSirenOn();

		/* Destroy the thread attributes object, since it is no longer needed */
		s = pthread_attr_destroy(&attr);
	}
}


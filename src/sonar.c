#include <stdlib.h>
#include <wiringPi.h>
#include <sys/time.h>

#include "tad.h"

/* any value other than LOW or HIGH */
#define TIMEOUT 999
// Microseconds (uS) it takes sound to travel round-trip 1 inch (2 inches total), uses integer to save compiled code space.
#define US_ROUNDTRIP_IN 146.0
// Microseconds (uS) it takes sound to travel round-trip 1cm (2cm total), uses integer to save compiled code space.
#define US_ROUNDTRIP_CM 57.0

void initSonar()
{
	pinMode(TRIGGER_PIN, OUTPUT);
	pinMode(ECHO_PIN, INPUT);
}

float Convert(unsigned int echoTime, double conversionFactor)
{
	float dist=((echoTime + conversionFactor / 2.0) / conversionFactor);
	//printf("%i  %f  D=%f\n",echoTime,conversionFactor,dist);
	return dist / 100.0; // From cm to m
}

int waitforpin(int pin, int level, int timeout)
{
        struct timeval now, start;
        int done;
        long micros;
        gettimeofday(&start, NULL);
        micros = 0;
        done=0;
        while (!done)
        {
                gettimeofday(&now, NULL);
                if (now.tv_sec > start.tv_sec) micros = 1000000L; else micros = 0;
                micros = micros + (now.tv_usec - start.tv_usec);
                if (micros > timeout) done=1;
                if (digitalRead(pin) == level) done = 1;
        }
        return micros;
}

void *readDistanceThread(void *args)
{
	struct sensorGrid *readings = (struct sensorGrid *)args;
	float measure;

	while(1) {
		int pulsewidth;
		/* trigger reading */
		digitalWrite(TRIGGER_PIN, HIGH);
		waitforpin(ECHO_PIN, TIMEOUT, 10); /* wait 10 microseconds */
		digitalWrite(TRIGGER_PIN, LOW);
		/* wait for reading to start */
		waitforpin(ECHO_PIN, HIGH, 5000); /* 5 ms timeout */
		
		if (digitalRead(ECHO_PIN) == HIGH)
		{
			pulsewidth = waitforpin(ECHO_PIN, LOW, 60000L); /* 60 ms timeout */
			if (digitalRead(ECHO_PIN) == LOW)
			{
				/* valid reading code */
				//printf("echo at %f cm\n", Convert(pulsewidth, US_ROUNDTRIP_CM));
				measure = Convert(pulsewidth, US_ROUNDTRIP_CM);
				readings->distance = measure;
				// printf("echo at %f cm\n", measure);
				addMeasure(readings,-1);
			}
			else
			{
				/* no object detected code */
				// printf("echo timed out\n");
			}
		}
		else
		{
			/* sensor not firing code */
			// printf("sensor didn't fire\n");
		}
	}

	return NULL;
}


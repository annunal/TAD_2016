#include <pthread.h>

#include "tad.h"
extern configuration Configuration;

void startSensorsReading(struct sensorGrid *readings) // ,  configuration * Configuration)
{
        pthread_t baroThread, distThread, voltThread, GPSThread;
        int s = 0;
        pthread_attr_t attr;

        s = pthread_attr_init(&attr);
        if (s) perror("pthread_attr_init");

        if(pthread_create(&baroThread, NULL, readBarometerThread, (void*)readings))
                perror("Unable to start barometer thread");

        /* Destroy the thread attributes object, since it is no longer needed */
        s = pthread_attr_destroy(&attr);

        s = pthread_attr_init(&attr);
        if (s) perror("pthread_attr_init");

        if(pthread_create(&distThread, NULL, readDistanceThread, (void*)readings))
                perror("Unable to start distance meter thread");
        
		/* Destroy the thread attributes object, since it is no longer needed */
        s = pthread_attr_destroy(&attr);

        s = pthread_attr_init(&attr);
        if (s) perror("pthread_attr_init");

		if(Configuration.gpsSerial[0] != '\0')
		{
			if(pthread_create(&GPSThread, NULL, readGPSThread, (void*)readings))
					perror("Unable to start distance meter thread");
					
			/* Destroy the thread attributes object, since it is no longer needed */
			s = pthread_attr_destroy(&attr);

			s = pthread_attr_init(&attr);
			if (s) perror("pthread_attr_init");
		}
        if(pthread_create(&voltThread, NULL, readVoltagesThread, (void*)readings))
                perror("Unable to start voltage sensor thread");

        /* Destroy the thread attributes object, since it is no longer needed */
        s = pthread_attr_destroy(&attr);
}

void initSensors()
{
	initBarometer();
	initSonar();
	initVoltage();
	initGPS();
}

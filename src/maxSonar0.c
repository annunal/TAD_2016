#include <unistd.h>
#include <pthread.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/time.h>
#include <ctype.h>
#include <fcntl.h>
#include <pthread.h>
#include "tad.h"

static FILE *ttyFP = NULL;
#define BUFFERSIZE (1024)
static char reading[BUFFERSIZE];

extern configuration Configuration;

FILE * openSerialPort1(char * portName, int rate)
{
    struct termios serial;
    int fd = 0, ret, speed;
    printf("Opening serial port %s\n", portName);

    fd = open(portName, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd == -1) {
        perror(portName);
        return NULL;
    }

    ret = fcntl(fd, F_SETFL, O_RDWR );
    if (tcgetattr(fd, &serial) < 0) {
        perror("Getting configuration");
        return NULL;
    }

    switch(rate) {
	case 9600: speed = B9600;
		break;
	case 57600: speed = B57600;
		break;
    }
    cfsetispeed(&serial, speed);                 /* Set the baud rates to 9600 */
    cfsetospeed(&serial, speed);

    /* Enable the receiver and set local mode */
         serial.c_cflag |= (CLOCAL | CREAD);
         serial.c_cflag &= ~PARENB; /* Mask the character size to 8 bits, no parity */
         serial.c_cflag &= ~CSTOPB;
         serial.c_cflag &= ~CSIZE;
         serial.c_cflag |=  CS8;                              /* Select 8 data bits */
         serial.c_cflag &= ~CRTSCTS;               /* Disable hardware flow control */

                                                                         /* Enable data to be proc essed as raw input */
         serial.c_lflag &= ~(ICANON | ECHO | ISIG);


    tcsetattr(fd, TCSANOW, &serial); // Apply configuration

    return fdopen(fd, "a+");
}

void initSonar()
{
	ttyFP = openSerialPort1(Configuration.sensorSerial, Configuration.sensorRate);
}

void *readDistanceThread(void *args)
{
	struct sensorGrid *readings = (struct sensorGrid *)args;
	float measure, average;
	int nSamples;
	char *s, *p, *end = reading + BUFFERSIZE;
	if (ttyFP==0) return NULL;
	while(1) {
for(p=reading;p<end;p++) *p=0;
		p = read(ttyFP->_fileno, reading, BUFFERSIZE);
p=reading;
//while(!*p) p++;
//s=p;
//for(;p < end;p++) if(*p=='\r') *p=' ';
//*--p=0;
//p=s;
//printf("%s\n", p);
		average = 0.0;
		nSamples = 0;
		while(p < end)
		{
			while(*p++ != 'R' && p < end);
			while(*p == '0' && p < end) p++;
			if(p < end) {
//printf("%s\n", p);
				measure = atof(p);
//printf("%f ", measure);
				average = average + (measure - average) / ++nSamples;
			}
		}
//printf("\n%f %d\n", average, nSamples);
		readings->distance = average / 1000;
		addMeasure(readings);
		sleep(1);
	}

	return NULL;
}


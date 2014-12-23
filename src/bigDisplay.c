#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

#include "tad.h"

int ttyFd;
struct termios serial;

void initBigDisplay ()
{
	ttyFd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);
	if(ttyFd == -1) {
		perror("Unable to open serial port");
		return;
	}

	if (tcgetattr(ttyFd, &serial) < 0) {
		perror("Getting configuration");
		return;
	}

	// Set up Serial Configuration
	serial.c_iflag = 0;
	serial.c_oflag = 0;
	serial.c_lflag = 0;
	serial.c_cflag = 0;

	serial.c_cc[VMIN] = 0;
	serial.c_cc[VTIME] = 0;

	serial.c_cflag = B9600 | CS8 | CREAD;

	tcsetattr(ttyFd, TCSANOW, &serial); // Apply configuration
}

char protocolBytes[] = { 0x02, 0x01, 0x18, 0x00, 0x20, 0x1F, 0x03, 0x1F, 0x17, 0x03, 0x1F, 0x04 };

void bigDisplay (int linec, char *lines[])
{
	int i, j, first = 1, l, scrolling;
	if(linec > MAX_LINES)
		linec = MAX_LINES;

	for(i = 0; i < MAX_LINES; i++)
	{
		scrolling = 0;
		if( i < linec) {
			l = strlen(lines[i]);
			scrolling = (l > 16);
		}
		write( ttyFd, protocolBytes, 1 );
		if( first ) {
			write( ttyFd, protocolBytes + 1, 1 );
			first = 0;
		}
		if(!scrolling)
			write( ttyFd, protocolBytes + 5, 5 );
		write( ttyFd, protocolBytes + 2, 1 );
		j = 0;
		if( i < linec)
			for( ; j < l; j++) write( ttyFd, lines[i] + j, 1 );

		for( ; j<16; j++ ) write( ttyFd, protocolBytes + 4, 1 );

		if(!scrolling)
			write( ttyFd, protocolBytes + 10, 2 );
	}
	write( ttyFd, protocolBytes + 0, 1 );
	write( ttyFd, protocolBytes + 3, 1 );
}


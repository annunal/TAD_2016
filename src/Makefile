#
# Makefile:
#	TAD - Raspberry Pi TAD Controller
#
#	Copyright (c) 2014 Joint Research Centre
#################################################################################


DEBUG	= -g -O0
#DEBUG	= -O3
CC	= gcc
INCLUDE	= -I/usr/local/include
CFLAGS	= $(DEBUG) -Wall $(INCLUDE) -Winline -pipe -Wno-write-strings -w

LDFLAGS	= -L/usr/local/lib
LIBS    = -lwiringPi -lwiringPiDev -lcurl -lm -l pthread

# Should not alter anything below this line
###############################################################################

INC     =	tad.h tadParameters.h stringutil.h

SRC	=	gps.c barometer.c lcdDisplay.c bigDisplay.c main.c sensors.c siren.c servo.c voltage.c maxSonar.c analysis.c stringutil.c
SRC1	=	gps.c barometer.c lcdDisplay.c bigDisplay.c main.c sensors.c siren.c servo.c voltage.c sonar.c analysis.c stringutil.c

OBJ	=	gps.o barometer.o lcdDisplay.o bigDisplay.o main.o sensors.o siren.o servo.o voltage.o maxSonar.o analysis.o stringutil.o
OBJ1	=	gps.o barometer.o lcdDisplay.o bigDisplay.o main.o sensors.o siren.o servo.o voltage.o sonar.o analysis.o stringutil.o

all:		tad tad1

tad:	$(OBJ)
	@echo [link]
	$(CC) -o $@ $(OBJ) $(LDFLAGS) $(LIBS)
	
tad1:	$(OBJ1)
	@echo [link]
	$(CC) -o $@ $(OBJ1) $(LDFLAGS) $(LIBS)
	
.c.o:
	@echo [CC] $<
	@$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJ) *~ core tags lcd

tags:	$(SRC)
	@echo [ctags]
	@ctags $(SRC)

depend:
	makedepend -Y $(SRC)

# DO NOT DELETE

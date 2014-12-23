#include <unistd.h>
#include <pthread.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/time.h>
#include <ctype.h>
#include <fcntl.h>
#include <pthread.h>
#include "tad.h"

static FILE *ttyGPS = NULL;
#define BUFFERSIZE (1024)
static char reading[BUFFERSIZE];
int si_processed;
extern configuration Configuration;
double CurrentTime_s, LastTimeGPSSave;

void retryToTransmitGPS()
{       return;
        FILE * infile;
        char line[1000];
		char dummy[1000];
		if(!fileExists("retryStoreGPS.txt")) return;
		
		
		system("rm -f retryStoreGPS1.txt");
        if((infile = fopen("retryStoreGPS.txt", "r")) == NULL) {
                printf("File %s does not exists retryStoreGPS.txt\n");
                return;
                }

        int isConnected=TRUE;
        FILE * retryStore1=fopen("retryStoreGPS1.txt", "w");
        while( fgets(line, sizeof(line), infile) != NULL ) {
                //printf("line x: %s\n",line);

                if (isConnected) { 
					printf("RETR: %s\n",line);
					system(line);
					if ( fileContains("outlogGPSwget.txt","EnterData.aspx") || fileContains("outlogGPSwget.txt","EnterAlert.aspx") )
							{ 
							system("rm -f EnterData.aspx*");}
					else if ( fileContains("outlogGPSwget.txt","EnterAlert.aspx") )
							{ 
							system("rm -f EnterAlert.aspx*");}
					else
                        { isConnected=FALSE ;
                          fprintf(retryStore1, "%s\n",line);
                        }
					}
					else
					     fprintf(retryStore1, "%s\n",line);
        }
        fclose(retryStore1);
		fclose(infile);
        if (fileExists("retryStoreGPS1.txt"))
			system("mv retryStoreGPS1.txt retryStoreGPS.txt");
		
        
}
FILE * openSerialPort2(char * portName, int rate)
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

void splitString(char * str, char **res, int * nfields)
{
 char * p    = strtok (str, ",");
int n_spaces = 0, i;

/* split string and append tokens to 'res' */

while (p) {
  n_spaces +=1;
  //res = realloc (res, sizeof (char*) * ++n_spaces);
 //if (res == NULL)
 //   exit (-1); /* memory allocation failed */

  res[n_spaces-1] = p;
  p = strtok (NULL, ",");
}

/* realloc one extra element for the last NULL */

//res = realloc (res, sizeof (char*) * (n_spaces+1));
res[n_spaces] = 0;

/* print the result */

//for (i = 0; i < (n_spaces+1); ++i)
//  printf ("res[%d] = %s\n", i, res[i]);

/* free the memory allocated */
*nfields=n_spaces;
}
void initGPS()
{
	ttyGPS = openSerialPort2(Configuration.gpsSerial, Configuration.gpsRate);
	if (ttyGPS==0)
	{  printf("GPS does not exist at %s\n",Configuration.gpsSerial);
	   return;
	   }
	//>>> to enable RXM-RAW
	//unsigned char init1[26] = {0xB5, 0x62, 0x9, 0x1, 0x10, 0x0, 0xC8, 0x16, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x97, 0x69, 0x21, 0x0, 0x0, 0x0, 0x2, 0x10, 0x2B, 0x22, 0x0D, 0x0A};
	//>>> to enable RXM-SFRB
	//unsigned char init2[26] = {0xB5, 0x62, 0x9, 0x1, 0x10, 0x0, 0x0C, 0x19, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x83, 0x69, 0x21, 0x0, 0x0, 0x0, 0x2, 0x11, 0x5F, 0xF0, 0x0D, 0x0A};
	
	unsigned char init[] = {0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0x02, 0x10, 0x01, 0x1D, 0x66, 0x0D, 0x0A};
	//unsigned char poll[10]={0xB5, 0x62, 0x02, 0x10, 0x00, 0x00, 0x12, 0x38, 0x0D, 0x0A};

	int written=write(ttyGPS->_fileno,init,sizeof(init)/sizeof(unsigned char));
	fflush(ttyGPS);
	printf("written=%i\n",written);
	//tcdrain(ttyGPS);
	//write(ttyGPS->_fileno,init1,26);
	//write(ttyGPS->_fileno,init2,26);
	//sleep(1);
	printf("GPS Initialized\n");
}

void *readGPSThread(void *args)
{    if (ttyGPS==0) return;
	struct sensorGrid *readings = (struct sensorGrid *)args;
	float measure, average;
	char * buffer[5*BUFFERSIZE];
	int n;
	char *s, *p, *end = reading + BUFFERSIZE;
    char * row;
	char success = 'N';
	si_processed=0;
	LastTimeGPSSave=-1000;
	
	while(1) {
		//for(p=reading;p<end;p++) *p=0;
		//unsigned char poll[10]={0xB5, 0x62, 0x02, 0x10, 0x00, 0x00, 0x12, 0x38, 0x0D, 0x0A};
		//int written=write(ttyGPS->_fileno,poll,10);
		int i;
		n = read(ttyGPS->_fileno, reading, BUFFERSIZE);
		for(i=0;i<n;i++)
		{
			if(reading[i]==0) reading[i]=32 ;//'\n';
			//printf("%c", reading[i]);
		}
		 
		row = strtok(reading,"\n");
		while (row != NULL)
		{   
		    if(row[0]=='$' && strstr(row,"*") !=0 )
			{
				//printf("%s\n",row);
				if(strstr(row,"$GPGGA") !=0)
				{
					char * res[100];
					int nfields;
					//printf("%s\n",row);
					splitString(row,res, &nfields);
					if (nfields>=12)
					{   float latDeg,lonDeg,lonMin,latMin;
						latDeg=(int) (atof(res[2])/100);
						lonDeg=(int) (atof(res[4])/100);
						latMin=(float) ((atof(res[2])/100)-latDeg)*100/60;
						lonMin=(float) ((atof(res[4])/100)-lonDeg)*100/60;
						
						readings->longitude=lonDeg+lonMin;
						if(res[5]=='W') readings->longitude *= (-1);
						
						readings->latitude =latDeg+latMin;
						if(res[3]=='S') readings->latitude *= (-1);
						
						readings->elevation=(float) atof(res[9]);
						readings->gpstime=res[1];
						FILE * gpslog = fopen("gpslog.log", "a");
						fprintf(gpslog, "%s, %s,  %s,  %f, %f, %f \n",readings->gpstime,res[2],res[4],readings->longitude,readings->latitude,readings->elevation);
						fclose(gpslog);
						//printf("lat-lon=%s  %s\n",res[2],res[4]);
						CurrentTime_s=(double) time(NULL);  // seconds from 1/1/1970					printf("elev=%s\n",res[9]);
						if(CurrentTime_s-LastTimeGPSSave>Configuration.gpsDeltaTSave)
						{
							LastTimeGPSSave=CurrentTime_s;
							char gpsURL0[4096];
							char dummy[4096];
							char dummy1[4096],*t;

							success = 'T';
							
							strcpy(gpsURL0,Configuration.gpsURL);
							sprintf(dummy1,"%f",readings->longitude);
							replace(gpsURL0,"$LON", dummy1,dummy); strcpy(gpsURL0, dummy);
							
							sprintf(dummy1, "%f",readings->latitude);
							replace(gpsURL0,"$LAT", dummy1,dummy); strcpy(gpsURL0, dummy);
							
							sprintf(dummy1, "%f",readings->elevation);
							replace(gpsURL0,"$HEI", dummy1,dummy); strcpy(gpsURL0, dummy);

							//printLog(gpsURL0);
							t = ctrim(gpsURL0);
							sprintf(dummy, "wget \"%s\" -T 10 -nv -o outlogGPSwget.txt", t);

							printf("Storing GPS location: %s\n",dummy);
							system(dummy);

							if ( fileContains("outlogGPSwget.txt","EnterData.aspx"))
							{
								system("rm -f outlogGPSwget.txt* EnterData.aspx*");
								retryToTransmitGPS();  // se sono riuscito a trasmettere, provo a ritrasmettere quelli non riusciti
							}
							else
							{  // se non sono riuscito a trasmettere salvo nel file retrysStore.txt i comandi da dare e poi riprovare.
								FILE * retryStore = fopen(RETRY_BUFFER_GPS, "a");
								//printf("string stored %s\n",dummy);
								if(retryStore)
								{
									fprintf(retryStore, "%s\n",dummy);
									fclose(retryStore);
									success = 'S';
								}
							}
						}
					}
				}
			}
			row=strtok(NULL,"\n");
		}
		//printf("----------------\n");
		sleep(1);
		
	}

	return NULL;
}


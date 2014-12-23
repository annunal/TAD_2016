#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <wiringPi.h>
#include <pthread.h>
#include <termios.h>
#include <curl/curl.h>
#include "tad.h"

/* size of the event structure, not counting name */
#define EVENT_SIZE  (sizeof (struct inotify_event))

/* reasonable guess as to size of 1024 events */
#define BUF_LEN        (1024 * (EVENT_SIZE + 16))

#define CONFIGFILENAME "config.txt"
#define STATUSFILENAME "/var/www/status.txt"
#define LINE1 3

static char *commands[] = {
        "DEFMSG1","DEFMSG2","DEFMSG3","LINE1","LINE2","LINE3","PAGE","SIREN","SPEAKER","DELETE","RESET","PWR","TIMEZONE","NAME","RESTORE",
                        "TIMETO","IPADDR","SUBNET","GATEWAY","TIME","GSM","GSMRES", NULL
	};

static char *properties[] = {
        	"DEFMSG1","DEFMSG2","DEFMSG3","LINE1","LINE2","LINE3", "LINE4","MODE","REL", "SIREN", NULL
	};

char *status[MAX_PROP];

static FILE *otp, *itp;

configuration Configuration;

FILE * openSerialPort(char * portName, int rate)
{
    struct termios serial;
    int fd = 0, ret;
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

    // Set up Serial Configuration
    serial.c_iflag = 0;
    serial.c_oflag = 0;
    serial.c_lflag = 0;
    serial.c_cflag = 0;

    serial.c_cc[VMIN] = 0;
    serial.c_cc[VTIME] = 0;

    serial.c_cflag = rate | CS8 | CREAD;

    tcsetattr(fd, TCSANOW, &serial); // Apply configuration

    return fdopen(fd, "a+");
}

void *navigateAsync(void * args)
{
	CURL *curl;
	CURLcode res;
	char url[2048];
	char *commands = (char*)args;
	char *encoded = NULL;

	curl = curl_easy_init();
	if(curl) {
		encoded = curl_easy_escape(curl, commands, 0);
		sprintf(url, "http://webcritech.jrc.ec.europa.eu/TAD_server/Savecmd.aspx?DeviceName=%s&log=%s&type=LAN", "RASP-01", encoded);
fprintf(stderr, "%s\n", url );
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		/* example.com is redirected, so we tell libcurl to follow redirection */ 
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

		/* Perform the request, res will get the return code */ 
		res = curl_easy_perform(curl);
		/* Check for errors */ 
		if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
		curl_easy_strerror(res));

		/* always cleanup */ 
		curl_free(encoded);
		curl_easy_cleanup(curl);
	}
	return NULL;
}

void logCommandAsync(char * commands)
{
	pthread_t thread;
	int s = 0;
	pthread_attr_t attr;

	s = pthread_attr_init(&attr);
	if (s) perror("pthread_attr_init");

	if(pthread_create(&thread, NULL, navigateAsync, (void*)commands))
		perror("Unable to log remotely");

	/* Destroy the thread attributes object, since it is no longer needed */
	s = pthread_attr_destroy(&attr);
}

void printStatus(FILE *fp)
{
	int idx;

	for(idx = 0; properties[idx]; idx++)
	    if(status[idx])
		fprintf(fp, "%s:%s\n", properties[idx], status[idx] );
}

void writeStatus()
{
	FILE *fs = fopen( STATUSFILENAME, "w+");
	if(fs) {
		printStatus(fs);
		fclose(fs);
	} else {
		perror("Unable to write the status file");
	}
}

void readStatus()
{
	char *line = NULL, *ptr, **propertyName;
	FILE *fp = NULL;
	int len, chars, prop;

	fp = fopen(STATUSFILENAME, "r");
	if(fp) {
		while ((chars = getline(&line, &len, fp)) != -1) {
			ptr=line;
			while(*ptr!=':') {
				*ptr=toupper(*ptr);
				ptr++;
			}
			for( prop=0, ptr = *(propertyName = properties); ptr; ptr=*(++propertyName), prop++)
				if(!strncmp(ptr, line, strlen(ptr)))
				{ // the property is present in the list and can be stored in status
					if(chars > 1 + strlen(ptr)) {
						char *end;
						ptr = line + 1 + strlen(ptr);	// skip property name and colon
						end = ptr + strlen(ptr);
						while( *--end == 13 || *end == 10) *end = '\0';
						status[prop] = strdup(ptr);
					}
					break;
				}
		}

		free(line);
		fclose(fp);
	    } else {
		perror("Unable to open the status file");
	    }
}

void parseLine(char * line)
{
        char *label, *value;

        if (!line || line[0] == '*' || line[0] == ' ' || strlen(line) == 1) return;

        label = ctrim(strtok(line,"=*"));
        value = strtok(NULL,"=*");

        if(value) value = ctrim(value);
        value = replace_str(value,"$EQ","=");
        value = replace_str(value,"$EQ","=");
        value = replace_str(value,"$EQ","=");

        if (strcasecmp(label,"SaveURL") == 0)
        {
                value=replace_str(value,"$IDdevice",Configuration.IDdevice);
                strcpy(Configuration.SaveURL, value);
		            return;
        }
        if (strcasecmp(label,"gpsURL") == 0)
        {
                value=replace_str(value,"$IDdevice",Configuration.IDdevice);
                strcpy(Configuration.gpsURL, value);
		            return;
        }
        if (strcasecmp(label,"AlertURL") == 0)
        {
                value=replace_str(value,"$IDdevice",Configuration.IDdevice);
                strcpy(Configuration.AlertURL, value);
		            return;
        }
                if (strcasecmp(label,"IDdevice") == 0)
        {
                strcpy(Configuration.IDdevice, value);
		            return;
        }

        if (!strcasecmp(label,"title")) {
		strcpy(Configuration.title, value);
		return;
	}
        if (!strcasecmp(label,"location")) {
		strcpy(Configuration.location, value);
		return;
	}
        if (!strcasecmp(label,"watchFolder")) {
		strcpy(Configuration.watchFolder, value);
		return;
	}
        if (!strcasecmp(label,"watchFile")) {
		strcpy(Configuration.watchFile, value);
		return;
	}
        if (!strcasecmp(label,"n300")) {
		Configuration.n300 = atoi(value);
		return;
	}
        if (!strcasecmp(label,"n30")) {
		Configuration.n30 = atoi(value);
		return;
	}
        if (!strcasecmp(label,"Interval")) {
		Configuration.Interval = (float) atof(value);
		return;
	}
        if (!strcasecmp(label,"threshold")) {
		Configuration.threshold = (float) atof(value);
		return;
	}
        if (!strcasecmp(label,"ratioRMS")) {
		Configuration.ratioRMS = (float) atof(value);
		return;
	}
        if (!strcasecmp(label,"AddRMS")) {
		Configuration.AddRMS = (float) atof(value);
		return;
	}
        if (!strcasecmp(label,"backFactor")) {
		Configuration.backFactor = (float) atof(value);
		return;
	}
        if (!strcasecmp(label,"BaudRate")) {
		Configuration.sensorRate = atoi(value);
		return;
	}
        if (!strcasecmp(label,"Serial")) {
		strcpy(Configuration.sensorSerial, value);
		return;
	}
        if (!strcasecmp(label,"sensorMultFac")) {
                Configuration.sensorMultFac = (float) atof(value);
                return;
        }
        if (!strcasecmp(label,"sensorAddFac")) {
                Configuration.sensorAddFac = (float) atof(value);
                return;
        }
        if (!strcasecmp(label,"gpsSerial")) {
		strcpy(Configuration.gpsSerial, value);
		return;
	}
        if (!strcasecmp(label,"gpsRate")) {
		Configuration.gpsRate = atoi(value);
		return;
	}

		if (!strcasecmp(label,"gpsDeltaTSave")) {
		Configuration.gpsDeltaTSave = atoi(value);
		return;
		}
        if (!strcasecmp(label,"Commands")) {
		strcpy(Configuration.commandSerial, value);
		return;
	}
        if (!strcasecmp(label,"batteryPin")) {
		Configuration.batteryPin = atoi(value);
		return;
	}
        if (!strcasecmp(label,"batteryMultiplier")) {
		Configuration.batteryMultiplier = (float) atof(value);
		return;
	}
        if (!strcasecmp(label,"panelPin")) {
		Configuration.panelPin = atoi(value);
		return;
	}
        if (!strcasecmp(label,"panelMultiplier")) {
		Configuration.panelMultiplier = (float) atof(value);
		return;
	}
        if (!strcasecmp(label,"voltageInterval")) {
		Configuration.voltageInterval = atoi(value);
		return;
	}
}

void readConfiguration(char * inputFile)
{
        FILE *infile;
        int lcount = 0;
        char line[400];

        Configuration.ratioRMS = 3;
        Configuration.threshold = 2.;
	Configuration.title[0] = '\0';
	Configuration.location[0] = '\0';
	Configuration.SaveURL[0] = '\0';
	Configuration.watchFolder[0] = '\0';
	Configuration.watchFile[0] = '\0';
	Configuration.sensorSerial[0] = '\0';
	Configuration.gpsSerial[0] = '\0';
	Configuration.commandSerial[0] = '\0';

        printf("Reading file: %s\n",inputFile);
        /* Open the file.  If NULL is returned there was an error */
        if(!(infile = fopen(inputFile, "r"))) {
                printf("Error Opening File.\n");
                return;
        }

        while( fgets(line, sizeof(line), infile) ) {
                /* Get each line from the infile */
                lcount++;
                /* print the line number and data */
                printf("Line %d: %s", lcount, line);
                parseLine(line);
        }

        fclose(infile);  /* Close the file */
        printf("\n\nEnd of Reading file: %s\n",inputFile);
}

int main(int argc, char *argv[])
{
  struct sensorGrid readings;
  int fd, wd, idx;
  char fullname[FILENAME_MAX], tmpname[FILENAME_MAX], logCommands[4096];

  readConfiguration(CONFIGFILENAME);

  //if(argc != 3 && argc != 4) {
    //fprintf(stderr, "Usage: %s foldername filename [output]\n", argv[0]);
    //return 1;
  //}

  //if(argc == 4) {
  if(Configuration.commandSerial[0]) {
    otp = openSerialPort(Configuration.commandSerial, B9600);
    if(otp) {
      itp = otp;
    } else {
      perror( "Unable to append to output device" );
      otp = stdout;
      itp = stdin;
    }
  } else {
    otp = stdout;
    itp = stdin;
  }

  /* read configuration */
  for(idx = 0; idx < MAX_PROP; idx++) status[idx] = NULL;

  readStatus(status);

  /* set up communication with the siren */
  if (wiringPiSetupGpio () == -1)  
  {  
     perror( "Can't initialise wiringPi" );
     return -2;
  }  


  initSiren(SIREN_PIN);  
  initBigDisplay();
  readings.pressure = 0;
  readings.temperature = 0;
  readings.distance = 0;
  initSensors();  
  initServo(SERVO_PIN, itp);  
  startServo();
  
  sleep(1);  // sleep 1 s to have at least 1 sensor value
  initAnalysis();  
  
  // lcd = lcdInit (2, 16, 4, RS, STRB, 0, 1, 2, 3, 4, 5, 6, 7);

  startSensorsReading(&readings);

	if(Configuration.watchFolder[0] && Configuration.watchFile) {
		sprintf(tmpname, "%s/%s", argv[1], argv[2]);
		realpath(tmpname, fullname);
	}

  /* set up file watcher */
  fd = inotify_init ();
  if (fd < 0) {
        perror ("unable to initialize inotify");
        return -1;
  }

  if( Configuration.watchFolder[0] )
	wd = inotify_add_watch (fd, Configuration.watchFolder, IN_CLOSE_WRITE );
  else
	wd = 0;

  if (wd < 0) {
        perror ("unable to add inotify watch");
        return -2;
  }

  /* loop on file change events and execute commands */
  while( 1 ) {
    char buf[BUF_LEN];
    int len, i = 0, process = 0;
    
    len = read (fd, buf, BUF_LEN);
    if (len < 0) {
            if (errno == EINTR)
                    ;	/* need to reissue system call */
            else
                    perror ("error reading inotify stream");
    } else if (!len)
		; /* BUF_LEN too small? */
    
    while (i < len) {
            struct inotify_event *event;
    
            event = (struct inotify_event *) &buf[i];
    
            // printf ("wd=%d mask=%u cookie=%u len=%u\n",
                    // event->wd, event->mask,
                    // event->cookie, event->len);
    
            if (event->len && strcmp(argv[2], event->name) == 0)
	    	process++;
    
            i += EVENT_SIZE + event->len;
    }

    if(process) { // It doesn't matter how many times the file changed, but it is good for debugging
	char *line = NULL, *ptr, *end, **commandName, *displayLines[MAX_LINES];
	FILE *fp = 0;
	int i, len, chars, cmd, statusDirty = 0, cmdPosition = 0;
	process = 0;

	fp=fopen(fullname, "r");
	if(fp) {
		unlink(fullname);	// the name is removed from the filesystem, the file isn't until it is closed
					// if a new file is created, it accumulates event in the watcher, and we'll cycle again next round

		while ((chars = getline(&line, &len, fp)) != -1) {
			ptr = line;
			while(*ptr != ':') {
				*ptr=toupper(*ptr);
				ptr++;
			}

			for( cmd=0, ptr = *(commandName = commands); ptr; ptr=*(++commandName), cmd++)
				if( !strncmp(ptr, line, strlen(ptr)) )
				{ // the command is present in the list and can be sent to the panel: it double checks the data from php
					switch(cmd)
					{
						case 7:		// SIREN
							ptr = line + 1 + strlen(ptr);   // skip property name and colon
                                                        end = ptr + strlen(ptr);
                                                        while( *--end == 13 || *end == 10) *end = '\0';
							siren(ptr);
							if(cmdPosition + strlen(line) < 3072)
								cmdPosition += sprintf(logCommands + cmdPosition, "%s;", line );
							statusDirty = -1;
						break;
						case 8:		// SPEAKER
						break;
						case 9:		// DELETE
						break;
						case 10:	// RESET
						break;
						case 11:	// PWR
						break;
						case 12:	// TIMEZONE
						break;
						case 13:	// NAME
						break;
						case 14:	// RESTORE
						break;
						case 15:	// TIMETO
						break;
						case 16:	// IPADDR
						break;
						case 17:	// SUBNET
						break;
						case 18:	// GSM
						break;
						case 19:	// GSMRES
						break;
						default:	// set property
							if(status[cmd]) free(status[cmd]);
							ptr = line + 1 + strlen(ptr);	// skip property name and colon
							end = ptr + strlen(ptr);	// end of command to retrieve EOL
							while( *--end == 13 || *end == 10) *end = '\0';
							status[cmd] = strdup(ptr);
							statusDirty = -1;
							fprintf(otp, "%s\r\n", line);
							fflush(otp);
							if(cmdPosition + strlen(line) < 3072)
								cmdPosition += sprintf(logCommands + cmdPosition, "%s;", line );
						break;
					}
					break;
				}
		}

		if(line) free(line);
		fclose(fp);
	    } else {
		perror("Unable to open the update file");
	    }
	    if(statusDirty)
		writeStatus();
	    logCommandAsync(logCommands);
	    for( i = 0; i < MAX_LINES; i++)
		displayLines[i] = status[LINE1 + i];
	    len = 4;
	    while(i && !displayLines[--i]) len--;
	    bigDisplay(len, displayLines);
    }
  }

  return 0;
}

void printLog(char *line)
{
	if(otp) {
		fprintf(otp, line);
		fflush(otp);
	}
	char dateName[32];
	time_t now = (time_t) time(0);
	struct tm *gmtm = (struct tm *) gmtime(&now);
        strftime(dateName, sizeof(dateName), "%Y-%m-%d.log", gmtm);

	FILE *logFile = fopen(dateName, "a+");
	if(logFile) {
		fprintf(logFile, line);
		fclose(logFile);
	}
}

void printRetry(char *line)
{
	char dateName[32], currDate[32];
	time_t now = (time_t) time(0);
	struct tm *gmtm = (struct tm *) gmtime(&now);
    strftime(dateName, sizeof(dateName), "retry_%Y-%m-%d.txt", gmtm);
	strftime(currDate, sizeof(currDate), "%d\/%m\/%Y %H:%M:%S", gmtm);

	FILE *logFile = fopen(dateName, "a+");
	fprintf(logFile,"echo \"%s\"\n",currDate);
	fprintf(logFile,"%s\n",line);
	fclose(logFile);
}

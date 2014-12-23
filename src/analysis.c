#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <wiringPi.h>
#include <time.h>

#include "tad.h"
#include "stringutil.h"
#define NOCONNECTION_TIME_MIN     3600.0 // after this time router is rebooted
#define TIME_INTERVAL_ALERT_MIN   5.0 // time interval between two alert messages

extern configuration Configuration;

int lf = 10;
float *y300;
double *x300;
float *yavg30, *yavg300,rms1;
float * diffe;
//time_t CurrentTime_s;
double CurrentTime_s, LastTimeOut, LastTimeAlert_s, LastTimeConnected_s;

float alertSignal1;
int alertValue1;

//unsigned long ;
int Npoints;
double Tot;

float DtAverage,multFac,addFac;
int backFactor;
int max300 = 100;
int max30 = 10;
int index300 = -1;
int index30 = -1;
int index1 = -1;
double alertSignal;
int alertValue;
double time0;
float ratioRMS,addRMS,threshold,offset,gain,Tmax300,Tmax30;
char alertURL[4096], saveURL[4096], location[4096], title[4096];

int stat;
float average(float * yvec, int i0, int i1);
float rootMeanSquare(float * yvec, int i0, int i1, float *ave);
double multi1(double a11, double a21, double a12, double a22);
double multi2(double a11, double a21, double a31, double a12, double a22, double a32, double a13, double a23, double a33);
void addpoint(double *xvect, float *yvect, double x , double value, int * i , int max0, float Tmax30, float Tmax300, int * offset30, int * offset300);
void shiftVect(double *x, float *y, int max0 , float tmax30, float tmax300, int * offset30, int * offset300);
double solve1(double * x, float * y, int i0 , int i1, double xForecast, float * rms ) ;
double solve2(double * x, float * y, int i0 , int i1, double xForecast, float * rms );

void interpretInput(char * line);
float rootmeasquare(float * y,int nmax);
void addpoint1(int * index1, int n30,float fore30,float fore300, float * y30, float * y300);
void computeFilterAverage(int n30, int n300, float fore30, float fore300, float * y30, float * y300, int * index1, float * rms1, float ratioRMS,float addRMS,float threshold, float * alertSignal1, int * alertValue1);
void saveBuffer(char * fullname, int index300, int index1);
void readBuffer(char * fullname);
int fileExists(char * fname);

struct timeval now0; 

void initAnalysis() {
	Tot = 0;
	Npoints = 0;
	DtAverage = Configuration.Interval; // 1.0; //0.1; // 5 sec acquisition interval
	//LastTimeOut = millis();
	//time0 = (float) LastTimeOut/1000.;
	CurrentTime_s=(double) time(NULL);

	time0=(double) CurrentTime_s/3600/24;
	LastTimeOut=CurrentTime_s;
	
	ratioRMS = Configuration.ratioRMS;
	threshold = Configuration.threshold;
	addRMS = Configuration.AddRMS;
	multFac = Configuration.sensorMultFac;
	addFac = Configuration.sensorAddFac;
	backFactor = Configuration.backFactor;
//AA
        strcpy(saveURL,Configuration.SaveURL);
		strcpy(alertURL,Configuration.AlertURL);
//AA
	offset = 0;
	gain = 1; // 

	stat = 1;

	max300 = Configuration.n300;
	max30 = Configuration.n30;
  
	Tmax300 = DtAverage*max300;
	Tmax30 = DtAverage*max30;
  
  	LastTimeAlert_s=-100000;
	printf("saveURL=%s\n",saveURL);
	printf("alertURL=%s\n",alertURL);
	printf("Interval=%f\n",DtAverage);
	printf("Tmax30, Tmax300=%f,%f\n",Tmax30,Tmax300);
	printf("n300=%i\n",max300);
	printf("n30=%i\n",max30);

	printf("threshold=%f\n",threshold);
	printf("ratioRMS=%f\n",ratioRMS);
	printf("AddRMS=%f\n",addRMS);
	printf("backFactor=%d\n",backFactor);
	printf("multFact=%f\n",multFac);
        printf("addFact=%f\n",addFac);	
	y300 =    (float *) calloc(1 + max300,sizeof(float));
	x300 =    (double *) calloc(1 + max300,sizeof(double));
	yavg30 =  (float *) calloc(1 + max300,sizeof(float));
	yavg300 = (float *) calloc(1 + max300,sizeof(float));
	diffe =   (float *) calloc(1 + max300,sizeof(float));
    readBuffer("buffer.txt");
}

int fileContains(char * fname, char * searchText)
{
	FILE * infile;
	char line[1000];
	int found;
	if(!(infile = fopen(fname, "r"))) {
		return FALSE;
	} else {
		found = FALSE;
	}

	while(fgets(line, sizeof(line), infile))
		if (strstr(line, searchText))
		{
			found = TRUE;
			break;
		}

	fclose(infile);
	return found;
}

void retryToTransmit()
{
        FILE * infile;
        char line[1000];
		char dummy[1000];
		if(!fileExists("retryStore.txt")) return;

		system("rm -f retryStore1.txt");
        if((infile = fopen("retryStore.txt", "r")) == NULL) {
                printf("File retryStore1.txt does not exists retryStore.txt\n");
                return;
                }

        int isConnected=TRUE;
        FILE * retryStore1=fopen("retryStore1.txt", "w");
        while( fgets(line, sizeof(line), infile) != NULL ) {
                //printf("line x: %s\n",line);

                if (isConnected) { 
					printf("RETR: %s\n",line);
					system(line);
					if ( fileContains("outlogwget.txt","EnterData.aspx") || fileContains("outlogwget.txt","EnterAlert.aspx") )
							{ 
							system("rm -f EnterData.aspx*");
							LastTimeConnected_s = CurrentTime_s;

							}
					else if ( fileContains("outlogwget.txt","EnterAlert.aspx") )
							{ 
							system("rm -f EnterAlert.aspx*");
							LastTimeConnected_s = CurrentTime_s;
							}
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
        if (fileExists("retryStore1.txt"))
			system("mv retryStore1.txt retryStore.txt");
		
        
}

void addMeasure(struct sensorGrid *grid, double time00)
{
	struct tm *gmtm;
	float sensorValue = grid->distance*multFac+addFac;
	int offset300, offset30;
	static char dummyD[32], logLine[1024];
	double forecast30 = 0;
	double forecast300 = 0;
	float rms30 = 0.0;
	float rms = 0.0;
	// float sensorValue = readDist()/100.0 ; //sonar1.ping() * 0.0242 ; // measurement in m
	//unsigned long currentTimems;
	char success = 'N';

	//currentTimems = millis();
	//time0 = (float) currentTimems/1000;
	
		// current date/time based on current system
	time_t now;
	

	if (time00==-1){
		CurrentTime_s=(double) time(NULL);  // seconds from 1/1/1970
		time0=(double) CurrentTime_s/(float)(3600)/ (float) 24 + 25569.0; // days from 1/1/1900  to be used in EXCEL
		// convert now to tm struct for UTC
		now = (time_t) time(0);
		gmtm = (struct tm *) gmtime(&now);
	}
	else
	{
		CurrentTime_s=(double) (time0*(float)24.0* (float) 3600.0);
		time0=time00;
		now = (double) (CurrentTime_s-25569.0*24.0*3600.0);
		gmtm = (struct tm *) gmtime(&now);
	}
	
	if(index300== 0 || index300==-1)
        {
		int k;
		for (k = 0;k <= max300;k++)
		{
			x300[k]   = time0-(max300-k)*DtAverage/(float)24.0/(float)3600.0;
			y300[k]   =sensorValue;
			yavg30[k] =sensorValue;
                        yavg300[k]=sensorValue;
                        diffe[k]=0;
	        }
		index300 = index1 = max300;
        }
			
	//printf("current time %i  \n",currentTimems);
	//  Tot += (sensorValue-offset)*gain;
	Tot += sensorValue;
	Npoints += 1;
	//printf("%i  %f  %i  %i  %f  %f\n",Npoints,sensorValue,currentTimems,LastTimeOut,DtAverage,(currentTimems-LastTimeOut)/1000.);
	if ((CurrentTime_s-LastTimeOut)>DtAverage)
	{ 
		// this allows to average within the Dtaverage interval
		Tot = (Tot/ (double) Npoints);// +random(5);
		// print out the value you read:
		sensorValue = Tot;  //devo riattribuire Tot a sensorValue perche' alcune stampe sono fatte con Tot altre con sensorValue che devono essere uguali

		addpoint(x300,y300,time0, Tot, &index300, max300, Tmax30, Tmax300, &offset30, &offset300);

		if (index300 >= max300) {
		   //printf("0.1  %i %i\n",offset30,offset300);
			forecast30 = solve2(x300, y300, offset30, max300 , time0, &rms30);
			forecast300 = solve2(x300, y300, offset300, max300, time0,&rms);
			alertSignal = fabs(forecast30 - forecast300);
			if (alertSignal>ratioRMS*rms30 && alertSignal>threshold)
			{
				if (alertValue<10)
					alertValue += 1;
			}
			else
			{
				if (alertValue>0 )
					alertValue -= 1;
			}
		}

			// based on average after stabilization
		if (index300 >= max300){
			computeFilterAverage(max30, max300, forecast30, forecast300, yavg30, yavg300, &index1, &rms1, ratioRMS, addRMS, threshold, &alertSignal1, &alertValue1);
			if (index1 >= max300) {
				rms = rms1;
				alertValue = alertValue1;
				alertSignal = alertSignal1;
			}
			else {
				rms1 = 0;
				alertSignal1 = 0;
				alertValue1 = 0;
			}
		}


		if(*saveURL)
		{
			//char * saveURL0 = (char *) calloc(4096, sizeof(char));
			//char * dummy = (char *) calloc(4096, sizeof(char));
			//char * dummy1 = (char *) calloc(4096, sizeof(char)), *t;
			char saveURL0[4096];
			char dummy[4096];
			char dummy1[4096], *t;

			success = 'T';

			strcpy(saveURL0,saveURL);

			strftime(dummyD, sizeof(dummyD), "%d/%m/%Y %H:%M:%S", gmtm);

//			printf("%s, %f, %f, %f, %i, %i, %i, %f, %f, %i \n",dummyD,Tot,forecast30,forecast300,alertValue,index300,index1,rms,alertSignal,Npoints);
			fflush(stdout);
			FILE * log = fopen("/tmp/calc.log", "w");
			if(log != NULL){
				fprintf(log, "%s, %f, %f, %f, %i, %i, %i, %f, %f, %i \n",dummyD,Tot,forecast30,forecast300,alertValue,index300,index1,rms,alertSignal,Npoints);
				fclose(log);
			}

			strftime(dummyD, sizeof(dummyD), "%d/%m/%Y", gmtm);
			replace(saveURL0,"$DATE",dummyD,dummy); strcpy(saveURL0, dummy);
			strftime(dummyD, sizeof(dummyD), "%H:%M:%S", gmtm);
			replace(saveURL0,"$TIME",dummyD,dummy); strcpy(saveURL0, dummy);

			sprintf(dummy1,"%f",sensorValue);
			replace(saveURL0,"$LEV", dummy1,dummy); strcpy(saveURL0, dummy);
			sprintf(dummy1,"%f",forecast300);
			replace(saveURL0,"$FORE300", dummy1,dummy); strcpy(saveURL0, dummy);
			sprintf(dummy1,"%f",forecast30);
			replace(saveURL0,"$FORE30", dummy1,dummy); strcpy(saveURL0, dummy);
			sprintf(dummy1, "%f",rms);
			replace(saveURL0,"$RMS", dummy1,dummy); strcpy(saveURL0, dummy);
			sprintf(dummy1, "%f",alertSignal);
			replace(saveURL0,"$ALERT_LEVEL", dummy1,dummy); strcpy(saveURL0, dummy);
			sprintf(dummy1, "%d",alertValue);
			replace(saveURL0,"$ALERT_SIGNAL", dummy1,dummy); strcpy(saveURL0, dummy);

			sprintf(dummy1, "%f",grid->temperature);
			replace(saveURL0,"$TEMP", dummy1,dummy); strcpy(saveURL0, dummy);
			sprintf(dummy1, "%f",grid->pressure);
			replace(saveURL0,"$PRESS", dummy1,dummy); strcpy(saveURL0, dummy);
			sprintf(dummy1, "%f",grid->batteryVoltage);
			replace(saveURL0,"$V1", dummy1,dummy); strcpy(saveURL0, dummy);
			sprintf(dummy1, "%f",grid->panelVoltage);
			replace(saveURL0,"$V2", dummy1,dummy); strcpy(saveURL0, dummy);
			replace(saveURL0,"$V3", "+998.0",dummy); strcpy(saveURL0, dummy);
			//printf ("SaveURL=%s\n",saveURL0);
			t = ctrim(saveURL0);
			sprintf(dummy, "wget \"%s\" -T 20 -nv -o outlogwget.txt", t);

			//printf("calling system: %s\n",dummy);
			system(dummy);

			if ( fileContains("outlogwget.txt","EnterData.aspx"))
			{
				system("rm -f outlogwget.txt* EnterData.aspx*");
				//retryToTransmit();  // se sono riuscito a trasmettere, provo a ritrasmettere quelli non riusciti
				LastTimeConnected_s = CurrentTime_s;

			}
			else
			{  // se non sono riuscito a trasmettere salvo nel file retrysStore.txt i comandi da dare e poi riprovare.
				printRetry(dummy);
				//FILE * retryStore = fopen(RETRY_BUFFER, "a");
                                //printf("string stored %s\n",dummy);
				//if(retryStore)
				//{
				//	fprintf(retryStore, "%s\n",dummy);
				//	fclose(retryStore);
					success = 'S';
				//}
			}
		}
		else
		{	
//			printf("%f, %f, %f, %f, %i, %i, %f, %f, %f \n",time0,Tot,forecast30,forecast300,alertValue,index300,rms30,rms,alertSignal);
		}
   //                   If (alertValue > 5 Or True) And alertURL <> "" And alertURL <> "none" Then
   //                     If Now.ToOADate - alertingTime.tooadate > 30 / 60 / 24 Or alertValue > 5 Then

		if (*alertURL && alertValue>5 && (CurrentTime_s-LastTimeAlert_s)>5*60)
		{
			char alertURL0[4096];
			char dummy[4096];
			char dummy1[4096], *t;

			success = 'T';
      		LastTimeAlert_s=CurrentTime_s;
			strcpy(alertURL0,alertURL);

			strftime(dummyD, sizeof(dummyD), "%d/%m/%Y %H:%M:%S", gmtm);

//			printf("%s, %f, %f, %f, %i, %i, %i, %f, %f, %i \n",dummyD,Tot,forecast30,forecast300,alertValue,index300,index1,rms,alertSignal,Npoints);
//			fflush(stdout);
			strftime(dummyD, sizeof(dummyD), "%d/%m/%Y", gmtm);
			replace(alertURL0,"$DATE",dummyD,dummy); strcpy(alertURL0, dummy);
			strftime(dummyD, sizeof(dummyD), "%H:%M:%S", gmtm);
			replace(alertURL0,"$TIME",dummyD,dummy); strcpy(alertURL0, dummy);

			sprintf(dummy1,"%f",sensorValue);
			replace(alertURL0,"$LEV", dummy1,dummy); strcpy(alertURL0, dummy);
			
			sprintf(dummy1, "%d",alertValue);
			replace(alertURL0,"$ALERT_SIGNAL", dummy1,dummy); strcpy(alertURL0, dummy);

			printLog(alertURL0);
			t = ctrim(alertURL0);
			sprintf(dummy, "wget \"%s\" -T 20 -nv -o outlogwget.txt", t);

			//printf("calling system: %s\n",dummy);
			system(dummy);

			if ( fileContains("outlogwget.txt","EnterAlert.aspx"))
			{
				system("rm -f outlogwget.txt* EnterAlert.aspx*");
				//retryToTransmit();  // se sono riuscito a trasmettere, provo a ritrasmettere quelli non riusciti
				LastTimeConnected_s = CurrentTime_s;
			}
			else
			{  // se non sono riuscito a trasmettere salvo nel file retrysStore.txt i comandi da dare e poi riprovare.
				printRetry(dummy);
				//FILE * retryStore = fopen(RETRY_BUFFER, "a");
                //printf("string stored %s\n",dummy);
				//if(retryStore)
				//{
				//	fprintf(retryStore, "%s\n",dummy);
				//	fclose(retryStore);
					success = 'S';
				//}
			}
		}
		if (CurrentTime_s - LastTimeConnected_s> NOCONNECTION_TIME_MIN * 60) {
			// se per 5 minuti non ho connessione faccio reboot del server, uno dei due comandi qui sotto per fare reboot del Teltonika
			system("sshpass -p 'Ecml2011' ssh root@192.168.1.1 -o StrictHostKeyChecking=no \"reboot\" ");
				//system("sshpass -p 'Ecml2011' ssh root@192.168.1.1 -o StrictHostKeyChecking=no \"shutdown -r now\" ")
				LastTimeConnected_s = CurrentTime_s + NOCONNECTION_TIME_MIN * 60;  //reset no connection timer to avoid continous reboot
		}

		strftime(dummyD, sizeof(dummyD), "%d/%m/%Y %H:%M:%S", gmtm);
		sprintf(logLine, "%s, %f, %f, %f, %f, %f, %i, %i, %f, %f, %f, %d, %f, %f, %c\n",
			dummyD,Tot,grid->pressure,grid->temperature,forecast30,forecast300,alertValue,index300,rms30,rms,alertSignal,Npoints,
			grid->batteryVoltage, grid->panelVoltage, success);
		printLog(logLine);
		//printf("-1-:%s\n",logLine);
		fflush(stdout);

		Tot = 0; 
		Npoints = 0;
		LastTimeOut = CurrentTime_s;
		saveBuffer("buffer.txt",index300,index1);
	}
	
}

// input deck //
int fileExists(char * fname)
{	
	FILE* fp = NULL;
 
    //this will fail if more capabilities to read the 
    //contents of the file is required (e.g. \private\...)
	fp = fopen(fname, "r");
 
	if(fp != NULL)
	{
		fclose(fp);
		return TRUE;
	}
 
	return FALSE;
}

void saveBuffer(char * fullname, int index300, int index1)
{
	 int k; 
	 char dummy[4096];
	 FILE * outfile = fopen("buffer_tmp.txt", "w");
	 fprintf(outfile,"%i,%i\n",index300,index1);
	 for (k = 0;k <= max300;k++)
     fprintf(outfile,"%f,%f,%f,%f\n",x300[k],y300[k],yavg30[k],yavg300[k]);
	 fclose(outfile);
	 sprintf(dummy, "sudo cp buffer_tmp.txt %s;rm buffer_tmp.txt",fullname);
	 system(dummy);
}

void readBuffer(char * fullname)
{
	FILE *infile;
	char line[400],*ele;
 
	 printf("Reading file: %s\n",fullname);
	 /* Open the file. If NULL is returned there was an error */
	 
	 if(!fileExists(fullname)) return;
         infile=fopen(fullname,"r");
	 if(fgets(line, sizeof(line), infile)!=NULL) {
	 index300 = atoi(ctrim(strtok(line,",")));
	 index1 = atoi(ctrim(strtok(NULL,",")));
	 printf("%i  %i\n",index300,index1);
	 int i = -1;
	 while( fgets(line, sizeof(line), infile) != NULL ) 
   {
                 //printf("%i: %s\n",i,line);
				 if(i==101)
				 {
				 i=i;
				 }
		 i = i+1;
		 x300[i]    = (atof)(ctrim(strtok(line,",")));
		 ele=strtok(NULL,",");
		 if(ele!=0)
			y300[i]    = (atof)(ctrim(ele));
		 ele=strtok(NULL,",");
		 if(ele!=0)
			yavg30[i]  = (atof)(ctrim(ele));
		 ele=strtok(NULL,",");
		 if(ele!=0)
			yavg300[i] = (atof)(ctrim(ele));
		 if(ele==0)
		 {
			index1=-1;
			index300=-1;
		}
		 //		 printf("%i\n",i);
	 }
	 if(i!=index300)
		{ index300=-1;index1=-1;}
	}
	fclose(infile);
	
}



//  MATH CALCULATIONS //
void computeFilterAverage(int n30, int n300, float fore30, float fore300, float * y30, float * y300, int * index1, float * rms1, float ratioRMS,float addRMS,float threshold, float * alertSignal1, int * alertValue1)
{
	addpoint1(index1, n300, fore30, fore300, y30, y300);
	//*alertValue1 = 0;
	if( *index1 >= n300) {
		int kk;
		for (kk = 0;kk <= n300/2;kk++) 
		{
			diffe[kk] = y30[kk] - y300[kk];
			//printf("kk=%i  diffe=%f   y30=%f   y300=%f\n",diffe[kk],y30[kk],y300[kk]);
		}
		*rms1 = rootmeasquare(diffe,n300/2);
		*alertSignal1 = fabs(fore30 - fore300);
//printf("alertSignal: %f, rms1: %f, ratio: %f, offset: %f, threshold: %f\n", *alertSignal1, *rms1, ratioRMS, addRMS, threshold);
//fflush(stdout);
		if (*alertSignal1 > (*rms1 * ratioRMS + addRMS) && *alertSignal1 > threshold) 
		{
			if (*alertValue1<10)
				*alertValue1 += 1;
		}
		else
		{
			if (*alertValue1>0 )
				*alertValue1 -= 1;
		}
		if (*index1 >= n300)
			*index1 = n300;
	}
	else {
		*alertSignal1 = 0;
		*alertValue1 = 0;
	}
}

void addpoint1(int * index1, int n30,float fore30,float fore300, float * y30, float * y300)
{
	if (*index1 < n30) {
		*index1 += 1;
		y30[*index1] = fore30;
		y300[*index1] = fore300;
	}
	else {
		int k;
		for (k = 1;k <= n30;k++) {
			y30[k - 1] = y30[k];
			y300[k - 1] = y300[k];
		}
		y30[n30] = fore30;
		y300[n30] = fore300;
	}
}

float rootmeasquare(float * y,int nmax)
{
	double average;
	int k;
	average = 0;
	for (k = 0;k <= nmax;k++)
		average += (y[k]) / (nmax + 1);

	double rm = 0;
	for (k = 0;k <= nmax;k++)
		rm += ((double)(y[k] - average))*((double)(y[k] - average)) / (nmax + 1);
	//for (k = 0;k <= nmax;k++)
	//printf("%i   %f   %f   %f\n",k,y[k],average,rm);

	rm = sqrt((double)rm);
	//printf("rootmeasquare: nmax=%i rm=%f  average=%f\n",nmax,rm,average);
	return rm;
}

void addpoint(double *xvect, float *yvect, double x , double value, int * i , int max0, float tmax30, float tmax300, int * offset30, int * offset300)
{
	if (*i < max0){
		*i += 1;
		xvect[*i] = x;
		yvect[*i] = value;
		*offset30 = 0;*offset300 = 0;
	}
	else {
		shiftVect(xvect,yvect,max0,Tmax30, Tmax300, offset30, offset300);
		yvect[max0] = value;
		xvect[max0] = x;
	}
}

void shiftVect(double *x, float *y, int max0 , float tmax30, float tmax300, int * offset30, int * offset300)
{
	int k;
	*offset30 = 0;*offset300 = 0;
	for (k = 1;k <= max0;k++) 
	{
		y[k - 1] = y[k];
		x[k - 1] = x[k];
		//printf("shiftVect   x[k]=%f  time0=%f   tmax300=%f  tmax30=%f  \n",x[k],time0,tmax300,tmax30);
		if(x[k]< time0 - tmax300/24.0/3600.0) 
			*offset300 = k;
		if(x[k]< time0 - tmax30/24.0/3600.0) 
			*offset30 = k;
	}
}

double solve2(double * x, float * y, int i0 , int i1, double xForecast, float * rms )
{
	int k;
	double a, b, c;
	double a11, a12, a13, a21, a22, a23, a31, a32, a33, c1, c2, c3;
	double sumx2, sumx1, sumy1,dx;
	double sumx3, sumx4, sumx2y, sumxy;
	int np;
	float yavg,rm;

	sumx2 = 0 ; 
	sumx1 = 0 ; 
	sumy1 = 0 ; 
	np = 0 ; 
	sumx3 = 0 ; 
	sumx4 = 0 ; 
	sumx2y = 0 ; 
	sumxy = 0 ;
	for (k = i0;k <= i1;k++)
	{
		dx = (double) (x[k] - x[i0]);
		sumx4 += dx*dx*dx*dx;
		sumx2 += dx*dx;
		sumx3 += dx*dx*dx;
		sumx1 += dx;
		sumy1 += y[k];
		sumxy += dx * y[k];
		sumx2y += dx*dx * y[k];
		np += 1;
	}

	c1 = sumx2y;
	c2 = sumxy;
	c3 = sumy1;

	a11 = sumx4;
	a12 = sumx3;
	a13 = sumx2;

	a21 = sumx3;
	a22 = sumx2;
	a23 = sumx1;

	a31 = sumx2;
	a32 = sumx1;
	a33 = np;

	double denom;

	denom = multi2(a11, a21, a31, a12, a22, a32, a13, a23, a33);
	if (denom != 0){
		a = multi2(c1, c2, c3, a12, a22, a32, a13, a23, a33) / denom;
		b = multi2(a11, a21, a31, c1, c2, c3, a13, a23, a33) / denom;
		c = multi2(a11, a21, a31, a12, a22, a32, c1, c2, c3) / denom;
		yavg = average(y,i0,i1);
		rm = rootMeanSquare(y,i0,i1,&yavg);
		*rms = rm;
		return a * (xForecast - x[i0]) *(xForecast - x[i0]) + b * (xForecast - x[i0]) + c;
	}
	else
		return sumy1 / np;
}

double solve1(double * x, float * y, int i0 , int i1, double xForecast, float * rms )
{
	double a, c;
	double a11, a12, a21, a22, c1, c2;
	double sumx2, sumx1, sumy1,sumxy ;
	double denom;
	float yavg,rm;
	int k;
	int np;
	sumx2 = 0 ; 
	sumx1 = 0 ; 
	sumy1 = 0 ; 
	np = 0 ; 
	sumxy = 0 ;
	for (k = i0;k <= i1;k++)
	{
		sumx2 += (double) (x[k] - x[i0])*(x[k] - x[i0]);
		sumx1 += (double) (x[k] - x[i0]);
		sumy1 += (double) y[k];
		sumxy += (double) (x[k] - x[i0]) * y[k];
		np += 1;
	}

	c1 = sumxy;
	c2 = sumy1;

	a11 = sumx2;
	a12 = sumx1;

	a21 = sumx1;
	a22 = (double) np;

	denom = multi1(a11, a21, a12, a22);
	if (denom != 0) {
		a = multi1(c1, c2, a12, a22) / denom;
		c = multi1(a11, a21, c1, c2) / denom;
		yavg = average(y,i0,i1);
		rm = rootMeanSquare(y,i0,i1,&yavg);
		*rms = rm;

		return a * (xForecast - x[i0]) + c;
	}
	return sumy1 / np;
}

double multi2(double a11, double a21, double a31, double a12, double a22, double a32, double a13, double a23, double a33)
{
	double multi0;
	multi0 = a11 * a22 * a33 + a12 * a23 * a31 + a13 * a21 * a32;
	multi0 -= (a13 * a22 * a31 + a11 * a23 * a32 + a33 * a12 * a21);
	return multi0;
}

double multi1(double a11, double a21, double a12, double a22)
{
	double multi0;
	multi0 = a11 * a22;
	multi0 -= (a12 * a21);
	return multi0;
}

float average(float * yvec, int i0, int i1)
{
	float avg = 0;
	int k;
	for(k = i0;k <= i1;k++)
		avg += yvec[k]/(float) (i1-i0+1);

	return avg;
}

float rootMeanSquare(float * yvec, int i0, int i1, float *ave)
{
	int k;
	float rm = 0;
	for( k = i0;k <= i1;k++)
		rm += (yvec[k]-*ave)*(yvec[k]-*ave)/(i1-i0+1);
	rm = sqrt((double)rm);
	return rm;
}


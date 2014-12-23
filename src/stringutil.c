#include "stringutil.h"

int sl(char *a)
{
	if(a != NULL)
		return strlen(a);
	else
		return 0;
}

/*
int compString(char *s1, char *s2)

{
	//if (size==NULL)
	int size=strlen(s1);
	int k;
	//bool equalstring=true;
	
	for (k=0; k<size+1;k++)
	if (s1[k]!=s2[k]) return false;

	return true;
}
*/

void replace(char *st, char *orig, char *repl, char * outString) {
  char *ch;
  if (!(ch = strstr(st, orig)))
  {	  copystring (st,outString);
	  return;
  }
	
    strncpy(outString, st, ch-st);  
    //outString[ch-st] = 0;
    sprintf(outString+(ch-st), "%s%s", repl, ch+strlen(orig));
    //strcpy (outString, buffer);
    return;
  }

void setmid (char *a, unsigned int n, unsigned int l, char * b)
{
	int l1;
	
	l1=l;
    	if (l>strlen(b))
		l1=strlen(b);
	if (n<=strlen(a) && n >0)
		strncpy(a+n-1,b,l1);
}

char * cleanValue1(char * value)
{
	if (value!=NULL)
	{
		int ch=strchr(value, 10)-value+1;
		if (ch>0 && ch<=sl(value))
		setmid(value,ch,1," ");
		ch=strchr(value, 13)-value+1;
		if (ch>0 && ch<=sl(value))
		setmid(value,ch,1," ");
		//if(mode=NULL)
		//	return ctrim(value);
		//else
		return value;
	}
	else
		return NULL;
}
double val(char *a)
{
	char * dstop;
	return strtod(a,&dstop);
}

void cmid(char * b,unsigned int n,unsigned int l, char *a)
{
	int l1;
	l1=l;
	if (l>strlen(b))
		l1=strlen(b);   
	if (n<=strlen(b) && n >0)
	{
		strncpy(a,b+n-1,l1);
		a[l1]='\0';
	}

}
char * _mid(char * b,unsigned int n,unsigned int l)
{
        char *a = NULL;
	int l1=l;
	if (l>strlen(b)) l1=strlen(b);
	if (n<=strlen(b) && n >0)
	{
                a=(char *) calloc(l1,sizeof(char));
		strncpy(a,b+n-1,l1);
		a[l1]='\0';
	}
	return a;
}

void mid_s(char * b,unsigned int n,unsigned int l, char *a)
{   //char *a;
	int l1;
	l1=l;
	if (l>strlen(b))
		l1=strlen(b);
	if (n<=strlen(b) && n >0)
	{   a=(char *) calloc(l1,sizeof(char));
		strncpy(a,b+n-1,l1);
		a[l1]='\0';
	}
	//return a;
}


char * AddString(char * s1, char * s2, char * s3, char * s4, char * s5)
{   int totLenght;
	int totAlloc;
	char * ss;
	totAlloc=strlen(s1)+strlen(s2)+sl(s3)+sl(s4)+sl(s5);
	//+sl(s6)+sl(s7)+sl(s8)+sl(s9)+sl(s10)+1;
	ss=(char *) calloc(totAlloc,sizeof(char));
	
	strncpy(ss,s1,sl(s1));
	totLenght =sl(s1);
	ss[totLenght]=0;
	strncpy(ss+sl(s1),s2,sl(s2));
	totLenght =sl(s1)+sl(s2);
	ss[totLenght]=0;
	if (s3 != NULL)
	{
		strncpy(ss+totLenght,s3,sl(s3));
		totLenght += sl(s3);
		ss[totLenght]=0;
	}
	if (s4 != NULL)
	{
		strncpy(ss+totLenght,s4,sl(s4));
		totLenght += sl(s4);
		ss[totLenght]=0;
	}
	if (s5 != NULL)
	{
		strncpy(ss+totLenght,s5,sl(s5));
		totLenght += sl(s5);
		ss[totLenght]=0;
	}

	return ss;
}

void AddString_s(char * ssOut,char * s1, char * s2, char * s3, char * s4, char * s5)
{	char * ss=(char *) calloc(1024,sizeof(char));
	sprintf(ss,"%s%s%s%s%s",s1,s2,s3?s3:"",s4?s4:"",s5?s5:"");
	copystring(ss,ssOut);	
	return;
}

char * crtrim(char *a)
{
  char *ptr = a + strlen(a);
  while(isspace(*--ptr)) *ptr = '\0';
  return a;
/*
   int n,pos;
   char * riga;
   riga= (char *) calloc(strlen(a)+1,sizeof(char));
   n=0;
   pos=strlen(a);
   while (n==0)
   {
	   n = strcspn( a+pos, " ");  // cerco la prima posizione non blanc
	   pos--;
   }
   
   cmid(a,1,pos+2,riga);
   strcpy(a,riga);
   free (riga);
   return a;
*/ 
}

char * cltrim(char *a)
{
  char *ptr = a, *b = a;
  int dont=1;
  while(isspace(*ptr)) {
    dont=0;
    ptr++;
  }
  if(dont) return a;

  while(*b++ = *ptr++);
  return a;
/*
   int n,pos;
   char * riga;
   riga= (char *) calloc(strlen(a)+1,sizeof(char));
   pos=0;
   n=0;
   while (n==0)
   {
		n = strcspn( a+pos, " ");  // cerco la prima posizione non blanc
		pos++;
   }
   
   cmid(a,pos,strlen(a)-pos+1,riga);
   strcpy(a,riga);
   free (riga);
   return a; 
*/
}
 
char * ctrim(char *a)
{
     return cltrim(crtrim(a));
/*
	char * tmp;
	tmp=cltrim(crtrim(a));
   return tmp;
*/
}

char *replace_str(char *str, char *orig, char *rep)
{
  char buffer[4096];
  char *p;

  if(!(p = strstr(str, orig)))  // Is 'orig' even in 'str'?
    return str;
 
  while (p)
  {
	  strncpy(buffer, str, p-str); // Copy characters from 'str' start to 'orig' st$
	  //buffer[p-str] = '\0';

	  sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));
	  strcpy(str, buffer);
	  p = strstr(str, orig);
  }
  return str;
}


void cstring (int ncount, char *a, char *riga)
{
	
	int i;
	
	for (i=1;i<ncount+1;i++)
		strncpy(riga+(i-1)*strlen(a),a,strlen(a));
	riga[ncount]='\0';
	
}
void copystring ( char *source, char *dest)
{
	
	strncpy(dest,source,sl(source));
	dest[sl(source)]='\0';
	
}

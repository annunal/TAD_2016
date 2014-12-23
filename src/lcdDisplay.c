/*
 * ldc.c:
 *	Simple test program to drive LCD device
 *
 *	Gordon Henderson, gordon@drogon.net
 */

#include <wiringPi.h>

#include <stdlib.h>
#include <stdint.h>

// LCD pins
 
#define	RS	12
#define	STRB	13  //  E 

// commands

#define	LCD_CLEAR	0x01
#define	LCD_HOME	0x02
#define	LCD_ENTRY	0x04
#define	LCD_ON_OFF	0x08
#define	LCD_CDSHIFT	0x10
#define	LCD_FUNC	0x20
#define	LCD_CGRAM	0x40
#define	LCD_DGRAM	0x80

#define	LCD_ENTRY_SH	0x01
#define	LCD_ENTRY_ID	0x02

#define	LCD_ON_OFF_B	0x01
#define	LCD_ON_OFF_C	0x02
#define	LCD_ON_OFF_D	0x04

#define	LCD_FUNC_F	0x04
#define	LCD_FUNC_N	0x08
#define	LCD_FUNC_DL	0x10

#define	LCD_CDSHIFT_RL	0x04


void dataByte (uint8_t data)
{
  uint8_t i ;

  for (i = 0 ; i < 8 ; ++i)
  {
    digitalWrite (i, (data & 1)) ;
    data >>= 1 ;
  }
}

void putCommand (uint8_t command)
{
  digitalWrite (RS,   0) ;
  digitalWrite (STRB, 1) ;
  dataByte     (command) ;
  delay (1) ;
  digitalWrite (STRB, 0) ;
  delay (1) ;
}

void lcdPosition (int x, int y)
{
  uint8_t l2 = LCD_DGRAM ;

  if (y == 1)
    l2 |= 0x40 ;

  l2 += x ;
  putCommand (l2) ;
}


void lcdPutchar (uint8_t data)
{
  digitalWrite (RS,   1) ;
  digitalWrite (STRB, 1) ;
  dataByte     (data) ;
  delay (1) ;
  digitalWrite (STRB, 0) ;
  delay (1) ;
}

void lcdPuts (char *string)
{
  while (*string)
    lcdPutchar (*string++) ;
}

void setup (void)
{
  uint8_t i ;

/*
  if (wiringPiSetup () == -1)
    exit (1) ;
*/

  for (i = 0 ; i < 8 ; ++i)
  {
    digitalWrite (i, 0) ;
    pinMode      (i, OUTPUT) ;
  }

  digitalWrite (RS, 0) ;
  pinMode      (RS, OUTPUT) ;

  digitalWrite (STRB, 1) ;
  pinMode      (STRB, OUTPUT) ;

  delay (35) ; // mS
  putCommand (LCD_FUNC | LCD_FUNC_DL | LCD_FUNC_N) ;
  delay (5) ;
  putCommand (LCD_FUNC | LCD_FUNC_DL | LCD_FUNC_N) ;
  delay (5) ;
  putCommand (LCD_FUNC | LCD_FUNC_DL | LCD_FUNC_N) ;
  delay (5) ;

  putCommand (LCD_ON_OFF | LCD_ON_OFF_D) ;
  delay (2) ;
  putCommand (LCD_ENTRY | LCD_ENTRY_ID) ;
  delay (2) ;
  putCommand (LCD_CDSHIFT | LCD_CDSHIFT_RL) ;
  delay (2) ;
  putCommand (LCD_CLEAR) ;
  delay (5) ;
}

void initDisplay ()
{
  setup () ;
}

void display (int linec, char *lines[])
{
	int i;
	for(i = 0; i < linec; i++)
	{
		lcdPosition (0, i) ;
		lcdPuts (lines[i]) ;
	}
}


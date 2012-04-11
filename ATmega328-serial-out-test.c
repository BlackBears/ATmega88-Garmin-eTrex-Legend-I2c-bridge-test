/*************************************************************************
Title:    example program for the Interrupt controlled UART library
Author:   Peter Fleury <pfleury@gmx.ch>   http://jump.to/fleury
File:     $Id: test_uart.c,v 1.4 2005/07/10 11:46:30 Peter Exp $
Software: AVR-GCC 3.3
Hardware: any AVR with built-in UART, tested on AT90S8515 at 4 Mhz

DESCRIPTION:
          This example shows how to use the UART library uart.c

*************************************************************************/
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>
#include <avr/pgmspace.h>
#include <string.h>

#include "uart.h"
#include "global.h"
#include "common_types.h"

/* define CPU frequency in Mhz here if not defined in Makefile */
#ifndef F_CPU
#define F_CPU 14745600UL
#endif

/* 9600 baud */
#define UART_BAUD_RATE      4800      

#define GPS2_YEAR_INDEX		1
#define GPS2_YEAR_LEN		2
#define GPS2_MONTH_INDEX	3
#define GPS2_MONTH_LEN		2
#define GPS2_DAY_INDEX		5
#define GPS2_DAY_LEN		2
#define GPS2_HOUR_INDEX		7
#define GPS2_HOUR_LEN		2
#define GPS2_MINUTE_INDEX	9
#define GPS2_MINUTE_LEN		2
#define GPS2_SECOND_INDEX	11
#define GPS2_SECOND_LEN		2
#define GPS2_LATITUDE_HEMISPHERE_INDEX	13
#define GPS2_LATITUDE_HEMISPHERE_LEN	1
#define GPS2_LATITUDE_POSITION	14
#define GPS2_LATITUDE_LEN	7
#define GPS2_LONGITUDE_HEMISPHERE_INDEX 21
#define GPS2_LONGITUDE_HEMISPHERE_LEN 1
#define GPS2_LONGITUDE_POSITION_INDEX	22
#define GPS2_LONGITUDE_POSITION_LEN 8
#define GPS2_POSITION_STATUS_INDEX			30
#define GPS2_POSITION_STATUS_LEN			1
#define GPS2_HPOS_ERROR_INDEX				31
#define GPS2_HPOS_ERROR_LEN					3
#define GPS2_ALT_SIGN_INDEX					34
#define GPS2_ALT_SIGN_LEN					1
#define GPS2_ALT_METERS_INDEX				35
#define GPS2_ALT_METERS_LEN					5
#define GPS2_EW_VELOCITY_DIR				40
#define GPS2_EW_VELOCITY_LEN				1
#define GPS2_EW_VELOCITY_MAGNITUDE_INDEX	41	// m/s in tenths, e.g. 1234 = 123.4 m/s
#define GPS2_EW_VELOCITY_MAGNITUDE_LEN		4
#define GPS2_NS_VELOCITY_DIR_INDEX			45	
#define GPS2_NS_VELOCITY_DIR_LEN			1
#define GPS2_NS_VELOCITY_MAGNITUDE_INDEX	46
#define GPS2_NS_VELOCITY_MAGNITUDE_LEN		4	//	m/s in tenths, e.g. 1234 = 123.4 m/s
#define GPS2_VERT_VELOCITY_DIR_INDEX		50
#define GPS2_VERT_VELOCITY_DIR_LEN			1
#define GPS2_VERT_VELOCITY_MAGNITUDE_INDEX	51
#define GPS2_VERT_VELOCITY_MAGNITUDE_LEN	4	//	m/s in hundredths, e.g. 1234 = 12.34 m/s

char gps_buffer[500];
u08 gps_buffer_index;
BOOL at_start;
time_t gps2_time;
gps_fix_t gps2_fix;

char print_buffer[25];

BOOL _is_missing(char *str);
void _parse_date(void);
void _parse_time(void);
void _parse_hemisphere(BOOL is_lat);
void _parse_position(BOOL is_lat);
void _parse_latitude(void);
void _parse_longitude(void);

int main(void)
{
    gps_buffer_index = 0;
	gps_buffer[0] = '\0';
	
    uart_init( UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU) ); 
   
    sei();

	at_start = FALSE;
    for(;;)
    {
        char c = uart_getc();
        if ( c & UART_NO_DATA ) {}
        else {
			if ( c & UART_FRAME_ERROR )
            {
                /* Framing Error detected, i.e no stop bit detected */
                uart_puts_P("UART Frame Error: ");
            }
            if ( c & UART_OVERRUN_ERROR )
            {
                /* 
                 * Overrun, a character already present in the UART UDR register was 
                 * not read by the interrupt handler before the next character arrived,
                 * one or more received characters have been dropped
                 */
                uart_puts_P("UART Overrun Error: ");
            }
            if ( c & UART_BUFFER_OVERFLOW )
            {
                /* 
                 * We are not reading the receive buffer fast enough,
                 * one or more received character have been dropped 
                 */
                uart_puts_P("Buffer overflow error: ");
            }
            /* 
             * send received character back
             */
            		
			if( c > 0x21 ) {
				//	this is not a control character
				if( c == '@' ) {
					if( gps_buffer_index != 0 ) {
						//uart_puts(gps_buffer);
						strncpy(gps_buffer,"@000607204655N6012249E01107556S015+00130E0021N0018U0000",55);
						_parse_date();
						_parse_time();
						_parse_latitude();
						_parse_longitude();
					
						sprintf(print_buffer,"%02d",gps2_fix.latitude.degree);
						uart_puts(print_buffer);
									
					}
					gps_buffer[0] = c;
					gps_buffer_index = 1;
				}
				else {
					gps_buffer[gps_buffer_index] = c;
					gps_buffer_index++;
				}
			}																						
        }

    }
}

BOOL _is_missing(char *str) {
	return (str[0] == '_');
}

void _parse_latitude(void) {
	_parse_hemisphere(TRUE);
	_parse_position(TRUE);
}

void _parse_longitude(void) {
	_parse_hemisphere(FALSE);
	_parse_position(FALSE);
}

void _parse_hemisphere(BOOL is_lat) {
	char hemisphere = gps_buffer[(is_lat)?GPS2_LATITUDE_HEMISPHERE_INDEX:GPS2_LONGITUDE_HEMISPHERE_INDEX];
	hemisphere = (hemisphere == '_')?INVALID_DATA:hemisphere;
	if( is_lat )
		gps2_fix.latitude.hemisphere = hemisphere;
	else
		gps2_fix.longitude.hemisphere = hemisphere;
}	/*	_parse_latitude */

void _parse_position(BOOL is_lat) {
	char *pos_str = (char *)malloc(8);
	u08 index = (is_lat)?GPS2_LATITUDE_POSITION:GPS2_LONGITUDE_POSITION_INDEX;
	u08 len = (is_lat)?GPS2_LATITUDE_LEN:GPS2_LONGITUDE_POSITION_LEN;
	strncpy(pos_str,gps_buffer + index, len);
	//uart_puts(pos_str);
	if( pos_str[0] == '_' ) {
		if( is_lat ) {
			gps2_fix.latitude.degree = INVALID_DATA;
			gps2_fix.latitude.minute = INVALID_DATA;
			gps2_fix.latitude.second = INVALID_DATA;
		}	/* lat position data is missing */
		else {
			gps2_fix.longitude.degree = INVALID_DATA;
			gps2_fix.longitude.minute = INVALID_DATA;
			gps2_fix.longitude.second = INVALID_DATA;
		}	/* lon position data is missing */
	}	/* position data is missing */
	else {
		char *degree_str = (char *)malloc(3);
		char *minute_str = (char *)malloc(2);
		char *fx_minute_str = (char *)malloc(3);
		
		if( is_lat ) {
			strncpy(degree_str,pos_str,2);
			gps2_fix.latitude.degree =atoi(degree_str);
			strncpy(minute_str,pos_str + 2,2);
			gps2_fix.latitude.minute = atoi(minute_str);
			strncpy(fx_minute_str,pos_str + 4,3);
			u16 fx_minute = atoi(fx_minute_str) * 6/100;
			gps2_fix.latitude.second = (u08)fx_minute;
		}	/*	parsing latitude position */
		else {
			strncpy(degree_str,pos_str,3);
			gps2_fix.longitude.degree =atoi(degree_str);
			strncpy(minute_str,pos_str + 3,2);
			gps2_fix.longitude.minute = atoi(minute_str);
			strncpy(fx_minute_str,pos_str + 5,3);
			u16 fx_minute = atoi(fx_minute_str) * 6/100;
			gps2_fix.longitude.second = (u08)fx_minute;
		}	/* parsing longitude position */
	}	/* position data is valid */
}

void _parse_date(void) {
	char *year_str = (char *)malloc(2);
	char *month_str = (char *)malloc(2);
	char *day_str = (char *)malloc(2);
						
	strncpy(year_str,gps_buffer+GPS2_YEAR_INDEX,GPS2_YEAR_LEN);
	strncpy(month_str,gps_buffer+GPS2_MONTH_INDEX,GPS2_MONTH_LEN);
	strncpy(day_str,gps_buffer+GPS2_DAY_INDEX,GPS2_DAY_LEN);
	gps2_time.year = (strlen(year_str) == 0)?INVALID_TIME:atoi(year_str);
	free(year_str);
	gps2_time.month = (strlen(month_str) == 0)?INVALID_TIME:atoi(month_str);
	free(month_str);
	gps2_time.day = ( strlen(day_str) == 0 )?INVALID_TIME:atoi(day_str);
	free(day_str);
}	/*	_parse_date */

void _parse_time(void) {
	char *hour_str = (char *)malloc(2);
	char *minute_str = (char *)malloc(2);
	char *second_str = (char *)malloc(2);	
						
	strncpy(hour_str, gps_buffer+GPS2_HOUR_INDEX,GPS2_HOUR_LEN);
	strncpy(minute_str, gps_buffer+GPS2_MINUTE_INDEX,GPS2_MINUTE_LEN);
	strncpy(second_str, gps_buffer+GPS2_SECOND_INDEX,GPS2_SECOND_LEN);
	gps2_time.hour = (_is_missing(&hour_str))?INVALID_TIME:atoi(hour_str);
	free(hour_str);
	gps2_time.minute = (_is_missing(&minute_str))?INVALID_TIME:atoi(minute_str);
	free(minute_str);
	gps2_time.second = (_is_missing(&second_str))?INVALID_TIME:atoi(second_str);
	free(second_str);
}
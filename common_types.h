/*
 * common_types.h
 *
 * Created: 4/11/2012 9:54:48 AM
 *  Author: Owner
 */ 


#ifndef COMMON_TYPES_H_
#define COMMON_TYPES_H_

typedef struct {
	u08 year;
	u08 month;
	u08 day;
	u08 hour;
	u08 minute;
	u08 second;
} time_t;

typedef struct {
	char hemisphere;
	u08 degree;
	u08 minute;
	u08 second;
} gps_coordinate_fix_t;

typedef struct {
	gps_coordinate_fix_t latitude;
	gps_coordinate_fix_t longitude;
} gps_fix_t;

#define INVALID_DATA 0xFE
#define INVALID_TIME INVALID_DATA

#endif /* COMMON_TYPES_H_ */
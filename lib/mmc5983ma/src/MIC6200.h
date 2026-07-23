
#ifndef _MIC6200_H__
#define _MIC6200_H__
#include "stdint.h"
#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif
/*
typedef   signed char  int8_t; 		// signed 8-bit number    (-128 to +127)
typedef unsigned char  uint8_t; 	// unsigned 8-bit number  (+0 to +255)
typedef   signed short int16_t; 	// signed 16-bt number    (-32768 to +32767)
typedef unsigned short uint16_t; 	// unsigned 16-bit number (+0 to +65535)
typedef   signed int   int32_t; 	// signed 32-bt number    (-2,147,483,648 to +2,147,483,647)
typedef unsigned int   uint32_t; 	// unsigned 32-bit number (+0 to +4,294,967,295)
*/
#define MIC6200_7BITI2C_ADDRESS		0x4C

/*MIC6200 Register Map*/
#define MIC6200_CHIP_ID_REG           0x00
#define MIC6200_CHIP_VER_REG          0x01

#define MIC6200_PAGE_SEL_REG          0xFF

#define MIC6200_CHIP_ID               0xF9

#define MIC6200_CHIP_VER              0x02

int MIC6200_Init(void);
int MIC6200_Enable(void);
int MIC6200_Disable(void);
int MIC6200_Check_ID(uint8_t *chip_id);
void MIC6200_Read_Acc_Data(int16_t *acc_data);
void MIC6200_Read_Gyro_Data(int16_t *gyro_data);

// extern SENSOR_HANDLE_S MIC6200_info;

// int Get_MIC6200_ID(unsigned char *chip_id);

// int MIC6200_Enable(void);
// int MIC6200_Disable(void);				// shutdown the device


#ifdef __cplusplus
}
#endif

#endif //_MIC6200_H__


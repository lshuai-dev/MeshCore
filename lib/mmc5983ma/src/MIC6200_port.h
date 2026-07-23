#ifndef MIC6200_PORT_H
#define MIC6200_PORT_H

#include <stdint.h>

#ifndef HAL_OK
#define HAL_OK 0
#endif

#ifndef HAL_ERROR
#define HAL_ERROR -1
#endif

// Used by MIC6200_MultiRead_Reg to bound local SPI buffers.
#ifndef FIFO_WATERMAKR_SIZE
#define FIFO_WATERMAKR_SIZE 32
#endif

#ifdef __cplusplus
extern "C" {
#endif

int I2C_Read_Reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data);
int SPI_Read_Reg(uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len);
int I2C_Write_Reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data);
int SPI_Write_Reg(uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len);
int I2C_MultiRead_Reg(uint8_t dev_addr, uint8_t reg_addr, int num, uint8_t *data);
int SPI_MultiRead_Reg(uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // MIC6200_PORT_H

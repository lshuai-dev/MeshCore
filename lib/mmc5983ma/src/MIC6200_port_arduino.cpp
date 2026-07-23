#include "MIC6200_port.h"

#include "MIC6200.h"

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#ifndef MIC6200_SPI_CLOCK_HZ
#define MIC6200_SPI_CLOCK_HZ 8000000UL
#endif

#ifndef MIC6200_SPI_MODE
#define MIC6200_SPI_MODE SPI_MODE0
#endif

#ifndef SS
#define SS 10
#endif

static uint8_t s_mic6200_spi_cs_pin = SS;
static bool s_mic6200_spi_ready = false;
static SPISettings s_mic6200_spi_settings(MIC6200_SPI_CLOCK_HZ, MSBFIRST, MIC6200_SPI_MODE);

static bool I2C_Probe(TwoWire& wire, uint8_t addr) {
  wire.beginTransmission(addr);
  uint8_t error = wire.endTransmission();
  return (error == 0);
}

static TwoWire* I2C_Get_Bus()
{
  static TwoWire * g_I2C_Bus = nullptr;
  static bool g_I2C_Inited = false;
  if(!g_I2C_Inited) {
    if(I2C_Probe(Wire,MIC6200_7BITI2C_ADDRESS)) {
      g_I2C_Bus = &Wire;
    }
#if WIRE_INTERFACES_COUNT > 1
    else if(I2C_Probe(Wire1,MIC6200_7BITI2C_ADDRESS)) {
      g_I2C_Bus = &Wire1;
    }
#endif
    g_I2C_Inited = true;
  }
  
  return g_I2C_Bus;
}

static void mic6200_spi_init_once()
{
  if (s_mic6200_spi_ready)
  {
    return;
  }

  SPI.begin();
  pinMode(s_mic6200_spi_cs_pin, OUTPUT);
  digitalWrite(s_mic6200_spi_cs_pin, HIGH);
  s_mic6200_spi_ready = true;
}

extern "C" int I2C_Read_Reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data)
{
  if (data == nullptr)
  {
    return HAL_ERROR;
  }

  TwoWire* i2cBus = I2C_Get_Bus();
  if(nullptr == i2cBus) return HAL_ERROR;

  i2cBus->beginTransmission(dev_addr);
  i2cBus->write(reg_addr);
  if (i2cBus->endTransmission(false) != 0)
  {
    return HAL_ERROR;
  }

  if (i2cBus->requestFrom((int)dev_addr, 1) != 1)
  {
    return HAL_ERROR;
  }

  if (i2cBus->available() < 1)
  {
    return HAL_ERROR;
  }

  *data = (uint8_t)i2cBus->read();
  return HAL_OK;
}

extern "C" int I2C_Write_Reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data)
{
  TwoWire* i2cBus = I2C_Get_Bus();
  if(nullptr == i2cBus) return HAL_ERROR;

  i2cBus->beginTransmission(dev_addr);
  i2cBus->write(reg_addr);
  i2cBus->write(data);
  return (i2cBus->endTransmission() == 0) ? HAL_OK : HAL_ERROR;
}

extern "C" int I2C_MultiRead_Reg(uint8_t dev_addr, uint8_t reg_addr, int num, uint8_t *data)
{
  if (data == nullptr || num <= 0)
  {
    return HAL_ERROR;
  }
  TwoWire* i2cBus = I2C_Get_Bus();
  if(nullptr == i2cBus) return HAL_ERROR;

  i2cBus->beginTransmission(dev_addr);
  i2cBus->write(reg_addr);
  if (i2cBus->endTransmission(false) != 0)
  {
    return HAL_ERROR;
  }

  int requested =i2cBus->requestFrom((int)dev_addr, num);
  if (requested != num)
  {
    return HAL_ERROR;
  }

  for (int i = 0; i < num; i++)
  {
    if (i2cBus->available() < 1)
    {
      return HAL_ERROR;
    }
    data[i] = (uint8_t)i2cBus->read();
  }

  return HAL_OK;
}

extern "C" int SPI_Read_Reg(uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len)
{
  if (tx_buf == nullptr || rx_buf == nullptr || len == 0)
  {
    return HAL_ERROR;
  }

  mic6200_spi_init_once();

  SPI.beginTransaction(s_mic6200_spi_settings);
  digitalWrite(s_mic6200_spi_cs_pin, LOW);
  for (uint16_t i = 0; i < len; i++)
  {
    rx_buf[i] = SPI.transfer(tx_buf[i]);
  }
  digitalWrite(s_mic6200_spi_cs_pin, HIGH);
  SPI.endTransaction();

  return HAL_OK;
}

extern "C" int SPI_Write_Reg(uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len)
{
  if (tx_buf == nullptr || len == 0)
  {
    return HAL_ERROR;
  }

  mic6200_spi_init_once();

  SPI.beginTransaction(s_mic6200_spi_settings);
  digitalWrite(s_mic6200_spi_cs_pin, LOW);
  for (uint16_t i = 0; i < len; i++)
  {
    uint8_t in = SPI.transfer(tx_buf[i]);
    if (rx_buf != nullptr)
    {
      rx_buf[i] = in;
    }
  }
  digitalWrite(s_mic6200_spi_cs_pin, HIGH);
  SPI.endTransaction();

  return HAL_OK;
}

extern "C" int SPI_MultiRead_Reg(uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len)
{
  return SPI_Read_Reg(tx_buf, rx_buf, len);
}

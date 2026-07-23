// MIC6200.c
// I2C/SPI driver for MEMSIC MIC6200 tri axis accelerometer + tri axis Gyro
/**********************************************************************************/

// #include "Config.h"
#include "math.h"
#include "string.h"
#include "MIC6200.h"
#include "MIC6200_port.h"

#define MIC6200_SAMPLE_DRV_VER "1.0.0.21244"

#define MIC6200_DROP_SAMPLE_CNT 8

#define MAX_ODR (500) // Related with sys osc and gyro gain / osr settings

typedef enum
{
  SENSOR_COM_I2C = 0,
  SENSOR_COM_SPI = 1,
} SENSOR_COM_IF;

static SENSOR_COM_IF g_com_intf = SENSOR_COM_I2C;
static uint8_t g_mic6200_enabled = 0;
static uint8_t g_mic6200_inited = 0;
// static uint8_t g_mic6200_ver = 0;

static int sys_osc = 819000; // 512000;
static uint16_t acc_odr = 200;
static uint16_t gyro_odr = 200; // ACC and Gyro ODR need be same except either is 0
static int acc_range = 8;
static int gyro_range = 2000;
static float acc_sensitivity = 4096.0;
static float gyro_sensitivity = 16.384;

static uint8_t part_id[4] = {0};

static uint8_t acc_drop_cnt = MIC6200_DROP_SAMPLE_CNT;
static uint8_t gyro_drop_cnt = MIC6200_DROP_SAMPLE_CNT;

static int8_t sxz = 0;
static int8_t szx = 0;

static int MIC6200_Read_Reg(unsigned char reg_add, unsigned char *data)
{
  int ret = -1;

  if (g_com_intf == SENSOR_COM_I2C)
  {
    ret = I2C_Read_Reg(MIC6200_7BITI2C_ADDRESS, reg_add, data);
  }
  else if (g_com_intf == SENSOR_COM_SPI)
  {
    uint8_t tx_buf[4] = {0};
    uint8_t rx_buf[4] = {0};

    tx_buf[0] = reg_add | 0x80;
    tx_buf[1] = reg_add & 0x80;
    ret = SPI_Read_Reg(tx_buf, rx_buf, 3);
    if (ret == HAL_OK)
    {
      *data = rx_buf[2];
    }
  }

  return ret;
}

static int MIC6200_Write_Reg(unsigned char reg_add, unsigned char data)
{
  int ret = -1;

  if (g_com_intf == SENSOR_COM_I2C)
  {
    ret = I2C_Write_Reg(MIC6200_7BITI2C_ADDRESS, reg_add, data);
  }
  else if (g_com_intf == SENSOR_COM_SPI)
  {
    uint8_t tx_buf[4] = {0};
    uint8_t rx_buf[4] = {0};

    tx_buf[0] = reg_add & 0x7F;
    tx_buf[1] = reg_add & 0x80;
    tx_buf[2] = data;
    ret = SPI_Write_Reg((uint8_t *)tx_buf, (uint8_t *)rx_buf, 3);
    // delay_us(100);
  }

#ifdef MIC6200_VERIFY_REG_WRITE
  unsigned char read_data = 0;
  ret = MIC6200_Read_Reg(reg_add, &read_data);
  // if ((reg_add != 0x0F) && (reg_add != 0x10) && (read_data != data))
  // {
  //   memsic_printf("Reg 0x%x(0x%x : 0x%x) write failed\r\n", reg_add, data, read_data);
  // }
#endif

  return ret;
}

static int MIC6200_MultiRead_Reg(unsigned char reg_add, int num, unsigned char *data)
{
  int ret = -1;

  if (g_com_intf == SENSOR_COM_I2C)
  {
    ret = I2C_MultiRead_Reg(MIC6200_7BITI2C_ADDRESS, reg_add, num, data);
  }
  else if (g_com_intf == SENSOR_COM_SPI)
  {
    // uint8_t tx_buf[SPI_BURST_READ_MAX_LEN + 2] = {0};
    // uint8_t rx_buf[SPI_BURST_READ_MAX_LEN + 2] = {0};
    uint8_t tx_buf[FIFO_WATERMAKR_SIZE * 8 + 2] = {0};
    uint8_t rx_buf[FIFO_WATERMAKR_SIZE * 8 + 2] = {0};

    if (num > (FIFO_WATERMAKR_SIZE * 8))
    {
      num = FIFO_WATERMAKR_SIZE * 8;
    }

    tx_buf[0] = reg_add | 0x80;
    tx_buf[1] = reg_add & 0x80;

    ret = SPI_MultiRead_Reg(tx_buf, rx_buf, num + 2);
    if (ret == HAL_OK)
    {
      memcpy(data, &rx_buf[2], num);
    }
  }

  if (ret == HAL_OK)
  {
    return num;
  }
  else
  {
    return ret;
  }
}

/**********************************************************************************/
int MIC6200_Check_ID(uint8_t *chip_id)
{
  uint8_t id;
  uint8_t ver;

  if(HAL_OK != MIC6200_Write_Reg(MIC6200_PAGE_SEL_REG, 0x00)) return -1;
  if(HAL_OK != MIC6200_Read_Reg(MIC6200_CHIP_ID_REG, &id)) return -2;
  *chip_id = id;

  if (id != MIC6200_CHIP_ID) return -3;

  if(HAL_OK != MIC6200_Read_Reg(MIC6200_CHIP_VER_REG, &ver)) return -4;
  if (ver != MIC6200_CHIP_VER) {
    *chip_id = ver;
    return -5;
  }

  return 0;
}

int MIC6200_Enter_Sleep(void)
{
  MIC6200_Write_Reg(MIC6200_PAGE_SEL_REG, 0x00);
  MIC6200_Write_Reg(0x2B, 0x00);
  MIC6200_Write_Reg(0x2C, 0x00);

  MIC6200_Write_Reg(0x4B, 0x00);
  MIC6200_Write_Reg(0x40, 0x00);
  MIC6200_Write_Reg(0x23, 0x00);
  return 0;
}

static int MIC6200_Setup(void)
{
  // power domain reset
  MIC6200_Write_Reg(0xFF, 0x00);
  MIC6200_Write_Reg(0x40, 0x33); // Switch to Standby mode
  MIC6200_Write_Reg(0x23, 0x80); // reset
  delay(1);
  MIC6200_Write_Reg(0x40, 0x33); // Switch to Standby mode
  MIC6200_Write_Reg(0x23, 0x77); // XL / GYRO / FIFO Power On & RESET
  delay(1);
  MIC6200_Write_Reg(0x23, 0x07); // XL / GYRO / FIFO Power On
  MIC6200_Write_Reg(0x23, 0x0F); // XL / GYRO / FIFO Power On, HOLD Disable

  MIC6200_Write_Reg(0x49, 0xC8); // Analog Control 2. CP_VPM_CLK = PLL Clk 64x use xC8 (1.6MHz)
  MIC6200_Write_Reg(0x4B, 0x00); // Gyro Drive Control, disable drive loop and PLL/AGC

  MIC6200_Write_Reg(0x4C, 0x0B); // X Axis: Sense Control 1. SCSA_EN, SMIX_EN, SCCSA_VDCIN_CTRL_EN
  MIC6200_Write_Reg(0x4D, 0x0B); // Y Axis: Sense Control 1. SCSA_EN, SMIX_EN, SCCSA_VDCIN_CTRL_EN
  MIC6200_Write_Reg(0x4E, 0x0B); // Z Axis: Sense Control 1. SCSA_EN, SMIX_EN, SCCSA_VDCIN_CTRL_EN

  MIC6200_Write_Reg(0x52, 0xA7); // Analog Control. Temp sensor Enable, PMU ALDO GYR bias current to minimum 3uA, it is benefit gyro noise for some devices at low temperature

  MIC6200_Write_Reg(0x56, 0x5A); // AGC ON

  MIC6200_Write_Reg(0x59, 0x84); // KP	new default x94 after wake up (prev set to x84 in Ap3), pollux x54 for Highgain if enable
  MIC6200_Write_Reg(0x5C, 0x00); // High SGAIN count (Ap3 uses 0x01), x01
  MIC6200_Write_Reg(0x5B, 0x50); // High SGAIN count (Ap3 uses 0x00), x2C

  // Enable dithering
  MIC6200_Write_Reg(0xE7, 0xAA); // Dithering ampl to 66% for drive, X, Y, Z Note, dithering amplitude control has been moved to Pg0 0xE7
  MIC6200_Write_Reg(0x7E, 0x03); // PRBS Control

  // PMU ibias fixed setting
  MIC6200_Write_Reg(0xF4, 0xA2); // SenZ/Y CSA ITRIM 1.25uA Also has PMU VCM ITRIM at 0.3uA and DRV BUF ITRIm at 1.6uA, Could not find SEN_X_CSA_OP_ITRIM. NOTE: SEN_X_CSA_OP_ITRIM is at Pg0 0xF9 [1:0]
  MIC6200_Write_Reg(0xF5, 0x1C); // PMU ITRIM 3  Max out the dcsa current
  MIC6200_Write_Reg(0xF7, 0x55); // PMU ITRIM 5  X & Y sense GSDM = '101': 0.4uA / 0.15uA
  MIC6200_Write_Reg(0xF8, 0x55); // PMU ITRIM 6  Z sense GSDM = '101': 0.4uA / 0.15uA, DRV GSDM = '101': 0.4uA / 0.15uA

  // Notch filter and selftest
  MIC6200_Write_Reg(0xD8, 0x10); // disable notch filter now
  MIC6200_Write_Reg(0xDC, 0x18); // notch filter decimation 24(minimum)
  MIC6200_Write_Reg(0xDB, 0x3F); // Disable all pilot tone TCO/TCS comp
  MIC6200_Write_Reg(0xE6, 0x0F); // Powerdown dummy, issue#1 for Pollux_B0, default 0x0F in B2,no fix

  // For Qmeas
  MIC6200_Write_Reg(0xE1, 0x47);
  MIC6200_Write_Reg(0xE3, 0x7E);
  MIC6200_Write_Reg(0xE0, 0x50);

  MIC6200_Write_Reg(0xFF, 0x02);
  MIC6200_Write_Reg(0x9E, 0xF0); // Vctrl output when setpoint exceed upper limit
  MIC6200_Write_Reg(0xAF, 0x10); // Vctrl output when setpoint under lower limit
  MIC6200_Write_Reg(0xA1, 0xA4);

  // Qmeas channel mode
  MIC6200_Write_Reg(0xA6, 0x20);

  MIC6200_Write_Reg(0xC8, 0x3F); // Sense CSA Slow Clock Select/SCSA RFB Itrim

  MIC6200_Write_Reg(0xFF, 0x00);

  MIC6200_Write_Reg(0x57, 0x08); // Trigger Manual of VCTRL
  MIC6200_Write_Reg(0x57, 0x88); // Trigger Manual of VCTRL
  MIC6200_Write_Reg(0x57, 0x08); // Trigger Manual of VCTRL

  MIC6200_Write_Reg(0x23, 0x00);
  MIC6200_Write_Reg(0x40, 0x00);

  return 0;
}

static void MIC6200_Read_PartID(void)
{
  MIC6200_Write_Reg(0xFF, 0x01);

  MIC6200_MultiRead_Reg(0x4C, 4, part_id);

  MIC6200_Write_Reg(0xFF, 0x00);
}

static void MIC6200_Read_Sensitivity_Coef(void)
{
  uint8_t reg_data = 0;
  uint8_t tmp_data = 0;

  MIC6200_Write_Reg(0xFF, 0x01);

  MIC6200_Read_Reg(0x4A, &reg_data);

  tmp_data = reg_data & 0x0F;
  if (tmp_data > 0x08)
  {
    sxz = (int8_t)(tmp_data - 16);
  }
  else
  {
    sxz = (int8_t)(tmp_data);
  }

  tmp_data = (reg_data & 0xF0) >> 4;
  if (tmp_data > 0x08)
  {
    szx = (int8_t)(tmp_data - 16);
  }
  else
  {
    szx = (int8_t)(tmp_data);
  }

  MIC6200_Write_Reg(0xFF, 0x00);
}
static void MIC6200_Set_Gyro_Odr(int odr)
{
  if (odr != 0)
  {
    uint8_t odr_ctl = 0;
    int odr_prescale = 0;

    MIC6200_Write_Reg(MIC6200_PAGE_SEL_REG, 0x00);

    MIC6200_Read_Reg(0x3C, &odr_ctl);
    odr_ctl = odr_ctl & 0xF9;

    odr_prescale = sys_osc / odr;
    if (odr_prescale <= 255)
    {
      odr_ctl = odr_ctl | 0x01;
      MIC6200_Write_Reg(0x3C, odr_ctl);
      MIC6200_Write_Reg(0x3A, (uint8_t)odr_prescale);
    }
    else if (odr_prescale <= (255 * 32))
    {
      odr_ctl = odr_ctl | 0x03;
      MIC6200_Write_Reg(0x3C, odr_ctl);
      odr_prescale = sys_osc / 32 / odr;
      MIC6200_Write_Reg(0x3A, (uint8_t)odr_prescale);
    }
    else if (odr_prescale <= (255 * 256))
    {
      odr_ctl = odr_ctl | 0x05;
      MIC6200_Write_Reg(0x3C, odr_ctl);
      odr_prescale = sys_osc / 256 / odr;
      MIC6200_Write_Reg(0x3A, (uint8_t)odr_prescale);
    }
    else
    {
      odr_ctl = odr_ctl | 0x07;
      MIC6200_Write_Reg(0x3C, odr_ctl);
      odr_prescale = sys_osc / 16384 / odr;
      MIC6200_Write_Reg(0x3A, (uint8_t)odr_prescale);
    }
  }
}
static void MIC6200_Set_Acc_Odr(int odr)
{
  if (odr != 0)
  {
    uint8_t odr_ctl = 0;
    int odr_prescale = 0;

    MIC6200_Write_Reg(MIC6200_PAGE_SEL_REG, 0x00);

    MIC6200_Read_Reg(0x3C, &odr_ctl);
    odr_ctl = odr_ctl & 0x9F;

    odr_prescale = sys_osc / odr;
    if (odr_prescale <= 255)
    {
      odr_ctl = odr_ctl | 0x10;
      MIC6200_Write_Reg(0x3C, odr_ctl);
      MIC6200_Write_Reg(0x3B, (uint8_t)odr_prescale);
    }
    else if (odr_prescale <= (255 * 32))
    {
      odr_ctl = odr_ctl | 0x30;
      MIC6200_Write_Reg(0x3C, odr_ctl);
      odr_prescale = sys_osc / 32 / odr;
      MIC6200_Write_Reg(0x3B, (uint8_t)odr_prescale);
    }
    else if (odr_prescale <= (255 * 256))
    {
      odr_ctl = odr_ctl | 0x50;
      MIC6200_Write_Reg(0x3C, odr_ctl);
      odr_prescale = sys_osc / 256 / odr;
      MIC6200_Write_Reg(0x3B, (uint8_t)odr_prescale);
    }
    else
    {
      odr_ctl = odr_ctl | 0x70;
      MIC6200_Write_Reg(0x3C, odr_ctl);
      odr_prescale = sys_osc / 16384 / odr;
      MIC6200_Write_Reg(0x3B, (uint8_t)odr_prescale);
    }
  }
}

static void set_acc_range(int range)
{
  MIC6200_Write_Reg(MIC6200_PAGE_SEL_REG, 0x00);
  switch (range)
  {
  case 2:
    MIC6200_Write_Reg(0x44, 0x10);
    acc_sensitivity = 16384;
    break;

  case 4:
    MIC6200_Write_Reg(0x44, 0x11);
    acc_sensitivity = 8192;
    break;

  case 8:
    MIC6200_Write_Reg(0x44, 0x12);
    acc_sensitivity = 4096;
    break;

  case 12:
    MIC6200_Write_Reg(0x44, 0x14);
    acc_sensitivity = 2730;
    break;

  case 16:
    MIC6200_Write_Reg(0x44, 0x13);
    acc_sensitivity = 2048;
    break;

  case 24:
    MIC6200_Write_Reg(0x44, 0x15);
    acc_sensitivity = 1365;
    break;
  }
}

static void set_gyro_range(int range)
{
  MIC6200_Write_Reg(MIC6200_PAGE_SEL_REG, 0x00);
  switch (range)
  {
  case 2000:
    MIC6200_Write_Reg(0x43, 0x90);
    gyro_sensitivity = 16.384;
    break;

  case 1000:
    MIC6200_Write_Reg(0x43, 0x91);
    gyro_sensitivity = 32.768;
    break;

  case 500:
    MIC6200_Write_Reg(0x43, 0x92);
    gyro_sensitivity = 65.536;
    break;

  case 250:
    MIC6200_Write_Reg(0x43, 0x93);
    gyro_sensitivity = 131.072;
    break;

  case 125:
    MIC6200_Write_Reg(0x43, 0x94);
    gyro_sensitivity = 262.144;
    break;
  }
}

static void MIC6200_Enable_6Axis(void)
{
  // Enable power domain
  MIC6200_Write_Reg(0xFF, 0x00);

  MIC6200_Write_Reg(0x50, 0x81); // XL continuous, use system clock, set to frontend, otherwise will rail-rail
  MIC6200_Write_Reg(0x40, 0x33); // Modes (Ctrl 1).                     GY + XL ASYNC mode, STANDBY Mode
  MIC6200_Write_Reg(0x23, 0x77);
  delay(1);
  MIC6200_Write_Reg(0x23, 0x07);
  MIC6200_Write_Reg(0x23, 0x0F);
  MIC6200_Write_Reg(0x50, 0x89); // XL continuous, use PLL clock

  // temp sensor clock
  MIC6200_Write_Reg(0x1D, 0x04);

  // For XL
  MIC6200_Write_Reg(0xF6, 0x29);
  MIC6200_Write_Reg(0xF9, 0xA2);
  MIC6200_Write_Reg(0x53, 0xD3); // Drive Sense                         agc mixer enabled, drv_buf=off and bypassed, issue #41;PMU_ALDO_XL_IB_N=69uA

  // system clock and ODR
  MIC6200_Write_Reg(0x39, 0x88);
  // MIC6200_Write_Reg(0x3A,	0x40);
  // MIC6200_Write_Reg(0x3B,	0x80);
  // MIC6200_Write_Reg(0x3C,	0x33);
  MIC6200_Set_Gyro_Odr(gyro_odr);
  MIC6200_Set_Acc_Odr(acc_odr);
  MIC6200_Write_Reg(0x3D, 0x00);
  MIC6200_Write_Reg(0x3E, 0x00);
  MIC6200_Write_Reg(0x3F, 0x00);

  // Gyro/XL Control Setup
  MIC6200_Write_Reg(0x41, 0x00); // Channel Enable (Ctrl 2).            Bit7 controls the signal PMU_TSR_PD_O. Disabled by HW in SLEEP mode, becomes available in STANDBY and WAKE modes
  MIC6200_Write_Reg(0x42, 0x42); // OSR (Ctrl 3)                        Gyro OSR128, IDR=12.8k/24, XL OSR is 512, XL WTD filter is disabled.
  // MIC6200_Write_Reg(0x43,	0x90); // Gyro Options (Ctrl 4).              Gyro Options (Ctrl 4).'_0[Gyro scale is set to +/-2000dps]'9_[Gyro filter is set to Sinc3, Clamp logic is enabled (Ap2 mode)]
  // MIC6200_Write_Reg(0x44,	0x12); // XL Options (Ctrl 5).                Disable XL_PWR_CTRL, disable XL_DUTY_CYCLE clocks, set SINC order to 3, set +/-8g range
  set_gyro_range(gyro_range);
  set_acc_range(acc_range);
  MIC6200_Write_Reg(0x46, 0x80); // XL Decimation (Ctrl 7).             0x0: 1 samples, Bit7 = '0', Decimation count = Sinc Order + 1; If Bit7 ='1' Decimation count is power of 2
  MIC6200_Write_Reg(0x47, 0x20); // Gyro/XL/TCO (Ctrl 8).        		Disable XL UPDATE mode, DRV_CMP-auto zero, XL analog PWR ctrl b[2]=1 =>enables the SDM pwr mngmt btwn samples: bit 2 (XL_ANA_PWR_CTRL) SDM power down between samples is ignored when register Pg0 0x50 bit 7 (XL_CONT_MODE) is enabled.
  MIC6200_Write_Reg(0x48, 0x00); // Analog Control 1.

  // Gyro LPF for notch Enable
  MIC6200_Write_Reg(0x60, 0x05);
  MIC6200_Write_Reg(0x61, 0x0F);
  MIC6200_Write_Reg(0x62, 0x0B);
  MIC6200_Write_Reg(0x63, 0x1E);
  MIC6200_Write_Reg(0x64, 0x05);
  MIC6200_Write_Reg(0x65, 0x0F);
  MIC6200_Write_Reg(0x66, 0xA1);
  MIC6200_Write_Reg(0x67, 0x69);
  MIC6200_Write_Reg(0x68, 0xB7);
  MIC6200_Write_Reg(0x69, 0x25);

  MIC6200_Write_Reg(0x45, 0x11); // Gyro/XL Filter (Ctrl 6).            Enable Gyro/XL LPF
  MIC6200_Write_Reg(0xD8, 0x17); // Enable notch filter

  // XL LPF
  MIC6200_Write_Reg(0x74, 0x15);
  MIC6200_Write_Reg(0x75, 0x01);
  MIC6200_Write_Reg(0x76, 0x29);
  MIC6200_Write_Reg(0x77, 0x02);
  MIC6200_Write_Reg(0x78, 0x15);
  MIC6200_Write_Reg(0x79, 0x01);
  MIC6200_Write_Reg(0x7A, 0xA5);
  MIC6200_Write_Reg(0x7B, 0xDC);
  MIC6200_Write_Reg(0x7C, 0xF7);
  MIC6200_Write_Reg(0x7D, 0x60);

  // Drive gain
  MIC6200_Write_Reg(0xFF, 0x01);
  MIC6200_Write_Reg(0x31, 0x00);

  MIC6200_Write_Reg(0xFF, 0x02);

  // YCO/TCS comp
  //  MIC6200_Write_Reg(0x93,	0x3F);  // GYRO_TCO_TCS DEBUG CTRL REG: 				TCO/TCS BYPASS EN
  //  MIC6200_Write_Reg(0x94,	0x30);  // GYRO_XL TCO_TCS_Debug CTRL REG: 			XL TCO/TCS BYPASS EN

  // XL ibias
  MIC6200_Write_Reg(0xC0, 0x00);

  // AGC work
  MIC6200_Write_Reg(0xFF, 0x00); //	Page0

  // gain swich will lost, after power domin off
  MIC6200_Write_Reg(0x54, 0x23); // Toggle gain switch count
  MIC6200_Write_Reg(0x54, 0x2B); // Toggle gain switch count, enable AGC LPF
  MIC6200_Write_Reg(0x54, 0x23); // Toggle gain switch count

  MIC6200_Write_Reg(0x4A, 0x48); // VPM = 20V, charge pump clock enable
  MIC6200_Write_Reg(0x57, 0x08); // Manual VCTRL DAC
  MIC6200_Write_Reg(0x4B, 0xDD); // Gyro Drive Control, enable drive loop and PLL/AGC
  MIC6200_Write_Reg(0x51, 0x85); // PLL_OSC_VCO idle, may no need?
  delay(1);

  MIC6200_Write_Reg(0x51, 0x05); // Exit idle mode
  delay(4);                   // Delay. Generally it need 1ms to 2ms to lock PLL
  MIC6200_Write_Reg(0xFF, 0x01); //	Page1
  MIC6200_Write_Reg(0x31, 0x25); // DCSA gain: higher gain for 20V device to reach ~3.2um displacement
  MIC6200_Write_Reg(0xFF, 0x00); //	Page0
  MIC6200_Write_Reg(0x57, 0x02); //	AGC_CTRL_REG_2			AGC OSR mode (set to 0x40 for OSR32, 0x00 for OSR64)

  // output,
  MIC6200_Write_Reg(0x40, 0x31); // Modes (Ctrl 1). 		6axis Gyro + Accel Wake Mode
  delay(7);                   // Delay. To DCSA target amplitude need maximum ~12ms(from 0mV), and high gain stage ~2.8ms, here set 10ms as it is not from 0mV start
  MIC6200_Write_Reg(0x4A, 0x40); // Disable the DRV charge pump, it only need at startup stage to speed time
}

static void MIC6200_Enable_3Axis(void)
{
  // Enable power domain
  MIC6200_Write_Reg(0xFF, 0x00);

  MIC6200_Write_Reg(0x50, 0x81); // XL continuous, use system clock, set to frontend, otherwise swtich from 6axis mode will rail-rail
  MIC6200_Write_Reg(0x40, 0x23); // Modes (Ctrl 1).                     XL STANDBY Mode
  MIC6200_Write_Reg(0x23, 0x66);
  delay(1);
  MIC6200_Write_Reg(0x23, 0x06);
  MIC6200_Write_Reg(0x23, 0x0E);

  // temp sensor clock
  MIC6200_Write_Reg(0x1D, 0x04);

  // For XL
  MIC6200_Write_Reg(0xF6, 0x29);
  MIC6200_Write_Reg(0xF9, 0xA2);
  MIC6200_Write_Reg(0x53, 0x43); // Drive Sense Ctrl Reg.               Bit6 DRV BUF PD EN, PMU_ALDO_XL_IB_N[2:0], '11' = 69nA

  // system clock and ODR
  MIC6200_Write_Reg(0x39, 0x88);
  // MIC6200_Write_Reg(0x3B,	0x80);
  // MIC6200_Write_Reg(0x3C,	0x33);
  MIC6200_Set_Acc_Odr(acc_odr);
  MIC6200_Write_Reg(0x3E, 0x00);
  MIC6200_Write_Reg(0x3F, 0x00);

  // Gyro/XL Control Setup
  MIC6200_Write_Reg(0x41, 0x07); // Gyro disable
  MIC6200_Write_Reg(0x42, 0x42); // OSR (Ctrl 3)                        XL OSR is 512, XL WTD filter is disabled.
  // MIC6200_Write_Reg(0x44,	0x12);  // XL Options (Ctrl 5).                Disable XL_PWR_CTRL, disable XL_DUTY_CYCLE clocks, set SINC order to 3, set +/-8g range
  set_acc_range(acc_range);
  MIC6200_Write_Reg(0x46, 0x80); // XL Decimation (Ctrl 7).             0x0: 1 samples, Bit7 = '0', Decimation count = Sinc Order + 1; If Bit7 ='1' Decimation count is power of 2
  MIC6200_Write_Reg(0x47, 0x24); // b[2]=1 =>enables the SDM pwr mngmt btwn samples: bit 2 (XL_ANA_PWR_CTRL) SDM power down between samples is ignored when register Pg0 0x50 bit 7 (XL_CONT_MODE) is enabled.
  MIC6200_Write_Reg(0x48, 0x81); // XL only mode is enabled. allows the analog to reduce/remove power to unneeded. Gyro_aldo PD.

  MIC6200_Write_Reg(0x45, 0x11); // Gyro/XL Filter (Ctrl 6).            Enable Gyro/XL LPF

  // XL LPF
  MIC6200_Write_Reg(0x74, 0x15);
  MIC6200_Write_Reg(0x75, 0x01);
  MIC6200_Write_Reg(0x76, 0x29);
  MIC6200_Write_Reg(0x77, 0x02);
  MIC6200_Write_Reg(0x78, 0x15);
  MIC6200_Write_Reg(0x79, 0x01);
  MIC6200_Write_Reg(0x7A, 0xA5);
  MIC6200_Write_Reg(0x7B, 0xDC);
  MIC6200_Write_Reg(0x7C, 0xF7);
  MIC6200_Write_Reg(0x7D, 0x60);

  MIC6200_Write_Reg(0xFF, 0x02);

  // YCO/TCS comp
  //  MIC6200_Write_Reg(0x94,	0x30);  // GYRO_XL TCO_TCS_Debug CTRL REG: 			XL TCO/TCS BYPASS EN

  // XL ibias
  MIC6200_Write_Reg(0xC0, 0x00);

  // AGC work
  MIC6200_Write_Reg(0xFF, 0x00); //	Page0

  MIC6200_Write_Reg(0x4B, 0x00); // Gyro Drive Control, disable drive loop and PLL/AGC

  // output
  MIC6200_Write_Reg(0x40, 0x21); // Modes (Ctrl 1). 		6axis Gyro + Accel Wake Mode
}

int MIC6200_Check_PLL_Status(void)
{
  uint8_t reg_data = 0;

  MIC6200_Write_Reg(0xFF, 0x00); //	Page0

  MIC6200_Read_Reg(0x03, &reg_data);
  // memsic_printf("PLL Lock status is 0x%x\n", reg_data);
  if ((reg_data & 0x20) == 0) // PLL is unlock, need restore it
  {
    MIC6200_Write_Reg(0xFF, 0x01); //	Page1
    MIC6200_Write_Reg(0x31, 0x00);

    MIC6200_Write_Reg(0xFF, 0x00); //	Page0
    MIC6200_Write_Reg(0x4A, 0x4C);
    delay(5); // Need be careful the delay in case in interrupt handler
    MIC6200_Write_Reg(0x4A, 0x48);
    delay(10); // Need be careful the delay in case in interrupt handler

    MIC6200_Write_Reg(0xFF, 0x01); //	Page1
    MIC6200_Write_Reg(0x31, 0x29);

    MIC6200_Write_Reg(0xFF, 0x00); //	Page0
    MIC6200_Write_Reg(0x4A, 0x40);

    return 1; // maybe need drop current data
  }

  return 0; // PLL is lock, no need drop current data
}

int MIC6200_Init(void)
{
  if (g_mic6200_inited == 0)
  {
    // MIC6200_Read_Reg(0x03, reg_val);
    // memsic_printf("MIC6200 Status is 0x%x\r\n", reg_val[0]);

    if (MIC6200_Setup() < 0)
    {
      // memsic_printf("MIC6200 Initialization failed\r\n");
      return -1;
    }

    // MIC6200_Read_Reg(0x03, reg_val);
    // memsic_printf("MIC6200 Status is 0x%x After initialization\r\n", reg_val[0]);
    MIC6200_Read_PartID();

    MIC6200_Read_Sensitivity_Coef();

    // MIC6200_Set_Acc_Odr(acc_odr);
    // MIC6200_Set_Gyro_Odr(gyro_odr);

    g_mic6200_inited = 1;
  }

  return 0;
}

/**********************************************************************************/
int MIC6200_Enable(void)
{

  if (g_mic6200_enabled != 0)
  {
    return 1;
  }
  acc_drop_cnt = MIC6200_DROP_SAMPLE_CNT;
  gyro_drop_cnt = MIC6200_DROP_SAMPLE_CNT;

  MIC6200_Enable_6Axis();
  // MIC6200_Enable_3Axis();
  g_mic6200_enabled = 1;
  return 1;
}

int MIC6200_Disable(void)
{
  if (g_mic6200_enabled == 0)
    return 1;
  // MIC6200_Enter_Standby();
  MIC6200_Enter_Sleep();
  g_mic6200_enabled = 0;

  return 1;
}

void MIC6200_Read_Acc_Data(int16_t *acc_data)
{
  uint8_t buffer[6] = {0};
  // int16_t  acc_data[3] = {0};
  int ret = 0;

  MIC6200_Write_Reg(MIC6200_PAGE_SEL_REG, 0x00);

  ret = MIC6200_MultiRead_Reg(0x0E, 6, buffer);
  if (ret > 0)
  {
    if (acc_drop_cnt == 0)
    {

      acc_data[0] = (buffer[1] << 8 | buffer[0]);
      acc_data[1] = (buffer[3] << 8 | buffer[2]);
      acc_data[2] = (buffer[5] << 8 | buffer[4]);

      // sensor_report_acc_data(MIC6200, data);
    }
    else
    {
      acc_drop_cnt--;
    }
  }
}

void MIC6200_Read_Gyro_Data(int16_t *gyro_data)
{
  uint8_t buffer[6] = {0};
  // uint16_t data[3] = {0};
  int ret = 0;

  if (MIC6200_Check_PLL_Status() != 0)
  {
    // PLL is unlock, need drop current data
    gyro_drop_cnt = 1;
  }

  MIC6200_Write_Reg(MIC6200_PAGE_SEL_REG, 0x00);

  ret = MIC6200_MultiRead_Reg(0x08, 6, buffer);
  if (ret > 0)
  {
    if (gyro_drop_cnt == 0)
    {
      int16_t tmp_data[3] = {0};
      tmp_data[0] = (int16_t)(buffer[1] << 8 | buffer[0]);
      tmp_data[1] = (int16_t)(buffer[3] << 8 | buffer[2]);
      tmp_data[2] = (int16_t)(buffer[5] << 8 | buffer[4]);

      // data[0] = (uint16_t)(tmp_data[0] + sxz * tmp_data[2] / 100);
      // data[1] = (uint16_t)(tmp_data[1]);
      // data[2] = (uint16_t)(tmp_data[2] + szx * tmp_data[0] / 100);

      gyro_data[0] = tmp_data[0] + sxz * tmp_data[2] / 100;
      gyro_data[1] = tmp_data[1];
      gyro_data[2] = tmp_data[2] + szx * tmp_data[0] / 100;
      // Serial.printf("gyro: %d %d %d \r\n", gyro_data[0], gyro_data[1], gyro_data[2]);

      // sensor_report_gyro_data(MIC6200, data);
    }
    else
    {
      gyro_drop_cnt--;
    }
  }
}

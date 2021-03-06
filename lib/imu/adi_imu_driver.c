/**
  * Copyright (c) 2020 Analog Devices, Inc. All Rights Reserved.
  * This software is proprietary to Analog Devices, Inc. and its licensors.
  *
  * Use of this file is governed by the license agreement
  * included in this repository.
  *
  * @file		adi_imu_driver.c
  * @author		Sundar Palani (sundar.palani@analog.com)
  * @brief 		Driver interface for ADIS16xxx IMU.
 **/

#include "adi_imu_driver.h"

/* Burst read transmit buf */
static const uint8_t BURST_REQ[MAX_BRF_LEN_BYTES] = {REG_BURST_CMD, 0x00};

int adi_imu_Init (adi_imu_Device_t *pDevice)
{
    int ret = Err_imu_Success_e;
    
    /* check SPI clock frequency */
    if (pDevice->devType == IMU_HW_SPI && pDevice->spiDev.speed > IMU_MAX_SPI_CLK && pDevice->enable_buffer == IMU_FALSE) 
    {
        DEBUG_PRINT("Warning: SPI clock out of range (%d Hz) (Setting to max speed = %d Hz)\n", pDevice->spiDev.speed, IMU_MAX_SPI_CLK);
        pDevice->spiDev.speed = IMU_MAX_SPI_CLK;
        if ((ret = hw_Init(pDevice)) < 0) return ret;
    }

    /* check stall time */
    if (pDevice->devType == IMU_HW_SPI && pDevice->spiDev.delay < IMU_MIN_STALL_US && pDevice->enable_buffer == IMU_FALSE) 
    {
        DEBUG_PRINT("Warning: SPI STALL time is low (%d us) (Setting to min stall time = %d us)\n", pDevice->spiDev.delay, IMU_MIN_STALL_US);
        pDevice->spiDev.delay = IMU_MIN_STALL_US;
    }

    if (pDevice->devType != IMU_HW_SPI && pDevice->enable_buffer == IMU_FALSE) 
    {
        DEBUG_PRINT("Error: Only SPI is supported.\n");
        return Err_imu_UnsupportedProtocol_e;
    }

    /* read current page ID */
    if ((ret = hw_GetPage(pDevice)) < 0) return ret;
    
    /* read and verify product id */
    uint16_t prodId = 0x0000;
    if ((ret = hw_ReadReg(pDevice, REG_PROD_ID, &prodId)) < 0) return ret;
    if (prodId != pDevice->prodId) {
        DEBUG_PRINT("Error: IMU product-id verification failed: Expected: %d, Read: %d.\n", pDevice->prodId, prodId);
        return Err_imu_ProdIdVerifyFailed_e;
    }
    else DEBUG_PRINT("\nIMU product ADIS%d found.\n\n", prodId);

    /* read range model for future gyro scale calc */
    if ((ret = hw_ReadReg(pDevice, REG_RANG_MDL, &(pDevice->rangeModel))) < 0) return ret;

    return ret;
}

static double getAccl16bitRes(adi_imu_Device_t *pDevice)
{
    switch(pDevice->prodId){
        case 16465:
            return IMU_RES_ACCL16_465;
        case 16467:
            return IMU_RES_ACCL16_467;
        case 16475:
            return IMU_RES_ACCL16_475;
        case 16477:
            return IMU_RES_ACCL16_477;
        case 16495:
            return IMU_RES_ACCL16_495;
        case 16497:
            return IMU_RES_ACCL16_497;
        case 16505:
            return IMU_RES_ACCL16_505;
        case 16507:
            return IMU_RES_ACCL16_507;
        case 16545:
            return IMU_RES_ACCL16_545;
        case 16547:
            return IMU_RES_ACCL16_547;
        default:
            return 1.0;
    }
}

static double getGyro16bitRes(adi_imu_Device_t *pDevice)
{
    double defScale = 1.0;
    uint16_t model = pDevice->rangeModel;
    
    if(pDevice->prodId == 16465 || pDevice->prodId == 16467)
        defScale = (model == 0x3) ? IMU_RES_GYRO16_46x1 : (model == 0x7) ? IMU_RES_GYRO16_46x2 : (model == 0xF) ? IMU_RES_GYRO16_46x3 : defScale;
    else if(pDevice->prodId == 16475 || pDevice->prodId == 16477)
        defScale = (model == 0x3) ? IMU_RES_GYRO16_47x1 : (model == 0x7) ? IMU_RES_GYRO16_47x2 : (model == 0xF) ? IMU_RES_GYRO16_47x3 : defScale;
    else if(pDevice->prodId == 16495 || pDevice->prodId == 16497)
        defScale = (model == 0x3) ? IMU_RES_GYRO16_49x1 : (model == 0x7) ? IMU_RES_GYRO16_49x2 : (model == 0xF) ? IMU_RES_GYRO16_49x3 : defScale;
    else if(pDevice->prodId == 16505 || pDevice->prodId == 16507)
        defScale = (model == 0x3) ? IMU_RES_GYRO16_50x1 : (model == 0x7) ? IMU_RES_GYRO16_50x2 : (model == 0xF) ? IMU_RES_GYRO16_50x3 : defScale;
    else if(pDevice->prodId == 16545 || pDevice->prodId == 16547)
        defScale = (model == 0x3) ? IMU_RES_GYRO16_54x1 : (model == 0x7) ? IMU_RES_GYRO16_54x2 : (model == 0xF) ? IMU_RES_GYRO16_54x3 : defScale;
    else
        defScale = 1.0;
    return defScale;
}

static double getAccl32bitRes(adi_imu_Device_t *pDevice)
{
    switch(pDevice->prodId){
        case 16465:
            return IMU_RES_ACCL32_465;
        case 16467:
            return IMU_RES_ACCL32_467;
        case 16475:
            return IMU_RES_ACCL32_475;
        case 16477:
            return IMU_RES_ACCL32_477;
        case 16495:
            return IMU_RES_ACCL32_495;
        case 16497:
            return IMU_RES_ACCL32_497;
        case 16505:
            return IMU_RES_ACCL32_505;
        case 16507:
            return IMU_RES_ACCL32_507;
        case 16545:
            return IMU_RES_ACCL32_545;
        case 16547:
            return IMU_RES_ACCL32_547;
        default:
            return 1.0;
    }
}

static double getGyro32bitRes(adi_imu_Device_t *pDevice)
{
    double defScale = 1.0;
    uint16_t model = pDevice->rangeModel;
    
    if(pDevice->prodId == 16465 || pDevice->prodId == 16467)
        defScale = (model == 0x3) ? IMU_RES_GYRO32_46x1 : (model == 0x7) ? IMU_RES_GYRO32_46x2 : (model == 0xF) ? IMU_RES_GYRO32_46x3 : defScale;
    else if(pDevice->prodId == 16475 || pDevice->prodId == 16477)
        defScale = (model == 0x3) ? IMU_RES_GYRO32_47x1 : (model == 0x7) ? IMU_RES_GYRO32_47x2 : (model == 0xF) ? IMU_RES_GYRO32_47x3 : defScale;
    else if(pDevice->prodId == 16495 || pDevice->prodId == 16497)
        defScale = (model == 0x3) ? IMU_RES_GYRO32_49x1 : (model == 0x7) ? IMU_RES_GYRO32_49x2 : (model == 0xF) ? IMU_RES_GYRO32_49x3 : defScale;
    else if(pDevice->prodId == 16505 || pDevice->prodId == 16507)
        defScale = (model == 0x3) ? IMU_RES_GYRO32_50x1 : (model == 0x7) ? IMU_RES_GYRO32_50x2 : (model == 0xF) ? IMU_RES_GYRO32_50x3 : defScale;
    else if(pDevice->prodId == 16545 || pDevice->prodId == 16547)
        defScale = (model == 0x3) ? IMU_RES_GYRO32_54x1 : (model == 0x7) ? IMU_RES_GYRO32_54x2 : (model == 0xF) ? IMU_RES_GYRO32_54x3 : defScale;
    else
        defScale = 1.0;
    return defScale;
}

static float getTempRes(adi_imu_Device_t *pDevice)
{
    switch(pDevice->prodId){
        case 16465:
            return IMU_RES_TEMP_46x;
        case 16467:
            return IMU_RES_TEMP_46x;
        case 16475:
            return IMU_RES_TEMP_47x;
        case 16477:
            return IMU_RES_TEMP_47x;
        case 16495:
            return IMU_RES_TEMP_49x;
        case 16497:
            return IMU_RES_TEMP_49x;
        case 16505:
            return IMU_RES_TEMP_50x;
        case 16507:
            return IMU_RES_TEMP_50x;
        case 16545:
            return IMU_RES_TEMP_54x;
        case 16547:
            return IMU_RES_TEMP_54x;
        default:
            return 1.0;
    }
}

static float getTempOffset(adi_imu_Device_t *pDevice)
{
    switch(pDevice->prodId){
        case 16465:
            return IMU_OFFSET_TEMP_46x;
        case 16467:
            return IMU_OFFSET_TEMP_46x;
        case 16475:
            return IMU_OFFSET_TEMP_47x;
        case 16477:
            return IMU_OFFSET_TEMP_47x;
        case 16495:
            return IMU_OFFSET_TEMP_49x;
        case 16497:
            return IMU_OFFSET_TEMP_49x;
        case 16505:
            return IMU_OFFSET_TEMP_50x;
        case 16507:
            return IMU_OFFSET_TEMP_50x;
        case 16545:
            return IMU_OFFSET_TEMP_54x;
        case 16547:
            return IMU_OFFSET_TEMP_54x;
        default:
            return 0.0;
    }
}

int adi_imu_SetOutputDataRate (adi_imu_Device_t *pDevice, uint16_t outputRate)
{
    int ret = Err_imu_Success_e;

    uint16_t maxOutputRate = 4250;
    if (pDevice->prodId == 16545 || pDevice->prodId == 16547)
        maxOutputRate = 4000;

    uint16_t decRate = (uint16_t)(maxOutputRate / outputRate) - 1;
    /* Set decimation rate */
    uint32_t spidelay = pDevice->spiDev.delay;
    pDevice->spiDev.delay = IMU_STALL_US_DEC_RATE;
    if ((ret = hw_WriteReg(pDevice, REG_DEC_RATE, decRate)) < 0) return ret; 
    pDevice->spiDev.delay = spidelay;
    return ret;
}

int adi_imu_GetDevInfo (adi_imu_Device_t *pDevice, adi_imu_DevInfo_t *pInfo)
{
    int ret = Err_imu_Success_e;

    uint32_t spidelay = pDevice->spiDev.delay;
    /* read function control IO: Control, I/O pins, functional definitions */
    pDevice->spiDev.delay = IMU_STALL_US_FNCTIO;
    if ((ret = hw_ReadReg(pDevice, REG_FNCTIO_CTRL, &(pInfo->fnctioCtrl))) < 0) return ret;

    /* read gpio ctrl io: Control, I/O pins, general-purpose */
    pDevice->spiDev.delay = IMU_STALL_US_GPIO_CTRL;
    if ((ret = hw_ReadReg(pDevice, REG_GPIO_CTRL, &(pInfo->gpioCtrl))) < 0) return ret;

    /* read clk cfg: Control, clock, and miscellaneous correction */
    pDevice->spiDev.delay = IMU_STALL_US_CONFIG;
    if ((ret = hw_ReadReg(pDevice, REG_CONFIG, &(pInfo->clkConfig))) < 0) return ret;

    /* read current decimation rate: Control, output sample rate decimation */
    pDevice->spiDev.delay = IMU_STALL_US_DEC_RATE;
    if ((ret = hw_ReadReg(pDevice, REG_DEC_RATE, &(pInfo->decimationRate))) < 0) return ret;
    
    /* read null cfg: Control, automatic bias correction configuration */
    pDevice->spiDev.delay = IMU_STALL_US_NULLCFG;
    if ((ret = hw_ReadReg(pDevice, REG_NULL_CNFG, &(pInfo->nullConfig))) < 0) return ret;

    /* read sync scale: Control, input clock scaling (PPS mode) */
    pDevice->spiDev.delay = IMU_STALL_US_SYNC_SCALE;
    if ((ret = hw_ReadReg(pDevice, REG_SYNC_SCALE, &(pInfo->syncScale))) < 0) return ret;

    /* read measurement range model identifier */
    pDevice->spiDev.delay = 100;
    if ((ret = hw_ReadReg(pDevice, REG_RANG_MDL, &(pInfo->gyroModelId))) < 0) return ret;

    /* read Filter bank 0 selection: Filter selection  */
    pDevice->spiDev.delay = IMU_STALL_US_FILTBNK0;
    if ((ret = hw_ReadReg(pDevice, REG_FILTR_BNK_0, &(pInfo->ftrBank0))) < 0) return ret;

    /* read Filter bank 1 selection: Filter selection  */
    pDevice->spiDev.delay = IMU_STALL_US_FILTBNK1;
    if ((ret = hw_ReadReg(pDevice, REG_FILTR_BNK_1, &(pInfo->ftrBank1))) < 0) return ret;

    /* read firmware revision */
    pDevice->spiDev.delay = 100;
    if ((ret = hw_ReadReg(pDevice, REG_FIRM_REV, &(pInfo->fwRev))) < 0) return ret;
    
    /* read firmware day/month */
    if ((ret = hw_ReadReg(pDevice, REG_FIRM_DM, &(pInfo->fwDayMonth))) < 0) return ret;

    /* read firmware year */
    if ((ret = hw_ReadReg(pDevice, REG_FIRM_Y, &(pInfo->fwYear))) < 0) return ret;

    /* read boot loader version */
    if ((ret = hw_ReadReg(pDevice, REG_BOOT_REV, &(pInfo->bootLoadVer))) < 0) return ret;

    /* read current page id */
    if ((ret = hw_ReadReg(pDevice, REG_PAGE_ID, &(pInfo->pageId))) < 0) return ret;

    /* read product id */
    if ((ret = hw_ReadReg(pDevice, REG_PROD_ID, &(pInfo->prodId))) < 0) return ret;

    /* read serial number */
    if ((ret = hw_ReadReg(pDevice, REG_SERIAL_NUM, &(pInfo->serialNumber))) < 0) return ret;
    
    pDevice->spiDev.delay = spidelay;
    return ret;
}

int adi_imu_PrintDevInfo(adi_imu_Device_t *pDevice, adi_imu_DevInfo_t *pInfo)
{
    DEBUG_PRINT("\n================================\n");
    DEBUG_PRINT("IMU Product ID: ADIS%d\n", pInfo->prodId);
    DEBUG_PRINT("IMU FW rev: %02x.%02x\n", IMU_FIRM_REV_UPPER(pInfo->fwRev), IMU_FIRM_REV_LOWER(pInfo->fwRev));
    DEBUG_PRINT("IMU FW date (MM-DD-YYYY): %02x-%02x-%x\n", IMU_FIRM_MONTH(pInfo->fwDayMonth), IMU_FIRM_DAY(pInfo->fwDayMonth), pInfo->fwYear);
    DEBUG_PRINT("IMU Bootloader ver: %d.%d\n", IMU_BOOT_REV_MAJOR(pInfo->bootLoadVer), IMU_BOOT_REV_MINOR(pInfo->bootLoadVer));
    DEBUG_PRINT("IMU Serial no: 0x%x\n", pInfo->serialNumber);
    DEBUG_PRINT("IMU Gyro Model: 0x%x [%s]\n", pInfo->gyroModelId, IMU_RANG_MDL(pInfo->gyroModelId));
    DEBUG_PRINT("IMU Page Id: 0x%x \n", pInfo->pageId);
    DEBUG_PRINT("IMU Decimation rate: %d \n", pInfo->decimationRate);
    DEBUG_PRINT("IMU Sync scale: 0x%x \n", pInfo->syncScale);
    DEBUG_PRINT("IMU Null config: 0x%x \n", pInfo->nullConfig);
    DEBUG_PRINT("IMU Config: 0x%x \n", pInfo->clkConfig);
    DEBUG_PRINT("IMU FNCTIO control: 0x%x \n", pInfo->fnctioCtrl);
    DEBUG_PRINT("IMU GPIO control: 0x%x \n", pInfo->gpioCtrl);
    DEBUG_PRINT("IMU Filter bank 0: 0x%x \n", pInfo->ftrBank0);
    DEBUG_PRINT("IMU Filter bank 1: 0x%x \n", pInfo->ftrBank1);
    DEBUG_PRINT("=================================\n\n");
    return Err_imu_Success_e;
}

int adi_imu_FindBurstPayloadIdx(const uint8_t *pBuf, unsigned bufLength, unsigned* pPayloadOffset)
{
    unsigned offset = 0; // offset cannot be zero
    const uint16_t* pShortBuf = (const uint16_t*)pBuf;
    const int max_index = 4;

    while(offset++ < max_index)
    {
        if (pShortBuf[offset] == 0xA5A5 && pShortBuf[offset+1] != 0xA5A5)
        {
            *pPayloadOffset = (offset+1)*2; // converting back to byte index
            return Err_imu_Success_e;
        }
    }
    // DEBUG_PRINT("Error: burst frame invalid. Could not find BURST_ID pattern (0xA5A5-0x0000).\n");
    return Err_imu_BurstFrameInvalid_e;
}

int adi_imu_ReadBurstRaw(adi_imu_Device_t *pDevice, uint8_t *pBuf, uint32_t numBursts)
{
    uint8_t pageId = (REG_BURST_CMD >> 8) & 0xFF;

    int ret = Err_imu_Success_e;
    /* ensure we are in right page */
    if ((ret = hw_SetPage(pDevice, pageId, 0)) < 0) return ret;

    /* send burst request and read response */
    /* as per ADIS16495 datasheet pg 7, its sufficient to send single 16-bit read access to read whole burst unlike regular read */
    for (int i=0; i<numBursts; i++)
        if ((ret = hw_ReadWriteRaw(pDevice, BURST_REQ, MAX_BRF_LEN_BYTES, pBuf+MAX_BRF_LEN_BYTES*i, MAX_BRF_LEN_BYTES)) < 0) return ret;

    return ret;
}

int adi_imu_ReadBurst(adi_imu_Device_t *pDevice, uint8_t *pBuf, uint32_t numBursts, adi_imu_BurstOutput_t *pData)
{
    int ret = Err_imu_Success_e;
    unsigned pPayloadOffset = 0;
    if ((ret = adi_imu_ReadBurstRaw(pDevice, pBuf, numBursts)) < 0) return ret;
    // Scale and copy data to output
    for (int i=0; i<numBursts; ++i)
        adi_imu_ScaleBurstOut_1(pDevice, pBuf + MAX_BRF_LEN_BYTES * i, IMU_TRUE, IMU_TRUE, &pData[i]);
    return ret;
}

int adi_imu_CheckDiagStatus(adi_imu_Device_t *pDevice, adi_imu_DiagStatus_t *pStatus)
{
    int ret = Err_imu_Success_e;
    uint16_t status;

    /* read diagnostic status */
    if ((ret = hw_ReadReg(pDevice, REG_DIAG_STS, &status)) < 0) return ret;
    pStatus->data = status;

    if (status & BITM_DIAG_STS_X_GYRO){
        ret = Err_imu_SelfTestFailed_e;
        DEBUG_PRINT("\nError: Self test FAILED for x-axis gyroscope.\n");
    }

    if (status & BITM_DIAG_STS_Y_GYRO){
        ret = Err_imu_SelfTestFailed_e;
        DEBUG_PRINT("\nError: Self test FAILED for y-axis gyroscope.\n");
    }

    if (status & BITM_DIAG_STS_Z_GYRO){
        ret = Err_imu_SelfTestFailed_e;
        DEBUG_PRINT("\nError: Self test FAILED for z-axis gyroscope.\n");
    }

    if (status & BITM_DIAG_STS_X_ACCL){
        ret = Err_imu_SelfTestFailed_e;
        DEBUG_PRINT("\nError: Self test FAILED for x-axis accelerometer.\n");
    }

    if (status & BITM_DIAG_STS_Y_ACCL){
        ret = Err_imu_SelfTestFailed_e;
        DEBUG_PRINT("\nError: Self test FAILED for y-axis accelerometer.\n");
    }

    if (status & BITM_DIAG_STS_Z_ACCL){
        ret = Err_imu_SelfTestFailed_e;
        DEBUG_PRINT("\nError: Self test FAILED for z-axis accelerometer.\n");
    }
    return ret;
}

int adi_imu_CheckSysStatus(adi_imu_Device_t *pDevice, adi_imu_SysStatus_t *pStatus)
{
    int ret = Err_imu_Success_e;
    uint16_t status;

    /* read diagnostic status */
    if ((ret = hw_ReadReg(pDevice, REG_SYS_E_FLAG, &status)) < 0) return ret;
    pStatus->data = status;

    if (status & BITM_SYS_E_FLAG_BOOT_MEM){
        ret = Err_imu_SystemError_e;
        DEBUG_PRINT("\nError: Boot memory failed."
                    "The device booted up using code from the backup memory bank."
                    "Replace the ADIS16495 if this error occurs. \n");
    }

    if (status & BITM_SYS_E_FLAG_SRAM_CRC){
        ret = Err_imu_SystemError_e;
        DEBUG_PRINT("\nError: SRAM error condition."
                    "CRC failure between the SRAM and flash memory. Initiate a reset to recover."
                    "Replace the ADIS16495 if this error occurs. \n");
    }

    if (status & BITM_SYS_E_FLAG_SPI_COMM){
        ret = Err_imu_SystemError_e;
        DEBUG_PRINT("\nError: SPI communication error."
                    "The total number of SCLK cycles is not equal to an integer multiple of 16."
                    "Repeat the previous communication sequence to recover."
                    "Persistence in this error can indicate a weakness in the SPI service from the master processor. \n");
    }

    if (status & BITM_SYS_E_FLAG_SENSOR_TEST){
        ret = Err_imu_SystemError_e;
        DEBUG_PRINT("\nError: Sensor failure."
                    "Failure in at least one of the inertial sensors."
                    "Replace the ADIS16495 if the error persists, when it is operating in static inertial conditions. \n");
        /* check diag status for more details on failed sensor */
        adi_imu_DiagStatus_t diag;
        adi_imu_CheckDiagStatus(pDevice, &diag);
    }

    if (status & BITM_SYS_E_FLAG_FLSH_MEM_UPD){
        ret = Err_imu_SystemError_e;
        DEBUG_PRINT("\nError: Flash memory update failure."
                    "The most recent flash memory update failed (GLOB_CMD, Bit 3, see Table 142)."
                    "Repeat the test and replace the ADIS16495 if this error persists. \n");
    }

    if (status & BITM_SYS_E_FLAG_PROC_OVERRUN){
        ret = Err_imu_SystemError_e;
        DEBUG_PRINT("\nError: Processing overrun."
                    "Initiate a reset to recover."
                    "Replace the ADIS16495 if this error persists. \n");
    }

    if (status & BITM_SYS_E_FLAG_SYNC_ERR){
        ret = Err_imu_SystemError_e;
        DEBUG_PRINT("\nError: Sync error."
                    "The sample timing is not scaling correctly, when operating in PPS mode (FNCTIO_CTRL Bit 8)."
                    "Verify that the input sync frequency is correct and that SYNC_SCALE (see Table 154) has the correct value \n");
    }
    return ret;
}

int hw_ReadRegAccl(adi_imu_Device_t *pDevice, adi_imu_AcclOutputRaw32_t *pData)
{
    int ret = Err_imu_Success_e;
    uint16_t low, high;
    double res = getAccl32bitRes(pDevice);

    /* read x-axis LOW output */
    if ((ret = hw_ReadReg(pDevice, REG_X_ACCL_LOW, &low)) < 0) return ret;
    /* read x-axis HIGH output */
    if ((ret = hw_ReadReg(pDevice, REG_X_ACCL_OUT, &high)) < 0) return ret;
    pData->x = (int32_t) (((high << 16) & 0xFFFF0000) | low) * res;

    /* read y-axis LOW output */
    if ((ret = hw_ReadReg(pDevice, REG_Y_ACCL_LOW, &low)) < 0) return ret;
    /* read y-axis HIGH output */
    if ((ret = hw_ReadReg(pDevice, REG_Y_ACCL_OUT, &high)) < 0) return ret;
    pData->y = (int32_t) (((high << 16) & 0xFFFF0000) | low) * res;

    /* read z-axis LOW output */
    if ((ret = hw_ReadReg(pDevice, REG_Z_ACCL_LOW, &low)) < 0) return ret;
    /* read z-axis HIGH output */
    if ((ret = hw_ReadReg(pDevice, REG_Z_ACCL_OUT, &high)) < 0) return ret;
    pData->z = (int32_t) (((high << 16) & 0xFFFF0000) | low) * res;
    return ret;
}

int hw_ReadRegGyro(adi_imu_Device_t *pDevice, adi_imu_GyroOutputRaw32_t *pData)
{
    int ret = Err_imu_Success_e;
    uint16_t low, high;

    double res = getGyro32bitRes(pDevice);

    /* read x-axis LOW output */
    if ((ret = hw_ReadReg(pDevice, REG_X_GYRO_LOW, &low)) < 0) return ret;
    /* read x-axis HIGH output */
    if ((ret = hw_ReadReg(pDevice, REG_X_GYRO_OUT, &high)) < 0) return ret;
    pData->x = (((high << 16) & 0xFFFF0000) | low) * res;

    /* read y-axis LOW output */
    if ((ret = hw_ReadReg(pDevice, REG_Y_GYRO_LOW, &low)) < 0) return ret;
    /* read y-axis HIGH output */
    if ((ret = hw_ReadReg(pDevice, REG_Y_GYRO_OUT, &high)) < 0) return ret;
    pData->y = (((high << 16) & 0xFFFF0000) | low) * res;

    /* read z-axis LOW output */
    if ((ret = hw_ReadReg(pDevice, REG_Z_GYRO_LOW, &low)) < 0) return ret;
    /* read z-axis HIGH output */
    if ((ret = hw_ReadReg(pDevice, REG_Z_GYRO_OUT, &high)) < 0) return ret;
    pData->z = (((high << 16) & 0xFFFF0000) | low) * res;
    return ret;
}

int hw_ReadRegDelAng(adi_imu_Device_t *pDevice, adi_imu_DelAngOutputRaw32_t *pData)
{
    int ret = Err_imu_Success_e;
    uint16_t low, high;
    /* read x-axis LOW output */
    if ((ret = hw_ReadReg(pDevice, REG_X_DELTANG_LOW, &low)) < 0) return ret;
    /* read x-axis HIGH output */
    if ((ret = hw_ReadReg(pDevice, REG_X_DELTANG_OUT, &high)) < 0) return ret;
    pData->x = ((high << 16) & 0xFFFF0000) | low;
    
    /* read y-axis LOW output */
    if ((ret = hw_ReadReg(pDevice, REG_Y_DELTANG_LOW, &low)) < 0) return ret;
    /* read y-axis HIGH output */
    if ((ret = hw_ReadReg(pDevice, REG_Y_DELTANG_OUT, &high)) < 0) return ret;
    pData->y = ((high << 16) & 0xFFFF0000) | low;

    /* read z-axis LOW output */
    if ((ret = hw_ReadReg(pDevice, REG_Z_DELTANG_LOW, &low)) < 0) return ret;
    /* read z-axis HIGH output */
    if ((ret = hw_ReadReg(pDevice, REG_Z_DELTANG_OUT, &high)) < 0) return ret;
    pData->z = ((high << 16) & 0xFFFF0000) | low;
    return ret;
}

int hw_ReadRegDelVel(adi_imu_Device_t *pDevice, adi_imu_DelVelOutputRaw32_t *pData)
{
    int ret = Err_imu_Success_e;
    uint16_t low, high;
    /* read x-axis LOW output */
    if ((ret = hw_ReadReg(pDevice, REG_X_DELTVEL_LOW, &low)) < 0) return ret;
    /* read x-axis HIGH output */
    if ((ret = hw_ReadReg(pDevice, REG_X_DELTVEL_OUT, &high)) < 0) return ret;
    pData->x = ((high << 16) & 0xFFFF0000) | low;

    /* read y-axis LOW output */
    if ((ret = hw_ReadReg(pDevice, REG_Y_DELTVEL_LOW, &low)) < 0) return ret;
    /* read y-axis HIGH output */
    if ((ret = hw_ReadReg(pDevice, REG_Y_DELTVEL_OUT, &high)) < 0) return ret;
    pData->y = ((high << 16) & 0xFFFF0000) | low;

    /* read z-axis LOW output */
    if ((ret = hw_ReadReg(pDevice, REG_Z_DELTVEL_LOW, &low)) < 0) return ret;
    /* read z-axis HIGH output */
    if ((ret = hw_ReadReg(pDevice, REG_Z_DELTVEL_OUT, &high)) < 0) return ret;
    pData->z = ((high << 16) & 0xFFFF0000) | low;
    return ret;
}

int adi_imu_GetAcclScale(adi_imu_Device_t *pDevice, adi_imu_AcclScale_t *pData)
{
    int ret = Err_imu_Success_e;
    /* read x-axis scale output */
    if ((ret = hw_ReadReg(pDevice, REG_X_ACCL_SCALE, (uint16_t*)&(pData->x))) < 0) return ret;

    /* read y-axis scale output */
    if ((ret = hw_ReadReg(pDevice, REG_Y_ACCL_SCALE, (uint16_t*)&(pData->y))) < 0) return ret;

    /* read z-axis scale output */
    if ((ret = hw_ReadReg(pDevice, REG_Z_ACCL_SCALE, (uint16_t*)&(pData->z))) < 0) return ret;
    return ret;
}

int adi_imu_GetGyroScale(adi_imu_Device_t *pDevice, adi_imu_GyroScale_t *pData)
{
    int ret = Err_imu_Success_e;
    /* read x-axis scale output */
    if ((ret = hw_ReadReg(pDevice, REG_X_GYRO_SCALE, (uint16_t*)&(pData->x))) < 0) return ret;

    /* read y-axis scale output */
    if ((ret = hw_ReadReg(pDevice, REG_Y_GYRO_SCALE, (uint16_t*)&(pData->y))) < 0) return ret;

    /* read z-axis scale output */
    if ((ret = hw_ReadReg(pDevice, REG_Z_GYRO_SCALE, (uint16_t*)&(pData->z))) < 0) return ret;
    return ret;
}

int adi_imu_GetAcclBias(adi_imu_Device_t *pDevice, adi_imu_AcclBiasRaw32_t *pData)
{
    int ret = Err_imu_Success_e;
    uint16_t low, high;
    /* read x-axis LOW output */
    if ((ret = hw_ReadReg(pDevice, REG_XA_BIAS_LOW, &low)) < 0) return ret;
    /* read x-axis HIGH output */
    if ((ret = hw_ReadReg(pDevice, REG_XA_BIAS_HIGH, &high)) < 0) return ret;
    pData->x = ((high << 16) & 0xFFFF0000) | low;

    /* read y-axis LOW output */
    if ((ret = hw_ReadReg(pDevice, REG_YA_BIAS_LOW, &low)) < 0) return ret;
    /* read y-axis HIGH output */
    if ((ret = hw_ReadReg(pDevice, REG_YA_BIAS_HIGH, &high)) < 0) return ret;
    pData->y = ((high << 16) & 0xFFFF0000) | low;

    /* read z-axis LOW output */
    if ((ret = hw_ReadReg(pDevice, REG_ZA_BIAS_LOW, &low)) < 0) return ret;
    /* read z-axis HIGH output */
    if ((ret = hw_ReadReg(pDevice, REG_ZA_BIAS_HIGH, &high)) < 0) return ret;
    pData->z = ((high << 16) & 0xFFFF0000) | low;
    return ret;
}

int adi_imu_GetGyroBias(adi_imu_Device_t *pDevice, adi_imu_GyroBiasRaw32_t *pData)
{
    int ret = Err_imu_Success_e;
    uint16_t low, high;
    /* read x-axis LOW output */
    if ((ret = hw_ReadReg(pDevice, REG_XG_BIAS_LOW, &low)) < 0) return ret;
    /* read x-axis HIGH output */
    if ((ret = hw_ReadReg(pDevice, REG_XG_BIAS_HIGH, &high)) < 0) return ret;
    pData->x = ((high << 16) & 0xFFFF0000) | low;

    /* read y-axis LOW output */
    if ((ret = hw_ReadReg(pDevice, REG_YG_BIAS_LOW, &low)) < 0) return ret;
    /* read y-axis HIGH output */
    if ((ret = hw_ReadReg(pDevice, REG_YG_BIAS_HIGH, &high)) < 0) return ret;
    pData->y = ((high << 16) & 0xFFFF0000) | low;

    /* read z-axis LOW output */
    if ((ret = hw_ReadReg(pDevice, REG_ZG_BIAS_LOW, &low)) < 0) return ret;
    /* read z-axis HIGH output */
    if ((ret = hw_ReadReg(pDevice, REG_ZG_BIAS_HIGH, &high)) < 0) return ret;
    pData->z = ((high << 16) & 0xFFFF0000) | low;
    return ret;
}

int adi_imu_ConfigGpio(adi_imu_Device_t *pDevice, adi_imu_GPIO_e id, adi_imu_Direction_e direction)
{
    int ret = Err_imu_Success_e;
    DEBUG_PRINT("Configuring GPIO %d as %s...", id, (direction == IMU_INPUT) ? "INPUT" : "OUTPUT");
    uint32_t spidelay = pDevice->spiDev.delay;
    pDevice->spiDev.delay = IMU_STALL_US_GPIO_CTRL;
    if ((ret = hw_WriteReg(pDevice, REG_GPIO_CTRL, ((direction) << id))) < 0) return ret; 
    pDevice->spiDev.delay = spidelay;
    DEBUG_PRINT("done.\n");
    return ret;
}

int adi_imu_SetGpio(adi_imu_Device_t *pDevice, adi_imu_GPIO_e id)
{
    int ret = Err_imu_Success_e;
    uint16_t data = 0x00;
    uint32_t spidelay = pDevice->spiDev.delay;
    pDevice->spiDev.delay = IMU_STALL_US_GPIO_CTRL;
    if ((ret = hw_ReadReg(pDevice, REG_GPIO_CTRL, &data)) < 0) return ret; 
    if ((ret = hw_WriteReg(pDevice, REG_GPIO_CTRL, data | (BITM_GPIO_CTRL_DIO1_DATA << id))) < 0) return ret; 
    pDevice->spiDev.delay = spidelay;
    return ret;
}

int adi_imu_ClearGpio(adi_imu_Device_t *pDevice, adi_imu_GPIO_e id)
{
    int ret = Err_imu_Success_e;
    uint16_t data = 0x00;
    uint32_t spidelay = pDevice->spiDev.delay;
    pDevice->spiDev.delay = IMU_STALL_US_GPIO_CTRL;
    if ((ret = hw_ReadReg(pDevice, REG_GPIO_CTRL, &data)) < 0) return ret; 
    if ((ret = hw_WriteReg(pDevice, REG_GPIO_CTRL, data & ~(BITM_GPIO_CTRL_DIO1_DATA << id))) < 0) return ret; 
    pDevice->spiDev.delay = spidelay;
    return ret;
}

int adi_imu_GetGpio(adi_imu_Device_t *pDevice, adi_imu_GPIO_e id, uint8_t* val)
{
    int ret = Err_imu_Success_e;
    uint16_t data = 0x00;
    uint32_t spidelay = pDevice->spiDev.delay;
    pDevice->spiDev.delay = IMU_STALL_US_GPIO_CTRL;
    if ((ret = hw_ReadReg(pDevice, REG_GPIO_CTRL, &data)) < 0) return ret; 
    *val = (uint8_t) (data & (BITM_GPIO_CTRL_DIO1_DATA << id));
    pDevice->spiDev.delay = spidelay;
    return ret;
}

int adi_imu_ConfigDataReady(adi_imu_Device_t *pDevice, adi_imu_GPIO_e id, adi_imu_Polarity_e polarity)
{
    int ret = Err_imu_Success_e;
    DEBUG_PRINT("Configuring data ready...");
    uint16_t fnctio = 0x00;
    uint32_t spidelay = pDevice->spiDev.delay;
    pDevice->spiDev.delay = IMU_STALL_US_FNCTIO;
    if ((ret = hw_ReadReg(pDevice, REG_FNCTIO_CTRL, &fnctio)) < 0) return ret;

    fnctio &= ~(BITM_FNCTIO_CTRL_DATA_RDY_POL | BITM_FNCTIO_CTRL_DATA_RDY_DIO);
    fnctio |= ((id) << BITP_FNCTIO_CTRL_DATA_RDY_DIO) | ((polarity) << BITP_FNCTIO_CTRL_DATA_RDY_POL);

    if ((ret = hw_WriteReg(pDevice, REG_FNCTIO_CTRL, fnctio)) < 0) return ret; 
    pDevice->spiDev.delay = spidelay;
    DEBUG_PRINT("done.\n");
    return ret;
}

int adi_imu_ConfigSyncClkMode(adi_imu_Device_t *pDevice, adi_imu_ClockMode_e mode, adi_imu_EnDis_e clkEn, \
                              adi_imu_EdgeType_e polarity, adi_imu_GPIO_e inputGpio)
{
    int ret = Err_imu_Success_e;
    DEBUG_PRINT("Configuring sync clock mode...");
    uint16_t fnctio = 0x00;
    uint32_t spidelay = pDevice->spiDev.delay;
    pDevice->spiDev.delay = IMU_STALL_US_FNCTIO;
    if ((ret = hw_ReadReg(pDevice, REG_FNCTIO_CTRL, &fnctio)) < 0) return ret; 
    
    fnctio &= ~(BITM_FNCTIO_CTRL_SYNC_CLK_MODE | BITM_FNCTIO_CTRL_SYNC_CLK_EN | BITM_FNCTIO_CTRL_SYNC_CLK_POL | BITM_FNCTIO_CTRL_SYNC_CLK_DIO);
    fnctio |= ((mode) << BITP_FNCTIO_CTRL_SYNC_CLK_MODE) | ((clkEn) << BITP_FNCTIO_CTRL_SYNC_CLK_EN) | \
                      ((polarity) << BITP_FNCTIO_CTRL_SYNC_CLK_POL) | ((inputGpio) << BITP_FNCTIO_CTRL_SYNC_CLK_DIO);
    
    if ((ret = hw_WriteReg(pDevice, REG_FNCTIO_CTRL, fnctio)) < 0) return ret; 
    pDevice->spiDev.delay = spidelay;
    DEBUG_PRINT("done.\n");
    return ret;
}

int adi_imu_SetDataReady(adi_imu_Device_t *pDevice, adi_imu_EnDis_e val)
{
    int ret = Err_imu_Success_e;
    DEBUG_PRINT("%s data ready...", (val == IMU_ENABLE) ? "Enabling" : "Disabling");
    uint16_t fnctio = 0x00;
    uint32_t spidelay = pDevice->spiDev.delay;
    pDevice->spiDev.delay = IMU_STALL_US_FNCTIO;
    if ((ret = hw_ReadReg(pDevice, REG_FNCTIO_CTRL, &fnctio)) < 0) return ret; 

    if ((ret = hw_WriteReg(pDevice, REG_FNCTIO_CTRL, (val == IMU_ENABLE) ? fnctio | BITM_FNCTIO_CTRL_DATA_RDY_EN : fnctio)) < 0) return ret; 
    pDevice->spiDev.delay = spidelay;
    DEBUG_PRINT("done.\n");
    return ret;
}

int adi_imu_SetLineargComp(adi_imu_Device_t *pDevice, adi_imu_EnDis_e val)
{
    int ret = Err_imu_Success_e;
    DEBUG_PRINT("%s linear g compensation...", (val == IMU_ENABLE) ? "Enabling" : "Disabling");
    uint32_t spidelay = pDevice->spiDev.delay;
    pDevice->spiDev.delay = IMU_STALL_US_CONFIG;
    if ((ret = hw_WriteReg(pDevice, REG_CONFIG, (val == IMU_ENABLE) ? BITM_CONFIG_LIN_G_COMP : 0x00)) < 0) return ret; 
    pDevice->spiDev.delay = spidelay;
    DEBUG_PRINT("done.\n");
    return ret;
}

int adi_imu_SetPPercAlignment(adi_imu_Device_t *pDevice, adi_imu_EnDis_e val)
{
    int ret = Err_imu_Success_e;
    DEBUG_PRINT("%s point of percussion alignment...", (val == IMU_ENABLE) ? "Enabling" : "Disabling");
    uint32_t spidelay = pDevice->spiDev.delay;
    pDevice->spiDev.delay = IMU_STALL_US_CONFIG;
    if ((ret = hw_WriteReg(pDevice, REG_CONFIG, (val == IMU_ENABLE) ? BITM_CONFIG_PNT_PERC_ALIGN : 0x00)) < 0) return ret; 
    pDevice->spiDev.delay = spidelay;
    DEBUG_PRINT("done.\n");
    return ret;
}

int adi_imu_SoftwareReset(adi_imu_Device_t *pDevice)
{
    int ret = Err_imu_Success_e;
    DEBUG_PRINT("Performing software reset...");
    if ((ret = hw_WriteReg(pDevice, REG_GLOB_CMD, BITM_GLOB_CMD_SOFT_RST)) < 0) return ret; 
    delay_MicroSeconds(350000); //350ms

    // Read page ID
    if ((ret = hw_GetPage(pDevice)) < 0) return ret;

    DEBUG_PRINT("Finished!\n");
    return ret;
}

int adi_imu_ClearUserCalibration(adi_imu_Device_t *pDevice)
{
    int ret = Err_imu_Success_e;
    DEBUG_PRINT("Clearing all User calibration data...");
    if ((ret = hw_WriteReg(pDevice, REG_GLOB_CMD, BITM_GLOB_CMD_CLR_USR_CALIB)) < 0) return ret; 
    DEBUG_PRINT("done.\n");
    return ret;
}

int adi_imu_UpdateFlashMemory(adi_imu_Device_t *pDevice)
{
    int ret = Err_imu_Success_e;
    DEBUG_PRINT("Writing IMU settings to flash...");
    if ((ret = hw_WriteReg(pDevice, REG_GLOB_CMD, BITM_GLOB_CMD_FLASH_MEM_UPD)) < 0) return ret;
    delay_MicroSeconds(600000); //600ms

    // Read page ID
    if ((ret = hw_GetPage(pDevice)) < 0) return ret;

    DEBUG_PRINT("Finished!\n");
    return ret;
}

int adi_imu_PerformSelfTest(adi_imu_Device_t *pDevice)
{
    int ret = Err_imu_Success_e;
    DEBUG_PRINT("Performing Self test...");
    if ((ret = hw_WriteReg(pDevice, REG_GLOB_CMD, BITM_GLOB_CMD_SELF_TEST)) < 0) return ret;
    delay_MicroSeconds(50000); //50ms
    DEBUG_PRINT("Finished!\n");

    DEBUG_PRINT("Checking test results..");
    adi_imu_DiagStatus_t diagStatus;
    if ((ret = adi_imu_CheckDiagStatus(pDevice, &diagStatus)) < 0) return ret;
    DEBUG_PRINT("%s\n", (diagStatus.data) ? "FAILED": "SUCCESS");
    return ret;
}

/* Configure the bias correction accumulation time */
int adi_imu_ConfigBiasCorrectionTime(adi_imu_Device_t *pDevice, uint8_t time)
{
    int ret = Err_imu_Success_e;
    DEBUG_PRINT("Updating bias correction time...");
    if (time > 13) 
    {
        DEBUG_PRINT("Bias error correction setting invalid! Forced to 13");
        time = 13;
    }
    uint16_t dat = (time & 0xFF);
    uint32_t spidelay = pDevice->spiDev.delay;
    pDevice->spiDev.delay = IMU_STALL_US_NULLCFG;
    if ((ret = hw_WriteReg(pDevice, REG_NULL_CNFG, dat)) < 0) return ret;
    pDevice->spiDev.delay = spidelay;
    DEBUG_PRINT("Finished!\n");
    return ret;
}

/* Trigger a bias correction update based on the NULL_CNFG register settings */
int adi_imu_UpdateBiasCorrection(adi_imu_Device_t *pDevice)
{
    int ret = Err_imu_Success_e;
    DEBUG_PRINT("Triggering bias correction update...");
    if ((ret = hw_WriteReg(pDevice, REG_GLOB_CMD, BITM_GLOB_CMD_BIAS_CORR_UPD)) < 0) return ret;
    DEBUG_PRINT("Finished!\n");
    return ret;
}

/* Select the sensors to be nulled */
int adi_imu_SelectBiasConfigAxes(adi_imu_Device_t *pDevice, adi_imu_NullConfig_e XG, adi_imu_NullConfig_e YG, \
                                 adi_imu_NullConfig_e ZG, adi_imu_NullConfig_e XA, adi_imu_NullConfig_e YA, \
                                 adi_imu_NullConfig_e ZA)
{
    int ret = Err_imu_Success_e;
    DEBUG_PRINT("Configuring bias config axes...");
    uint16_t nullcnfg = 0x00;
    uint32_t spidelay = pDevice->spiDev.delay;
    pDevice->spiDev.delay = IMU_STALL_US_NULLCFG;
    if ((ret = hw_ReadReg(pDevice, REG_NULL_CNFG, &nullcnfg)) < 0) return ret; 
    nullcnfg &= ~(BITM_NULL_CNFG_EN_XG | BITM_NULL_CNFG_EN_YG | BITM_NULL_CNFG_EN_ZG | BITM_NULL_CNFG_EN_XA | \
                BITM_NULL_CNFG_EN_YA | BITM_NULL_CNFG_EN_ZA);
    nullcnfg |= ((XG) << BITP_NULL_CNFG_EN_XG) | ((YG) << BITP_NULL_CNFG_EN_YG) | ((ZG) << BITP_NULL_CNFG_EN_ZG) | \
                ((XA) << BITP_NULL_CNFG_EN_XA) | ((YA) << BITP_NULL_CNFG_EN_YA) | ((ZA) << BITP_NULL_CNFG_EN_ZA);
    if ((ret = hw_WriteReg(pDevice, REG_NULL_CNFG, nullcnfg)) < 0) return ret;
    pDevice->spiDev.delay = spidelay;
    DEBUG_PRINT("Finished!\n");
    return ret;
}

int adi_imu_ParseBurstOut(adi_imu_Device_t *pDevice, const uint8_t *pBuf, adi_imu_Boolean_e checkBurstID, adi_imu_Boolean_e enByteSwap, adi_imu_BurstOutputRaw_t *pRawData)
{
    unsigned payloadOffset = 0;
    if (checkBurstID) {
        int ret = adi_imu_FindBurstPayloadIdx(pBuf, MAX_BRF_LEN_BYTES, &payloadOffset);
        if (ret < 0) return ret;
    }

    if(pDevice->devType == IMU_HW_SPI && pDevice->spiDev.status == IMUBUF_SPI_READY)
    {
        pRawData->sysEFlag= IMU_BSWAP_16(*(uint16_t*)(pBuf + payloadOffset));
        pRawData->tempOut = (int16_t) (IMU_BSWAP_16(*(uint16_t*)(pBuf + payloadOffset + 2)));

        pRawData->gyro.x = (int32_t) (IMU_BSWAP_32(*(uint32_t*)(pBuf + payloadOffset + 4)));
        pRawData->gyro.y = (int32_t) (IMU_BSWAP_32(*(uint32_t*)(pBuf + payloadOffset + 8)));
        pRawData->gyro.z = (int32_t) (IMU_BSWAP_32(*(uint32_t*)(pBuf + payloadOffset + 12)));

        pRawData->accl.x = (int32_t) (IMU_BSWAP_32(*(uint32_t*)(pBuf + payloadOffset + 16)));
        pRawData->accl.y = (int32_t) (IMU_BSWAP_32(*(uint32_t*)(pBuf + payloadOffset + 20)));
        pRawData->accl.z = (int32_t) (IMU_BSWAP_32(*(uint32_t*)(pBuf + payloadOffset + 24)));
        
        pRawData->dataCntOrTimeStamp = (unsigned) IMU_BSWAP_16(*(uint16_t*)(pBuf + payloadOffset + 28));
        pRawData->crc = (uint32_t) (IMU_BSWAP_32(*(uint32_t*)(pBuf + payloadOffset + 30)));
    }
    else
    {
        pRawData->sysEFlag= *(uint16_t*)(pBuf + payloadOffset);
        pRawData->tempOut = (int16_t) (*(uint16_t*)(pBuf + payloadOffset + 2));

        pRawData->gyro.x = (int32_t) (*(uint32_t*)(pBuf + payloadOffset + 4));
        pRawData->gyro.y = (int32_t) (*(uint32_t*)(pBuf + payloadOffset + 8));
        pRawData->gyro.z = (int32_t) (*(uint32_t*)(pBuf + payloadOffset + 12));

        pRawData->accl.x = (int32_t) (*(uint32_t*)(pBuf + payloadOffset + 16));
        pRawData->accl.y = (int32_t) (*(uint32_t*)(pBuf + payloadOffset + 20));
        pRawData->accl.z = (int32_t) (*(uint32_t*)(pBuf + payloadOffset + 24));
        
        pRawData->dataCntOrTimeStamp = (unsigned) *(uint16_t*)(pBuf + payloadOffset + 28);
        pRawData->crc = (uint32_t) (*(uint32_t*)(pBuf + payloadOffset + 30));
    }
    return Err_imu_Success_e;
}

int adi_imu_ScaleBurstOut_1(adi_imu_Device_t *pDevice, const uint8_t *pBuf, adi_imu_Boolean_e checkBurstID, adi_imu_Boolean_e enByteSwap, adi_imu_BurstOutput_t *pData)
{
    unsigned payloadOffset = 0;
    if (checkBurstID) {
        int ret = adi_imu_FindBurstPayloadIdx(pBuf, MAX_BRF_LEN_BYTES, &payloadOffset);
        if (ret < 0) return ret;
    }
    double gyroscale = getGyro32bitRes(pDevice);
    double acclscale = getAccl32bitRes(pDevice) * pDevice->g;

    uint16_t* pBuf16 = (uint16_t*) pBuf;
    uint32_t* pBuf32 = (uint32_t*) pBuf;
    
    if(enByteSwap == IMU_TRUE)
    {
        pData->sysEFlag= (unsigned) IMU_BSWAP_16(*(uint16_t*)(pBuf + payloadOffset));
        pData->tempOut = getTempOffset(pDevice) + (int32_t) (IMU_BSWAP_16(*(uint16_t*)(pBuf + payloadOffset + 2))) * getTempRes(pDevice);

        pData->gyro.x = (int32_t) (IMU_BSWAP_32(*(uint32_t*)(pBuf + payloadOffset + 4))) * gyroscale;
        pData->gyro.y = (int32_t) (IMU_BSWAP_32(*(uint32_t*)(pBuf + payloadOffset + 8))) * gyroscale;
        pData->gyro.z = (int32_t) (IMU_BSWAP_32(*(uint32_t*)(pBuf + payloadOffset + 12))) * gyroscale;

        pData->accl.x = (int32_t) (IMU_BSWAP_32(*(uint32_t*)(pBuf + payloadOffset + 16))) * acclscale;
        pData->accl.y = (int32_t) (IMU_BSWAP_32(*(uint32_t*)(pBuf + payloadOffset + 20))) * acclscale;
        pData->accl.z = (int32_t) (IMU_BSWAP_32(*(uint32_t*)(pBuf + payloadOffset + 24))) * acclscale;

        pData->dataCntOrTimeStamp = (unsigned) IMU_BSWAP_16(*(uint16_t*)(pBuf + payloadOffset + 28));
        pData->crc = (uint32_t) (IMU_BSWAP_32(*(uint32_t*)(pBuf + payloadOffset + 30)));
    }
    else
    {
        pData->sysEFlag= (unsigned) (*(uint16_t*)(pBuf + payloadOffset));
        pData->tempOut = getTempOffset(pDevice) + (int32_t) (*(uint16_t*)(pBuf + payloadOffset + 2)) * getTempRes(pDevice);

        pData->gyro.x = (int32_t) (*(uint32_t*)(pBuf + payloadOffset + 4)) * gyroscale;
        pData->gyro.y = (int32_t) (*(uint32_t*)(pBuf + payloadOffset + 8)) * gyroscale;
        pData->gyro.z = (int32_t) (*(uint32_t*)(pBuf + payloadOffset + 12)) * gyroscale;

        pData->accl.x = (int32_t) (*(uint32_t*)(pBuf + payloadOffset + 16)) * acclscale;
        pData->accl.y = (int32_t) (*(uint32_t*)(pBuf + payloadOffset + 20)) * acclscale;
        pData->accl.z = (int32_t) (*(uint32_t*)(pBuf + payloadOffset + 24)) * acclscale;

        pData->dataCntOrTimeStamp = (unsigned) *(uint16_t*)(pBuf + payloadOffset + 28);
        pData->crc = (uint32_t) (*(uint32_t*)(pBuf + payloadOffset + 30));
    }

    return Err_imu_Success_e;
}

void adi_imu_ScaleBurstOut_2(adi_imu_Device_t *pDevice, const adi_imu_BurstOutputRaw_t *pRawData, adi_imu_BurstOutput_t *pData)
{
    pData->sysEFlag= (unsigned) pRawData->sysEFlag;
    adi_imu_ScaleTempOut(pDevice, pRawData->tempOut, &(pData->tempOut));
    adi_imu_ScaleGyro32Out(pDevice, &(pRawData->gyro), &(pData->gyro));
    adi_imu_ScaleAccl32Out(pDevice, &(pRawData->accl), &(pData->accl));
    pData->dataCntOrTimeStamp = (unsigned) pRawData->dataCntOrTimeStamp;
    pData->crc = pRawData->crc;
}

void adi_imu_ScaleTempOut(adi_imu_Device_t *pDevice, uint16_t rawData, float *pData)
{
    *pData = getTempOffset(pDevice) + ((int32_t) rawData) * getTempRes(pDevice);
}

void adi_imu_ScaleAccl32Out(adi_imu_Device_t *pDevice, const adi_imu_AcclOutputRaw32_t *rawData, adi_imu_AcclOutput_t *pOut)
{
    double scale = getAccl32bitRes(pDevice) * pDevice->g;
    pOut->x = (int32_t) (rawData->x) * scale;
    pOut->y = (int32_t) (rawData->y) * scale;
    pOut->z = (int32_t) (rawData->z) * scale;
}

void adi_imu_ScaleGyro32Out(adi_imu_Device_t *pDevice, const adi_imu_GyroOutputRaw32_t *rawData, adi_imu_GyroOutput_t *pOut)
{
    double scale = getGyro32bitRes(pDevice);
    pOut->x = (int32_t) (rawData->x) * scale;
    pOut->y = (int32_t) (rawData->y) * scale;
    pOut->z = (int32_t) (rawData->z) * scale;
}

void adi_imu_ScaleAccl16Out(adi_imu_Device_t *pDevice, const adi_imu_AcclOutputRaw16_t *rawData, adi_imu_AcclOutput_t *pOut)
{
    double scale = getAccl16bitRes(pDevice) * pDevice->g;
    pOut->x = (int16_t) (rawData->x) * scale;
    pOut->y = (int16_t) (rawData->y) * scale;
    pOut->z = (int16_t) (rawData->z) * scale;
}

void adi_imu_ScaleGyro16Out(adi_imu_Device_t *pDevice, const adi_imu_GyroOutputRaw16_t *rawData, adi_imu_GyroOutput_t *pOut)
{
    double scale = getGyro16bitRes(pDevice);
    pOut->x = (int16_t) (rawData->x) * scale;
    pOut->y = (int16_t) (rawData->y) * scale;
    pOut->z = (int16_t) (rawData->z) * scale;
}

adi_imu_BuildInfo_t adi_imu_GetBuildInfo (adi_imu_Device_t *pDevice)
{
    adi_imu_BuildInfo_t info;
    info.version = IMU_LIB_VERSION;
    snprintf(info.version_full, sizeof(info.version_full), IMU_LIB_VERSION_FULL);
    snprintf(info.build_time, sizeof(info.build_time), IMU_LIB_BUILD_DATETIME);
    snprintf(info.build_type, sizeof(info.build_type), IMU_LIB_BUILD_TYPE);
    return info;
}

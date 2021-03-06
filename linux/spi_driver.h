/**
  * Copyright (c) 2020 Analog Devices, Inc. All Rights Reserved.
  * This software is proprietary to Analog Devices, Inc. and its licensors.
  *
  * Use of this file is governed by the license agreement
  * included in this repository.
  *
  * @file		spi_driver.h
  * @author		Sundar Palani (sundar.palani@analog.com)
  * @brief 		Driver interface for linux SPI devices.
 **/


#ifndef __SPI_DRIVER_H_
#define __SPI_DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "adi_imu_common.h"
	
int spi_Init(adi_imu_SpiDevice_t* device);
int spi_ReadWrite(adi_imu_SpiDevice_t* device, const uint8_t *txBuf, uint8_t *rxBuf, uint32_t xferLen, uint32_t numXfers, uint32_t numRepeats, uint32_t enRepeatTx);

#ifdef __cplusplus
}
#endif
#endif

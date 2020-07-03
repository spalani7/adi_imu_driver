/*******************************************************************************
 *   @file   spi_driver.c
 *   @brief  Spidev Driver interface
 *   @author Sundar Palani <sundar.palani@analog.com>
********************************************************************************/

#include <unistd.h>        // Needed for SPI port
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>            // Needed for SPI port
#include <sys/ioctl.h>        // Needed for SPI port
#include <linux/types.h>
#include <linux/spi/spidev.h> // Needed for SPI port
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "spi_driver.h"

#ifdef WIN32
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h>   // for nanosleep
#else
#include <unistd.h> // for usleep
#endif
void adi_imu_DelayMicroSeconds (uint32_t microseconds)
{
#ifdef WIN32
    Sleep(microseconds); // For windows, not tested. Might not work.
#elif _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = microseconds / 1000000;
    ts.tv_nsec = (microseconds % 1000000) * 1000;
    nanosleep(&ts, NULL);
#else
    usleep(microseconds);
#endif
}

int adi_imu_SpiInit(adi_imu_Device_t *pDevice)
{
    int spi_fd = open(pDevice->spiDev, O_RDWR);
    if (spi_fd < 0) DEBUG_PRINT_RET(adi_imu_SpiInitFailed_e, "Error: failed to open spi device %s\n", pDevice->spiDev);
    
    pDevice->spiHandle = (adi_imu_DevHandler_t) spi_fd;

    int ret = adi_imu_Success_e;
    /*
    * spi mode
    */
    ret = ioctl(spi_fd, SPI_IOC_WR_MODE, &pDevice->spiMode);
    if (ret == -1) DEBUG_PRINT_RET(adi_imu_SpiInitFailed_e, "Error: Failed to set spi mode (wr). Error: %s\n", strerror(errno));

    ret = ioctl(spi_fd, SPI_IOC_RD_MODE, &pDevice->spiMode);
    if (ret == -1) DEBUG_PRINT_RET(adi_imu_SpiInitFailed_e, "Error: Failed to set spi mode (rd). Error: %s\n", strerror(errno));

    /*
    * bits per word
    */
    ret = ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &pDevice->spiBitsPerWord);
    if (ret == -1) DEBUG_PRINT_RET(adi_imu_SpiInitFailed_e, "Error: Failed to set bits per word (wr). Error: %s\n", strerror(errno));

    ret = ioctl(spi_fd, SPI_IOC_RD_BITS_PER_WORD, &pDevice->spiBitsPerWord);
    if (ret == -1) DEBUG_PRINT_RET(adi_imu_SpiInitFailed_e, "Error: Failed to set bits per word (rd). Error: %s\n", strerror(errno));

    /*
    * max speed hz
    */
    ret = ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &pDevice->spiSpeed);
    if (ret == -1) DEBUG_PRINT_RET(adi_imu_SpiInitFailed_e, "Error: Failed to set max speed hz (wr). Error: %s\n", strerror(errno));

    ret = ioctl(spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &pDevice->spiSpeed);
    if (ret == -1) DEBUG_PRINT_RET(adi_imu_SpiInitFailed_e, "Error: Failed to set max speed hz (rd). Error: %s\n", strerror(errno));

    DEBUG_PRINT("SPI mode: %d\n", pDevice->spiMode);
    DEBUG_PRINT("SPI bits per word: %d\n", pDevice->spiBitsPerWord);
    DEBUG_PRINT("SPI max speed: %d Hz (%d KHz)\n", pDevice->spiSpeed, pDevice->spiSpeed/1000);
    DEBUG_PRINT("SPI successfully initialized.\n");
    return adi_imu_Success_e;
}

int adi_imu_SpiReadWrite(adi_imu_Device_t *pDevice, uint8_t *txBuf, uint8_t *rxBuf, uint32_t length)
{
    int ret = adi_imu_Success_e;
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)txBuf,
        .rx_buf = (unsigned long)rxBuf,
        .len = length,
        .speed_hz = pDevice->spiSpeed,
        .delay_usecs = pDevice->spiDelay,
        .bits_per_word = pDevice->spiBitsPerWord,
        .cs_change = 1
    };

    ret = ioctl((int) pDevice->spiHandle, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 1) DEBUG_PRINT_RET(adi_imu_SpiRwFailed_e, "Error: Failed to send spi message. Error: %s\n", strerror(errno));
    return adi_imu_Success_e;
}
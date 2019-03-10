/***********************************************************************************
 * @author Joshua Malburg
 * joshua.malburg@colorado.edu
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 7, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file tempSensor.h
 * @brief temp sensor library
 *
 ************************************************************************************
 */

#ifndef TEMP_SENSOR_H_
#define TEMP_SENSOR_H_

#include <stdint.h>

typedef enum 
{
    TMP102_CONV_RATE_0P25HZ,
    TMP102_CONV_RATE_1HZ,
    TMP102_CONV_RATE_4HZ,
    TMP102_CONV_RATE_8HZ
} Tmp102_ConvRate_e;

typedef enum
{
    TMP102_ADDR_MODE_NORMAL,
    TMP102_ADDR_MODE_EXTENDED
    
} Tmp102_AddrMode_e;

typedef enum
{
    TMP102_ALERT_ACTIVE,
    TMP102_ALERT_OFF
} Tmp102_Alert_e;

typedef enum
{
    TMP102_REQ_ONE_FAULT,
    TMP102_REQ_TWO_FAULT,
    TMP102_REQ_FOUR_FAULT,
    TMP102_REQ_SIX_FAULT,
} Tmp102_FaultCount_e;

typedef enum
{
    TMP102_DEVICE_IN_NORMAL,
    TMP102_DEVICE_IN_SHUTDOWN
} Tmp102_Shutdown_e;

/**
 * @brief get temperature from sensor in celcius.
 * 
 * @param file handle for i2c bus
 * @param pTemp pointer to temperature variable / return value
 * @return int8_t status, EXIT_SUCCESS or EXIT_FAILURE
 */
int8_t tmp102_getTempC(uint8_t file, float *pTemp);

/**
 * @brief get low temperature threshold value in celcius.
 * 
 * @param file handle to i2c bus
 * @param pLow pointer to store threshold value
 * @return int8_t status, EXIT_SUCCESS if succeeeds
 */
int8_t tmp102_getLowThreshold(uint8_t file, float *pLow);

/**
 * @brief get high temperature threshold value in celcius.
 * 
 * @param file handle to i2c bus
 * @param pLow pointer to store threshold value
 * @return int8_t status, EXIT_SUCCESS if succeeeds
 */
int8_t tmp102_getHighThreshold(uint8_t file, float *pHigh);

/**
 * @brief change the low temperature threshold sensor value
 * 
 * @param file handle to i2c bus
 * @param high new threshold value to write
 * @return int8_t return status, EXIT_SUCCESS if succeeds
 */
int8_t tmp102_setLowThreshold(uint8_t file, float low);

/**
 * @brief change the high temperature threshold sensor value
 * 
 * @param file handle to i2c bus
 * @param high new threshold value to write
 * @return int8_t return status, EXIT_SUCCESS if succeeds
 */
int8_t tmp102_setHighThreshold(uint8_t file, float high);

/**
 * @brief get fault queue size: number of measurements beyond admissible
 * range to set alert pin; see enum definition for acceptable values
 * 
 * @param file handle to i2c bus
 * @param pFaults pointer to results variable
 * @return int8_t return status, EXIT_SUCCESS if succeeds
 */
int8_t tmp102_getFaultQueueSize(uint8_t file, Tmp102_FaultCount_e *pFaults);

/**
 * @brief get address mode, normal (12-bits) or extended (13-bits)
 * 
 * @param file handle to i2c bus
 * @param pEM pointer to variable to store mode
 * @return int8_t status, returns EXIT_SUCCESS if succeeds
 */
int8_t tmp102_getExtendedMode(uint8_t file, Tmp102_AddrMode_e *pMode);

/**
 * @brief determine if sensor is in shutdown mode
 * 
 * @param file handle to i2c bus
 * @param pState pointer to state variable, to store results
 * @return int8_t status, EXIT_SUCCESS if succeeds
 */
int8_t tmp102_getShutdownState(uint8_t file, Tmp102_Shutdown_e *pState);

/**
 * @brief get current value of alert pin; pin is set when temperature
 * goes beyong high temp threshold for required fault queue samples and 
 * stays set until the temperature decreases below low temp threshold.
 * 
 * @param file handle to i2c bus
 * @param pAlert pointer to results variable
 * @return int8_t status, EXIT_SUCCESS if succeeds
 */
int8_t tmp102_getAlert(uint8_t file, Tmp102_Alert_e *pAlert);

/**
 * @brief change the sampling conversion rate of the sensor; see 
 * enumeration definition for acceptable values
 * 
 * @param file handle to i2c bus
 * @param pConvRt pointer to results variable
 * @return int8_t status, EXIT_SUCCESS if succeeds
 */
int8_t tmp102_getConvRate(uint8_t file, Tmp102_ConvRate_e *pConvRt);

/**
 * @brief set fault queue size: number of measurements beyond admissible
 * range to set alert pin; see enum definition for acceptable values
 * 
 * @param file handle to i2c bus
 * @param size number of queue samples required to trigger alert
 * @return int8_t return status, EXIT_SUCCESS if succeeds
 */
int8_t tmp102_setFaultQueueSize(uint8_t file, Tmp102_FaultCount_e size);

/**
 * @brief change the address mode to/from Extended; if extended mode then
 * Temp, Tlow and Thigh values are 13-bits wide; else they're 12-bit
 * 
 * @param file handle to i2c bus
 * @param mode desired mode
 * @return int8_t return status, EXIT_SUCCESS if succeeds
 */
int8_t tmp102_setExtendedMode(uint8_t file, Tmp102_AddrMode_e mode);

/**
 * @brief command the sensor into / out of shutdown mode. In shutdown mode
 * the sensor doesn't take any readings to save power, unless triggered by
 * the one-shot mode.  When not in shutdown mode the sensor continuously
 * samples the temperature.
 * 
 * @param file handle to i2c bus
 * @param state desired state
 * @return int8_t return status, EXIT_SUCCESS if succeeds
 */
int8_t tmp102_setShutdownState(uint8_t file, Tmp102_Shutdown_e state);

/**
 * @brief set the sample conversion rate; see enumeration definition
 * for acceptable values.
 * 
 * @param file handle to i2c bus
 * @param pConvRt desired sample conversion rate
 * @return int8_t return status, EXIT_SUCCESS if succeeds
 */
int8_t tmp102_setConvRate(uint8_t file, uint8_t *pConvRt);

/* unit conversions to/from C/F/K */
#define TMP_DEGC_TO_DEGK(DEG)    (((float)(DEG)) + 273.15f)
#define TMP_DEGC_TO_DEGF(DEG)    ((((float)(DEG)) * (9.0f / 5.0f)) + 32.0f)
#define TMP_DEGF_TO_DEGC(DEG)    ((((float)(DEG)) - 32.0f) * (5.0f / 9.0f))
#define TMP_DEGF_TO_DEGK(DEG)    (TMP_DEGC_TO_DEGK(TMP_DEGF_TO_DEGC(DEG)))
#define TMP_DEGK_TO_DEGC(DEG)    (((float)(DEG)) - 273.15f)
#define TMP_DEGK_TO_DEGF(DEG)    (TMP_DEGC_TO_DEGF(TMP_DEGK_TO_DEGC(DEG)))

#endif /* TEMP_SENSOR_H_ */

/***********************************************************************************
 * @author Josh Malburg
 * 
 * Advanced Embedded Software Development
 * ECEN5013 - Rick Heidebrecht
 * @date March 15, 2019
 * arm-linux-gnueabi (Buildroot)
 * gcc (Ubuntu)
 ************************************************************************************
 *
 * @file test_logger.c
 * @brief verify logger writes and reads to queue
 *
 ************************************************************************************
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include "debug.h"

#include "logger.h"

int main(void)
{
    SYSTEM_INITIALIZED();
    INFO("Hi, this is the logger from ECEN5813\n");
    PROFILING_STARTED();
    PROFILING_COMPLETED();

    return EXIT_SUCCESS;
}
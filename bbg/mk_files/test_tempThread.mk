#*****************************************************************************
# @author Joshua Malburg
# joshua.malburg@colorado.edu
# Advanced Embedded Software Development
# ECEN5013-002 - Rick Heidebrecht
# @date March 20, 2018
#*****************************************************************************
# @file test_tempThread.mk
# @brief unit tests for temp thread
#
#*****************************************************************************

# source files
SRCS += unittest/test_tempThread.c \
        src/tempSensor.c \
        src/lu_iic.c \
        src/tempThread.c \
        src/cmn_timer.c \
        src/logger_queue.c \
        src/logger_helper.c \
        src/memory.c \
        src/conversion.c \
        src/remoteThread.c \
        src/tempThread.c \
        src/lightThread.c \
        src/tempSensor.c \
        src/lightSensor.c \
        src/loggingThread.c \
        src/healthMonitor.c

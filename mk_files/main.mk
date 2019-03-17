#*****************************************************************************
# @author Brian Ibeling and Joshua Malburg (joma0364)
# brian.ibeling@colorado.edu
# joshua.malburg@colorado.edu
# Advanced Embedded Software Development
# ECEN5013-002 - Rick Heidebrecht
# @date March 7, 2018
#*****************************************************************************
# @file main.mk
# @brief project 1 source library
#
#*****************************************************************************

# source files
SRCS += src/tempSensor.c \
        src/lightSensor.c \
        src/loggingThread.c \
        src/remoteThread.c \
        src/tempThread.c \
        src/lightThread.c \
        src/lu_iic.c \
        src/logger_queue.c \
        src/logger_helper.c \
        src/memory.c \
        src/conversion.c \
        src/bbgLeds.c \
        src/main.c

PLATFORM = BBG

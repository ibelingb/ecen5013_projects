#*****************************************************************************
# @author Joshua Malburg
# joshua.malburg@colorado.edu
# Advanced Embedded Software Development
# ECEN5013-002 - Rick Heidebrecht
# @date March 23, 2018
#*****************************************************************************
# @file test_healthMonitor.mk
# @brief unit tests for temp sensor library
#
#*****************************************************************************

# source files
SRCS += unittest/test_healthMonitor.c \
src/healthMonitor.c \
src/tempThread.c \
src/lightThread.c \
src/logger_queue.c \
src/logger_helper.c \
src/memory.c \
src/conversion.c \
src/remoteThread.c \
src/loggingThread.c \
src/tempSensor.c \
src/lightSensor.c \
src/cmn_timer.c \
src/lu_iic.c \
src/logger_helper.c

#*****************************************************************************
# @author Brian Ibeling
# brian.ibeling@colorado.edu
# Advanced Embedded Software Development
# ECEN5013-002 - Rick Heidebrecht
# @date March 17, 2018
#*****************************************************************************
# @file test_light.mk
# @brief unit tests for light sensor library
#
#*****************************************************************************

# source files
SRCS += unittest/test_light.c \
        src/lightSensor.c \
        src/lu_iic.c

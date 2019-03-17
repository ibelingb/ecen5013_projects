#*****************************************************************************
# @author Joshua Malburg (joma0364)
# joshua.malburg@colorado.edu
# Advanced Embedded Software Development
# ECEN5013-002 - Rick Heidebrecht
# @date March 7, 2018
#*****************************************************************************
# @file temp.mk
# @brief unit tests for temp sensor library
#
#*****************************************************************************

# source files
SRCS += unittest/test_temp.c \
        src/tempSensor.c \
        src/lu_iic.c

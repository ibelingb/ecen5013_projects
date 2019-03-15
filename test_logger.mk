#*****************************************************************************
# @author Joshua Malburg (joma0364)
# joshua.malburg@colorado.edu
# Advanced Embedded Software Development
# ECEN5013-002 - Rick Heidebrecht
# @date March 15, 2018
#*****************************************************************************
# @file temp.mk
# @brief unit tests for logger writes and reads to logger msg queue
#
#*****************************************************************************

# source files
SRCS += unittest/test_logger.c \
src/logger_block.c \
src/logger_helper.c \
src/memory.c \
src/conversion.c
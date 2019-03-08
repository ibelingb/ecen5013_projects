#*****************************************************************************
# @author Joshua Malburg (joma0364)
# joshua.malburg@colorado.edu
# Advanced Embedded Software Development
# ECEN5013-002 - Rick Heidebrecht
# @date March 7, 2018
#*****************************************************************************
#
# @file Makefile
# @brief Make targets and recipes to generate object code, executable, etc
#
#*****************************************************************************

# General / default variables for all platforms / architectures
CFLAGS = -Wall -g -O0 -Werror
CPPFLAGS = -MD -MP
TARGET = temp
LDFLAGS = 
include $(TARGET).mk
INCLDS = -I.

ifeq ($(PLATFORM),BBG)
CROSS_COMP_NAME = arm-buildroot-linux-uclibcgnueabihf
CC = $(CROSS_COMP_NAME)-gcc
LD = $(CROSS_COMP_NAME)-ld
AR = $(CROSS_COMP_NAME)-ar
SZ = $(CROSS_COMP_NAME)-size
READELF = $(CROSS_COMP_NAME)-readelf
else 
CC = gcc
LD = ld
AR = ar
SZ = size
READELF = readelf
endif

# for recursive clean
GARBAGE_TYPES := *.o *.elf *.map *.i *.asm *.d *.out
DIR_TO_CLEAN = src test
DIR_TO_CLEAN += $(shell find -not -path "./.git**" -type d)
GARBAGE_TYPED_FOLDERS := $(foreach DIR,$(DIR_TO_CLEAN), $(addprefix $(DIR)/,$(GARBAGE_TYPES)))

# 
OBJS  = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

.PHONY: clean
clean: 
	@$(RM) -rf $(GARBAGE_TYPED_FOLDERS)
	@ echo "Clean complete"

$(OBJS): %.o : %.c %.d
	@$(CC) -c $(CPPFLAGS) $(INCLDS) $(CFLAGS) $< -o $@ 
	@ echo "Compiling $@"	

%.i : %.c
	@$(CC) -E $(CPPFLAGS) $(INCLDS) $< -o $@ 
	@ echo "Compiling $@"
	
%.d : %.c
	@$(CC) -E $(CPPFLAGS) $(INCLDS) $< -o $@ 
	@ echo "Compiling $@"
	
%.asm : %.c
	@$(CC) -S $(CPPFLAGS) $(INCLDS) $(CFLAGS) $< -o $@ 
	@ echo "Compiling $@"
	
.PHONY: build
build: all

.PHONY: all
all: $(TARGET).elf
	
$(TARGET).elf: $(OBJS)
	@echo PLATFORM = $(PLATFORM)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $(TARGET).elf $^ $(LDFLAGS)
	@echo build complete
	@$(SZ) -Bx $(TARGET).elf
	@echo
	@$(READELF) -h $(TARGET).elf
	@echo
	@$(READELF) -A $(TARGET).elf
export CROSS_COMPILE = arm-none-eabi-
#root directory
export ROOTDIR=$(shell pwd)

# Sources
SRCS = main.c stm32f4xx_it.c system_stm32f4xx.c syscalls.c

# USB
SRCS += usbd_usr.c usbd_cdc_vcp.c usbd_desc.c usb_bsp.c

# Project name
PROJ_NAME = ws2812_panel

# output folder
OUTPATH=build

###################################################

CC=$(CROSS_COMPILE)gcc
OBJCOPY=$(CROSS_COMPILE)objcopy
SIZE=$(CROSS_COMPILE)size

CFLAGS  = -std=gnu99 -g -O2 -Wall
CFLAGS += -mlittle-endian -mthumb -mthumb-interwork -nostartfiles -mcpu=cortex-m4

CFLAGS += -fsingle-precision-constant -Wdouble-promotion
CFLAGS += -mfpu=fpv4-sp-d16 -mfloat-abi=hard

###################################################

vpath %.c src

LIBPATH = lib

# Includes
CFLAGS += -Iinc
CFLAGS += -Ilib/Core/cmsis
CFLAGS += -Ilib/Core/stm32
CFLAGS += -Ilib/Conf

# Library paths
LIBPATHS  = -Llib/StdPeriph
LIBPATHS += -Llib/USB_Device/Core
LIBPATHS += -Llib/USB_Device/Class/cdc
LIBPATHS += -Llib/USB_OTG
LIBPATHS += -Llib/ws2812
# LIBPATHS += -Llib/FreeRTOS

# Libraries to link
LIBS  = -lm
LIBS += -lws2812
LIBS += -lstdperiph
LIBS += -lusbdevcore
LIBS += -lusbdevcdc
LIBS += -lusbcore
# LIBS += -lfreertos

# Extra includes
CFLAGS += -Ilib/StdPeriph/inc
CFLAGS += -Ilib/USB_OTG/inc
CFLAGS += -Ilib/USB_Device/Core/inc
CFLAGS += -Ilib/USB_Device/Class/cdc/inc
CFLAGS += -Ilib/ws2812/inc
# CFLAGS += -Ilib/FreeRTOS/inc

# add startup file to build
SRCS += lib/startup_stm32f4xx.s

OBJS = $(SRCS:.c=.o)

###################################################

.PHONY: lib clean cleanlib proj 

all: lib proj
	$(SIZE) $(OUTPATH)/$(PROJ_NAME).elf

lib:
	$(MAKE) -C $(LIBPATH)/StdPeriph
	$(MAKE) -C $(LIBPATH)/USB_OTG
	$(MAKE) -C $(LIBPATH)/USB_Device/Core
	$(MAKE) -C $(LIBPATH)/USB_Device/Class/cdc
#	$(MAKE) -C $(LIBPATH)/USB_Host/Core
#	$(MAKE) -C $(LIBPATH)/USB_Host/Class/MSC
#	$(MAKE) -C $(LIBPATH)/fat_fs
	$(MAKE) -C $(LIBPATH)/ws2812
#	$(MAKE) -C $(LIBPATH)/FreeRTOS

cleanlib:
	$(MAKE) clean -C $(LIBPATH)/StdPeriph
	$(MAKE) clean -C $(LIBPATH)/USB_OTG
	$(MAKE) clean -C $(LIBPATH)/USB_Device/Core
	$(MAKE) clean -C $(LIBPATH)/USB_Device/Class/cdc
#	$(MAKE) clean -C $(LIBPATH)/USB_Host/Core
#	$(MAKE) clean -C $(LIBPATH)/USB_Host/Class/MSC
#	$(MAKE) clean -C $(LIBPATH)/fat_fs
	$(MAKE) clean -C $(LIBPATH)/ws2812
#	$(MAKE) clean -C $(LIBPATH)/FreeRTOS

proj: $(OUTPATH)/$(PROJ_NAME).bin

$(OUTPATH)/$(PROJ_NAME).elf: $(SRCS)
	$(CC) $(CFLAGS) -Tstm32_flash.ld $^ -o $@ $(LIBPATHS) $(LIBS)

$(OUTPATH)/$(PROJ_NAME).hex: $(OUTPATH)/$(PROJ_NAME).elf
	$(OBJCOPY) -O ihex $< $@

$(OUTPATH)/$(PROJ_NAME).bin: $(OUTPATH)/$(PROJ_NAME).elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f *.o
	rm -f $(OUTPATH)/$(PROJ_NAME).elf
#	rm -f $(OUTPATH)/$(PROJ_NAME).hex
	rm -f $(OUTPATH)/$(PROJ_NAME).bin


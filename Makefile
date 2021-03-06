export CROSS_COMPILE = arm-none-eabi-
STLINK = st-flash

#root directory
export ROOTDIR=$(shell pwd)

# Sources
SRCS  = main_testing.c
#SRCS  = main.c
#SRCS += main_web_content_handler.c
SRCS += stm32f4xx_it.c
SRCS += system_stm32f4xx.c
SRCS += syscalls.c
SRCS += init.c

#conditionally add ap_config.c
ifneq ("$(wildcard src/ap_config.c)","")
SRCS += ap_config.c
endif

CPPSRCS = 

# USB
SRCS += usbd_usr.c
SRCS += usbd_cdc_vcp.c
SRCS += usbd_desc.c
SRCS += usb_bsp.c

# add startup file to build
STARTUP = lib/startup_stm32f4xx.s

# Project name
PROJ_NAME = ws2812_panel

###############################
# Add sources before this line
###############################

include config.mk

###############################
# Add compilation parameters after this line
###############################

LIBPATH = lib

# libs to link
LIBS += usbdevcore
LIBS += usbdevcdc
LIBS += usbcore
LIBS += ws2812
LIBS += esp8266
LIBS += http
LIBS += paho
LIBS += stdperiph
LIBS += freertos
LIBS += color_tools
LIBS += math_tools

LIBPATH_color_tools = $(LIBPATH)/color_tools
LIBPATH_math_tools  = $(LIBPATH)/math_tools
LIBPATH_usbdevcore  = $(LIBPATH)/USB_Device/Core
LIBPATH_usbdevcdc   = $(LIBPATH)/USB_Device/Class/cdc
LIBPATH_usbcore     = $(LIBPATH)/USB_OTG
LIBPATH_ws2812      = $(LIBPATH)/ws2812
LIBPATH_esp8266     = $(LIBPATH)/esp8266
LIBPATH_http        = $(LIBPATH)/http
LIBPATH_paho        = $(LIBPATH)/paho
LIBPATH_stdperiph   = $(LIBPATH)/StdPeriph
LIBPATH_freertos    = $(LIBPATH)/FreeRTOS

# concatenate all library paths
LIB = $(addprefix -L,$(foreach lib,$(LIBS),$(LIBPATH_$(lib)))) $(addprefix -l,$(LIBS))

# LIBS includes
CFLAGS += $(addprefix -I,$(addsuffix /inc,$(foreach lib,$(LIBS),$(LIBPATH_$(lib)))))

# Libraries in style path/libxyz.a
LIBS_BUILD = $(foreach lib,$(LIBS),$(addsuffix /lib$(lib).a,$(LIBPATH_$(lib))))

# Includes
CFLAGS += -Iinc
CFLAGS += -I$(LIBPATH)/Core/cmsis
CFLAGS += -I$(LIBPATH)/Core/stm32
CFLAGS += -I$(LIBPATH)/Conf

# add system libraries here
LIB += -lm

###################################################

.PHONY: clean cleanlib $(LIBS_BUILD) flash dirs cleandirs

all: $(OUTPATH)/$(PROJ_NAME).elf
	$(SIZE) $(OUTPATH)/$(PROJ_NAME).elf

$(LIBS_BUILD): config.mk rules.mk
	$(MAKE) $(notdir $@) -C $(dir $@)

$(OUTPATH)/$(PROJ_NAME).elf: $(OBJS) $(CPPOBJS) $(LIBS_BUILD)
	$(CC) $(CFLAGS) -Tstm32_flash.ld $(OBJS) $(CPPOBJS) $(STARTUP) -o $@ $(SYSLIBS) $(LIB) -Wl,--gc-sections -Wl,--cref -Wl,-Map=$(OUTPATH)/$(PROJ_NAME).map

$(OUTPATH)/$(PROJ_NAME).hex: $(OUTPATH)/$(PROJ_NAME).elf
	$(OBJCOPY) -O ihex $< $@

$(OUTPATH)/$(PROJ_NAME).bin: $(OUTPATH)/$(PROJ_NAME).elf
	$(OBJCOPY) -O binary $< $@

$(OUTPATH)/$(PROJ_NAME).dump: $(OUTPATH)/$(PROJ_NAME).elf
	$(OBJDUMP) -D $< $@

cleanlib:
	$(foreach lib,$(LIBS),$(MAKE) clean -C $(LIBPATH_$(lib));)

clean::
	rm -f $(OUTPATH)/$(PROJ_NAME).map
	rm -f $(OUTPATH)/$(PROJ_NAME).elf
	rm -f $(OUTPATH)/$(PROJ_NAME).hex
	rm -f $(OUTPATH)/$(PROJ_NAME).bin

flash: $(OUTPATH)/$(PROJ_NAME).bin
	$(STLINK) write $< 0x08000000

dirs::
	$(foreach lib,$(LIBS),$(MAKE) dirs -C $(LIBPATH_$(lib));)

cleandirs::
	$(foreach lib,$(LIBS),$(MAKE) cleandirs -C $(LIBPATH_$(lib));)

include rules.mk

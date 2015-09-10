
# Sources
SRCS += ws2812.c

# Config
include $(ROOTDIR)/config.mk

# Includes
CFLAGS += -Iinc
CFLAGS += -I../Core/cmsis
CFLAGS += -I../Core/stm32
CFLAGS += -I../StdPeriph/inc
CFLAGS += -I../FreeRTOS/inc
#output name
LIBNAME = libws2812.a

#rules
include $(ROOTDIR)/rules.mk

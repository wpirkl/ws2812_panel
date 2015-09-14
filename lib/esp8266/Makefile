# Sources
SRCS  = esp8266.c
SRCS += uart_dma.c

# Configuration
include $(ROOTDIR)/config.mk

# Includes
CFLAGS += -Iinc
CFLAGS += -I../Conf
CFLAGS += -I../Core/cmsis
CFLAGS += -I../Core/stm32
CFLAGS += -I../StdPeriph/inc
CFLAGS += -I../FreeRTOS/inc


LIBNAME = libesp8266.a

# rules
include $(ROOTDIR)/rules.mk

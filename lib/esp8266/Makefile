# Sources
SRCS  = esp8266.c
SRCS += uart_dma.c
SRCS += esp8266_http_server.c

# Configuration
include $(ROOTDIR)/config.mk

# Includes
CFLAGS += -Iinc
CFLAGS += -I../Conf
CFLAGS += -I../Core/cmsis
CFLAGS += -I../Core/stm32
CFLAGS += -I../StdPeriph/inc
CFLAGS += -I../FreeRTOS/inc
CFLAGS += -I../http/inc


LIBNAME = libesp8266.a

# rules
include $(ROOTDIR)/rules.mk

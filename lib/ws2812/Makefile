
# Sources
SRCS += ws2812.c
SRCS += ws2812_anim.c
SRCS += ws2812_anim_const_color.c


# Config
include $(ROOTDIR)/config.mk

# Includes
CFLAGS += -Iinc
CFLAGS += -I../Conf
CFLAGS += -I../Core/cmsis
CFLAGS += -I../Core/stm32
CFLAGS += -I../StdPeriph/inc
CFLAGS += -I../FreeRTOS/inc
#output name
LIBNAME = libws2812.a

#rules
include $(ROOTDIR)/rules.mk

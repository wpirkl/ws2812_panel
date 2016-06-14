
# Sources
SRCS += ws2812.c
SRCS += ws2812_anim.c
SRCS += ws2812_anim_const_color.c
SRCS += ws2812_anim_gradient.c
SRCS += ws2812_anim_color_palette.c
SRCS += ws2812_anim_fire.c
SRCS += ws2812_transition_fade.c


# Config
include $(ROOTDIR)/config.mk

# Includes
CFLAGS += -Iinc
CFLAGS += -I../Conf
CFLAGS += -I../Core/cmsis
CFLAGS += -I../Core/stm32
CFLAGS += -I../StdPeriph/inc
CFLAGS += -I../FreeRTOS/inc
CFLAGS += -I../color_tools/inc

#output name
LIBNAME = libws2812.a

#rules
include $(ROOTDIR)/rules.mk

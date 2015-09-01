
# Sources
SRCS += croutine.c
SRCS += event_groups.c
SRCS += list.c
SRCS += port.c
SRCS += queue.c
SRCS += tasks.c
SRCS += timers.c

# Config
include $(ROOTDIR)/config.mk

# Includes
CFLAGS += -Iinc
CFLAGS += -I../Core/cmsis
CFLAGS += -I../Core/stm32
CFLAGS += -I../StdPeriph/inc

LIBNAME = libfreertos.a

# Rules
include $(ROOTDIR)/rules.mk

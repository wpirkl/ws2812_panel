# Sources
SRCS = usbd_core.c usbd_ioreq.c usbd_req.c

# Config
include $(ROOTDIR)/config.mk

# Includes
CFLAGS += -Iinc
CFLAGS += -I../../Core/cmsis
CFLAGS += -I../../Core/stm32
CFLAGS += -I../../Conf
CFLAGS += -I../../USB_OTG/inc

LIBNAME = libusbdevcore.a

# Rules
include $(ROOTDIR)/rules.mk

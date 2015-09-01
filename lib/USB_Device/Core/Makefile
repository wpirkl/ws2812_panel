# Sources
SRCS = usbd_core.c usbd_ioreq.c usbd_req.c

# Config
include $(ROOTDIR)/config.mk

# Includes
CFLAGS += -Iinc -I../../Core/cmsis -I../../Core/stm32
CFLAGS += -I../../Conf -I../../USB_OTG/inc

LIBNAME = libusbdevcore.a

# Rules
include $(ROOTDIR)/rules.mk

include $(ROOTDIR)/config.mk

# Includes
CFLAGS += -Iinc -I../../Core/cmsis -I../../Core/stm32
CFLAGS += -I../../Conf -I../../USB_OTG/inc

# Sources
SRCS = usbd_core.c usbd_ioreq.c usbd_req.c

LIBNAME = libusbdevcore.a

#rules
include $(ROOTDIR)/rules.mk

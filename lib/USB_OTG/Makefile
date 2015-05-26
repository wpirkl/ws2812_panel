include $(ROOTDIR)/config.mk

# Includes
CFLAGS += -Iinc -I../Core/cmsis -I../Core/stm32
CFLAGS += -I../Conf

# Sources
SRCS = usb_core.c usb_dcd.c usb_dcd_int.c usb_hcd.c
SRCS += usb_hcd_int.c usb_otg.c

LIBNAME = libusbcore.a

#rules
include $(ROOTDIR)/rules.mk

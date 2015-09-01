
# Sources
SRCS = usbd_cdc_core.c

# Config
include $(ROOTDIR)/config.mk

# Includes
CFLAGS += -Iinc
CFLAGS += -I../../../Core/cmsis
CFLAGS += -I../../../Core/stm32
CFLAGS += -I../../../Conf
CFLAGS += -I../../../USB_OTG/inc
CFLAGS += -I../../Core/inc
LIBNAME = libusbdevcdc.a

#rules
include $(ROOTDIR)/rules.mk

CC=$(CROSS_COMPILE)gcc
OBJCOPY=$(CROSS_COMPILE)objcopy
SIZE=$(CROSS_COMPILE)size

SRCDIR := src
OBJDIR := obj

vpath %.c $(SRCDIR)

OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
DEPS = $(addprefix $(OBJDIR)/,$(SRCS:.c=.dep))

CFLAGS  = -g -O2 -Wall
CFLAGS += -mlittle-endian
CFLAGS += -mthumb
CFLAGS += -mthumb-interwork
CFLAGS += -mcpu=cortex-m4

CFLAGS += -fsingle-precision-constant
CFLAGS += -Wdouble-promotion
CFLAGS += -mfpu=fpv4-sp-d16
CFLAGS += -mfloat-abi=hard

CFLAGS += -ffreestanding
CFLAGS += -nostdlib

CC=$(CROSS_COMPILE)gcc
OBJCOPY=$(CROSS_COMPILE)objcopy
SIZE=$(CROSS_COMPILE)size
OBJDUMP=$(CROSS_COMPILE)objdump

SRCDIR := src
OBJDIR := obj
OUTPATH := build

vpath %.c $(SRCDIR)
vpath %.cpp $(SRCDIR)

OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
DEPS = $(addprefix $(OBJDIR)/,$(SRCS:.c=.dep) $(CPPSRCS:.cpp=.dep))

CPPOBJS = $(addprefix $(OBJDIR)/,$(CPPSRCS:.cpp=.obj))

CFLAGS  = -O2 -Wall
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

CFLAGS += -fdata-sections
CFLAGS += -ffunction-sections

CPPFLAGS = $(CFLAGS) -lgcc

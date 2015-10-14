# Sources
SRCS += MQTTConnectClient.c
SRCS += MQTTConnectServer.c
SRCS += MQTTDeserializePublish.c
SRCS += MQTTFormat.c
SRCS += MQTTPacket.c
SRCS += MQTTSerializePublish.c
SRCS += MQTTSubscribeClient.c
SRCS += MQTTSubscribeServer.c
SRCS += MQTTUnsubscribeClient.c
SRCS += MQTTUnsubscribeServer.c
SRCS += MQTTClient.c
SRCS += MQTTFreeRTOS.c
SRCS += MQTTesp8266.c


# Configuration
include $(ROOTDIR)/config.mk

# Includes
CFLAGS += -Iinc
CFLAGS += -I../FreeRTOS/inc
CFLAGS += -I../esp8266/inc


LIBNAME = libpaho.a

# rules
include $(ROOTDIR)/rules.mk

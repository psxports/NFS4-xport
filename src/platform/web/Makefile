PROJECT_ROOT := $(abspath ../../..)
SHORT_NAME := NFS4
FULL_NAME := Need for Speed: High Stakes
CONFIG ?= Release

APP_SOURCES := $(filter-out $(PROJECT_ROOT)/src/platform/%,$(wildcard \
	$(PROJECT_ROOT)/src/*.cpp \
	$(PROJECT_ROOT)/src/*/*.cpp \
	$(PROJECT_ROOT)/src/*/*/*.cpp \
	$(PROJECT_ROOT)/src/*.c \
	$(PROJECT_ROOT)/src/*/*.c \
	$(PROJECT_ROOT)/src/*/*/*.c))
APP_DEFINES := -DFIELD_RATE=60 -DXPORT_REGISTERED_GPU_PACKETS -DWND_TITLE='"$(FULL_NAME) (xport)"' -DWND_WIDTH=960 -DWND_HEIGHT=720

include ../../../../xport/src/platform/web/Makefile

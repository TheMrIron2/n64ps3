# Makefile-friendly source export for downstream n64ps3 consumers.
# Keep this in sync with the n64ps3 CMake library source list.

N64PS3_MAKEFILE_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
N64PS3_ROOT ?= $(patsubst %/make/,%,$(N64PS3_MAKEFILE_DIR))

N64PS3_INCLUDE_DIRS := $(N64PS3_ROOT)/include
N64PS3_CPPFLAGS := $(addprefix -I,$(N64PS3_INCLUDE_DIRS))

N64PS3_COMMON_C_FILES := \
	$(N64PS3_ROOT)/src/core/backend.c \
	$(N64PS3_ROOT)/src/core/config.c \
	$(N64PS3_ROOT)/src/core/log.c \
	$(N64PS3_ROOT)/src/core/runtime.c \
	$(N64PS3_ROOT)/src/os/os_common.c \
	$(N64PS3_ROOT)/src/os/os_event.c \
	$(N64PS3_ROOT)/src/pad/pad_common.c \
	$(N64PS3_ROOT)/src/audio/audio_common.c \
	$(N64PS3_ROOT)/src/pi/pi_common.c \
	$(N64PS3_ROOT)/src/vi/vi_common.c \
	$(N64PS3_ROOT)/src/gfx/gfx_frontend.c

N64PS3_GFX_BACKEND_C_FILES ?= \
	$(N64PS3_ROOT)/src/backends/pc/rdpnull.c

N64PS3_C_FILES := \
	$(N64PS3_COMMON_C_FILES) \
	$(N64PS3_GFX_BACKEND_C_FILES)

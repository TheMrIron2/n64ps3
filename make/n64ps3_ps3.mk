# PS3/PSL1GHT Makefile integration for n64ps3.

include $(dir $(lastword $(MAKEFILE_LIST)))n64ps3_sources.mk

N64PS3_PS3_CPPFLAGS := $(N64PS3_CPPFLAGS) -DN64PS3_PS3=1
N64PS3_PS3_CFLAGS ?= -std=gnu99 -Wall -Wextra -Werror -O0
N64PS3_GFX_BACKEND_C_FILES := $(N64PS3_ROOT)/src/backends/ps3/rdprsx.c
N64PS3_C_FILES := $(N64PS3_COMMON_C_FILES) $(N64PS3_GFX_BACKEND_C_FILES)
N64PS3_PS3_SHADER_DIR := $(N64PS3_ROOT)/src/backends/ps3/shaders

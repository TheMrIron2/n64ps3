# Host Makefile integration for n64ps3.

include $(dir $(lastword $(MAKEFILE_LIST)))n64ps3_sources.mk

N64PS3_HOST_CPPFLAGS := $(N64PS3_CPPFLAGS) -DN64PS3_HOST=1
N64PS3_HOST_CFLAGS ?= -std=c99 -Wall -Wextra -Werror -pedantic

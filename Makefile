PROJECT_NAME := silver
REL := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
include $(REL)../A/build.mk

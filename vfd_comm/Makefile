#
#
#

PRJ_ROOT:=..
MOD_NAME:=vfdcommon
include $(PRJ_ROOT)/scripts/def.mak


CSRC = $(shell find . -name "*.c")
CXXSRC= $(shell find . -name "*.cpp")

EXTRA_FLAGS:=-Wno-deprecated
CDEFS+= 



all: $(STATIC_DEST)
additional_clean:
	rm -f $(STATIC_DEST)

include $(PRJ_ROOT)/scripts/common.mak




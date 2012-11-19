#  
#  Unified Building System for Linux
#  By CSK (csk@live.com)
#  

PRJ_ROOT:=..
MOD_NAME:=serialvfddrv

include $(PRJ_ROOT)/scripts/def.mak


CSRC = $(shell find . -name "*.c")
CXXSRC= $(shell find . -name "*.cpp")

EXTRA_FLAGS:=-Wno-deprecated
CDEFS+= 
DEP_AR+= $(OUTPUT_ROOT)/vfdcommon.a


all: $(DYNAMIC_DEST)
additional_clean:
	rm -f $(DYNAMIC_DEST)

include $(PRJ_ROOT)/scripts/common.mak




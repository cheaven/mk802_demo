#
#  Simple VFD Dispatcher via Serial port
#
#  By CSK

PRJ_ROOT:=..
MOD_NAME:=simple_serial_vfd


include $(PRJ_ROOT)/scripts/def.mak


CSRC = $(shell find . -name "*.c")
CXXSRC= $(shell find . -name "*.cpp")


CDEFS+= 
INCLUDES+= 
DEP_LIBS += -lserialvfddrv

all: $(EXEC_DEST)

additional_clean:
	rm -r -f $(EXEC_DEST)

include $(PRJ_ROOT)/scripts/common.mak

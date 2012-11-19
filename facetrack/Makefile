#
#  Display the extracted foreground mask on a VFD
#  
#  By CSK

PRJ_ROOT:=..
MOD_NAME:=facetrack


include $(PRJ_ROOT)/scripts/def.mak


CSRC = $(shell find . -name "*.c")
CXXSRC= $(shell find . -name "*.cpp")


CDEFS+= 
INCLUDES+=.
DEP_LIBS += -lserialvfddrv $(OPENCV_LIBS) 

all: $(EXEC_DEST)

additional_clean:
	rm -r -f $(EXEC_DEST)

include $(PRJ_ROOT)/scripts/common.mak

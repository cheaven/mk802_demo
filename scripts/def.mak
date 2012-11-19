#  
#  Unified Building System for Linux
#  By CSK (csk@live.com)
#  

include $(PRJ_ROOT)/Makefile.conf

.PHONY: all clean install distclean listsubs

# Path
PRJ_ABS_ROOT:=$(shell cd $(PRJ_ROOT) && pwd)

OBJ_PARENT_ROOT:=$(PRJ_ROOT)/.obj/$(ARCH_PLATFORM)
OBJ_ROOT:=$(OBJ_PARENT_ROOT)/$(MOD_NAME)
OUTPUT_ROOT:=$(PRJ_ROOT)/.output/$(ARCH_PLATFORM)

DEP_FILE += $(patsubst %.o, %.d, $(OBJ))


EXEC_FILENAME:=$(MOD_NAME)
EXEC_DEST:=$(OUTPUT_ROOT)/$(EXEC_FILENAME)

DYNAMIC_FILENAME:=lib$(MOD_NAME).so
DYNAMIC_DEST:=$(OUTPUT_ROOT)/$(DYNAMIC_FILENAME)

STATIC_FILENAME:=$(MOD_NAME).a
STATIC_DEST:=$(OUTPUT_ROOT)/$(STATIC_FILENAME)


# Building Flags

ifeq ($(OPT_DBG), yes)
OPT_LVL?=-g
CDEFS+= -D_DEBUG
else
OPT_LVL?=-O2
endif


ifeq ($(ARCH_PLATFORM), armv7l)
TARGET_DEF:= 
else
TARGET_DEF:= 
endif

CDEFS+= -DTARGET_$(ARCH_PLATFORM)


OPENCV_LIBS:=-L$(PREFIX)/lib -ljpeg -lpng -lz -lopencv_core -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_contrib -lopencv_legacy -lopencv_flann

# Dependencies
INCLUDES+= -I. \
	   -I$(PRJ_ABS_ROOT) \
	   -I$(PRJ_ABS_ROOT)/include \
           -I$(PREFIX)/include

DEP_LIBS+= -lm \
	    -ldl \
	    -lpthread \
	    -lstdc++ \
	    -lrt



CFLAGS+= $(CDEFS) $(OPT_LVL) $(INCLUDES) $(TARGET_DEF) $(EXTRA_FLAGS) -Wconversion-null
CXXFLAGS+= $(CFLAGS)

LDFLAGS+= $(DEP_LIBS) #-Wl,-rpath-link=$(BUILD_ROOT)/usr/lib


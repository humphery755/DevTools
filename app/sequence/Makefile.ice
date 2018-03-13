# **********************************************************************
#
# Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
#
# **********************************************************************

top_srcdir	= /home/humphery/tmp/ice-3.6.3/cpp

#CLIENT		= client

LIBFILENAME     = $(call mklibfilename,tdd_sequence_SequenceServiceI)
SONAME          = $(call mksoname,tdd_sequence_SequenceServiceI)  

TARGETS		= $(CLIENT) $(LIBFILENAME)

SRC = $(wildcard ./generated/*.cpp) 
OBJS	= $(patsubst %.cpp, %.o, $(SRC))
SLICE_OBJS = $(OBJS)
$(warning  $(SLICE_OBJS) "|" $(SRC))


_SRC = $(wildcard ./Classes/*.cpp) 
_OBJS = $(patsubst %.cpp, %.o, $(_SRC))

SOBJS = $(SLICE_OBJS) \
		$(_OBJS)

$(warning  $(SOBJS) "|" $(SRC))
		  
include $(top_srcdir)/config/Make.rules

$(warning  $(SOBJS))

CPPFLAGS	:= -I. $(CPPFLAGS) -Igenerated -IClasses
LINKWITH	:= -lIceBox $(BZIP2_RPATH_LINK) -lIce -lIceUtil

$(LIBFILENAME): $(SOBJS)
	rm -f $@	
	$(call mkshlib,$@,$(SONAME),$(notdir $(SOBJS)),$(LINKWITH))

$(CLIENT): $(COBJS)
	rm -f $@
	$(CXX) $(LDFLAGS) $(LDEXEFLAGS) -o $@ $(COBJS) $(LIBS)

clean::
	rm -rf *.o *.so

###########################################################################
#
# Some consistent rules for building C++ files:

.SUFFIXES: .cpp

.cpp.o:
	$(CXX) $(CPPFLAGS) -c $< -o $@

audio/%.o: audio/%.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

video/%.o: video/%.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

###########################################################################

# The library to be built
LIB = libsmpeg.a
CC = $(CXX)
USEMATH=true

# Set these variables to the location of your QT libraries if you want to
# build the 'lokiTV' application
#QT_DIR = /usr
#QT_INC_DIR = $(QT_DIR)/include/qt
#QT_LIB_DIR = $(QT_DIR)/lib
#QT_MOC_DIR = $(QT_DIR)/bin

# Uncomment this if you want the library built with mixer hook support
# (i.e. you want to play audio at the same time you use the example mixer)
#USE_MIXER = true

# This library hooks into the mixer library
MIXER = ../mixer	# The location of the SDL mixer example library

# Get the list of objects
HDRS = -I. -Iaudio -Ivideo
DEFS = -DNOCONTROLS -DNDEBUG
COMMONOBJS = common/MPEGstream.o common/MPEGring.o common/smpeg.o
AUDIOOBJS = \
	audio/MPEGaudio.o audio/bitwindow.o audio/filter.o audio/filter_2.o \
	audio/huffmantable.o audio/mpeglayer1.o audio/mpeglayer2.o \
	audio/mpeglayer3.o audio/mpegtable.o audio/mpegtoraw.o
VIDEOOBJS = \
	video/16bit.o video/MPEGvideo.o video/decoders.o video/floatdct.o \
	video/gdith.o video/jrevdct.o video/motionvector.o \
	video/parseblock.o video/readfile.o video/util.o video/video.o
SUBDIRS = common audio video
LIBOBJS = $(COMMONOBJS) $(AUDIOOBJS) $(VIDEOOBJS)

# The demos to be built
TARGET = plaympeg
LIBS = -L. -lsmpeg

# Support for the SDL example mixer library
ifeq ($(USE_MIXER), true)
HDRS += -I$(MIXER)
DEFS += -DSDL_MIXER
LIBS += -L$(MIXER), -lmixer
LIBS_TO_BUILD =  $(MIXER)/libmixer.a
SUBDIRS += $(MIXER)
endif

# Support for building sample QT application
ifneq ($(QT_DIR), )
TARGET += lokiTV
HDRS += -I$(QT_INC_DIR)
LIBS += -L$(QT_LIB_DIR) -lqt -L/usr/X11R6/lib -lX11 -lXext
%_moc.cpp: %.h
	$(QT_MOC_DIR)/moc $< -o $@
endif

include ../GNUmake

plaympeg plaympeg.exe: $(LIB) plaympeg.o $(LIBS_TO_BUILD)
lokiTV lokiTV.exe: $(LIB) lokiTV.o lokiTV_moc.o $(LIBS_TO_BUILD)

$(MIXER)/libmixer.a:
	$(MAKE) -C $(MIXER)


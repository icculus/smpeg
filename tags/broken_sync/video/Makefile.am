
noinst_LTLIBRARIES = libvideo.la

libvideo_la_SOURCES =		\
	MPEGvideo.cpp		\
	decoders.cpp		\
	decoders.h		\
	dither.h		\
	floatdct.cpp		\
	gdith.cpp		\
	jrevdct.cpp		\
	motionvector.cpp	\
	parseblock.cpp		\
	proto.h			\
	readfile.cpp		\
	util.cpp		\
	util.h			\
	video.cpp		\
	video.h			\
	mmxflags_asm.S		\
	mmxidct_asm.S

EXTRA_DIST =			\
	README			\
	COPYRIGHT

#16bit_mmx.o: 16bit_mmx.cpp
#	$(CXX) -c -o $@ $^ $(CXXFLAGS)
16bit_mmx.lo: 16bit_mmx.cpp
	$(CXX) -c -o $@ $^ $(CXXFLAGS)
#32bit_mmx.o: 32bit_mmx.cpp
#	$(CXX) -c -o $@ $^ $(CXXFLAGS)
32bit_mmx.lo: 32bit_mmx.cpp
	$(CXX) -c -o $@ $^ $(CXXFLAGS)

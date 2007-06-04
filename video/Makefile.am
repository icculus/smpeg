
noinst_LTLIBRARIES = libvideo.la

libvideo_la_SOURCES =		\
	MPEGvideo.c		\
	decoders.c		\
	decoders.h		\
	dither.h		\
	floatdct.c		\
	gdith.c		\
	jrevdct.c		\
	motionvector.c	\
	parseblock.c		\
	proto.h			\
	readfile.c		\
	util.c		\
	util.h			\
	video.c		\
	video.h			\
	vhar128.c		\
	vhar128.h		\
	mmxflags_asm.S		\
	mmxidct_asm.S

EXTRA_DIST =			\
	README			\
	COPYRIGHT

##16bit_mmx.o: 16bit_mmx.c
##	$(CXX) -c -o $@ $^ $(CXXFLAGS)
#16bit_mmx.lo: 16bit_mmx.c
#	$(CXX) -c -o $@ $^ $(CXXFLAGS)
##32bit_mmx.o: 32bit_mmx.c
##	$(CXX) -c -o $@ $^ $(CXXFLAGS)
#32bit_mmx.lo: 32bit_mmx.c
#	$(CXX) -c -o $@ $^ $(CXXFLAGS)

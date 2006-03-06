
## Makefile.am for the smpeg library

# These are the subdirectories that are always built
SUBDIRS = audio video

bin_SCRIPTS = smpeg-config

CCLD = $(CXXLD)

# The smpeg library target
lib_LTLIBRARIES = libsmpeg.la

libsmpeg_la_SOURCES =		\
	MPEG.cpp		\
	MPEGring.cpp		\
	MPEGlist.cpp		\
	MPEGstream.cpp		\
	MPEGsystem.cpp		\
	MPEGfilter.c		\
	smpeg.cpp

libsmpegincludedir = $(includedir)/smpeg
libsmpeginclude_HEADERS =	\
	MPEG.h			\
	MPEGaction.h		\
	MPEGaudio.h		\
	MPEGerror.h		\
	MPEGfilter.h		\
	MPEGring.h		\
	MPEGlist.h		\
	MPEGstream.h		\
	MPEGsystem.h		\
	MPEGvideo.h		\
	smpeg.h

libsmpeg_la_LDFLAGS = 		\
        -release $(LT_RELEASE)	\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

libsmpeg_la_LIBADD =		\
	audio/libaudio.la	\
	video/libvideo.la

EXTRA_DIST =		\
	CHANGES		\
	COPYING		\
	TODO		\
	README		\
	README.SDL_mixer \
	plaympeg.1	\
	gtv.1		\
	VisualC.zip	\
	smpeg.m4	\
	autogen.sh

# Sample MPEG players
if HAVE_GTK
GTK_PLAYER = gtv
else
GTK_PLAYER =
endif
if HAVE_OPENGL
OPENGL_PLAYER = glmovie
else
OPENGL_PLAYER =
endif
bin_PROGRAMS = plaympeg $(GTK_PLAYER) $(OPENGL_PLAYER)

# Sources for plaympeg
man_MANS = plaympeg.1 gtv.1
plaympeg_SOURCES = plaympeg.c
plaympeg_LDADD = libsmpeg.la

# Sources for gtv
gtv_SOURCES = gtv.c gtv.h
gtv_LDADD = @GTK_LIBS@ libsmpeg.la

# Sources for glmovie
glmovie_SOURCES = glmovie-tile.c glmovie.c glmovie.h
glmovie_LDADD = @GL_LIBS@ libsmpeg.la

# M4 macro file for inclusion with autoconf
m4datadir = $(datadir)/aclocal
m4data_DATA = smpeg.m4

# Rule to build tar-gzipped distribution package
$(PACKAGE)-$(VERSION).tar.gz: dist

# Rule to build RPM distribution package
rpm: $(PACKAGE)-$(VERSION).tar.gz
	cp $(PACKAGE)-$(VERSION).tar.gz /usr/src/redhat/SOURCES
	rpm -ba smpeg.spec


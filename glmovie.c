/* HACK
 * If you stick glx.h before MPEG.h, the preprocessor
 * will start replacing the MPEG methods Status with an
 * X11 variable type... blech.
 */
#include "smpeg.h"
#include <GL/glx.h>
#include <SDL/SDL.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>

#include "glmovie.h"

static void glmpeg_create_window( unsigned int, unsigned int );
static void glmpeg_update( SDL_Surface*, Sint32, Sint32, Uint32, Uint32 );

static Display* display = NULL;
static Window window = 0;

int main( int argc, char* argv[] )
{
    SMPEG* mpeg;
    SMPEG_Info mpeg_info;
    SDL_Surface* surface;
    GLuint window_height;
    GLuint window_width;

    if( argc < 2 ) {
	fprintf( stderr, "glmpeg-test: I need a file here, damnit!\n" );
	return 1;
    }

    display = XOpenDisplay( NULL );

    if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO ) < 0 ) {
	fprintf( stderr, "glmpeg-test: I couldn't initizlize SDL (shrug)\n" );
	return 1;
    }

    mpeg = SMPEG_new( argv[1], &mpeg_info, 1 );

    if( !mpeg ) {
	fprintf( stderr, "glmpeg-test: I'm not so sure about this %s file...\n", argv[1] );
	return 1;
    }

    window_width = glmovie_next_power_of_2( mpeg_info.width );
    window_height = glmovie_next_power_of_2( mpeg_info.height );
    glmpeg_create_window( 640, 480 );
    XMapWindow( display, window );

    /* Everything needs to be in RGB for GL, but needs to be 32-bit for SMPEG. */
    surface = SDL_AllocSurface( SDL_SWSURFACE,
				mpeg_info.width,
				mpeg_info.height,
				32,
				0x000000FF,
				0x0000FF00,
				0x00FF0000,
				0xFF000000 );

    if( !surface ) {
	fprintf( stderr, "glmpeg-test: I couldn't make a surface (boo hoo)\n" );
	exit( 1 );
    }

    /* *Initialize* with mpeg size. */
    if( glmovie_init( mpeg_info.width, mpeg_info.height ) != GL_NO_ERROR ) {
	fprintf( stderr, "glmpeg-test: glmovie_init() failed!\n" );
	exit( 1 );
    }

    /* *Resize* with window size. */
    glmovie_resize( 640, 480 );
    SMPEG_setdisplay( mpeg, surface, NULL, glmpeg_update );
    SMPEG_play( mpeg );

    while( SMPEG_status( mpeg ) == SMPEG_PLAYING ) {
	sleep( 1 );
    }

    glmovie_quit( );

    return 0;
}

static void glmpeg_update( SDL_Surface* surface, Sint32 x, Sint32 y, Uint32 w, Uint32 h )
{
    GLenum error;

    glmovie_draw( (GLubyte*) surface->pixels );

    error = glGetError( );

    if( error != GL_NO_ERROR ) {
	fprintf( stderr, "glmpeg-test: GL error: %s\n", gluErrorString( error ) );
	exit( 1 );
    }

    glXSwapBuffers( display, window );
}

static void glmpeg_create_window( unsigned int width, unsigned int height )
{
    int screen;
    Window root;
    int glx_attributes[] = { GLX_RGBA,
			     GLX_RED_SIZE, 1,
			     GLX_GREEN_SIZE, 1,
			     GLX_BLUE_SIZE, 1,
			     GLX_DOUBLEBUFFER,
			     None };
    XVisualInfo* visual_info;
    XSetWindowAttributes attributes;
    unsigned long mask;
    GLXContext context;

    screen = DefaultScreen( display );
    root = RootWindow( display, screen );
    visual_info = glXChooseVisual( display, screen, glx_attributes );

    if( !visual_info ) {
	fprintf( stderr, "glmpeg: could not allocate Visual, exiting\n" );
	exit( 1 );
    }

    attributes.background_pixel = 0;
    attributes.border_pixel = 0;
    attributes.colormap = XCreateColormap( display, root, visual_info->visual, AllocNone );
    attributes.event_mask = StructureNotifyMask | ExposureMask;
    mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

    window = XCreateWindow( display,
			    root,
			    0, 0,
			    width, height,
			    0,
			    visual_info->depth,
			    InputOutput,
			    visual_info->visual,
			    mask,
			    &attributes );

    context = glXCreateContext( display, visual_info, NULL, True );
    glXMakeCurrent( display, window, context );
}

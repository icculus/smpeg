/*
 * TODO:
 * None of the really hard stuff has been tackled, which is
 * setting up the objects so that they move across rows and
 * columns properly. Only the trivial 1x2 case has been
 * covered.
 *
 * The best way to do this will probably be to alter the
 * texture object so that it stores the proper x, y, skip_row
 * and skip_pixels values. Since you'll have to calculate most
 * of this during the initial structure creation anyway, you
 * might as well record it and just iterate linearly, rather
 * than nested i/j, inside the draw function.
 *
 * -michael
 */

#include "glmovie.h"
#include <malloc.h>
#include <string.h>

/* Some data is redundant at this stage. */
typedef struct glmovie_texture_t {
    GLuint id;           /* OpenGL texture id. */
    GLuint poly_width;   /* Quad width for tile. */
    GLuint poly_height;  /* Quad height for tile. */
    GLuint offset_x;     /* X offset into tile for movie. */
    GLuint offset_y;     /* Y offset into tile for movie. */
    GLuint movie_width;  /* Width of movie inside tile. */
    GLuint movie_height; /* Height of movie inside tile. */
    GLuint row;          /* Row number of tile in scheme. */
    GLuint col;          /* Column number of tile in scheme. */
} glmovie_texture;

/* Boy, is this not thread safe. */

/* Our evil maximum texture size. Boo 3Dfx! */
static GLuint max_texture_size = 256;

/* Keep this around for easy freeing later. */
static GLuint* texture_ids = NULL;
/* Our main data. */
static glmovie_texture* textures = NULL;
static GLuint num_texture_rows = 0;
static GLuint num_texture_cols = 0;
/* Width and height of all tiling. */
static GLuint tiled_width = 0;
static GLuint tiled_height = 0;
/* Width and height of entire movie. */
static GLuint movie_width = 0;
static GLuint movie_height = 0;

/*
 * Draw the frame data.
 *
 * Parameters:
 *    frame: Actual RGBA frame data
 */
void glmovie_draw( GLubyte* frame )
{
    glClear( GL_COLOR_BUFFER_BIT );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity( );

    if( num_texture_rows == 1 && num_texture_cols == 2 ) {
	glBindTexture( GL_TEXTURE_2D, textures[0].id );
	glPixelStorei( GL_UNPACK_SKIP_PIXELS, 0 );
	glPixelStorei( GL_UNPACK_ROW_LENGTH, movie_width );

	glTexSubImage2D( GL_TEXTURE_2D,
			 0,
			 textures[0].offset_x,
			 textures[0].offset_y,
			 textures[0].movie_width,
			 textures[0].movie_height,
			 GL_RGBA,
			 GL_UNSIGNED_BYTE,
			 frame );

	glBegin( GL_QUADS );
	glTexCoord2f( 0.0, 0.0 );
	glVertex2i( 0, 0 );
	glTexCoord2f( 0.0, 1.0 );
	glVertex2i( 0, textures[0].poly_height );
	glTexCoord2f( 1.0, 1.0 );
	glVertex2i( textures[0].poly_width, textures[0].poly_height );
	glTexCoord2f( 1.0, 0.0 );
	glVertex2i( textures[0].poly_width, 0 );
	glEnd( );

	glBindTexture( GL_TEXTURE_2D, textures[1].id );
	glPixelStorei( GL_UNPACK_SKIP_PIXELS, textures[1].movie_width );

	glTexSubImage2D( GL_TEXTURE_2D,
			 0,
			 textures[1].offset_x,
			 textures[1].offset_y,
			 textures[1].movie_width,
			 textures[1].movie_height,
			 GL_RGBA,
			 GL_UNSIGNED_BYTE,
			 frame );

	glBegin( GL_QUADS );
	glTexCoord2f( 0.0, 0.0 );
	glVertex2i( textures[0].poly_width, 0 );
	glTexCoord2f( 0.0, 1.0 );
	glVertex2i( textures[0].poly_width, textures[1].poly_height );
	glTexCoord2f( 1.0, 1.0 );
	glVertex2i( textures[0].poly_width + textures[1].poly_width, textures[1].poly_height );
	glTexCoord2f( 1.0, 0.0 );
	glVertex2i( textures[0].poly_width + textures[1].poly_width, 0 );
	glEnd( );
    } else {
	/* PENDING - implement me. */
	return;
    }

}

/*
 * Here we need to center the OpenGL viewport within the
 * window size that we are given.
 *
 * Parameters:
 *     width: Width of the window in pixels
 *     height: Height of the window in pixels
 */
void glmovie_resize( GLuint width, GLuint height )
{
    glViewport( ( width - tiled_width ) / 2, ( height - tiled_height ) / 2, tiled_width, tiled_height );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity( );
    gluOrtho2D( 0, tiled_width, tiled_height, 0 );
}

/*
 * Determines if a given value is a power of 2.
 *
 * Parameters:
 *     value: Value to query against
 * Retruns:
 *     GL_TRUE on success
 *     GL_FALSE on failure
 */
GLboolean glmovie_is_power_of_2( GLuint value )
{

    switch( value ) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
    case 32:
    case 64:
    case 128:
    case 256:
    case 512:
    case 1024:
    case 2048:
	return GL_TRUE;
    default:
	return GL_FALSE;
    }

}

/*
 * Calculates the next power of 2 given a particular value.
 * Useful for calculating proper texture sizes for non power-of-2
 * aligned texures.
 *
 * Parameters:
 *     seed: Value to begin from
 * Returns:
 *     Next power of 2 beginning from 'seed'
 */
GLuint glmovie_next_power_of_2( GLuint seed )
{
    GLuint i;

    for( i = 1; i < seed; i *= 2 ) { };

    return i;
}

/*
 * Initialize the movie player subsystem with the width and height
 * of the *movie data* (as opposed to the window).
 *
 * Parameters:
 *     width: Width of movie in pixels
 *     height: Height of movie in pixels
 * Returns:
 *     GL_NO_ERROR on success
 *     Any of the enumerated GL errors on failure
 */
GLenum glmovie_init( GLuint width, GLuint height )
{
    /* Initial black texels. */
    GLubyte* pixels;
    /* Absolute offsets from within tiled frame. */
    GLuint offset_x;
    GLuint offset_y;
    GLuint i;

    /* Save original movie dimensions. */
    movie_width = width;
    movie_height = height;

    /* Get the power of 2 dimensions. */
    if( !glmovie_is_power_of_2( width ) ) {
	tiled_width = glmovie_next_power_of_2( width );
	offset_x = ( tiled_width - width ) / 2;
    } else {
	tiled_width = width;
    }

    if( !glmovie_is_power_of_2( height ) ) {
	tiled_height = glmovie_next_power_of_2( height );
	offset_y = ( tiled_height - height ) / 2;
    } else {
	tiled_height = height;
    }

    /* Now break it up into quads. */
    num_texture_rows = tiled_height / max_texture_size;
    num_texture_cols = tiled_width / max_texture_size;

    /* Time for fun with data structures. */
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    glEnable( GL_TEXTURE_2D );
    texture_ids = (GLuint*) malloc( sizeof( GLuint ) * num_texture_rows * num_texture_cols );

    if( !texture_ids ) {
	return GL_OUT_OF_MEMORY;
    }

    glGenTextures( num_texture_rows * num_texture_cols, texture_ids );

    /* Special case optimization for clarity, etc. */
    if( num_texture_rows == 1 && num_texture_cols == 2 ) {
	textures = (glmovie_texture*) malloc( sizeof( glmovie_texture ) * 2 );

	if( !textures ) {
	    glDeleteTextures( num_texture_rows * num_texture_cols, texture_ids );
	    free( texture_ids );
	    return GL_OUT_OF_MEMORY;
	}

	/* Setup first texture. */
	textures[0].id = texture_ids[0];
	textures[0].poly_width = max_texture_size;
	textures[0].poly_height = max_texture_size;
	textures[0].offset_x = offset_x;
	textures[0].offset_y = offset_y;
	textures[0].movie_width = width / 2;
	textures[0].movie_height = height;
	textures[0].row = 0;
	textures[0].col = 0;
	/* Second. */
	textures[1].id = texture_ids[1];
	textures[1].poly_width = max_texture_size;
	textures[1].poly_height = max_texture_size;
	textures[1].offset_x = 0;
	textures[1].offset_y = offset_y;
	textures[1].movie_width = width / 2;
	textures[1].movie_height = height;
	textures[1].row = 0;
	textures[1].col = 1;

	for( i = 0; i < 2; ++i ) {
	    pixels = (GLubyte*) malloc( textures[i].poly_width * textures[i].poly_height * 4 );
	    memset( pixels, 0, textures[i].poly_width * textures[i].poly_height * 4 );

	    if( !pixels ) {
		glDeleteTextures( num_texture_rows * num_texture_cols, texture_ids );
		free( texture_ids );
		free( textures );
		return GL_OUT_OF_MEMORY;
	    }


	    /* Do all of our useful binding. */
	    glBindTexture( GL_TEXTURE_2D, textures[i].id );
	    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
	    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
	    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

	    /* Specify our 256x256 black texture. */
	    glTexImage2D( GL_TEXTURE_2D,
			  0,
			  GL_RGB,
			  textures[i].poly_width,
			  textures[i].poly_height,
			  0,
			  GL_RGBA,
			  GL_UNSIGNED_BYTE,
			  pixels );

	    free( pixels );
	}

    } else {
	/* PENDING - implement me. */
	return GL_INVALID_OPERATION;
    }

    /* Simple state setup at the end. */
    glClearColor( 0.0, 0.0, 0.0, 0.0 );

    return glGetError( );
}

/*
 * Free any resources associated with the movie player.
 */
void glmovie_quit( void )
{
    glDeleteTextures( num_texture_rows * num_texture_cols, texture_ids );
    free( texture_ids );
    free( textures );
}

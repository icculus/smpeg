/*
 * Copyright (c) 1995 The Regents of the University of California.
 * All rights reserved.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

/*
 * Copyright (c) 1995 Erik Corry
 * All rights reserved.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL ERIK CORRY BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
 * SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF
 * THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF ERIK CORRY HAS BEEN ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * ERIK CORRY SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
 * BASIS, AND ERIK CORRY HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT,
 * UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

/*
 * Portions of this software Copyright (c) 1995 Brown University.
 * All rights reserved.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement
 * is hereby granted, provided that the above copyright notice and the
 * following two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF BROWN
 * UNIVERSITY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * BROWN UNIVERSITY SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
 * BASIS, AND BROWN UNIVERSITY HAS NO OBLIGATION TO PROVIDE MAINTENANCE,
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#include "video.h"
#include "dither.h"
#include "proto.h"
#include <math.h>

/* Use special optimized routines for color conversion */
#define OPTIMIZE_CODE

/*
   Changes to make the code reentrant:
     None
   Additional changes:
     do not define INTERPOLATE, add #ifdef INTERPOLATE
   -lsh@cs.brown.edu (Loring Holden)
 */
/* #define INTERPOLATE */

/*
 * Erik Corry's multi-byte dither routines.
 *
 * The basic idea is that the Init generates all the necessary tables.
 * The tables incorporate the information about the layout of pixels
 * in the XImage, so that it should be able to cope with 15-bit, 16-bit
 * 24-bit (non-packed) and 32-bit (10-11 bits per color!) screens.
 * At present it cannot cope with 24-bit packed mode, since this involves
 * getting down to byte level again. It is assumed that the bits for each
 * color are contiguous in the longword.
 * 
 * Writing to memory is done in shorts or ints. (Unfortunately, short is not
 * very fast on Alpha, so there is room for improvement here). There is no
 * dither time check for overflow - instead the tables have slack at
 * each end. This is likely to be faster than an 'if' test as many modern
 * architectures are really bad at ifs. Potentially, each '&&' causes a 
 * pipeline flush!
 *
 * There is no shifting and fixed point arithmetic, as I really doubt you
 * can see the difference, and it costs. This may be just my bias, since I
 * heard that Intel is really bad at shifting.
 */

/*
 * How many 1 bits are there in the longword.
 * Low performance, do not call often.
 */
static int number_of_bits_set( unsigned long a )
{
    if(!a) return 0;
    if(a & 1) return 1 + number_of_bits_set(a >> 1);
    return(number_of_bits_set(a >> 1));
}

/*
 * Shift the 0s in the least significant end out of the longword.
 * Low performance, do not call often.
 */
static unsigned long shifted_down( unsigned long a )
{
    if(!a) return 0;
    if(a & 1) return a;
    return a >> 1;
}

/*
 * How many 0 bits are there at most significant end of longword.
 * Low performance, do not call often.
 */
static int free_bits_at_top( unsigned long a )
{
      /* assume char is 8 bits */
    if(!a) return sizeof(unsigned long) * 8;
        /* assume twos complement */
    if(((long)a) < 0l) return 0;
    return 1 + free_bits_at_top ( a << 1);
}

/*
 * How many 0 bits are there at least significant end of longword.
 * Low performance, do not call often.
 */
static int free_bits_at_bottom( unsigned long a )
{
      /* assume char is 8 bits */
    if(!a) return sizeof(unsigned long) * 8;
    if(((long)a) & 1l) return 0;
    return 1 + free_bits_at_bottom ( a >> 1);
}

static int *L_tab=NULL;
static int *colortab=NULL,
           *Cr_r_tab=NULL, *Cr_g_tab=NULL, *Cb_g_tab=NULL, *Cb_b_tab=NULL;

/*
 * We define tables that convert a color value between -256 and 512
 * into the R, G and B parts of the pixel. The normal range is 0-255.
 */

static long *r_2_pix=NULL;
static long *g_2_pix=NULL;
static long *b_2_pix=NULL;
static long *rgb_2_pix=NULL;
static long *r_2_pix_alloc=NULL;
static long *g_2_pix_alloc=NULL;
static long *b_2_pix_alloc=NULL;


/*
 *--------------------------------------------------------------
 *
 * InitColor16Dither --
 *
 *        To get rid of the multiply and other conversions in color
 *        dither, we use a lookup table.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        The lookup tables are initialized.
 *
 *--------------------------------------------------------------
 */

void InitColorDither( int bpp, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask )
{
    int L, CR, CB, i;
    int thirty2;

    if (L_tab==NULL)
       L_tab = (int *)malloc(256*sizeof(int)); 

#if 0  /* We can exploit cache by allocating contiguous blocks */
    if (Cr_r_tab==NULL)
       Cr_r_tab = (int *)malloc(256*sizeof(int));
    if (Cr_g_tab==NULL)
       Cr_g_tab = (int *)malloc(256*sizeof(int));
    if (Cb_g_tab==NULL)
       Cb_g_tab = (int *)malloc(256*sizeof(int));
    if (Cb_b_tab==NULL)
       Cb_b_tab = (int *)malloc(256*sizeof(int));

    if (r_2_pix_alloc==NULL)
       r_2_pix_alloc = (long *)malloc(768*sizeof(long));
    if (g_2_pix_alloc==NULL) 
       g_2_pix_alloc = (long *)malloc(768*sizeof(long));
    if (b_2_pix_alloc==NULL) 
       b_2_pix_alloc = (long *)malloc(768*sizeof(long));
#else
    if (colortab==NULL)
       colortab = (int *)malloc(4*256*sizeof(int));
    Cr_r_tab = &colortab[0*256];
    Cr_g_tab = &colortab[1*256];
    Cb_g_tab = &colortab[2*256];
    Cb_b_tab = &colortab[3*256];

    if (rgb_2_pix==NULL)
       rgb_2_pix = (long *)malloc(3*768*sizeof(int));
    r_2_pix_alloc = &rgb_2_pix[0*768];
    g_2_pix_alloc = &rgb_2_pix[1*768];
    b_2_pix_alloc = &rgb_2_pix[2*768];
#endif

    if (L_tab == NULL ||
        Cr_r_tab == NULL ||
        Cr_g_tab == NULL ||
        Cb_g_tab == NULL ||
        Cb_b_tab == NULL ||
        r_2_pix_alloc == NULL ||
        g_2_pix_alloc == NULL ||
        b_2_pix_alloc == NULL) {
      fprintf(stderr, "Could not get enough memory in InitColorDither\n");
      exit(1);
    }

    /* Set the 32-bpp flag */
    thirty2 = (bpp >= 24);

    for (i=0; i<256; i++) {
      L_tab[i] = i;
      if (gammaCorrectFlag) {
        L_tab[i] = GAMMA_CORRECTION(i);
      }
      
      CB = CR = i;

      if (chromaCorrectFlag) {
        CB -= 128; 
        CB = CHROMA_CORRECTION128(CB);
        CR -= 128;
        CR = CHROMA_CORRECTION128(CR);
      } else {
        CB -= 128; CR -= 128;
      }
/* was
      Cr_r_tab[i] =  1.596 * CR;
      Cr_g_tab[i] = -0.813 * CR;
      Cb_g_tab[i] = -0.391 * CB;   
      Cb_b_tab[i] =  2.018 * CB;
  but they were just messed up.
  Then was (_Video Deymstified_):
      Cr_r_tab[i] =  1.366 * CR;
      Cr_g_tab[i] = -0.700 * CR;
      Cb_g_tab[i] = -0.334 * CB;   
      Cb_b_tab[i] =  1.732 * CB;
  but really should be:
   (from ITU-R BT.470-2 System B, G and SMPTE 170M )
*/
      Cr_r_tab[i] = (int) ( (0.419/0.299) * CR);
      Cr_g_tab[i] = (int) (-(0.299/0.419) * CR);
      Cb_g_tab[i] = (int) (-(0.114/0.331) * CB); 
      Cb_b_tab[i] = (int) ( (0.587/0.331) * CB);

/*
  though you could argue for:
    SMPTE 240M
      Cr_r_tab[i] =  (0.445/0.212) * CR;
      Cr_g_tab[i] = -(0.212/0.445) * CR;
      Cb_g_tab[i] = -(0.087/0.384) * CB; 
      Cb_b_tab[i] =  (0.701/0.384) * CB;
    FCC 
      Cr_r_tab[i] =  (0.421/0.30) * CR;
      Cr_g_tab[i] = -(0.30/0.421) * CR;
      Cb_g_tab[i] = -(0.11/0.331) * CB; 
      Cb_b_tab[i] =  (0.59/0.331) * CB;
    ITU-R BT.709 
      Cr_r_tab[i] =  (0.454/0.2125) * CR;
      Cr_g_tab[i] = -(0.2125/0.454) * CR;
      Cb_g_tab[i] = -(0.0721/0.386) * CB; 
      Cb_b_tab[i] =  (0.7154/0.386) * CB;
*/
    }

    /* 
     * Set up entries 0-255 in rgb-to-pixel value tables.
     */
    for (i = 0; i < 256; i++) {
      r_2_pix_alloc[i + 256] = i >> (8 - number_of_bits_set(Rmask));
      r_2_pix_alloc[i + 256] <<= free_bits_at_bottom(Rmask);
      g_2_pix_alloc[i + 256] = i >> (8 - number_of_bits_set(Gmask));
      g_2_pix_alloc[i + 256] <<= free_bits_at_bottom(Gmask);
      b_2_pix_alloc[i + 256] = i >> (8 - number_of_bits_set(Bmask));
      b_2_pix_alloc[i + 256] <<= free_bits_at_bottom(Bmask);
      /*
       * If we have 16-bit output depth, then we double the value
       * in the top word. This means that we can write out both
       * pixels in the pixel doubling mode with one op. It is 
       * harmless in the normal case as storing a 32-bit value
       * through a short pointer will lose the top bits anyway.
       * A similar optimisation for Alpha for 64 bit has been
       * prepared for, but is not yet implemented.
       */
      if(!thirty2) {

        r_2_pix_alloc[i + 256] |= (r_2_pix_alloc[i + 256]) << 16;
        g_2_pix_alloc[i + 256] |= (g_2_pix_alloc[i + 256]) << 16;
        b_2_pix_alloc[i + 256] |= (b_2_pix_alloc[i + 256]) << 16;

      }
#ifdef SIXTYFOUR_BIT
      if(thirty2) {

        r_2_pix_alloc[i + 256] |= (r_2_pix_alloc[i + 256]) << 32;
        g_2_pix_alloc[i + 256] |= (g_2_pix_alloc[i + 256]) << 32;
        b_2_pix_alloc[i + 256] |= (b_2_pix_alloc[i + 256]) << 32;

      }
#endif
    }

    /*
     * Spread out the values we have to the rest of the array so that
     * we do not need to check for overflow.
     */
    for (i = 0; i < 256; i++) {
      r_2_pix_alloc[i] = r_2_pix_alloc[256];
      r_2_pix_alloc[i+ 512] = r_2_pix_alloc[511];
      g_2_pix_alloc[i] = g_2_pix_alloc[256];
      g_2_pix_alloc[i+ 512] = g_2_pix_alloc[511];
      b_2_pix_alloc[i] = b_2_pix_alloc[256];
      b_2_pix_alloc[i+ 512] = b_2_pix_alloc[511];
    }

    r_2_pix = r_2_pix_alloc + 256;
    g_2_pix = g_2_pix_alloc + 256;
    b_2_pix = b_2_pix_alloc + 256;

}


/*
 * Profiling results:
 *  This function takes about 5ms per call, and is called once per
 *  frame, taking about 30% of the total time used by playback.
 *
 *--------------------------------------------------------------
 */
void Color16DitherImageMod( unsigned char *lum, unsigned char *cr,
                         unsigned char *cb, unsigned char *out,
                         int rows, int cols, int mod )
{
    unsigned short* row1;
    unsigned short* row2;
    unsigned char* lum2;
    int x, y;
    int cr_r;
    int crb_g;
    int cb_b;
    int cols_2 = cols / 2;

    row1 = (unsigned short*) out;
    row2 = row1 + cols + mod;
    lum2 = lum + cols;

    mod += cols + mod;

    y = rows / 2;
    while( y-- )
    {
        x = cols_2;
        while( x-- )
        {
            register int L;

            cr_r   = 0*768+256 + colortab[ cr[0] + 0*256 ];
            crb_g  = 1*768+256 + colortab[ cr[0] + 1*256 ]
                               + colortab[ cb[0] + 2*256 ];
            cb_b   = 2*768+256 + colortab[ cb[0] + 3*256 ];
            ++cr; ++cb;

            L = L_tab[ (int) *lum++ ];
            *row1++ = (rgb_2_pix[ L + cr_r ] |
                       rgb_2_pix[ L + crb_g ] |
                       rgb_2_pix[ L + cb_b ]);

            L = L_tab[ (int) *lum++ ];
            *row1++ = (rgb_2_pix[ L + cr_r ] |
                       rgb_2_pix[ L + crb_g ] |
                       rgb_2_pix[ L + cb_b ]);


            /* Now, do second row.  */

            L = L_tab[ (int) *lum2++ ];
            *row2++ = (rgb_2_pix[ L + cr_r ] |
                       rgb_2_pix[ L + crb_g ] |
                       rgb_2_pix[ L + cb_b ]);

            L = L_tab[ (int) *lum2++ ];
            *row2++ = (rgb_2_pix[ L + cr_r ] |
                       rgb_2_pix[ L + crb_g ] |
                       rgb_2_pix[ L + cb_b ]);
        }

        /*
         * These values are at the start of the next line, (due
         * to the ++'s above),but they need to be at the start
         * of the line after that.
         */
        lum  += cols;
        lum2 += cols;
        row1 += mod;
        row2 += mod;
    }
}

void Color32DitherImageMod( unsigned char *lum, unsigned char *cr,
                         unsigned char *cb, unsigned char *out,
                         int rows, int cols, int mod )
{
    unsigned int* row1;
    unsigned int* row2;
    unsigned char* lum2;
    int x, y;
    int cr_r;
    int crb_g;
    int cb_b;
    int cols_2 = cols / 2;

    row1 = (unsigned int*) out;
    row2 = row1 + cols + mod;
    lum2 = lum + cols;

    mod += cols + mod;

    y = rows / 2;
    while( y-- )
    {
        x = cols_2;
        while( x-- )
        {
            register int L;

            cr_r   = 0*768+256 + colortab[ cr[0] + 0*256 ];
            crb_g  = 1*768+256 + colortab[ cr[0] + 1*256 ]
                               + colortab[ cb[0] + 2*256 ];
            cb_b   = 2*768+256 + colortab[ cb[0] + 3*256 ];
            ++cr; ++cb;

            L = L_tab[ (int) *lum++ ];
            *row1++ = (rgb_2_pix[ L + cr_r ] |
                       rgb_2_pix[ L + crb_g ] |
                       rgb_2_pix[ L + cb_b ]);

            L = L_tab[ (int) *lum++ ];
            *row1++ = (rgb_2_pix[ L + cr_r ] |
                       rgb_2_pix[ L + crb_g ] |
                       rgb_2_pix[ L + cb_b ]);


            /* Now, do second row.  */

            L = L_tab[ (int) *lum2++ ];
            *row2++ = (rgb_2_pix[ L + cr_r ] |
                       rgb_2_pix[ L + crb_g ] |
                       rgb_2_pix[ L + cb_b ]);

            L = L_tab[ (int) *lum2++ ];
            *row2++ = (rgb_2_pix[ L + cr_r ] |
                       rgb_2_pix[ L + crb_g ] |
                       rgb_2_pix[ L + cb_b ]);
        }

        /*
         * These values are at the start of the next line, (due
         * to the ++'s above),but they need to be at the start
         * of the line after that.
         */
        lum  += cols;
        lum2 += cols;
        row1 += mod;
        row2 += mod;
    }
}

/* This allows interlaced video display
   - Actually the current implementation only renders even scanlines since
     rendering alternating even and odd scanlines gives terrible results.
   - This is made a compile-time option because it doesn't increase the
     framerate very much.  Optimization is better done in stream parsing.
 */
#ifdef USE_INTERLACED_VIDEO
void Color16DitherImageModInterlace( unsigned char *lum, unsigned char *cr,
                                     unsigned char *cb, unsigned char *out,
                                     int rows, int cols, int mod, int start )
{
    unsigned short* row1;
    unsigned short* row2;
    unsigned char* lum2;
    int x, y;
    int cr_r;
    int crb_g;
    int cb_b;
    int cols_2 = cols / 2;

    row1 = (unsigned short*) out;
    // Uncomment this to enable even-odd scanline rendering (looks terrible)
    //row1 += start * (cols + mod);
    //lum += start * cols;
    row2 = row1 + 2*(cols + mod);
    lum2 = lum + 2*cols;

    mod += cols + mod;

    y = ((rows-2) / 2);
    while( y-- )
    {
        x = cols_2;
        while( x-- )
        {
            register int L;

            cr_r   = 0*768+256 + colortab[ cr[0] + 0*256 ];
            crb_g  = 1*768+256 + colortab[ cr[0] + 1*256 ]
                               + colortab[ cb[0] + 2*256 ];
            cb_b   = 2*768+256 + colortab[ cb[0] + 3*256 ];
            ++cr; ++cb;

            L = L_tab[ (int) *lum++ ];
            *row1++ = (rgb_2_pix[ L + cr_r ] |
                       rgb_2_pix[ L + crb_g ] |
                       rgb_2_pix[ L + cb_b ]);

            L = L_tab[ (int) *lum++ ];
            *row1++ = (rgb_2_pix[ L + cr_r ] |
                       rgb_2_pix[ L + crb_g ] |
                       rgb_2_pix[ L + cb_b ]);


            /* Now, do second row.  */

            L = L_tab[ (int) *lum2++ ];
            *row2++ = (rgb_2_pix[ L + cr_r ] |
                       rgb_2_pix[ L + crb_g ] |
                       rgb_2_pix[ L + cb_b ]);

            L = L_tab[ (int) *lum2++ ];
            *row2++ = (rgb_2_pix[ L + cr_r ] |
                       rgb_2_pix[ L + crb_g ] |
                       rgb_2_pix[ L + cb_b ]);
        }

        /*
         * These values are at the start of the next line, (due
         * to the ++'s above),but they need to be at the start
         * of the line after that.
         */
        lum  += cols;
        lum2 += cols;
        row1 += mod;
        row2 += mod;
    }
}
#endif /* USE_INTERLACED_VIDEO */


/*
 * Erik Corry's pixel doubling routines for 15/16/24/32 bit screens.
 */


/*
 *--------------------------------------------------------------
 *
 * Twox2Color16DitherImage --
 *
 *        Converts image into 16 bit color at double size.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        None.
 *
 * Profiling results:
 *  This function takes about 10ms per call, and is called once per
 *  frame, taking about 40% of the total time used by playback.
 *
 *--------------------------------------------------------------
 */

/*
 * In this function I make use of a nasty trick. The tables have the lower
 * 16 bits replicated in the upper 16. This means I can write ints and get
 * the horisontal doubling for free (almost).
 */

void Twox2Color16DitherImageMod( unsigned char *lum, unsigned char *cr,
                                 unsigned char *cb, unsigned char *out,
                                 int rows, int cols, int mod )
{
    unsigned int* row1 = (unsigned int*) out;
    const int next_row = cols+(mod/2);
    unsigned int* row2 = row1 + 2*next_row;
    unsigned char* lum2;
    int x, y;
    int cr_r;
    int crb_g;
    int cb_b;
    int cols_2 = cols / 2;

    lum2 = lum + cols;

    mod = (next_row * 3) + (mod/2);

    y = rows / 2;
    while( y-- )
    {
        x = cols_2;
        while( x-- )
        {
            register int L;

            cr_r   = 0*768+256 + colortab[ cr[0] + 0*256 ];
            crb_g  = 1*768+256 + colortab[ cr[0] + 1*256 ]
                               + colortab[ cb[0] + 2*256 ];
            cb_b   = 2*768+256 + colortab[ cb[0] + 3*256 ];
            ++cr; ++cb;

            L = L_tab[ (int) *lum++ ];
            row1[0] = row1[next_row] = (rgb_2_pix[ L + cr_r ] |
                                        rgb_2_pix[ L + crb_g ] |
                                        rgb_2_pix[ L + cb_b ]);
            row1++;

            L = L_tab[ (int) *lum++ ];
            row1[0] = row1[next_row] = (rgb_2_pix[ L + cr_r ] |
                                        rgb_2_pix[ L + crb_g ] |
                                        rgb_2_pix[ L + cb_b ]);
            row1++;


            /* Now, do second row. */

            L = L_tab[ (int) *lum2++ ];
            row2[0] = row2[next_row] = (rgb_2_pix[ L + cr_r ] |
                                        rgb_2_pix[ L + crb_g ] |
                                        rgb_2_pix[ L + cb_b ]);
            row2++;

            L = L_tab[ (int) *lum2++ ];
            row2[0] = row2[next_row] = (rgb_2_pix[ L + cr_r ] |
                                        rgb_2_pix[ L + crb_g ] |
                                        rgb_2_pix[ L + cb_b ]);
            row2++;
        }

        /*
         * These values are at the start of the next line, (due
         * to the ++'s above),but they need to be at the start
         * of the line after that.
         */
        lum  += cols;
        lum2 += cols;
        row1 += mod;
        row2 += mod;
    }
}

void Twox2Color32DitherImageMod( unsigned char *lum, unsigned char *cr,
                                 unsigned char *cb, unsigned char *out,
                                 int rows, int cols, int mod )
{
    unsigned int* row1 = (unsigned int*) out;
    const int next_row = cols*2+mod;
    unsigned int* row2 = row1 + 2*next_row;
    unsigned char* lum2;
    int x, y;
    int cr_r;
    int crb_g;
    int cb_b;
    int cols_2 = cols / 2;

    lum2 = lum + cols;

    mod = (next_row * 3) + mod;

    y = rows / 2;
    while( y-- )
    {
        x = cols_2;
        while( x-- )
        {
            register int L;

            cr_r   = 0*768+256 + colortab[ cr[0] + 0*256 ];
            crb_g  = 1*768+256 + colortab[ cr[0] + 1*256 ]
                               + colortab[ cb[0] + 2*256 ];
            cb_b   = 2*768+256 + colortab[ cb[0] + 3*256 ];
            ++cr; ++cb;

            L = L_tab[ (int) *lum++ ];
            row1[0] = row1[1] = row1[next_row] = row1[next_row+1] =
                                       (rgb_2_pix[ L + cr_r ] |
                                        rgb_2_pix[ L + crb_g ] |
                                        rgb_2_pix[ L + cb_b ]);
            row1 += 2;

            L = L_tab[ (int) *lum++ ];
            row1[0] = row1[1] = row1[next_row] = row1[next_row+1] =
                                       (rgb_2_pix[ L + cr_r ] |
                                        rgb_2_pix[ L + crb_g ] |
                                        rgb_2_pix[ L + cb_b ]);
            row1 += 2;


            /* Now, do second row. */

            L = L_tab[ (int) *lum2++ ];
            row2[0] = row2[1] = row2[next_row] = row2[next_row+1] =
                                       (rgb_2_pix[ L + cr_r ] |
                                        rgb_2_pix[ L + crb_g ] |
                                        rgb_2_pix[ L + cb_b ]);
            row2 += 2;

            L = L_tab[ (int) *lum2++ ];
            row2[0] = row2[1] = row2[next_row] = row2[next_row+1] =
                                       (rgb_2_pix[ L + cr_r ] |
                                        rgb_2_pix[ L + crb_g ] |
                                        rgb_2_pix[ L + cb_b ]);
            row2 += 2;
        }

        /*
         * These values are at the start of the next line, (due
         * to the ++'s above),but they need to be at the start
         * of the line after that.
         */
        lum  += cols;
        lum2 += cols;
        row1 += mod;
        row2 += mod;
    }
}

/* This allows interlaced video display
   - Actually the current implementation only renders even scanlines since
     rendering alternating even and odd scanlines gives terrible results.
   - This is made a compile-time option because it doesn't increase the
     framerate very much.  Optimization is better done in stream parsing.
 */
#ifdef USE_INTERLACED_VIDEO
void Twox2Color16DitherImageModInterlace( unsigned char *lum, unsigned char *cr,
                                 unsigned char *cb, unsigned char *out,
                                 int rows, int cols, int mod, int start )
{
    unsigned long* row1;
    const int next_row = cols+(mod/2);
    unsigned long* row2;
    unsigned char* lum2;
    int x, y;
    int cr_r;
    int crb_g;
    int cb_b;
    int cols_2 = cols / 2;

    row1 = (unsigned long*) out;
    // Uncomment this to enable even-odd scanline rendering (looks terrible)
    //row1 += 2*start * next_row;
    //lum += start * cols;
    row2 = row1 + 4*next_row;
    lum2 = lum + 2*cols;

    mod = ((cols + (mod/2)) * 3) + (mod/2);

    y = ((rows-2) / 2);
    while( y-- )
    {
        x = cols_2;
        while( x-- )
        {
            register int L;

            cr_r   = 0*768+256 + colortab[ cr[0] + 0*256 ];
            crb_g  = 1*768+256 + colortab[ cr[0] + 1*256 ]
                               + colortab[ cb[0] + 2*256 ];
            cb_b   = 2*768+256 + colortab[ cb[0] + 3*256 ];
            ++cr; ++cb;

            L = L_tab[ (int) *lum++ ];
            row1[0] = row1[next_row] = (rgb_2_pix[ L + cr_r ] |
                                        rgb_2_pix[ L + crb_g ] |
                                        rgb_2_pix[ L + cb_b ]);
            row1++;

            L = L_tab[ (int) *lum++ ];
            row1[0] = row1[next_row] = (rgb_2_pix[ L + cr_r ] |
                                        rgb_2_pix[ L + crb_g ] |
                                        rgb_2_pix[ L + cb_b ]);
            row1++;


            /* Now, do second row. */

            L = L_tab[ (int) *lum2++ ];
            row2[0] = row2[next_row] = (rgb_2_pix[ L + cr_r ] |
                                        rgb_2_pix[ L + crb_g ] |
                                        rgb_2_pix[ L + cb_b ]);
            row2++;

            L = L_tab[ (int) *lum2++ ];
            row2[0] = row2[next_row] = (rgb_2_pix[ L + cr_r ] |
                                        rgb_2_pix[ L + crb_g ] |
                                        rgb_2_pix[ L + cb_b ]);
            row2++;
        }

        /*
         * These values are at the start of the next line, (due
         * to the ++'s above),but they need to be at the start
         * of the line after that.
         */
        lum  += cols;
        lum2 += cols;
        row1 += mod;
        row2 += mod;
    }
}
#endif /* USE_INTERLACED_VIDEO */

/* EOF */

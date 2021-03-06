/* MPEG/WAVE Sound library

   (C) 1997 by Jung woo-jae */

// Mpeglayer2.cc
// It's for MPEG Layer 2

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#if defined(_WIN32) && defined(_MSC_VER)
// disable warnings about double to float conversions
#pragma warning(disable: 4244 4305)
#endif

#include "MPEGaudio.h"

#define MAXTABLE 2

// Tables for layer 2
static const int bitalloclengthtable[MAXTABLE][MAXSUBBAND]=
{{4,4,3,3,3,3,3,3,3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3},
 {4,4,4,4,4,4,4,4,4,4,4,3,3,3,3,3, 3,3,3,3,3,3,3,2,2,2,2,2,2,2,2,2}};

static const REAL group5bits[27*3]=
{
  -2.0/3.0, -2.0/3.0, -2.0/3.0,
       0.0, -2.0/3.0, -2.0/3.0,
   2.0/3.0, -2.0/3.0, -2.0/3.0,
  -2.0/3.0,      0.0, -2.0/3.0,
       0.0,      0.0, -2.0/3.0,
   2.0/3.0,      0.0, -2.0/3.0,
  -2.0/3.0,  2.0/3.0, -2.0/3.0,
       0.0,  2.0/3.0, -2.0/3.0,
   2.0/3.0,  2.0/3.0, -2.0/3.0,
  -2.0/3.0, -2.0/3.0,      0.0,
       0.0, -2.0/3.0,      0.0,
   2.0/3.0, -2.0/3.0,      0.0,
  -2.0/3.0,      0.0,      0.0,
       0.0,      0.0,      0.0,
   2.0/3.0,      0.0,      0.0,
  -2.0/3.0,  2.0/3.0,      0.0,
       0.0,  2.0/3.0,      0.0,
   2.0/3.0,  2.0/3.0,      0.0,
  -2.0/3.0, -2.0/3.0,  2.0/3.0,
       0.0, -2.0/3.0,  2.0/3.0,
   2.0/3.0, -2.0/3.0,  2.0/3.0,
  -2.0/3.0,      0.0,  2.0/3.0,
       0.0,      0.0,  2.0/3.0,
   2.0/3.0,      0.0,  2.0/3.0,
  -2.0/3.0,  2.0/3.0,  2.0/3.0,
       0.0,  2.0/3.0,  2.0/3.0,
   2.0/3.0,  2.0/3.0,  2.0/3.0
};

static const REAL group7bits[125*3]=
{
 -0.8,-0.8,-0.8, -0.4,-0.8,-0.8, 0.0,-0.8,-0.8, 0.4,-0.8,-0.8, 0.8,-0.8,-0.8,
 -0.8,-0.4,-0.8, -0.4,-0.4,-0.8, 0.0,-0.4,-0.8, 0.4,-0.4,-0.8, 0.8,-0.4,-0.8,
 -0.8, 0.0,-0.8, -0.4, 0.0,-0.8, 0.0, 0.0,-0.8, 0.4, 0.0,-0.8, 0.8, 0.0,-0.8,
 -0.8, 0.4,-0.8, -0.4, 0.4,-0.8, 0.0, 0.4,-0.8, 0.4, 0.4,-0.8, 0.8, 0.4,-0.8,
 -0.8, 0.8,-0.8, -0.4, 0.8,-0.8, 0.0, 0.8,-0.8, 0.4, 0.8,-0.8, 0.8, 0.8,-0.8,
 -0.8,-0.8,-0.4, -0.4,-0.8,-0.4, 0.0,-0.8,-0.4, 0.4,-0.8,-0.4, 0.8,-0.8,-0.4,
 -0.8,-0.4,-0.4, -0.4,-0.4,-0.4, 0.0,-0.4,-0.4, 0.4,-0.4,-0.4, 0.8,-0.4,-0.4,
 -0.8, 0.0,-0.4, -0.4, 0.0,-0.4, 0.0, 0.0,-0.4, 0.4, 0.0,-0.4, 0.8, 0.0,-0.4,
 -0.8, 0.4,-0.4, -0.4, 0.4,-0.4, 0.0, 0.4,-0.4, 0.4, 0.4,-0.4, 0.8, 0.4,-0.4,
 -0.8, 0.8,-0.4, -0.4, 0.8,-0.4, 0.0, 0.8,-0.4, 0.4, 0.8,-0.4, 0.8, 0.8,-0.4,
 -0.8,-0.8, 0.0, -0.4,-0.8, 0.0, 0.0,-0.8, 0.0, 0.4,-0.8, 0.0, 0.8,-0.8, 0.0,
 -0.8,-0.4, 0.0, -0.4,-0.4, 0.0, 0.0,-0.4, 0.0, 0.4,-0.4, 0.0, 0.8,-0.4, 0.0,
 -0.8, 0.0, 0.0, -0.4, 0.0, 0.0, 0.0, 0.0, 0.0, 0.4, 0.0, 0.0, 0.8, 0.0, 0.0,
 -0.8, 0.4, 0.0, -0.4, 0.4, 0.0, 0.0, 0.4, 0.0, 0.4, 0.4, 0.0, 0.8, 0.4, 0.0,
 -0.8, 0.8, 0.0, -0.4, 0.8, 0.0, 0.0, 0.8, 0.0, 0.4, 0.8, 0.0, 0.8, 0.8, 0.0,
 -0.8,-0.8, 0.4, -0.4,-0.8, 0.4, 0.0,-0.8, 0.4, 0.4,-0.8, 0.4, 0.8,-0.8, 0.4,
 -0.8,-0.4, 0.4, -0.4,-0.4, 0.4, 0.0,-0.4, 0.4, 0.4,-0.4, 0.4, 0.8,-0.4, 0.4,
 -0.8, 0.0, 0.4, -0.4, 0.0, 0.4, 0.0, 0.0, 0.4, 0.4, 0.0, 0.4, 0.8, 0.0, 0.4,
 -0.8, 0.4, 0.4, -0.4, 0.4, 0.4, 0.0, 0.4, 0.4, 0.4, 0.4, 0.4, 0.8, 0.4, 0.4,
 -0.8, 0.8, 0.4, -0.4, 0.8, 0.4, 0.0, 0.8, 0.4, 0.4, 0.8, 0.4, 0.8, 0.8, 0.4,
 -0.8,-0.8, 0.8, -0.4,-0.8, 0.8, 0.0,-0.8, 0.8, 0.4,-0.8, 0.8, 0.8,-0.8, 0.8,
 -0.8,-0.4, 0.8, -0.4,-0.4, 0.8, 0.0,-0.4, 0.8, 0.4,-0.4, 0.8, 0.8,-0.4, 0.8,
 -0.8, 0.0, 0.8, -0.4, 0.0, 0.8, 0.0, 0.0, 0.8, 0.4, 0.0, 0.8, 0.8, 0.0, 0.8,
 -0.8, 0.4, 0.8, -0.4, 0.4, 0.8, 0.0, 0.4, 0.8, 0.4, 0.4, 0.8, 0.8, 0.4, 0.8,
 -0.8, 0.8, 0.8, -0.4, 0.8, 0.8, 0.0, 0.8, 0.8, 0.4, 0.8, 0.8, 0.8, 0.8, 0.8
};

static const REAL group10bits[729*3]=
{
 -8.0/9.0,-8.0/9.0,-8.0/9.0, -6.0/9.0,-8.0/9.0,-8.0/9.0, -4.0/9.0,-8.0/9.0,-8.0/9.0,
 -2.0/9.0,-8.0/9.0,-8.0/9.0,      0.0,-8.0/9.0,-8.0/9.0,  2.0/9.0,-8.0/9.0,-8.0/9.0,
  4.0/9.0,-8.0/9.0,-8.0/9.0,  6.0/9.0,-8.0/9.0,-8.0/9.0,  8.0/9.0,-8.0/9.0,-8.0/9.0,
 -8.0/9.0,-6.0/9.0,-8.0/9.0, -6.0/9.0,-6.0/9.0,-8.0/9.0, -4.0/9.0,-6.0/9.0,-8.0/9.0,
 -2.0/9.0,-6.0/9.0,-8.0/9.0,      0.0,-6.0/9.0,-8.0/9.0,  2.0/9.0,-6.0/9.0,-8.0/9.0,
  4.0/9.0,-6.0/9.0,-8.0/9.0,  6.0/9.0,-6.0/9.0,-8.0/9.0,  8.0/9.0,-6.0/9.0,-8.0/9.0,
 -8.0/9.0,-4.0/9.0,-8.0/9.0, -6.0/9.0,-4.0/9.0,-8.0/9.0, -4.0/9.0,-4.0/9.0,-8.0/9.0,
 -2.0/9.0,-4.0/9.0,-8.0/9.0,      0.0,-4.0/9.0,-8.0/9.0,  2.0/9.0,-4.0/9.0,-8.0/9.0,
  4.0/9.0,-4.0/9.0,-8.0/9.0,  6.0/9.0,-4.0/9.0,-8.0/9.0,  8.0/9.0,-4.0/9.0,-8.0/9.0,
 -8.0/9.0,-2.0/9.0,-8.0/9.0, -6.0/9.0,-2.0/9.0,-8.0/9.0, -4.0/9.0,-2.0/9.0,-8.0/9.0,
 -2.0/9.0,-2.0/9.0,-8.0/9.0,      0.0,-2.0/9.0,-8.0/9.0,  2.0/9.0,-2.0/9.0,-8.0/9.0,
  4.0/9.0,-2.0/9.0,-8.0/9.0,  6.0/9.0,-2.0/9.0,-8.0/9.0,  8.0/9.0,-2.0/9.0,-8.0/9.0,
 -8.0/9.0,     0.0,-8.0/9.0, -6.0/9.0,     0.0,-8.0/9.0, -4.0/9.0,     0.0,-8.0/9.0,
 -2.0/9.0,     0.0,-8.0/9.0,      0.0,     0.0,-8.0/9.0,  2.0/9.0,     0.0,-8.0/9.0,
  4.0/9.0,     0.0,-8.0/9.0,  6.0/9.0,     0.0,-8.0/9.0,  8.0/9.0,     0.0,-8.0/9.0,
 -8.0/9.0, 2.0/9.0,-8.0/9.0, -6.0/9.0, 2.0/9.0,-8.0/9.0, -4.0/9.0, 2.0/9.0,-8.0/9.0,
 -2.0/9.0, 2.0/9.0,-8.0/9.0,      0.0, 2.0/9.0,-8.0/9.0,  2.0/9.0, 2.0/9.0,-8.0/9.0,
  4.0/9.0, 2.0/9.0,-8.0/9.0,  6.0/9.0, 2.0/9.0,-8.0/9.0,  8.0/9.0, 2.0/9.0,-8.0/9.0,
 -8.0/9.0, 4.0/9.0,-8.0/9.0, -6.0/9.0, 4.0/9.0,-8.0/9.0, -4.0/9.0, 4.0/9.0,-8.0/9.0,
 -2.0/9.0, 4.0/9.0,-8.0/9.0,      0.0, 4.0/9.0,-8.0/9.0,  2.0/9.0, 4.0/9.0,-8.0/9.0,
  4.0/9.0, 4.0/9.0,-8.0/9.0,  6.0/9.0, 4.0/9.0,-8.0/9.0,  8.0/9.0, 4.0/9.0,-8.0/9.0,
 -8.0/9.0, 6.0/9.0,-8.0/9.0, -6.0/9.0, 6.0/9.0,-8.0/9.0, -4.0/9.0, 6.0/9.0,-8.0/9.0,
 -2.0/9.0, 6.0/9.0,-8.0/9.0,      0.0, 6.0/9.0,-8.0/9.0,  2.0/9.0, 6.0/9.0,-8.0/9.0,
  4.0/9.0, 6.0/9.0,-8.0/9.0,  6.0/9.0, 6.0/9.0,-8.0/9.0,  8.0/9.0, 6.0/9.0,-8.0/9.0,
 -8.0/9.0, 8.0/9.0,-8.0/9.0, -6.0/9.0, 8.0/9.0,-8.0/9.0, -4.0/9.0, 8.0/9.0,-8.0/9.0,
 -2.0/9.0, 8.0/9.0,-8.0/9.0,      0.0, 8.0/9.0,-8.0/9.0,  2.0/9.0, 8.0/9.0,-8.0/9.0,
  4.0/9.0, 8.0/9.0,-8.0/9.0,  6.0/9.0, 8.0/9.0,-8.0/9.0,  8.0/9.0, 8.0/9.0,-8.0/9.0,
 -8.0/9.0,-8.0/9.0,-6.0/9.0, -6.0/9.0,-8.0/9.0,-6.0/9.0, -4.0/9.0,-8.0/9.0,-6.0/9.0,
 -2.0/9.0,-8.0/9.0,-6.0/9.0,      0.0,-8.0/9.0,-6.0/9.0,  2.0/9.0,-8.0/9.0,-6.0/9.0,
  4.0/9.0,-8.0/9.0,-6.0/9.0,  6.0/9.0,-8.0/9.0,-6.0/9.0,  8.0/9.0,-8.0/9.0,-6.0/9.0,
 -8.0/9.0,-6.0/9.0,-6.0/9.0, -6.0/9.0,-6.0/9.0,-6.0/9.0, -4.0/9.0,-6.0/9.0,-6.0/9.0,
 -2.0/9.0,-6.0/9.0,-6.0/9.0,      0.0,-6.0/9.0,-6.0/9.0,  2.0/9.0,-6.0/9.0,-6.0/9.0,
  4.0/9.0,-6.0/9.0,-6.0/9.0,  6.0/9.0,-6.0/9.0,-6.0/9.0,  8.0/9.0,-6.0/9.0,-6.0/9.0,
 -8.0/9.0,-4.0/9.0,-6.0/9.0, -6.0/9.0,-4.0/9.0,-6.0/9.0, -4.0/9.0,-4.0/9.0,-6.0/9.0,
 -2.0/9.0,-4.0/9.0,-6.0/9.0,      0.0,-4.0/9.0,-6.0/9.0,  2.0/9.0,-4.0/9.0,-6.0/9.0,
  4.0/9.0,-4.0/9.0,-6.0/9.0,  6.0/9.0,-4.0/9.0,-6.0/9.0,  8.0/9.0,-4.0/9.0,-6.0/9.0,
 -8.0/9.0,-2.0/9.0,-6.0/9.0, -6.0/9.0,-2.0/9.0,-6.0/9.0, -4.0/9.0,-2.0/9.0,-6.0/9.0,
 -2.0/9.0,-2.0/9.0,-6.0/9.0,      0.0,-2.0/9.0,-6.0/9.0,  2.0/9.0,-2.0/9.0,-6.0/9.0,
  4.0/9.0,-2.0/9.0,-6.0/9.0,  6.0/9.0,-2.0/9.0,-6.0/9.0,  8.0/9.0,-2.0/9.0,-6.0/9.0,
 -8.0/9.0,     0.0,-6.0/9.0, -6.0/9.0,     0.0,-6.0/9.0, -4.0/9.0,     0.0,-6.0/9.0,
 -2.0/9.0,     0.0,-6.0/9.0,      0.0,     0.0,-6.0/9.0,  2.0/9.0,     0.0,-6.0/9.0,
  4.0/9.0,     0.0,-6.0/9.0,  6.0/9.0,     0.0,-6.0/9.0,  8.0/9.0,     0.0,-6.0/9.0,
 -8.0/9.0, 2.0/9.0,-6.0/9.0, -6.0/9.0, 2.0/9.0,-6.0/9.0, -4.0/9.0, 2.0/9.0,-6.0/9.0,
 -2.0/9.0, 2.0/9.0,-6.0/9.0,      0.0, 2.0/9.0,-6.0/9.0,  2.0/9.0, 2.0/9.0,-6.0/9.0,
  4.0/9.0, 2.0/9.0,-6.0/9.0,  6.0/9.0, 2.0/9.0,-6.0/9.0,  8.0/9.0, 2.0/9.0,-6.0/9.0,
 -8.0/9.0, 4.0/9.0,-6.0/9.0, -6.0/9.0, 4.0/9.0,-6.0/9.0, -4.0/9.0, 4.0/9.0,-6.0/9.0,
 -2.0/9.0, 4.0/9.0,-6.0/9.0,      0.0, 4.0/9.0,-6.0/9.0,  2.0/9.0, 4.0/9.0,-6.0/9.0,
  4.0/9.0, 4.0/9.0,-6.0/9.0,  6.0/9.0, 4.0/9.0,-6.0/9.0,  8.0/9.0, 4.0/9.0,-6.0/9.0,
 -8.0/9.0, 6.0/9.0,-6.0/9.0, -6.0/9.0, 6.0/9.0,-6.0/9.0, -4.0/9.0, 6.0/9.0,-6.0/9.0,
 -2.0/9.0, 6.0/9.0,-6.0/9.0,      0.0, 6.0/9.0,-6.0/9.0,  2.0/9.0, 6.0/9.0,-6.0/9.0,
  4.0/9.0, 6.0/9.0,-6.0/9.0,  6.0/9.0, 6.0/9.0,-6.0/9.0,  8.0/9.0, 6.0/9.0,-6.0/9.0,
 -8.0/9.0, 8.0/9.0,-6.0/9.0, -6.0/9.0, 8.0/9.0,-6.0/9.0, -4.0/9.0, 8.0/9.0,-6.0/9.0,
 -2.0/9.0, 8.0/9.0,-6.0/9.0,      0.0, 8.0/9.0,-6.0/9.0,  2.0/9.0, 8.0/9.0,-6.0/9.0,
  4.0/9.0, 8.0/9.0,-6.0/9.0,  6.0/9.0, 8.0/9.0,-6.0/9.0,  8.0/9.0, 8.0/9.0,-6.0/9.0,
 -8.0/9.0,-8.0/9.0,-4.0/9.0, -6.0/9.0,-8.0/9.0,-4.0/9.0, -4.0/9.0,-8.0/9.0,-4.0/9.0,
 -2.0/9.0,-8.0/9.0,-4.0/9.0,      0.0,-8.0/9.0,-4.0/9.0,  2.0/9.0,-8.0/9.0,-4.0/9.0,
  4.0/9.0,-8.0/9.0,-4.0/9.0,  6.0/9.0,-8.0/9.0,-4.0/9.0,  8.0/9.0,-8.0/9.0,-4.0/9.0,
 -8.0/9.0,-6.0/9.0,-4.0/9.0, -6.0/9.0,-6.0/9.0,-4.0/9.0, -4.0/9.0,-6.0/9.0,-4.0/9.0,
 -2.0/9.0,-6.0/9.0,-4.0/9.0,      0.0,-6.0/9.0,-4.0/9.0,  2.0/9.0,-6.0/9.0,-4.0/9.0,
  4.0/9.0,-6.0/9.0,-4.0/9.0,  6.0/9.0,-6.0/9.0,-4.0/9.0,  8.0/9.0,-6.0/9.0,-4.0/9.0,
 -8.0/9.0,-4.0/9.0,-4.0/9.0, -6.0/9.0,-4.0/9.0,-4.0/9.0, -4.0/9.0,-4.0/9.0,-4.0/9.0,
 -2.0/9.0,-4.0/9.0,-4.0/9.0,      0.0,-4.0/9.0,-4.0/9.0,  2.0/9.0,-4.0/9.0,-4.0/9.0,
  4.0/9.0,-4.0/9.0,-4.0/9.0,  6.0/9.0,-4.0/9.0,-4.0/9.0,  8.0/9.0,-4.0/9.0,-4.0/9.0,
 -8.0/9.0,-2.0/9.0,-4.0/9.0, -6.0/9.0,-2.0/9.0,-4.0/9.0, -4.0/9.0,-2.0/9.0,-4.0/9.0,
 -2.0/9.0,-2.0/9.0,-4.0/9.0,      0.0,-2.0/9.0,-4.0/9.0,  2.0/9.0,-2.0/9.0,-4.0/9.0,
  4.0/9.0,-2.0/9.0,-4.0/9.0,  6.0/9.0,-2.0/9.0,-4.0/9.0,  8.0/9.0,-2.0/9.0,-4.0/9.0,
 -8.0/9.0,     0.0,-4.0/9.0, -6.0/9.0,     0.0,-4.0/9.0, -4.0/9.0,     0.0,-4.0/9.0,
 -2.0/9.0,     0.0,-4.0/9.0,      0.0,     0.0,-4.0/9.0,  2.0/9.0,     0.0,-4.0/9.0,
  4.0/9.0,     0.0,-4.0/9.0,  6.0/9.0,     0.0,-4.0/9.0,  8.0/9.0,     0.0,-4.0/9.0,
 -8.0/9.0, 2.0/9.0,-4.0/9.0, -6.0/9.0, 2.0/9.0,-4.0/9.0, -4.0/9.0, 2.0/9.0,-4.0/9.0,
 -2.0/9.0, 2.0/9.0,-4.0/9.0,      0.0, 2.0/9.0,-4.0/9.0,  2.0/9.0, 2.0/9.0,-4.0/9.0,
  4.0/9.0, 2.0/9.0,-4.0/9.0,  6.0/9.0, 2.0/9.0,-4.0/9.0,  8.0/9.0, 2.0/9.0,-4.0/9.0,
 -8.0/9.0, 4.0/9.0,-4.0/9.0, -6.0/9.0, 4.0/9.0,-4.0/9.0, -4.0/9.0, 4.0/9.0,-4.0/9.0,
 -2.0/9.0, 4.0/9.0,-4.0/9.0,      0.0, 4.0/9.0,-4.0/9.0,  2.0/9.0, 4.0/9.0,-4.0/9.0,
  4.0/9.0, 4.0/9.0,-4.0/9.0,  6.0/9.0, 4.0/9.0,-4.0/9.0,  8.0/9.0, 4.0/9.0,-4.0/9.0,
 -8.0/9.0, 6.0/9.0,-4.0/9.0, -6.0/9.0, 6.0/9.0,-4.0/9.0, -4.0/9.0, 6.0/9.0,-4.0/9.0,
 -2.0/9.0, 6.0/9.0,-4.0/9.0,      0.0, 6.0/9.0,-4.0/9.0,  2.0/9.0, 6.0/9.0,-4.0/9.0,
  4.0/9.0, 6.0/9.0,-4.0/9.0,  6.0/9.0, 6.0/9.0,-4.0/9.0,  8.0/9.0, 6.0/9.0,-4.0/9.0,
 -8.0/9.0, 8.0/9.0,-4.0/9.0, -6.0/9.0, 8.0/9.0,-4.0/9.0, -4.0/9.0, 8.0/9.0,-4.0/9.0,
 -2.0/9.0, 8.0/9.0,-4.0/9.0,      0.0, 8.0/9.0,-4.0/9.0,  2.0/9.0, 8.0/9.0,-4.0/9.0,
  4.0/9.0, 8.0/9.0,-4.0/9.0,  6.0/9.0, 8.0/9.0,-4.0/9.0,  8.0/9.0, 8.0/9.0,-4.0/9.0,
 -8.0/9.0,-8.0/9.0,-2.0/9.0, -6.0/9.0,-8.0/9.0,-2.0/9.0, -4.0/9.0,-8.0/9.0,-2.0/9.0,
 -2.0/9.0,-8.0/9.0,-2.0/9.0,      0.0,-8.0/9.0,-2.0/9.0,  2.0/9.0,-8.0/9.0,-2.0/9.0,
  4.0/9.0,-8.0/9.0,-2.0/9.0,  6.0/9.0,-8.0/9.0,-2.0/9.0,  8.0/9.0,-8.0/9.0,-2.0/9.0,
 -8.0/9.0,-6.0/9.0,-2.0/9.0, -6.0/9.0,-6.0/9.0,-2.0/9.0, -4.0/9.0,-6.0/9.0,-2.0/9.0,
 -2.0/9.0,-6.0/9.0,-2.0/9.0,      0.0,-6.0/9.0,-2.0/9.0,  2.0/9.0,-6.0/9.0,-2.0/9.0,
  4.0/9.0,-6.0/9.0,-2.0/9.0,  6.0/9.0,-6.0/9.0,-2.0/9.0,  8.0/9.0,-6.0/9.0,-2.0/9.0,
 -8.0/9.0,-4.0/9.0,-2.0/9.0, -6.0/9.0,-4.0/9.0,-2.0/9.0, -4.0/9.0,-4.0/9.0,-2.0/9.0,
 -2.0/9.0,-4.0/9.0,-2.0/9.0,      0.0,-4.0/9.0,-2.0/9.0,  2.0/9.0,-4.0/9.0,-2.0/9.0,
  4.0/9.0,-4.0/9.0,-2.0/9.0,  6.0/9.0,-4.0/9.0,-2.0/9.0,  8.0/9.0,-4.0/9.0,-2.0/9.0,
 -8.0/9.0,-2.0/9.0,-2.0/9.0, -6.0/9.0,-2.0/9.0,-2.0/9.0, -4.0/9.0,-2.0/9.0,-2.0/9.0,
 -2.0/9.0,-2.0/9.0,-2.0/9.0,      0.0,-2.0/9.0,-2.0/9.0,  2.0/9.0,-2.0/9.0,-2.0/9.0,
  4.0/9.0,-2.0/9.0,-2.0/9.0,  6.0/9.0,-2.0/9.0,-2.0/9.0,  8.0/9.0,-2.0/9.0,-2.0/9.0,
 -8.0/9.0,     0.0,-2.0/9.0, -6.0/9.0,     0.0,-2.0/9.0, -4.0/9.0,     0.0,-2.0/9.0,
 -2.0/9.0,     0.0,-2.0/9.0,      0.0,     0.0,-2.0/9.0,  2.0/9.0,     0.0,-2.0/9.0,
  4.0/9.0,     0.0,-2.0/9.0,  6.0/9.0,     0.0,-2.0/9.0,  8.0/9.0,     0.0,-2.0/9.0,
 -8.0/9.0, 2.0/9.0,-2.0/9.0, -6.0/9.0, 2.0/9.0,-2.0/9.0, -4.0/9.0, 2.0/9.0,-2.0/9.0,
 -2.0/9.0, 2.0/9.0,-2.0/9.0,      0.0, 2.0/9.0,-2.0/9.0,  2.0/9.0, 2.0/9.0,-2.0/9.0,
  4.0/9.0, 2.0/9.0,-2.0/9.0,  6.0/9.0, 2.0/9.0,-2.0/9.0,  8.0/9.0, 2.0/9.0,-2.0/9.0,
 -8.0/9.0, 4.0/9.0,-2.0/9.0, -6.0/9.0, 4.0/9.0,-2.0/9.0, -4.0/9.0, 4.0/9.0,-2.0/9.0,
 -2.0/9.0, 4.0/9.0,-2.0/9.0,      0.0, 4.0/9.0,-2.0/9.0,  2.0/9.0, 4.0/9.0,-2.0/9.0,
  4.0/9.0, 4.0/9.0,-2.0/9.0,  6.0/9.0, 4.0/9.0,-2.0/9.0,  8.0/9.0, 4.0/9.0,-2.0/9.0,
 -8.0/9.0, 6.0/9.0,-2.0/9.0, -6.0/9.0, 6.0/9.0,-2.0/9.0, -4.0/9.0, 6.0/9.0,-2.0/9.0,
 -2.0/9.0, 6.0/9.0,-2.0/9.0,      0.0, 6.0/9.0,-2.0/9.0,  2.0/9.0, 6.0/9.0,-2.0/9.0,
  4.0/9.0, 6.0/9.0,-2.0/9.0,  6.0/9.0, 6.0/9.0,-2.0/9.0,  8.0/9.0, 6.0/9.0,-2.0/9.0,
 -8.0/9.0, 8.0/9.0,-2.0/9.0, -6.0/9.0, 8.0/9.0,-2.0/9.0, -4.0/9.0, 8.0/9.0,-2.0/9.0,
 -2.0/9.0, 8.0/9.0,-2.0/9.0,      0.0, 8.0/9.0,-2.0/9.0,  2.0/9.0, 8.0/9.0,-2.0/9.0,
  4.0/9.0, 8.0/9.0,-2.0/9.0,  6.0/9.0, 8.0/9.0,-2.0/9.0,  8.0/9.0, 8.0/9.0,-2.0/9.0,
 -8.0/9.0,-8.0/9.0,     0.0, -6.0/9.0,-8.0/9.0,     0.0, -4.0/9.0,-8.0/9.0,     0.0,
 -2.0/9.0,-8.0/9.0,     0.0,      0.0,-8.0/9.0,     0.0,  2.0/9.0,-8.0/9.0,     0.0,
  4.0/9.0,-8.0/9.0,     0.0,  6.0/9.0,-8.0/9.0,     0.0,  8.0/9.0,-8.0/9.0,     0.0,
 -8.0/9.0,-6.0/9.0,     0.0, -6.0/9.0,-6.0/9.0,     0.0, -4.0/9.0,-6.0/9.0,     0.0,
 -2.0/9.0,-6.0/9.0,     0.0,      0.0,-6.0/9.0,     0.0,  2.0/9.0,-6.0/9.0,     0.0,
  4.0/9.0,-6.0/9.0,     0.0,  6.0/9.0,-6.0/9.0,     0.0,  8.0/9.0,-6.0/9.0,     0.0,
 -8.0/9.0,-4.0/9.0,     0.0, -6.0/9.0,-4.0/9.0,     0.0, -4.0/9.0,-4.0/9.0,     0.0,
 -2.0/9.0,-4.0/9.0,     0.0,      0.0,-4.0/9.0,     0.0,  2.0/9.0,-4.0/9.0,     0.0,
  4.0/9.0,-4.0/9.0,     0.0,  6.0/9.0,-4.0/9.0,     0.0,  8.0/9.0,-4.0/9.0,     0.0,
 -8.0/9.0,-2.0/9.0,     0.0, -6.0/9.0,-2.0/9.0,     0.0, -4.0/9.0,-2.0/9.0,     0.0,
 -2.0/9.0,-2.0/9.0,     0.0,      0.0,-2.0/9.0,     0.0,  2.0/9.0,-2.0/9.0,     0.0,
  4.0/9.0,-2.0/9.0,     0.0,  6.0/9.0,-2.0/9.0,     0.0,  8.0/9.0,-2.0/9.0,     0.0,
 -8.0/9.0,     0.0,     0.0, -6.0/9.0,     0.0,     0.0, -4.0/9.0,     0.0,     0.0,
 -2.0/9.0,     0.0,     0.0,      0.0,     0.0,     0.0,  2.0/9.0,     0.0,     0.0,
  4.0/9.0,     0.0,     0.0,  6.0/9.0,     0.0,     0.0,  8.0/9.0,     0.0,     0.0,
 -8.0/9.0, 2.0/9.0,     0.0, -6.0/9.0, 2.0/9.0,     0.0, -4.0/9.0, 2.0/9.0,     0.0,
 -2.0/9.0, 2.0/9.0,     0.0,      0.0, 2.0/9.0,     0.0,  2.0/9.0, 2.0/9.0,     0.0,
  4.0/9.0, 2.0/9.0,     0.0,  6.0/9.0, 2.0/9.0,     0.0,  8.0/9.0, 2.0/9.0,     0.0,
 -8.0/9.0, 4.0/9.0,     0.0, -6.0/9.0, 4.0/9.0,     0.0, -4.0/9.0, 4.0/9.0,     0.0,
 -2.0/9.0, 4.0/9.0,     0.0,      0.0, 4.0/9.0,     0.0,  2.0/9.0, 4.0/9.0,     0.0,
  4.0/9.0, 4.0/9.0,     0.0,  6.0/9.0, 4.0/9.0,     0.0,  8.0/9.0, 4.0/9.0,     0.0,
 -8.0/9.0, 6.0/9.0,     0.0, -6.0/9.0, 6.0/9.0,     0.0, -4.0/9.0, 6.0/9.0,     0.0,
 -2.0/9.0, 6.0/9.0,     0.0,      0.0, 6.0/9.0,     0.0,  2.0/9.0, 6.0/9.0,     0.0,
  4.0/9.0, 6.0/9.0,     0.0,  6.0/9.0, 6.0/9.0,     0.0,  8.0/9.0, 6.0/9.0,     0.0,
 -8.0/9.0, 8.0/9.0,     0.0, -6.0/9.0, 8.0/9.0,     0.0, -4.0/9.0, 8.0/9.0,     0.0,
 -2.0/9.0, 8.0/9.0,     0.0,      0.0, 8.0/9.0,     0.0,  2.0/9.0, 8.0/9.0,     0.0,
  4.0/9.0, 8.0/9.0,     0.0,  6.0/9.0, 8.0/9.0,     0.0,  8.0/9.0, 8.0/9.0,     0.0,
 -8.0/9.0,-8.0/9.0, 2.0/9.0, -6.0/9.0,-8.0/9.0, 2.0/9.0, -4.0/9.0,-8.0/9.0, 2.0/9.0,
 -2.0/9.0,-8.0/9.0, 2.0/9.0,      0.0,-8.0/9.0, 2.0/9.0,  2.0/9.0,-8.0/9.0, 2.0/9.0,
  4.0/9.0,-8.0/9.0, 2.0/9.0,  6.0/9.0,-8.0/9.0, 2.0/9.0,  8.0/9.0,-8.0/9.0, 2.0/9.0,
 -8.0/9.0,-6.0/9.0, 2.0/9.0, -6.0/9.0,-6.0/9.0, 2.0/9.0, -4.0/9.0,-6.0/9.0, 2.0/9.0,
 -2.0/9.0,-6.0/9.0, 2.0/9.0,      0.0,-6.0/9.0, 2.0/9.0,  2.0/9.0,-6.0/9.0, 2.0/9.0,
  4.0/9.0,-6.0/9.0, 2.0/9.0,  6.0/9.0,-6.0/9.0, 2.0/9.0,  8.0/9.0,-6.0/9.0, 2.0/9.0,
 -8.0/9.0,-4.0/9.0, 2.0/9.0, -6.0/9.0,-4.0/9.0, 2.0/9.0, -4.0/9.0,-4.0/9.0, 2.0/9.0,
 -2.0/9.0,-4.0/9.0, 2.0/9.0,      0.0,-4.0/9.0, 2.0/9.0,  2.0/9.0,-4.0/9.0, 2.0/9.0,
  4.0/9.0,-4.0/9.0, 2.0/9.0,  6.0/9.0,-4.0/9.0, 2.0/9.0,  8.0/9.0,-4.0/9.0, 2.0/9.0,
 -8.0/9.0,-2.0/9.0, 2.0/9.0, -6.0/9.0,-2.0/9.0, 2.0/9.0, -4.0/9.0,-2.0/9.0, 2.0/9.0,
 -2.0/9.0,-2.0/9.0, 2.0/9.0,      0.0,-2.0/9.0, 2.0/9.0,  2.0/9.0,-2.0/9.0, 2.0/9.0,
  4.0/9.0,-2.0/9.0, 2.0/9.0,  6.0/9.0,-2.0/9.0, 2.0/9.0,  8.0/9.0,-2.0/9.0, 2.0/9.0,
 -8.0/9.0,     0.0, 2.0/9.0, -6.0/9.0,     0.0, 2.0/9.0, -4.0/9.0,     0.0, 2.0/9.0,
 -2.0/9.0,     0.0, 2.0/9.0,      0.0,     0.0, 2.0/9.0,  2.0/9.0,     0.0, 2.0/9.0,
  4.0/9.0,     0.0, 2.0/9.0,  6.0/9.0,     0.0, 2.0/9.0,  8.0/9.0,     0.0, 2.0/9.0,
 -8.0/9.0, 2.0/9.0, 2.0/9.0, -6.0/9.0, 2.0/9.0, 2.0/9.0, -4.0/9.0, 2.0/9.0, 2.0/9.0,
 -2.0/9.0, 2.0/9.0, 2.0/9.0,      0.0, 2.0/9.0, 2.0/9.0,  2.0/9.0, 2.0/9.0, 2.0/9.0,
  4.0/9.0, 2.0/9.0, 2.0/9.0,  6.0/9.0, 2.0/9.0, 2.0/9.0,  8.0/9.0, 2.0/9.0, 2.0/9.0,
 -8.0/9.0, 4.0/9.0, 2.0/9.0, -6.0/9.0, 4.0/9.0, 2.0/9.0, -4.0/9.0, 4.0/9.0, 2.0/9.0,
 -2.0/9.0, 4.0/9.0, 2.0/9.0,      0.0, 4.0/9.0, 2.0/9.0,  2.0/9.0, 4.0/9.0, 2.0/9.0,
  4.0/9.0, 4.0/9.0, 2.0/9.0,  6.0/9.0, 4.0/9.0, 2.0/9.0,  8.0/9.0, 4.0/9.0, 2.0/9.0,
 -8.0/9.0, 6.0/9.0, 2.0/9.0, -6.0/9.0, 6.0/9.0, 2.0/9.0, -4.0/9.0, 6.0/9.0, 2.0/9.0,
 -2.0/9.0, 6.0/9.0, 2.0/9.0,      0.0, 6.0/9.0, 2.0/9.0,  2.0/9.0, 6.0/9.0, 2.0/9.0,
  4.0/9.0, 6.0/9.0, 2.0/9.0,  6.0/9.0, 6.0/9.0, 2.0/9.0,  8.0/9.0, 6.0/9.0, 2.0/9.0,
 -8.0/9.0, 8.0/9.0, 2.0/9.0, -6.0/9.0, 8.0/9.0, 2.0/9.0, -4.0/9.0, 8.0/9.0, 2.0/9.0,
 -2.0/9.0, 8.0/9.0, 2.0/9.0,      0.0, 8.0/9.0, 2.0/9.0,  2.0/9.0, 8.0/9.0, 2.0/9.0,
  4.0/9.0, 8.0/9.0, 2.0/9.0,  6.0/9.0, 8.0/9.0, 2.0/9.0,  8.0/9.0, 8.0/9.0, 2.0/9.0,
 -8.0/9.0,-8.0/9.0, 4.0/9.0, -6.0/9.0,-8.0/9.0, 4.0/9.0, -4.0/9.0,-8.0/9.0, 4.0/9.0,
 -2.0/9.0,-8.0/9.0, 4.0/9.0,      0.0,-8.0/9.0, 4.0/9.0,  2.0/9.0,-8.0/9.0, 4.0/9.0,
  4.0/9.0,-8.0/9.0, 4.0/9.0,  6.0/9.0,-8.0/9.0, 4.0/9.0,  8.0/9.0,-8.0/9.0, 4.0/9.0,
 -8.0/9.0,-6.0/9.0, 4.0/9.0, -6.0/9.0,-6.0/9.0, 4.0/9.0, -4.0/9.0,-6.0/9.0, 4.0/9.0,
 -2.0/9.0,-6.0/9.0, 4.0/9.0,      0.0,-6.0/9.0, 4.0/9.0,  2.0/9.0,-6.0/9.0, 4.0/9.0,
  4.0/9.0,-6.0/9.0, 4.0/9.0,  6.0/9.0,-6.0/9.0, 4.0/9.0,  8.0/9.0,-6.0/9.0, 4.0/9.0,
 -8.0/9.0,-4.0/9.0, 4.0/9.0, -6.0/9.0,-4.0/9.0, 4.0/9.0, -4.0/9.0,-4.0/9.0, 4.0/9.0,
 -2.0/9.0,-4.0/9.0, 4.0/9.0,      0.0,-4.0/9.0, 4.0/9.0,  2.0/9.0,-4.0/9.0, 4.0/9.0,
  4.0/9.0,-4.0/9.0, 4.0/9.0,  6.0/9.0,-4.0/9.0, 4.0/9.0,  8.0/9.0,-4.0/9.0, 4.0/9.0,
 -8.0/9.0,-2.0/9.0, 4.0/9.0, -6.0/9.0,-2.0/9.0, 4.0/9.0, -4.0/9.0,-2.0/9.0, 4.0/9.0,
 -2.0/9.0,-2.0/9.0, 4.0/9.0,      0.0,-2.0/9.0, 4.0/9.0,  2.0/9.0,-2.0/9.0, 4.0/9.0,
  4.0/9.0,-2.0/9.0, 4.0/9.0,  6.0/9.0,-2.0/9.0, 4.0/9.0,  8.0/9.0,-2.0/9.0, 4.0/9.0,
 -8.0/9.0,     0.0, 4.0/9.0, -6.0/9.0,     0.0, 4.0/9.0, -4.0/9.0,     0.0, 4.0/9.0,
 -2.0/9.0,     0.0, 4.0/9.0,      0.0,     0.0, 4.0/9.0,  2.0/9.0,     0.0, 4.0/9.0,
  4.0/9.0,     0.0, 4.0/9.0,  6.0/9.0,     0.0, 4.0/9.0,  8.0/9.0,     0.0, 4.0/9.0,
 -8.0/9.0, 2.0/9.0, 4.0/9.0, -6.0/9.0, 2.0/9.0, 4.0/9.0, -4.0/9.0, 2.0/9.0, 4.0/9.0,
 -2.0/9.0, 2.0/9.0, 4.0/9.0,      0.0, 2.0/9.0, 4.0/9.0,  2.0/9.0, 2.0/9.0, 4.0/9.0,
  4.0/9.0, 2.0/9.0, 4.0/9.0,  6.0/9.0, 2.0/9.0, 4.0/9.0,  8.0/9.0, 2.0/9.0, 4.0/9.0,
 -8.0/9.0, 4.0/9.0, 4.0/9.0, -6.0/9.0, 4.0/9.0, 4.0/9.0, -4.0/9.0, 4.0/9.0, 4.0/9.0,
 -2.0/9.0, 4.0/9.0, 4.0/9.0,      0.0, 4.0/9.0, 4.0/9.0,  2.0/9.0, 4.0/9.0, 4.0/9.0,
  4.0/9.0, 4.0/9.0, 4.0/9.0,  6.0/9.0, 4.0/9.0, 4.0/9.0,  8.0/9.0, 4.0/9.0, 4.0/9.0,
 -8.0/9.0, 6.0/9.0, 4.0/9.0, -6.0/9.0, 6.0/9.0, 4.0/9.0, -4.0/9.0, 6.0/9.0, 4.0/9.0,
 -2.0/9.0, 6.0/9.0, 4.0/9.0,      0.0, 6.0/9.0, 4.0/9.0,  2.0/9.0, 6.0/9.0, 4.0/9.0,
  4.0/9.0, 6.0/9.0, 4.0/9.0,  6.0/9.0, 6.0/9.0, 4.0/9.0,  8.0/9.0, 6.0/9.0, 4.0/9.0,
 -8.0/9.0, 8.0/9.0, 4.0/9.0, -6.0/9.0, 8.0/9.0, 4.0/9.0, -4.0/9.0, 8.0/9.0, 4.0/9.0,
 -2.0/9.0, 8.0/9.0, 4.0/9.0,      0.0, 8.0/9.0, 4.0/9.0,  2.0/9.0, 8.0/9.0, 4.0/9.0,
  4.0/9.0, 8.0/9.0, 4.0/9.0,  6.0/9.0, 8.0/9.0, 4.0/9.0,  8.0/9.0, 8.0/9.0, 4.0/9.0,
 -8.0/9.0,-8.0/9.0, 6.0/9.0, -6.0/9.0,-8.0/9.0, 6.0/9.0, -4.0/9.0,-8.0/9.0, 6.0/9.0,
 -2.0/9.0,-8.0/9.0, 6.0/9.0,      0.0,-8.0/9.0, 6.0/9.0,  2.0/9.0,-8.0/9.0, 6.0/9.0,
  4.0/9.0,-8.0/9.0, 6.0/9.0,  6.0/9.0,-8.0/9.0, 6.0/9.0,  8.0/9.0,-8.0/9.0, 6.0/9.0,
 -8.0/9.0,-6.0/9.0, 6.0/9.0, -6.0/9.0,-6.0/9.0, 6.0/9.0, -4.0/9.0,-6.0/9.0, 6.0/9.0,
 -2.0/9.0,-6.0/9.0, 6.0/9.0,      0.0,-6.0/9.0, 6.0/9.0,  2.0/9.0,-6.0/9.0, 6.0/9.0,
  4.0/9.0,-6.0/9.0, 6.0/9.0,  6.0/9.0,-6.0/9.0, 6.0/9.0,  8.0/9.0,-6.0/9.0, 6.0/9.0,
 -8.0/9.0,-4.0/9.0, 6.0/9.0, -6.0/9.0,-4.0/9.0, 6.0/9.0, -4.0/9.0,-4.0/9.0, 6.0/9.0,
 -2.0/9.0,-4.0/9.0, 6.0/9.0,      0.0,-4.0/9.0, 6.0/9.0,  2.0/9.0,-4.0/9.0, 6.0/9.0,
  4.0/9.0,-4.0/9.0, 6.0/9.0,  6.0/9.0,-4.0/9.0, 6.0/9.0,  8.0/9.0,-4.0/9.0, 6.0/9.0,
 -8.0/9.0,-2.0/9.0, 6.0/9.0, -6.0/9.0,-2.0/9.0, 6.0/9.0, -4.0/9.0,-2.0/9.0, 6.0/9.0,
 -2.0/9.0,-2.0/9.0, 6.0/9.0,      0.0,-2.0/9.0, 6.0/9.0,  2.0/9.0,-2.0/9.0, 6.0/9.0,
  4.0/9.0,-2.0/9.0, 6.0/9.0,  6.0/9.0,-2.0/9.0, 6.0/9.0,  8.0/9.0,-2.0/9.0, 6.0/9.0,
 -8.0/9.0,     0.0, 6.0/9.0, -6.0/9.0,     0.0, 6.0/9.0, -4.0/9.0,     0.0, 6.0/9.0,
 -2.0/9.0,     0.0, 6.0/9.0,      0.0,     0.0, 6.0/9.0,  2.0/9.0,     0.0, 6.0/9.0,
  4.0/9.0,     0.0, 6.0/9.0,  6.0/9.0,     0.0, 6.0/9.0,  8.0/9.0,     0.0, 6.0/9.0,
 -8.0/9.0, 2.0/9.0, 6.0/9.0, -6.0/9.0, 2.0/9.0, 6.0/9.0, -4.0/9.0, 2.0/9.0, 6.0/9.0,
 -2.0/9.0, 2.0/9.0, 6.0/9.0,      0.0, 2.0/9.0, 6.0/9.0,  2.0/9.0, 2.0/9.0, 6.0/9.0,
  4.0/9.0, 2.0/9.0, 6.0/9.0,  6.0/9.0, 2.0/9.0, 6.0/9.0,  8.0/9.0, 2.0/9.0, 6.0/9.0,
 -8.0/9.0, 4.0/9.0, 6.0/9.0, -6.0/9.0, 4.0/9.0, 6.0/9.0, -4.0/9.0, 4.0/9.0, 6.0/9.0,
 -2.0/9.0, 4.0/9.0, 6.0/9.0,      0.0, 4.0/9.0, 6.0/9.0,  2.0/9.0, 4.0/9.0, 6.0/9.0,
  4.0/9.0, 4.0/9.0, 6.0/9.0,  6.0/9.0, 4.0/9.0, 6.0/9.0,  8.0/9.0, 4.0/9.0, 6.0/9.0,
 -8.0/9.0, 6.0/9.0, 6.0/9.0, -6.0/9.0, 6.0/9.0, 6.0/9.0, -4.0/9.0, 6.0/9.0, 6.0/9.0,
 -2.0/9.0, 6.0/9.0, 6.0/9.0,      0.0, 6.0/9.0, 6.0/9.0,  2.0/9.0, 6.0/9.0, 6.0/9.0,
  4.0/9.0, 6.0/9.0, 6.0/9.0,  6.0/9.0, 6.0/9.0, 6.0/9.0,  8.0/9.0, 6.0/9.0, 6.0/9.0,
 -8.0/9.0, 8.0/9.0, 6.0/9.0, -6.0/9.0, 8.0/9.0, 6.0/9.0, -4.0/9.0, 8.0/9.0, 6.0/9.0,
 -2.0/9.0, 8.0/9.0, 6.0/9.0,      0.0, 8.0/9.0, 6.0/9.0,  2.0/9.0, 8.0/9.0, 6.0/9.0,
  4.0/9.0, 8.0/9.0, 6.0/9.0,  6.0/9.0, 8.0/9.0, 6.0/9.0,  8.0/9.0, 8.0/9.0, 6.0/9.0,
 -8.0/9.0,-8.0/9.0, 8.0/9.0, -6.0/9.0,-8.0/9.0, 8.0/9.0, -4.0/9.0,-8.0/9.0, 8.0/9.0,
 -2.0/9.0,-8.0/9.0, 8.0/9.0,      0.0,-8.0/9.0, 8.0/9.0,  2.0/9.0,-8.0/9.0, 8.0/9.0,
  4.0/9.0,-8.0/9.0, 8.0/9.0,  6.0/9.0,-8.0/9.0, 8.0/9.0,  8.0/9.0,-8.0/9.0, 8.0/9.0,
 -8.0/9.0,-6.0/9.0, 8.0/9.0, -6.0/9.0,-6.0/9.0, 8.0/9.0, -4.0/9.0,-6.0/9.0, 8.0/9.0,
 -2.0/9.0,-6.0/9.0, 8.0/9.0,      0.0,-6.0/9.0, 8.0/9.0,  2.0/9.0,-6.0/9.0, 8.0/9.0,
  4.0/9.0,-6.0/9.0, 8.0/9.0,  6.0/9.0,-6.0/9.0, 8.0/9.0,  8.0/9.0,-6.0/9.0, 8.0/9.0,
 -8.0/9.0,-4.0/9.0, 8.0/9.0, -6.0/9.0,-4.0/9.0, 8.0/9.0, -4.0/9.0,-4.0/9.0, 8.0/9.0,
 -2.0/9.0,-4.0/9.0, 8.0/9.0,      0.0,-4.0/9.0, 8.0/9.0,  2.0/9.0,-4.0/9.0, 8.0/9.0,
  4.0/9.0,-4.0/9.0, 8.0/9.0,  6.0/9.0,-4.0/9.0, 8.0/9.0,  8.0/9.0,-4.0/9.0, 8.0/9.0,
 -8.0/9.0,-2.0/9.0, 8.0/9.0, -6.0/9.0,-2.0/9.0, 8.0/9.0, -4.0/9.0,-2.0/9.0, 8.0/9.0,
 -2.0/9.0,-2.0/9.0, 8.0/9.0,      0.0,-2.0/9.0, 8.0/9.0,  2.0/9.0,-2.0/9.0, 8.0/9.0,
  4.0/9.0,-2.0/9.0, 8.0/9.0,  6.0/9.0,-2.0/9.0, 8.0/9.0,  8.0/9.0,-2.0/9.0, 8.0/9.0,
 -8.0/9.0,     0.0, 8.0/9.0, -6.0/9.0,     0.0, 8.0/9.0, -4.0/9.0,     0.0, 8.0/9.0,
 -2.0/9.0,     0.0, 8.0/9.0,      0.0,     0.0, 8.0/9.0,  2.0/9.0,     0.0, 8.0/9.0,
  4.0/9.0,     0.0, 8.0/9.0,  6.0/9.0,     0.0, 8.0/9.0,  8.0/9.0,     0.0, 8.0/9.0,
 -8.0/9.0, 2.0/9.0, 8.0/9.0, -6.0/9.0, 2.0/9.0, 8.0/9.0, -4.0/9.0, 2.0/9.0, 8.0/9.0,
 -2.0/9.0, 2.0/9.0, 8.0/9.0,      0.0, 2.0/9.0, 8.0/9.0,  2.0/9.0, 2.0/9.0, 8.0/9.0,
  4.0/9.0, 2.0/9.0, 8.0/9.0,  6.0/9.0, 2.0/9.0, 8.0/9.0,  8.0/9.0, 2.0/9.0, 8.0/9.0,
 -8.0/9.0, 4.0/9.0, 8.0/9.0, -6.0/9.0, 4.0/9.0, 8.0/9.0, -4.0/9.0, 4.0/9.0, 8.0/9.0,
 -2.0/9.0, 4.0/9.0, 8.0/9.0,      0.0, 4.0/9.0, 8.0/9.0,  2.0/9.0, 4.0/9.0, 8.0/9.0,
  4.0/9.0, 4.0/9.0, 8.0/9.0,  6.0/9.0, 4.0/9.0, 8.0/9.0,  8.0/9.0, 4.0/9.0, 8.0/9.0,
 -8.0/9.0, 6.0/9.0, 8.0/9.0, -6.0/9.0, 6.0/9.0, 8.0/9.0, -4.0/9.0, 6.0/9.0, 8.0/9.0,
 -2.0/9.0, 6.0/9.0, 8.0/9.0,      0.0, 6.0/9.0, 8.0/9.0,  2.0/9.0, 6.0/9.0, 8.0/9.0,
  4.0/9.0, 6.0/9.0, 8.0/9.0,  6.0/9.0, 6.0/9.0, 8.0/9.0,  8.0/9.0, 6.0/9.0, 8.0/9.0,
 -8.0/9.0, 8.0/9.0, 8.0/9.0, -6.0/9.0, 8.0/9.0, 8.0/9.0, -4.0/9.0, 8.0/9.0, 8.0/9.0,
 -2.0/9.0, 8.0/9.0, 8.0/9.0,      0.0, 8.0/9.0, 8.0/9.0,  2.0/9.0, 8.0/9.0, 8.0/9.0,
  4.0/9.0, 8.0/9.0, 8.0/9.0,  6.0/9.0, 8.0/9.0, 8.0/9.0,  8.0/9.0, 8.0/9.0, 8.0/9.0
};


static const REAL *grouptableA[16] =
{ 0,group5bits,group7bits,group10bits,0,0,0,0,0,0,0,0,0,0,0,0};
static const REAL *grouptableB1[16] =
{ 0,group5bits,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
static const REAL *grouptableB234[16] =
{ 0,group5bits,group7bits,0,group10bits,0,0,0,0,0,0,0,0,0,0,0};

static const int codelengthtableA[16] =
{ 0, 5, 7, 10, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
static const int codelengthtableB1[16] =
{ 0, 5, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
static const int codelengthtableB2[16] =
{ 0, 5, 7, 3, 10, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 16 };
static const int codelengthtableB3[8] = { 0, 5, 7, 3, 10, 4, 5, 16 };
static const int codelengthtableB4[4] = { 0, 5, 7, 16 };


static const REAL factortableA[16] =
{ 0.0,        1.0/2.0,    1.0/4.0,    1.0/8.0,
  1.0/8.0,    1.0/16.0,   1.0/32.0,   1.0/64.0,
  1.0/128.0,  1.0/256.0,  1.0/512.0,  1.0/1024.0,
  1.0/2048.0, 1.0/4096.0, 1.0/8192.0, 1.0/16384.0 };
static const REAL factortableB1[16] =
{ 0.0,        1.0/2.0,    1.0/4.0,     1.0/8.0, 
  1.0/16.0,   1.0/32.0,   1.0/64.0,    1.0/128.0, 
  1.0/256.0,  1.0/512.0,  1.0/1024.0,  1.0/2048.0,
  1.0/4096.0, 1.0/8192.0, 1.0/16384.0, 1.0/32768.0 };
static const REAL factortableB2[16] =
{ 0.0,        1.0/2.0,    1.0/4.0,    1.0/4.0,
  1.0/8.0,    1.0/8.0,    1.0/16.0,   1.0/32.0,
  1.0/64.0,   1.0/128.0,  1.0/256.0,  1.0/512.0,
  1.0/1024.0, 1.0/2048.0, 1.0/4096.0, 1.0/32768.0 };
static const REAL factortableB3[8] =
{ 0.0, 1.0/2.0, 1.0/4.0, 1.0/4.0, 1.0/8.0, 1.0/8.0, 1.0/16.0, 1.0/32768.0 };
static const REAL factortableB4[4] = { 0.0, 1.0/2.0, 1.0/4.0, 1.0/32768.0 };


static const REAL ctableA[16]=
{           0.0, 1.33333333333, 1.60000000000, 1.77777777777,
  1.06666666666, 1.03225806452, 1.01587301587, 1.00787401575,
  1.00392156863, 1.00195694716, 1.00097751711, 1.00048851979,
  1.00024420024, 1.00012208522, 1.00006103888, 1.00003051851};
static const REAL ctableB1[16]=
{           0.0, 1.33333333333, 1.14285714286, 1.06666666666,
  1.03225806452, 1.01587301587, 1.00787401575, 1.00392156863,
  1.00195694716, 1.00097751711, 1.00048851979, 1.00024420024,
  1.00012208522, 1.00006103888, 1.00003051851, 1.00001525902};
static const REAL ctableB2[16] =
{           0.0, 1.33333333333, 1.60000000000, 1.14285714286,
  1.77777777777, 1.06666666666, 1.03225806452, 1.01587301587,
  1.00787401575, 1.00392156863, 1.00195694716, 1.00097751711,
  1.00048851979, 1.00024420024, 1.00012208522, 1.00001525902};
static const REAL ctableB3[8] =
{           0.0, 1.33333333333, 1.60000000000, 1.14285714286,
  1.77777777777, 1.06666666666, 1.03225806452, 1.00001525902 };
static const REAL ctableB4[4] = 
{ 0.0, 1.33333333333, 1.60000000000, 1.00001525902 };


static const REAL dtableA[16]=
{           0.0, 0.50000000000, 0.50000000000, 0.50000000000,
  0.12500000000, 0.06250000000, 0.03125000000, 0.01562500000,
  0.00781250000, 0.00390625000, 0.00195312500, 0.00097656250,
  0.00048828125, 0.00024414063, 0.00012207031, 0.00006103516};

static const REAL dtableB1[16]=
{           0.0, 0.50000000000, 0.25000000000, 0.12500000000,
  0.06250000000, 0.03125000000, 0.01562500000, 0.00781250000,
  0.00390625000, 0.00195312500, 0.00097656250, 0.00048828125,
  0.00024414063, 0.00012207031, 0.00006103516, 0.00003051758};

static const REAL dtableB2[16]=
{ 0.0,           0.50000000000, 0.50000000000, 0.25000000000,
  0.50000000000, 0.12500000000, 0.06250000000, 0.03125000000,
  0.01562500000, 0.00781250000, 0.00390625000, 0.00195312500,
  0.00097656250, 0.00048828125, 0.00024414063, 0.00003051758};

static const REAL dtableB3[8]=
{           0.0, 0.50000000000, 0.50000000000, 0.25000000000,
  0.50000000000, 0.12500000000, 0.06250000000, 0.00003051758};

static const REAL dtableB4[4]=
{0.0, 0.50000000000, 0.50000000000, 0.00003051758};


// Mpeg layer 2
void MPEGaudio::extractlayer2(void)
{
  REAL fraction[MAXCHANNEL][3][MAXSUBBAND];
  unsigned int bitalloc[MAXCHANNEL][MAXSUBBAND],
               scaleselector[MAXCHANNEL][MAXSUBBAND];
  REAL scalefactor[2][3][MAXSUBBAND];

  const REAL *group[MAXCHANNEL][MAXSUBBAND];
  unsigned int codelength[MAXCHANNEL][MAXSUBBAND];
  REAL factor[MAXCHANNEL][MAXSUBBAND];
  REAL c[MAXCHANNEL][MAXSUBBAND],d[MAXCHANNEL][MAXSUBBAND];

  int s=stereobound,n=subbandnumber;


// Bitalloc
  {
    int i;
    const int *t=bitalloclengthtable[tableindex];

    for(i=0;i<s;i++,t++)
    {
      bitalloc[LS][i]=getbits(*t);
      bitalloc[RS][i]=getbits(*t);
    }
    for(;i<n;i++,t++)
      bitalloc[LS][i]=bitalloc[RS][i]=getbits(*t);
  }


  // Scale selector
  if(inputstereo)
    for(int i=0;i<n;i++)
    {
      if(bitalloc[LS][i])scaleselector[LS][i]=getbits(2);
      if(bitalloc[RS][i])scaleselector[RS][i]=getbits(2);
    }
  else
    for(int i=0;i<n;i++)
      if(bitalloc[LS][i])scaleselector[LS][i]=getbits(2);

  // Scale index
  {
    int i,j;

    for(i=0;i<n;i++)
    {
      if((j=bitalloc[LS][i]))
      {
	if(!tableindex)
	{
	  group[LS][i]=grouptableA[j];
	  codelength[LS][i]=codelengthtableA[j];
	  factor[LS][i]=factortableA[j];
	  c[LS][i]=ctableA[j];
	  d[LS][i]=dtableA[j];
	}
	else
	{
	  if(i<=2)
	  {
	    group[LS][i]=grouptableB1[j];
	    codelength[LS][i]=codelengthtableB1[j];
	    factor[LS][i]=factortableB1[j];
	    c[LS][i]=ctableB1[j];
	    d[LS][i]=dtableB1[j];
	  }
	  else
	  {
	    group[LS][i]=grouptableB234[j];
	    if(i<=10)
	    {
	      codelength[LS][i]=codelengthtableB2[j];
	      factor[LS][i]=factortableB2[j];
	      c[LS][i]=ctableB2[j];
	      d[LS][i]=dtableB2[j];
	    }
	    else if(i<=22)
	    {
	      codelength[LS][i]=codelengthtableB3[j];
	      factor[LS][i]=factortableB3[j];
	      c[LS][i]=ctableB3[j];
	      d[LS][i]=dtableB3[j];
	    }
	    else
	    {
	      codelength[LS][i]=codelengthtableB4[j];
	      factor[LS][i]=factortableB4[j];
	      c[LS][i]=ctableB4[j];
	      d[LS][i]=dtableB4[j];
	    }
	  }
	}

	switch(scaleselector[LS][i])
	{
	  case 0:scalefactor[LS][0][i]=scalefactorstable[getbits(6)];
		 scalefactor[LS][1][i]=scalefactorstable[getbits(6)];
		 scalefactor[LS][2][i]=scalefactorstable[getbits(6)];
		 break;
	  case 1:scalefactor[LS][0][i]=
		 scalefactor[LS][1][i]=scalefactorstable[getbits(6)];
		 scalefactor[LS][2][i]=scalefactorstable[getbits(6)];
		 break;
	  case 2:scalefactor[LS][0][i]=
		 scalefactor[LS][1][i]=
		 scalefactor[LS][2][i]=scalefactorstable[getbits(6)];
		 break;
	  case 3:scalefactor[LS][0][i]=scalefactorstable[getbits(6)];
		 scalefactor[LS][1][i]=
		 scalefactor[LS][2][i]=scalefactorstable[getbits(6)];
		 break;
	}
      }

      if(inputstereo && (j=bitalloc[RS][i]))
      {
	if(!tableindex)
	{
	  group[RS][i]=grouptableA[j];
	  codelength[RS][i]=codelengthtableA[j];
	  factor[RS][i]=factortableA[j];
	  c[RS][i]=ctableA[j];
	  d[RS][i]=dtableA[j];
	}
	else
	{
	  if(i<=2)
	  {
	    group[RS][i]=grouptableB1[j];
	    codelength[RS][i]=codelengthtableB1[j];
	    factor[RS][i]=factortableB1[j];
	    c[RS][i]=ctableB1[j];
	    d[RS][i]=dtableB1[j];
	  }
	  else
	  {
	    group[RS][i]=grouptableB234[j];
	    if(i<=10)
	    {
	      codelength[RS][i]=codelengthtableB2[j];
	      factor[RS][i]=factortableB2[j];
	      c[RS][i]=ctableB2[j];
	      d[RS][i]=dtableB2[j];
	    }
	    else if(i<=22)
	    {
	      codelength[RS][i]=codelengthtableB3[j];
	      factor[RS][i]=factortableB3[j];
	      c[RS][i]=ctableB3[j];
	      d[RS][i]=dtableB3[j];
	    }
	    else
	    {
	      codelength[RS][i]=codelengthtableB4[j];
	      factor[RS][i]=factortableB4[j];
	      c[RS][i]=ctableB4[j];
	      d[RS][i]=dtableB4[j];
	    }
	  }
	}

	switch(scaleselector[RS][i])
	{
	  case 0 : scalefactor[RS][0][i]=scalefactorstable[getbits(6)];
		   scalefactor[RS][1][i]=scalefactorstable[getbits(6)];
		   scalefactor[RS][2][i]=scalefactorstable[getbits(6)];
		   break;
	  case 1 : scalefactor[RS][0][i]=
		   scalefactor[RS][1][i]=scalefactorstable[getbits(6)];
		   scalefactor[RS][2][i]=scalefactorstable[getbits(6)];
		   break;
	  case 2 : scalefactor[RS][0][i]=
		   scalefactor[RS][1][i]=
		   scalefactor[RS][2][i]=scalefactorstable[getbits(6)];
		   break;
	  case 3 : scalefactor[RS][0][i]=scalefactorstable[getbits(6)];
		   scalefactor[RS][1][i]=
		   scalefactor[RS][2][i]=scalefactorstable[getbits(6)];
		   break;
	}
      }
    }
  }


// Read Sample
  {
    int i;

    for(int l=0;l<SCALEBLOCK;l++)
    {
      // Read Sample
      for(i=0;i<s;i++)
      {
	if(bitalloc[LS][i])
	{
	  if(group[LS][i])
	  {
	    const REAL *s;
	    int code=getbits(codelength[LS][i]);

	    code+=code<<1;
            if (code > 2184) {
//printf("fraction LS OverFlow code %d -> 2184 (1)\n", code);
              code=2184;
            }
	    s=group[LS][i]+code;

	    fraction[LS][0][i]=s[0];
	    fraction[LS][1][i]=s[1];
	    fraction[LS][2][i]=s[2];
	  }
	  else
	  {
	    fraction[LS][0][i]=
	      REAL(getbits(codelength[LS][i]))*factor[LS][i]-1.0;
	    fraction[LS][1][i]=
	      REAL(getbits(codelength[LS][i]))*factor[LS][i]-1.0;
	    fraction[LS][2][i]=
	      REAL(getbits(codelength[LS][i]))*factor[LS][i]-1.0;
	  }
	}
	else fraction[LS][0][i]=fraction[LS][1][i]=fraction[LS][2][i]=0.0;

	if(inputstereo && bitalloc[RS][i])
	{
	  if(group[RS][i])
	  {
	    const REAL *s;
	    int code=getbits(codelength[RS][i]);

	    code+=code<<1;
            if (code > 2184) {
//printf("fraction LS OverFlow code %d -> 2184 (2)\n", code);
              code=2184;
            }
	    s=group[RS][i]+code;

	    fraction[RS][0][i]=s[0];
	    fraction[RS][1][i]=s[1];
	    fraction[RS][2][i]=s[2];
	  }
	  else
	  {
	    fraction[RS][0][i]=
	      REAL(getbits(codelength[RS][i]))*factor[RS][i]-1.0;
	    fraction[RS][1][i]=
	      REAL(getbits(codelength[RS][i]))*factor[RS][i]-1.0;
	    fraction[RS][2][i]=
	      REAL(getbits(codelength[RS][i]))*factor[RS][i]-1.0;
	  }
	}
	else fraction[RS][0][i]=fraction[RS][1][i]=fraction[RS][2][i]=0.0;
      }

      for(;i<n;i++)
      {
	if(bitalloc[LS][i])
	{
	  if(group[LS][i])
	  {
	    const REAL *s;
	    int code=getbits(codelength[LS][i]);

	    code+=code<<1;
	    s=group[LS][i]+code;

	    fraction[LS][0][i]=fraction[RS][0][i]=s[0];
	    fraction[LS][1][i]=fraction[RS][1][i]=s[1];
	    fraction[LS][2][i]=fraction[RS][2][i]=s[2];
	  }
	  else
	  {
	    fraction[LS][0][i]=fraction[RS][0][i]=
	      REAL(getbits(codelength[LS][i]))*factor[LS][i]-1.0;
	    fraction[LS][1][i]=fraction[RS][1][i]=
	      REAL(getbits(codelength[LS][i]))*factor[LS][i]-1.0;
	    fraction[LS][2][i]=fraction[RS][2][i]=
	      REAL(getbits(codelength[LS][i]))*factor[LS][i]-1.0;
	  }
	}
	else fraction[LS][0][i]=fraction[LS][1][i]=fraction[LS][2][i]=
	     fraction[RS][0][i]=fraction[RS][1][i]=fraction[RS][2][i]=0.0;
      }



      //Fraction
      if(outputstereo)
	for(i=0;i<n;i++)
	{
	  if(bitalloc[LS][i])
	  {
	    if(!group[LS][i])
	    {
	      fraction[LS][0][i]=(fraction[LS][0][i]+d[LS][i])*c[LS][i];
	      fraction[LS][1][i]=(fraction[LS][1][i]+d[LS][i])*c[LS][i];
	      fraction[LS][2][i]=(fraction[LS][2][i]+d[LS][i])*c[LS][i];
	    }

	    REAL t=scalefactor[LS][l>>2][i];
	    fraction[LS][0][i]*=t;
	    fraction[LS][1][i]*=t;
	    fraction[LS][2][i]*=t;
	  }

	  if(bitalloc[RS][i])
	  {
	    if(!group[RS][i])
	    {
	      fraction[RS][0][i]=(fraction[RS][0][i]+d[RS][i])*c[RS][i];
	      fraction[RS][1][i]=(fraction[RS][1][i]+d[RS][i])*c[RS][i];
	      fraction[RS][2][i]=(fraction[RS][2][i]+d[RS][i])*c[RS][i];
	    }

	    REAL t=scalefactor[RS][l>>2][i];
	    fraction[RS][0][i]*=t;
	    fraction[RS][1][i]*=t;
	    fraction[RS][2][i]*=t;
	  }
	}
      else
	for(i=0;i<n;i++)
	  if(bitalloc[LS][i])
	  {
	    if(!group[LS][i])
	    {
	      fraction[LS][0][i]=(fraction[LS][0][i]+d[LS][i])*c[LS][i];
	      fraction[LS][1][i]=(fraction[LS][1][i]+d[LS][i])*c[LS][i];
	      fraction[LS][2][i]=(fraction[LS][2][i]+d[LS][i])*c[LS][i];
	    }

	    REAL t=scalefactor[LS][l>>2][i];
	    fraction[LS][0][i]*=t;
	    fraction[LS][1][i]*=t;
	    fraction[LS][2][i]*=t;
	  }

      for(;i<MAXSUBBAND;i++)
	fraction[LS][0][i]=fraction[LS][1][i]=fraction[LS][2][i]=
	fraction[RS][0][i]=fraction[RS][1][i]=fraction[RS][2][i]=0.0;

      for(i=0;i<3;i++)
	subbandsynthesis(fraction[LS][i],fraction[RS][i]);
    }
  }
}


#include <stdio.h>

static unsigned int  MMX_10w[]         = {0x00100010, 0x00100010};                     //dd    00010 0010h, 000100010h
static unsigned int  MMX_80w[]         = {0x00800080, 0x00800080};                     //dd    00080 0080h, 000800080h

static unsigned int  MMX_00FFw[]       = {0x00ff00ff, 0x00ff00ff};                     //dd    000FF 00FFh, 000FF00FFh

static unsigned short MMX_Ublucoeff[]   = {0x81, 0x81, 0x81, 0x81};                     //dd    00081 0081h, 000810081h
static unsigned short MMX_Vredcoeff[]   = {0x66, 0x66, 0x66, 0x66};                     //dd    00066 0066h, 000660066h

#ifdef RGB_555
static unsigned short MMX_Ugrncoeff[]   = {0xffe7, 0xffe7, 0xffe7, 0xffe7};             //dd    0FFE7 FFE7h, 0FFE7FFE7h
static unsigned short MMX_Vgrncoeff[]   = {0xffcc, 0xffcc, 0xffcc, 0xffcc};             //dd    0FFCC FFCCh, 0FFCCFFCCh
#else
static unsigned short MMX_Ugrncoeff[]   = {0xffe8, 0xffe8, 0xffe8, 0xffe8};             //dd    0FFE7 FFE7h, 0FFE7FFE7h
static unsigned short MMX_Vgrncoeff[]   = {0xffcd, 0xffcd, 0xffcd, 0xffcd};             //dd    0FFCC FFCCh, 0FFCCFFCCh
#endif

static unsigned short MMX_Ycoeff[]      = {0x4a, 0x4a, 0x4a, 0x4a};                     //dd    0004A 004Ah, 0004A004Ah
#ifdef RGB_555
static unsigned short MMX_redmask[]     = {0x7c00, 0x7c00, 0x7c00, 0x7c00};             //dd    07c00 7c00h, 07c007c00h
#else
static unsigned short MMX_redmask[]     = {0xf800, 0xf800, 0xf800, 0xf800};             //dd    07c00 7c00h, 07c007c00h
#endif

#ifdef RGB_555
static unsigned short MMX_grnmask[]     = {0x3e0, 0x3e0, 0x3e0, 0x3e0};                 //dd    003e0 03e0h, 003e003e0h
#else
static unsigned short MMX_grnmask[]     = {0x7e0, 0x7e0, 0x7e0, 0x7e0};                 //dd    003e0 03e0h, 003e003e0h
#endif

// static unsigned short MMX_blumask[]  = {0x1f, 0x1f, 0x1f, 0x1f};                     //dd    0001f 001fh, 0001f001fh

void Color16DitherImageMMX( unsigned char *lum, unsigned char *cr,
                         unsigned char *cb, unsigned char *out,
                         int rows, int cols, int mod )
{
#ifdef USE_MMX
    unsigned short *row1;
    unsigned short *row2;
    row1 = (unsigned short* )out;         // 32 bit target

    unsigned char* y = lum +cols*rows;    // Pointer to the end
    int x=0;
    row2=row1+mod+cols;                   // start of second row 
    mod=2*cols+2*mod;                     // increment for row1 in byte


      __asm__ __volatile__(
         ".align 8\n"
         "1:\n"
         "movd           (%1),                   %%mm0\n"        // 4 Cb         0  0  0  0 u3 u2 u1 u0
         "pxor           %%mm7,                  %%mm7\n"
         "movd           (%0),                   %%mm1\n" // 4 Cr                0  0  0  0 v3 v2 v1 v0
         "punpcklbw      %%mm7,                  %%mm0\n" // 4 W cb   0 u3  0 u2  0 u1  0 u0
         "punpcklbw      %%mm7,                  %%mm1\n" // 4 W cr   0 v3  0 v2  0 v1  0 v0
         "psubw          MMX_80w,                %%mm0\n"
         "psubw          MMX_80w,                %%mm1\n"
         "movq           %%mm0,                  %%mm2\n"        // Cb                   0 u3  0 u2  0 u1  0 u0
         "movq           %%mm1,                  %%mm3\n" // Cr
         "pmullw         MMX_Ugrncoeff,          %%mm2\n" // Cb2green 0 R3  0 R2  0 R1  0 R0
         "movq           (%2),                   %%mm6\n"        // L1      l7 L6 L5 L4 L3 L2 L1 L0
         "pmullw         MMX_Ublucoeff,          %%mm0\n" // Cb2blue
         "pand           MMX_00FFw,              %%mm6\n" // L1      00 L6 00 L4 00 L2 00 L0
         "pmullw         MMX_Vgrncoeff,          %%mm3\n" // Cr2green
         "movq           (%2),                   %%mm7\n" // L2
         "pmullw         MMX_Vredcoeff,          %%mm1\n" // Cr2red
         //                      "psubw          MMX_10w,                %%mm6\n"
         "psrlw          $8,                     %%mm7\n"        // L2           00 L7 00 L5 00 L3 00 L1
         "pmullw         MMX_Ycoeff,             %%mm6\n" // lum1
         //                      "psubw          MMX_10w,                %%mm7\n" // L2
         "paddw          %%mm3,                  %%mm2\n" // Cb2green + Cr2green == green
         "pmullw         MMX_Ycoeff,             %%mm7\n"  // lum2

         "movq           %%mm6,                  %%mm4\n"  // lum1
         "paddw          %%mm0,                  %%mm6\n"  // lum1 +blue 00 B6 00 B4 00 B2 00 B0
         "movq           %%mm4,                  %%mm5\n"  // lum1
         "paddw          %%mm1,                  %%mm4\n"  // lum1 +red  00 R6 00 R4 00 R2 00 R0
         "paddw          %%mm2,                  %%mm5\n"  // lum1 +green 00 G6 00 G4 00 G2 00 G0
         "psraw          $6,                     %%mm4\n"  // R1 0 .. 64
         "movq           %%mm7,                  %%mm3\n"  // lum2                       00 L7 00 L5 00 L3 00 L1
         "psraw          $6,                     %%mm5\n"  // G1  - .. +
         "paddw          %%mm0,                  %%mm7\n"  // Lum2 +blue 00 B7 00 B5 00 B3 00 B1
         "psraw          $6,                     %%mm6\n"  // B1         0 .. 64
         "packuswb       %%mm4,                  %%mm4\n"  // R1 R1
         "packuswb       %%mm5,                  %%mm5\n"  // G1 G1
         "packuswb       %%mm6,                  %%mm6\n"  // B1 B1
         "punpcklbw      %%mm4,                  %%mm4\n"
         "punpcklbw      %%mm5,                  %%mm5\n"

         "pand           MMX_redmask,            %%mm4\n"
         "psllw          $3,                     %%mm5\n"  // GREEN       1
         "punpcklbw      %%mm6,                  %%mm6\n"
         "pand           MMX_grnmask,            %%mm5\n"
         "pand           MMX_redmask,            %%mm6\n"
         "por            %%mm5,                  %%mm4\n" //
         "psrlw          $11,                    %%mm6\n"                // BLUE        1
         "movq           %%mm3,                  %%mm5\n" // lum2
         "paddw          %%mm1,                  %%mm3\n"        // lum2 +red      00 R7 00 R5 00 R3 00 R1
         "paddw          %%mm2,                  %%mm5\n" // lum2 +green 00 G7 00 G5 00 G3 00 G1
         "psraw          $6,                     %%mm3\n" // R2
         "por            %%mm6,                  %%mm4\n" // MM4
         "psraw          $6,                     %%mm5\n" // G2
         "movq           (%2, %4),               %%mm6\n" // L3 load lum2
         "psraw          $6,                     %%mm7\n"
         "packuswb       %%mm3,                  %%mm3\n"
         "packuswb       %%mm5,                  %%mm5\n"
         "packuswb       %%mm7,                  %%mm7\n"
         "pand                   MMX_00FFw,              %%mm6\n"  // L3
         "punpcklbw      %%mm3,                  %%mm3\n"
         //                              "psubw          MMX_10w,                        %%mm6\n"  // L3
         "punpcklbw      %%mm5,                  %%mm5\n"
         "pmullw         MMX_Ycoeff,             %%mm6\n"  // lum3
         "punpcklbw      %%mm7,                  %%mm7\n"
         "psllw          $3,                             %%mm5\n"  // GREEN 2
         "pand                   MMX_redmask,    %%mm7\n"
         "pand                   MMX_redmask,    %%mm3\n"
         "psrlw          $11,                            %%mm7\n"  // BLUE  2
         "pand                   MMX_grnmask,    %%mm5\n"
         "por                    %%mm7,                  %%mm3\n"
         "movq                   (%2,%4),        %%mm7\n"  // L4 load lum2
         "por                    %%mm5,                  %%mm3\n"     //
         "psrlw          $8,                             %%mm7\n"    // L4
         "movq                   %%mm4,                  %%mm5\n"
         //                              "psubw          MMX_10w,                        %%mm7\n"                // L4
         "punpcklwd      %%mm3,                  %%mm4\n"
         "pmullw         MMX_Ycoeff,             %%mm7\n"    // lum4
         "punpckhwd      %%mm3,                  %%mm5\n"

         "movq                   %%mm4,                  (%3)\n" // write row1
         "movq                   %%mm5,                  8(%3)\n" // write row1

         "movq                   %%mm6,                  %%mm4\n"        // Lum3
         "paddw          %%mm0,                  %%mm6\n"                // Lum3 +blue

         "movq                   %%mm4,                  %%mm5\n"                        // Lum3
         "paddw          %%mm1,                  %%mm4\n"       // Lum3 +red
         "paddw          %%mm2,                  %%mm5\n"                        // Lum3 +green
         "psraw          $6,                             %%mm4\n"
         "movq                   %%mm7,                  %%mm3\n"                        // Lum4
         "psraw          $6,                             %%mm5\n"
         "paddw          %%mm0,                  %%mm7\n"                   // Lum4 +blue
         "psraw          $6,                             %%mm6\n"                        // Lum3 +blue
         "movq                   %%mm3,                  %%mm0\n"  // Lum4
         "packuswb       %%mm4,                  %%mm4\n"
         "paddw          %%mm1,                  %%mm3\n"  // Lum4 +red
         "packuswb       %%mm5,                  %%mm5\n"
         "paddw          %%mm2,                  %%mm0\n"         // Lum4 +green
         "packuswb       %%mm6,                  %%mm6\n"
         "punpcklbw      %%mm4,                  %%mm4\n"
         "punpcklbw      %%mm5,                  %%mm5\n"
         "punpcklbw      %%mm6,                  %%mm6\n"
         "psllw          $3,                             %%mm5\n" // GREEN 3
         "pand                   MMX_redmask,    %%mm4\n"
         "psraw          $6,                             %%mm3\n" // psr 6
         "psraw          $6,                             %%mm0\n"
         "pand                   MMX_redmask,    %%mm6\n" // BLUE
         "pand                   MMX_grnmask,    %%mm5\n"
         "psrlw          $11,                            %%mm6\n"  // BLUE  3
         "por                    %%mm5,                  %%mm4\n"
         "psraw          $6,                             %%mm7\n"
         "por                    %%mm6,                  %%mm4\n"
         "packuswb       %%mm3,                  %%mm3\n"
         "packuswb       %%mm0,                  %%mm0\n"
         "packuswb       %%mm7,                  %%mm7\n"
         "punpcklbw      %%mm3,                  %%mm3\n"
         "punpcklbw      %%mm0,                  %%mm0\n"
         "punpcklbw      %%mm7,                  %%mm7\n"
         "pand                   MMX_redmask,    %%mm3\n"
         "pand                   MMX_redmask,    %%mm7\n" // BLUE
         "psllw          $3,                             %%mm0\n" // GREEN 4
         "psrlw          $11,                            %%mm7\n"
         "pand                   MMX_grnmask,    %%mm0\n"
         "por                    %%mm7,                  %%mm3\n"
         "por                    %%mm0,                  %%mm3\n"

         "movq                   %%mm4,                  %%mm5\n"

         "punpcklwd      %%mm3,                  %%mm4\n"
         "punpckhwd      %%mm3,                  %%mm5\n"

         "movq                   %%mm4,                  (%5)\n"
	 "movq                   %%mm5,                  8(%5)\n"

         "addl                   $8,             %6\n"
         "addl                   $8,                             %2\n"
         "addl                   $4,                             %0\n"
         "addl                   $4,                             %1\n"
         "cmpl                   %4,                             %6\n"
         "leal                   16(%3),                 %3\n"
	 "leal  16(%5),%5\n"        // row2+16


         "jl             1b\n"
	 "addl           %4,     %2\n" // lum += cols 
	 "addl           %8,     %3\n" // row1+= mod
	 "addl           %8,     %5\n" // row2+= mod
	 "movl           $0,     %6\n" // x=0
	 "cmpl           %7,     %2\n"
	 "jl             1b\n"

         :
         :"r" (cr), "r"(cb),"r"(lum),
	 "r"(row1),"r"(cols),"r"(row2),"m"(x),"m"(y),"m"(mod)

         );
      __asm__ __volatile__(
         "emms\n"
         );
#endif /* USE_MMX */
}


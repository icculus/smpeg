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

#include <stdlib.h>	/* used by almost all modules */

/* util.c */
void correct_underflow (VidStream *vid_stream);
int next_bits (int num , unsigned int mask , VidStream *vid_stream);
char *get_ext_data (VidStream *vid_stream);
int next_start_code (VidStream *vid_stream);
char *get_extra_bit_info (VidStream *vid_stream);

/* video.c */
void init_stats (void);
void PrintAllStats (VidStream *vid_stream);
double ReadSysClock (void);
void PrintTimeInfo (VidStream *vid_stream);
void InitCrop (void);
void InitIDCT (void);
VidStream *NewVidStream (unsigned int buffer_len);
void ResetVidStream (VidStream *vid);
void DestroyVidStream (VidStream *astream);
PictImage *NewPictImage (VidStream *vid_stream);
bool InitPictImages (VidStream *vid_stream, int w, int h);
void DestroyPictImage (VidStream *vid_stream, PictImage *apictimage);
VidStream *mpegVidRsrc (TimeStamp time_stamp,VidStream *vid_stream, int first);
void SetBFlag (BOOLEAN val);
void SetPFlag (BOOLEAN val);

/* parseblock.c */
void ParseReconBlock (int n, VidStream *vid_stream);
void ParseAwayBlock (int n , VidStream *vid_stream);

/* motionvec.c */
void ComputeForwVector (int *recon_right_for_ptr , int *recon_down_for_ptr , VidStream *the_stream);
void ComputeBackVector (int *recon_right_back_ptr, int *recon_down_back_ptr, VidStream *the_stream);

/* decoders.c */
void decodeInitTables (void);
void decodeDCTDCSizeLum (unsigned int *value);
void decodeDCTDCSizeChrom (unsigned int *value);
void decodeDCTCoeffFirst (unsigned int *run, int *level);
void decodeDCTCoeffNext (unsigned int *run , int *level);

/* gdith.c */
void InitColor (void);

/* fs2.c */
void InitFS2Dither (void);
void FS2DitherImage (unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *disp , int rows , int cols);

/* fs2fast.c */
void InitFS2FastDither (void);
void FS2FastDitherImage (unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w);

/* fs4.c */
void InitFS4Dither (void);
void FS4DitherImage (unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *disp , int rows , int cols);

/* hybrid.c */
void InitHybridDither (void);
void HybridDitherImage (unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w);

/* hybriderr.c */
void InitHybridErrorDither (void);
void HybridErrorDitherImage (unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w);

/* gray.c */
void GrayDitherImage (unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w);
void Gray2DitherImage (unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w);
void Gray16DitherImage (unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w);
void Gray216DitherImage (unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w);
void Gray32DitherImage (unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w);
void Gray232DitherImage (unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w);

/* mono.c */
void MonoThresholdImage(unsigned char *lum, unsigned char *cr, unsigned char *cb, unsigned char *out, int h, int w);
void MonoDitherImage(unsigned char *lum, unsigned char *cr, unsigned char *cb, unsigned char *out, int h, int w);

/* jrevdct.c */
void init_pre_idct (void);
void j_rev_dct_sparse (DCTBLOCK data , int pos);
void j_rev_dct (DCTBLOCK data);
void j_rev_dct_sparse (DCTBLOCK data , int pos);
void j_rev_dct (DCTBLOCK data);

/* floatdct.c */
void init_float_idct (void);
void float_idct (short* block);

/* 16bit.c */
void InitColorDither (int bpp, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask);
void Color16DitherImageMod (unsigned char *lum, unsigned char *cr, unsigned char *cb, unsigned char *out, int rows, int cols, int mod);
void Color16DitherImageMMX (unsigned char *lum, unsigned char *cr, unsigned char *cb, unsigned char *out, int rows, int cols, int mod);
void Color32DitherImageMod (unsigned char *lum, unsigned char *cr, unsigned char *cb, unsigned char *out, int rows, int cols, int mod);
void Color32DitherImageMMX (unsigned char *lum, unsigned char *cr, unsigned char *cb, unsigned char *out, int rows, int cols, int mod);
void Color16DitherImageModInterlace (unsigned char *lum, unsigned char *cr, unsigned char *cb, unsigned char *out, int rows, int cols, int mod, int start);
void Color32DitherImage (unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int rows , int cols);
void ScaleColor16DitherImageMod (unsigned char *lum, unsigned char *cr, unsigned char *cb, unsigned char *out, int rows, int cols, int mod, int scale);
void ScaleColor32DitherImageMod (unsigned char *lum, unsigned char *cr, unsigned char *cb, unsigned char *out, int rows, int cols, int mod, int scale);
void ScaleColor16DitherImageModInterlace (unsigned char *lum, unsigned char *cr, unsigned char *cb, unsigned char *out, int rows, int cols, int mod, int start, int scale);
void Twox2Color32DitherImage (unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int rows , int cols);

/* ordered.c */
void InitOrderedDither (void);
void OrderedDitherImage (unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w);

/* ordered2.c */
void InitOrdered2Dither (void);
void Ordered2DitherImage (unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w);

/* mb_ordered.c */
void InitMBOrderedDither (void);
void MBOrderedDitherImage (unsigned char *lum , unsigned char *cr , unsigned char *cb , unsigned char *out , int h , int w, char *ditherFlags);
void MBOrderedDitherDisplayCopy (VidStream *vid_stream , int mb_addr , int motion_forw , int r_right_forw , int r_down_forw , int motion_back , int r_right_back , int r_down_back , unsigned char *past , unsigned char *future);

/* readfile.c */
void SeekStream (VidStream *vid_stream);
void clear_data_stream (VidStream *vid_stream);
int get_more_data (VidStream *vid_stream);
int pure_get_more_data (unsigned int *buf_start , int max_length , int *length_ptr , unsigned int **buf_ptr, VidStream *vid_stream);
int read_sys (VidStream *vid_stream, unsigned int start);
int ReadStartCode (unsigned int *startCode, VidStream *vid_stream);

int ReadPackHeader (
   double *systemClockTime,
   unsigned long *muxRate,
   VidStream *vid_stream);

int ReadSystemHeader (VidStream *vid_stream);

int find_start_code (FILE *input);

int ReadPacket (unsigned char packetID, VidStream *vid_stream);

void ReadTimeStamp (
   unsigned char *inputBuffer,
   unsigned char *hiBit,
   unsigned long *low4Bytes);

void ReadSTD (
   unsigned char *inputBuffer,
   unsigned char *stdBufferScale,
   unsigned long *stdBufferSize);

void ReadRate (
   unsigned char *inputBuffer,
   unsigned long *rate);

int MakeFloatClockTime (
   unsigned char hiBit,
   unsigned long low4Bytes,
   double *floatClockTime);



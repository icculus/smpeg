/*
    SMPEG - SDL MPEG Player Library
    Copyright (C) 1999  Loki Entertainment Software

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* A class used for error reporting in the MPEG classes */

#ifndef _MPEGERROR_H_
#define _MPEGERROR_H_

#include <stdio.h>
#include <stdarg.h>

struct MPEGerror {
    char errbuf[512];
    char *error;
};

typedef struct MPEGerror MPEGerror;

MPEGerror *MPEGerror_create();
void MPEGerror_SetError(MPEGerror *self, char *fmt, ...);
int MPEGerror_WasError(MPEGerror *self);
char *MPEGerror_TheError(MPEGerror *self);
void MPEGerror_ClearError(MPEGerror *self);

#endif /* _MPEGERROR_H_ */

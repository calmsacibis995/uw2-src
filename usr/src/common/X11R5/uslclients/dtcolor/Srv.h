/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtcolor:Srv.h	1.2"
 
/*************************************<+>*************************************
 *****************************************************************************
 **
 **  File:        Srv.h
 **
 *****************************************************************************
 *************************************<+>*************************************/
/*  Standard C headers  */
#include <stdio.h>

#ifdef _AIX
#include <sys/types.h>
#include <sys/dir.h>
#else
#include <dirent.h>
#include <limits.h>
#endif /* _AIX */

/*  Xm headers  */
#include "common.h"

/* #defines */

#define NUM_OF_COLORS        8
#define MAX_NUM_SCREENS      5


/*******************************************************************************
 * Color Palette data structures
 *
 ******************************************************************************/


typedef struct _hsv {
    int hue;
    int saturation;
    int value;
} HSVset;

/****** Global Variables ********/

typedef struct {
   int                TypeOfMonitor[MAX_NUM_SCREENS];
   int                Depth[MAX_NUM_SCREENS];
   int                FgColor[MAX_NUM_SCREENS];
   Bool               UsePixmaps[MAX_NUM_SCREENS];
   Bool               DynamicColor[MAX_NUM_SCREENS];
   int                NumOfScreens;
   Atom               XA_CUSTOMIZE[MAX_NUM_SCREENS];
   Atom               XA_PIXEL_SET;
   Atom               XA_TYPE_MONITOR;
   Atom               XA_UPDATE;
   struct _palette    *pCurrentPalette[MAX_NUM_SCREENS];
} ColorSrv;
extern ColorSrv colorSrv; /* defined in SrvPalette.c */
extern intstr_t istrs; /* defined in SrvPalette.c */


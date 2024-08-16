/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtcolor:common.h	1.1.1.6"
#ifndef	_common_h
#define _common_h

#include <Xm/ColorObj.h>

#define HOME   		   "HOME"
#define USER_PALETTE_DIR   "/.palettes"
#define DEF_SYS_PDIR       "/usr/X/lib/palettes"
#define DEFAULT_MONOCHROME    "3"
#define DEFAULT_PALETTE    "12"
#define DEFAULT_16PALETTE    "37"
#define DEFAULT_GRAYSCALE_PALETTE    "14"
/* message catelog */
#define CATALOGFILE   "dtpalette"

/* defines for foreground colors */
#define DYNAMIC    0
#define BLACK      1
#define WHITE      2


/*
 *  Define a colorset as foreground, background, topshadow, bottomshadow
 *  and selectcolor (also known as arm color).
 */
typedef struct _colorset {
    XColor fg;
    XColor bg;
    XColor ts;
    XColor bs;
    XColor sc;
} ColorSet;
/*
 *  Structure which stores the palettes for the customizer
 */
typedef struct _palette {
      char *name;
      char *fname;
      char *directory;
      int num_of_colors;
      ColorSet color[MAX_NUM_COLORS];
      uchar_t flags[MAX_NUM_COLORS];
      short active;
      short inactive;
      short primary;
      short secondary;
      struct _palette *next;
      int item_position;
      int p_num;
} palettes;

typedef struct Intstr {
	char *blackwhite;
	char *whiteblack;
	char *white;
	char *black;
	char *prefix;
	char *suffix;
} intstr_t;

extern char *defpname[];

#define FOREGROUND	0x01
#define SELECT		0x02
#define TOPSHADOW	0x04
#define BOTTOMSHADOW	0x08
/* External Interface */
extern char *getsyspdir(void);
extern char *gethomepdir(void); 
extern char *getdefaultpalette(int); 
extern void SwitchAItoPS(palettes *);
extern void ReadInInfo(char *, int, palettes *, Display *, Colormap, int, 
		Boolean, int,  XmColorProc);
extern Boolean verifyname(int, int, char **, intstr_t);
extern char *getdefault(Display *, int, int);

#endif		/* _common_h */

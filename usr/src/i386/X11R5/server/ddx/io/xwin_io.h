/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/io/xwin_io.h	1.2"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

#ifndef HI386
#define HI386	1

#include "misc.h"

typedef struct _xqCursorRec {
    short x;	
    short y;	
} xqCursorRec, *xqCursorPtr;

typedef struct _DsplControl {
    void	(*saveScreen)();
    void	(*restoreScreen)();
} DsplControl;

extern int	modeNumber;
extern int	displayType;
extern unsigned long	lastEventTime;	/* Time (in ms.) of last event */

/* Display card/mode flags */

#define VDC750		0x2
#define STDEGA		0x4
#define VDC600		0x8
#define STDVGA		0x10
#define LINES350	0x20
#define LINES400	0x40
#define LINES480	0x80
#define LINES600	0x100
#define MONO1		0x200
#define MONOCHROME	0x400
#define GREYSCALE	0x800
#define ALTSTATIC	0x1000
#define STDMONITOR	0x2000
#define VGASTATIC	0x4000
#define LINES768	0x8000	/* from Intel */
#define DIRECT		0x10000	/* from Intel */

/* For use in aligning frame buffer boundaries up to 4K boundaries when
 * mapping them into user space via the KDMAPDISP ioctl() */
#define ALIGN_TO_4K(SIZE) (((SIZE) + 4095) & ~4095)

void i386QueryBestSize();
extern void      constrainXY2Scr();

#endif	/* HI386 */

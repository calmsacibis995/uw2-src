/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/ListDIconI.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.2
*/ 
/*   $RCSfile: ListDIconI.h,v $ $Revision: 1.8 $ $Date: 93/03/03 16:28:10 $ */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmListDIconP_h
#define _XmListDIconP_h

#ifdef __cplusplus
extern "C" {
#endif

#define XmLIST_DRAG_ICON_WIDTH_16 16
#define XmLIST_DRAG_ICON_HEIGHT_16 16
#define XmLIST_DRAG_ICON_X_HOT_16 0
#define XmLIST_DRAG_ICON_Y_HOT_16 0

#define XmLIST_DRAG_ICON_WIDTH_32 32
#define XmLIST_DRAG_ICON_HEIGHT_32 32
#define XmLIST_DRAG_ICON_X_HOT_32 0
#define XmLIST_DRAG_ICON_Y_HOT_32 0

static char XmLIST_DRAG_ICON_BITS_16[] = {
   0x00, 0x00, 0xfe, 0x0f, 0x02, 0x08, 0xf2, 0x09, 0x02, 0x08, 0xf2, 0x09,
   0x02, 0x08, 0xf2, 0x09, 0x02, 0x08, 0xf2, 0x09, 0x02, 0x08, 0xf2, 0x09,
   0x02, 0x08, 0xfe, 0x0f, 0x00, 0x00, 0x00, 0x00};

static char XmLIST_DRAG_ICON_MASK_BITS_16[] = {
   0xff, 0x1f, 0xff, 0x1f, 0xff, 0x1f, 0xff, 0x1f, 0xff, 0x1f, 0xff, 0x1f,
   0xff, 0x1f, 0xff, 0x1f, 0xff, 0x1f, 0xff, 0x1f, 0xff, 0x1f, 0xff, 0x1f,
   0xff, 0x1f, 0xff, 0x1f, 0xff, 0x1f, 0x00, 0x00};

static char XmLIST_DRAG_ICON_BITS_32[] = {
   0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0x01, 0x02, 0x00, 0x00, 0x01,
   0x02, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00, 0x01,
   0xc2, 0xff, 0x0f, 0x01, 0xc2, 0xff, 0x0f, 0x01, 0xc2, 0xff, 0x0f, 0x01,
   0x02, 0x00, 0x00, 0x01, 0xc2, 0xff, 0x0f, 0x01, 0xc2, 0xff, 0x0f, 0x01,
   0xc2, 0xff, 0x0f, 0x01, 0x02, 0x00, 0x00, 0x01, 0xc2, 0xff, 0x0f, 0x01,
   0xc2, 0xff, 0x0f, 0x01, 0xc2, 0xff, 0x0f, 0x01, 0x02, 0x00, 0x00, 0x01,
   0xc2, 0xff, 0x0f, 0x01, 0xc2, 0xff, 0x0f, 0x01, 0xc2, 0xff, 0x0f, 0x01,
   0x02, 0x00, 0x00, 0x01, 0xc2, 0xff, 0x0f, 0x01, 0xc2, 0xff, 0x0f, 0x01,
   0xc2, 0xff, 0x0f, 0x01, 0x02, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00, 0x01,
   0x02, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00, 0x01, 0xfe, 0xff, 0xff, 0x01,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


static char XmLIST_DRAG_ICON_MASK_BITS_32[] = {
   0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0xff, 0x03,
   0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0xff, 0x03,
   0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0xff, 0x03,
   0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0xff, 0x03,
   0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0xff, 0x03,
   0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0xff, 0x03,
   0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0xff, 0x03,
   0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0xff, 0x03,
   0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0xff, 0x03,
   0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0xff, 0x03,
   0xff, 0xff, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00};

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmListDIconP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)oldtext:TextPosP.h	1.3"
#endif

#ifndef _OlPositionP_h
#define _OlPositionP_h


#define _OL_POS_TAB_INIT_SIZE		128
#define _OL_POS_TAB_REALLOC_CHUNK	64

/*
** This table keeps tracks of the first position for each line in the buffer.
** These are wrapped lines not physical (newline of NULL terminated) lines.
** It was invented for fixes to the scrolling code.
*/
typedef struct {
  unsigned		lines;
  OlTextPosition	*pos;	/* dynamic array of pos.'s,indexed by line */
  unsigned		size;
} OlPositionTable;


extern void _OlPtBuild();
/*
  TextPaneWidget	self;
  OlTextPosition	pos;
*/

extern void _OlPtDestroy();
/*
  OlPositionTable	*pt;
*/

extern void _OlPtInitialize();
/* 
  OlPositionTable	*pt;
*/

extern unsigned _OlPtLineFromPos();
/*
  TextPaneWidget	self;
  OlTextPosition	pos;
*/

extern OlTextPosition _OlPtPosFromLine();
/*
  TextPaneWidget	self;
  unsigned		line;
*/

extern void _OlPtSetLine();
/*
  OlPositionTable	*pt;
  unsigned		line;
  OlTextPosition	pos;
*/

#endif

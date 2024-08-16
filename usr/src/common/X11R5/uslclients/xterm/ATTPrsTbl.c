/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:ATTPrsTbl.c	1.11"
#endif
/*
 ATTPrsTbl.c (C source file)
	Acc: 601052146 Tue Jan 17 09:55:46 1989
	Mod: 601053901 Tue Jan 17 10:25:01 1989
	Sta: 601053901 Tue Jan 17 10:25:01 1989
	Owner: 7007
	Group: 1985
	Permissions: 666
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/

#include "VTparse.h"

/*
	EHR3 - All AT&T special escape sequences are handled here
*/
int atttable[] = {
/*	NUL		SOH		STX		ETX	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	EOT		ENQ		ACK		BEL	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	BS		HT		NL		VT	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	NP		CR		SO		SI	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	DLE		DC1		DC2		DC3	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	DC4		NAK		SYN		ETB	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	CAN		EM		SUB		ESC	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	FS		GS		RS		US	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	SP		!		"		#	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	$		%		&		'	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	(		)		*		+	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	,		-		.		/	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	0		1		2		3	*/
CASE_OUTCURSES_STATE,		/* EHR3 - \E@0 - Not in curses */
CASE_INCURSES_STATE,		/* EHR3 - \E@1 - In curses */
CASE_IOCTLRECV_STATE,		/* EHR3 - \E@2... - Receive ioctl */
CASE_IOCTLREPLY_STATE,		/* EHR3 - \E@3... - Reply to ioctl */
/*	4		5		6		7	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	8		9		:		;	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	<		=		>		?	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	@		A		B		C	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	D		E		F		G	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	H		I		J		K	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	L		M		N		O	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	P		Q		R		S	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	T		U		V		W	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	X		Y		Z		[	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	\		]		^		_	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	`		a		b		c	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	d		e		f		g	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	h		i		j		k	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	l		m		n		o	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	p		q		r		s	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	t		u		v		w	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	x		y		z		{	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	|		}		~		DEL	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
};

/* EHR3 - Table of valid ioctls */
int ioctltable[] = {
/*	NUL		SOH		STX		ETX	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	EOT		ENQ		ACK		BEL	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	BS		HT		NL		VT	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	NP		CR		SO		SI	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	DLE		DC1		DC2		DC3	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	DC4		NAK		SYN		ETB	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	CAN		EM		SUB		ESC	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	FS		GS		RS		US	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	SP		!		"		#	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	$		%		&		'	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	(		)		*		+	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	,		-		.		/	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	0		1		2		3	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	4		5		6		7	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	8		9		:		;	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	<		=		>		?	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	@		A		B		C	*/
CASE_GROUND_STATE,
CASE_CONS_GET_STATE,
CASE_TIOCVTNAME_STATE,
CASE_GROUND_STATE,
/*	D		E		F		G	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	H		I		J		K	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	L		M		N		O	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	P		Q		R		S	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	T		U		V		W	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	X		Y		Z		[	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	\		]		^		_	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
/*	`		a		b		c	*/
CASE_GROUND_STATE,
CASE_KDDISPTYPE_STATE,
CASE_KDGKBENT_STATE,
CASE_KDSKBENT_STATE,
/*	d		e		f		g	*/
CASE_KDGKBMODE_STATE,
CASE_KDSKBMODE_STATE,
CASE_GIO_ATTR_STATE,
CASE_GIO_COLOR_STATE,
/*	h		i		j		k	*/
CASE_GIO_KEYMAP_STATE,
CASE_PIO_KEYMAP_STATE,
CASE_GIO_STRMAP_STATE,
CASE_PIO_STRMAP_STATE,
/*	l		m		n		o	*/
CASE_GETFKEY_STATE,
CASE_SETFKEY_STATE,
CASE_SW_B40X25_STATE,
CASE_SW_C40X25_STATE,
/*	p		q		r		s	*/
CASE_SW_B80X25_STATE,
CASE_SW_C80X25_STATE,
CASE_SW_EGAMONO80X25_STATE,
CASE_SW_ENHB40X25_STATE,
/*	t		u		v		w	*/
CASE_SW_ENHC40X25_STATE,
CASE_SW_ENHB80X25_STATE,
CASE_SW_ENHC80X25_STATE,
CASE_SW_ENHB80X43_STATE,
/*	x		y		z		{	*/
CASE_SW_ENHC80X43_STATE,
CASE_SW_MCAMODE_STATE,
CASE_CONS_CURRENT_STATE,
CASE_GROUND_STATE,
/*	|		}		~		DEL	*/
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
CASE_GROUND_STATE,
};
/* EHR3 - END Table of valid ioctls */
/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)proto:desktop/instcmd/keycomp.h	1.3"

#define STD_MAP "keymap"

/*
 * This table is used to translate keyboard scan codes to ASCII character
 * sequences for the AT386 keyboard/display driver.  It is the default table,
 * and may be changed with system calls.
 */
keyinfo_t	nop_key = {
	{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O 
};

struct {
	int	escape_code;
	int	sco_ext_key;
	int	usl_ext_key;
} usl_sco_extkeys[] = {
	0x0,	0x8c,	  -1,	  /* Extended Num '/' */
	0x1a,	0x54,	  -1,	  /* [ { or Extended Sys Request/PrtScrn */
	0x1c,	0x8d,	  0x74,	  /* enter key */
	0x1d,	0x80,	  0x73,	  /* right control key */
	0x2a,	-1,	  0x00,	  /* map to no key stroke */
	0x35,	0x8c,	  0x75,	  /* keypad '/'	key */
	0x36,	-1,	  0x00,	  /* map to no key stroke */
	0x37,	-1,	  0x54,	  /* print screen key */
	0x38,	0x81,	  0x72,	  /* right alt key */
	0x46,	-1,	  0x77,	  /* pause/break key */
	0x47,	0x84,	  0x7f,	  /* home key */
	0x48,	0x8a,	  0x78,	  /* up	arrow key */
	0x49,	0x86,	  0x6f,	  /* page up key */
	0x4b,	0x89,	  0x6b,	  /* left arrow	key */
	0x4d,	0x88,	  0x7d,	  /* right arrow key */
	0x4f,	0x85,	  0x7a,	  /* end key */
	0x50,	0x8b,	  0x55,	  /* down arrow	key */
	0x51,	0x87,	  0x7e,	  /* page down key */
	0x52,	0x82,	  0x7b,	  /* insert key	*/
	0x53,	0x83,	  0x79	  /* delete key	*/
	-1,	-1,	-1
};

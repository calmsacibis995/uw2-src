/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/fnt/codeset0.c	1.1"
#include <io/fnt/fnt.h>

char *codeset0registry = "JISX0201.1976";

char *codeset2registry = "JISX0201.1976";

unsigned char altcharsetrow0[96][16] = {
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
/* AT386 alternate character '3' */
{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10
 },
/* AT386 alternate character '4' */
{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
 0xf0, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
/* AT386 alternate character '?' */
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0xf0, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10
 },
/* AT386 alternate character '@' */
{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
/* AT386 alternate character 'A' */
{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
/* AT386 alternate character 'B' */
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0xff, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10
 },
/* AT386 alternate character 'C' */
{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
 0x1f, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10
 },
/* AT386 alternate character 'D' */
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
/* AT386 alternate character 'E' */
{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
 0xff, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
/* AT386 alternate character 'Y' */
{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
/* AT386 alternate character 'Z' */
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x1f, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x10, 0x10, 0x38, 0x38, 0x7c, 0x7c, 0xfe, 0xfe, 
 0x7c, 0x7c, 0x38, 0x38, 0x10, 0x10, 0x00, 0x00
 },
{0x92, 0x92, 0x44, 0x44, 0x92, 0x92, 0x44, 0x44, 
 0x92, 0x92, 0x44, 0x44, 0x92, 0x92, 0x00, 0x00
 },
{0x00, 0x88, 0x88, 0x88, 0xf8, 0x88, 0x88, 0x88, 
 0x00, 0x3e, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08
 },
{0x00, 0xf8, 0x80, 0x80, 0xf0, 0x80, 0x80, 0x80, 
 0x3e, 0x20, 0x20, 0x3c, 0x20, 0x20, 0x20, 0x00
 },
{0x00, 0x70, 0x88, 0x80, 0x80, 0x80, 0x88, 0x70, 
 0x00, 0x3c, 0x22, 0x22, 0x3c, 0x28, 0x24, 0x22
 },
{0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xf8, 
 0x00, 0x3e, 0x20, 0x20, 0x3c, 0x20, 0x20, 0x20
 },
{0x00, 0x00, 0x38, 0x44, 0x44, 0x44, 0x38, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x10, 0x10, 0x10, 0x10, 0xfe, 0x10, 
 0x10, 0x10, 0x10, 0x00, 0xfe, 0x00, 0x00, 0x00
 },
{0x00, 0x84, 0xc4, 0xa4, 0xa4, 0x94, 0x94, 0x8c, 
 0x84, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3e
 },
{0x00, 0x00, 0x88, 0x88, 0x88, 0x50, 0x50, 0x20, 
 0x00, 0x3e, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00
 },
{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0xf0, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x1f, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10
 },
{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
 0xff, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10
 },
{0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00
 },
{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
 0x1f, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10
 },
{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
 0xf0, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10
 },
{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0xff, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10
 },
{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10
 },
{0x00, 0x00, 0x00, 0x00, 0x02, 0x0c, 0x30, 0xc0, 
 0x30, 0x0c, 0x02, 0xfe, 0x00, 0xfe, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x80, 0x60, 0x18, 0x06, 
 0x18, 0x60, 0x80, 0xfe, 0x00, 0xfe, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 
 0x24, 0x24, 0x24, 0x24, 0x44, 0x84, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x02, 0x04, 0x08, 0xfe, 
 0x10, 0xfe, 0x20, 0x40, 0x80, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x0c, 0x12, 0x10, 0x10, 
 0x10, 0x7c, 0x10, 0x10, 0x3c, 0x52, 0x20, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
};

unsigned char *altcharsetdata[1]= { altcharsetrow0[0] };

unsigned char codeset0row0[96][16] = {
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x10, 
 0x10, 0x10, 0x10, 0x00, 0x00, 0x10, 0x38, 0x10
 },
{0x6c, 0x6c, 0x24, 0x24, 0x48, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x12, 0x12, 0x12, 0x7f, 0x24, 0x24, 0x24, 
 0x24, 0x24, 0xfe, 0x48, 0x48, 0x48, 0x48, 0x00
 },
{0x10, 0x38, 0x54, 0x92, 0x96, 0x90, 0x50, 0x38, 
 0x14, 0x12, 0xd2, 0x92, 0x94, 0x78, 0x10, 0x10
 },
{0x02, 0x62, 0x94, 0x94, 0x94, 0x98, 0x68, 0x10, 
 0x10, 0x2c, 0x32, 0x52, 0x52, 0x52, 0x8c, 0x80
 },
{0x00, 0x30, 0x48, 0x48, 0x48, 0x50, 0x20, 0x2e, 
 0x54, 0x54, 0x94, 0x88, 0x8c, 0x72, 0x00, 0x00
 },
{0xe0, 0xe0, 0x20, 0x20, 0xc0, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x02, 0x04, 0x08, 0x08, 0x10, 0x10, 0x10, 0x10, 
 0x10, 0x10, 0x10, 0x10, 0x08, 0x08, 0x04, 0x02
 },
{0x80, 0x40, 0x20, 0x20, 0x10, 0x10, 0x10, 0x10, 
 0x10, 0x10, 0x10, 0x10, 0x20, 0x20, 0x40, 0x80
 },
{0x00, 0x00, 0x00, 0x10, 0x38, 0x92, 0xd6, 0x38, 
 0xd6, 0x92, 0x38, 0x10, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10, 0xfe, 
 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0xe0, 0xe0, 0x20, 0x20, 0xc0
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x40, 0xe0, 0xe0, 0x40, 0x00
 },
{0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x10, 
 0x10, 0x10, 0x20, 0x20, 0x40, 0x40, 0x80, 0x80
 },
{0x00, 0x18, 0x24, 0x24, 0x42, 0x42, 0x42, 0x42, 
 0x42, 0x42, 0x42, 0x42, 0x24, 0x24, 0x18, 0x00
 },
{0x00, 0x10, 0x70, 0x10, 0x10, 0x10, 0x10, 0x10, 
 0x10, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x00, 0x00
 },
{0x00, 0x18, 0x24, 0x42, 0x62, 0x02, 0x04, 0x08, 
 0x08, 0x10, 0x20, 0x22, 0x42, 0x7e, 0x00, 0x00
 },
{0x00, 0x38, 0x44, 0x82, 0x82, 0x02, 0x04, 0x38, 
 0x04, 0x02, 0x82, 0x82, 0x44, 0x38, 0x00, 0x00
 },
{0x00, 0x08, 0x18, 0x28, 0x28, 0x48, 0x48, 0x88, 
 0x88, 0xfe, 0x08, 0x08, 0x08, 0x3c, 0x00, 0x00
 },
{0x00, 0xfc, 0x80, 0x80, 0x80, 0xb8, 0xc4, 0x82, 
 0x02, 0x02, 0xc2, 0x82, 0x44, 0x38, 0x00, 0x00
 },
{0x00, 0x3c, 0x42, 0x46, 0x80, 0x80, 0xb8, 0xc4, 
 0x82, 0x82, 0x82, 0x82, 0x44, 0x38, 0x00, 0x00
 },
{0x00, 0xfe, 0x82, 0x82, 0x04, 0x04, 0x04, 0x08, 
 0x08, 0x08, 0x08, 0x10, 0x10, 0x10, 0x10, 0x00
 },
{0x00, 0x38, 0x44, 0x82, 0x82, 0x82, 0x44, 0x38, 
 0x44, 0x82, 0x82, 0x82, 0x44, 0x38, 0x00, 0x00
 },
{0x00, 0x38, 0x44, 0x82, 0x82, 0x82, 0x82, 0x46, 
 0x3a, 0x02, 0x02, 0x82, 0x44, 0x38, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x38, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x38, 0x38, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x38, 0x00, 
 0x00, 0x00, 0x00, 0x38, 0x38, 0x18, 0x10, 0x30
 },
{0x02, 0x04, 0x04, 0x08, 0x08, 0x10, 0x10, 0x20, 
 0x20, 0x10, 0x10, 0x08, 0x08, 0x04, 0x04, 0x02
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00, 
 0x00, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x80, 0x40, 0x40, 0x20, 0x20, 0x10, 0x10, 0x08, 
 0x08, 0x10, 0x10, 0x20, 0x20, 0x40, 0x40, 0x80
 },
{0x00, 0x38, 0x44, 0x82, 0xc2, 0x02, 0x04, 0x04, 
 0x08, 0x10, 0x10, 0x00, 0x00, 0x10, 0x38, 0x10
 },
{0x00, 0x3c, 0x42, 0x82, 0x9a, 0xa6, 0xa2, 0xa2, 
 0xa2, 0xa6, 0x9a, 0x80, 0x42, 0x3c, 0x00, 0x00
 },
{0x00, 0x10, 0x28, 0x28, 0x28, 0x44, 0x44, 0x44, 
 0x44, 0x7c, 0x82, 0x82, 0x82, 0xc6, 0x00, 0x00
 },
{0x00, 0xf8, 0x44, 0x42, 0x42, 0x42, 0x44, 0x78, 
 0x44, 0x42, 0x42, 0x42, 0x42, 0xfc, 0x00, 0x00
 },
{0x00, 0x3a, 0x46, 0x42, 0x80, 0x80, 0x80, 0x80, 
 0x80, 0x80, 0x82, 0x42, 0x42, 0x3c, 0x00, 0x00
 },
{0x00, 0xf8, 0x44, 0x44, 0x42, 0x42, 0x42, 0x42, 
 0x42, 0x42, 0x42, 0x44, 0x44, 0xf8, 0x00, 0x00
 },
{0x00, 0xfe, 0x42, 0x42, 0x40, 0x48, 0x48, 0x78, 
 0x48, 0x48, 0x42, 0x42, 0x42, 0xfe, 0x00, 0x00
 },
{0x00, 0xfe, 0x42, 0x42, 0x40, 0x48, 0x48, 0x78, 
 0x48, 0x48, 0x40, 0x40, 0x40, 0xf0, 0x00, 0x00
 },
{0x00, 0x1a, 0x26, 0x42, 0x40, 0x80, 0x80, 0x8f, 
 0x82, 0x82, 0x82, 0x42, 0x66, 0x1a, 0x00, 0x00
 },
{0x00, 0xe7, 0x42, 0x42, 0x42, 0x42, 0x7e, 0x42, 
 0x42, 0x42, 0x42, 0x42, 0x42, 0xe7, 0x00, 0x00
 },
{0x00, 0xfe, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
 0x10, 0x10, 0x10, 0x10, 0x10, 0xfe, 0x00, 0x00
 },
{0x00, 0x1f, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
 0x02, 0x82, 0x82, 0x82, 0x44, 0x38, 0x00, 0x00
 },
{0x00, 0xe6, 0x44, 0x44, 0x48, 0x48, 0x70, 0x50, 
 0x48, 0x48, 0x44, 0x44, 0x42, 0xe3, 0x00, 0x00
 },
{0x00, 0xf0, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 
 0x40, 0x40, 0x42, 0x42, 0x42, 0xfe, 0x00, 0x00
 },
{0x00, 0x82, 0xc6, 0xaa, 0xaa, 0xaa, 0x92, 0x92, 
 0x92, 0x82, 0x82, 0x82, 0x82, 0xc6, 0x00, 0x00
 },
{0x00, 0x87, 0xc2, 0xa2, 0xa2, 0xa2, 0x92, 0x92, 
 0x92, 0x8a, 0x8a, 0x8a, 0x86, 0xc2, 0x00, 0x00
 },
{0x00, 0x38, 0x44, 0x82, 0x82, 0x82, 0x82, 0x82, 
 0x82, 0x82, 0x82, 0x82, 0x44, 0x38, 0x00, 0x00
 },
{0x00, 0xf8, 0x44, 0x42, 0x42, 0x42, 0x42, 0x44, 
 0x78, 0x40, 0x40, 0x40, 0x40, 0xf0, 0x00, 0x00
 },
{0x00, 0x38, 0x44, 0x44, 0x82, 0x82, 0x82, 0x82, 
 0x82, 0x82, 0xba, 0x44, 0x44, 0x38, 0x08, 0x06
 },
{0x00, 0xf8, 0x44, 0x42, 0x42, 0x42, 0x44, 0x78, 
 0x48, 0x44, 0x44, 0x44, 0x42, 0xe3, 0x00, 0x00
 },
{0x00, 0x34, 0x4c, 0x84, 0x80, 0x80, 0x60, 0x18, 
 0x04, 0x82, 0x82, 0x82, 0xc4, 0xb8, 0x00, 0x00
 },
{0x00, 0xfe, 0x92, 0x92, 0x10, 0x10, 0x10, 0x10, 
 0x10, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x00, 0x00
 },
{0x00, 0xe7, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 
 0x42, 0x42, 0x42, 0x42, 0x42, 0x3c, 0x00, 0x00
 },
{0x00, 0xc6, 0x82, 0x82, 0x82, 0x82, 0x44, 0x44, 
 0x44, 0x44, 0x28, 0x28, 0x10, 0x10, 0x00, 0x00
 },
{0x00, 0xc6, 0x82, 0x82, 0x82, 0x92, 0x92, 0x92, 
 0xaa, 0xaa, 0xaa, 0x44, 0x44, 0x44, 0x00, 0x00
 },
{0x00, 0xee, 0x44, 0x44, 0x28, 0x28, 0x10, 0x28, 
 0x28, 0x28, 0x44, 0x44, 0x82, 0xc6, 0x00, 0x00
 },
{0x00, 0xc6, 0x82, 0x44, 0x44, 0x44, 0x28, 0x28, 
 0x10, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x00, 0x00
 },
{0x00, 0xfe, 0x84, 0x88, 0x08, 0x10, 0x10, 0x10, 
 0x20, 0x20, 0x42, 0x42, 0x82, 0xfe, 0x00, 0x00
 },
{0x1e, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1e
 },
{0x82, 0x82, 0x44, 0x44, 0xfe, 0x28, 0x10, 0xfe, 
 0x10, 0x10, 0x10, 0x10, 0x10, 0x38, 0x00, 0x00
 },
{0xf0, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0xf0
 },
{0x10, 0x28, 0x44, 0x82, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe
 },
{0x30, 0x30, 0x20, 0x20, 0x10, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x02, 
 0x3e, 0x42, 0x82, 0x82, 0x86, 0x7b, 0x00, 0x00
 },
{0x00, 0xc0, 0x40, 0x40, 0x40, 0x78, 0x44, 0x42, 
 0x42, 0x42, 0x42, 0x42, 0x44, 0x78, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x3a, 0x46, 0x82, 
 0x80, 0x80, 0x80, 0x82, 0x42, 0x3c, 0x00, 0x00
 },
{0x00, 0x06, 0x04, 0x04, 0x04, 0x3c, 0x44, 0x84, 
 0x84, 0x84, 0x84, 0x84, 0x44, 0x3e, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x44, 0x82, 
 0xfe, 0x80, 0x80, 0x82, 0x42, 0x3c, 0x00, 0x00
 },
{0x00, 0x0e, 0x11, 0x10, 0x10, 0xfe, 0x10, 0x10, 
 0x10, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x44, 0x44, 
 0x44, 0x38, 0x40, 0x78, 0x84, 0x82, 0x82, 0x7c
 },
{0x00, 0xc0, 0x40, 0x40, 0x40, 0x5c, 0x62, 0x42, 
 0x42, 0x42, 0x42, 0x42, 0x42, 0xe7, 0x00, 0x00
 },
{0x18, 0x18, 0x00, 0x00, 0x00, 0x78, 0x08, 0x08, 
 0x08, 0x08, 0x08, 0x08, 0x08, 0xff, 0x00, 0x00
 },
{0x06, 0x06, 0x00, 0x00, 0x00, 0x3e, 0x02, 0x02, 
 0x02, 0x02, 0x02, 0x02, 0x82, 0x82, 0x44, 0x38
 },
{0x00, 0xc0, 0x40, 0x40, 0x40, 0x42, 0x44, 0x48, 
 0x58, 0x64, 0x44, 0x42, 0x42, 0xe3, 0x00, 0x00
 },
{0x00, 0x78, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 
 0x08, 0x08, 0x08, 0x08, 0x08, 0xff, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x6c, 0x92, 0x92, 
 0x92, 0x92, 0x92, 0x92, 0x92, 0xdb, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0xdc, 0x62, 0x42, 
 0x42, 0x42, 0x42, 0x42, 0x42, 0xe7, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x44, 0x82, 
 0x82, 0x82, 0x82, 0x82, 0x44, 0x38, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x44, 0x42, 
 0x42, 0x42, 0x42, 0x44, 0x78, 0x40, 0x40, 0xf0
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x44, 0x84, 
 0x84, 0x84, 0x84, 0x44, 0x3c, 0x04, 0x04, 0x1e
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0xec, 0x32, 0x22, 
 0x20, 0x20, 0x20, 0x20, 0x20, 0xfc, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x3a, 0x46, 0x42, 
 0x40, 0x3c, 0x02, 0x82, 0xc2, 0xbc, 0x00, 0x00
 },
{0x00, 0x00, 0x20, 0x20, 0x20, 0xfc, 0x20, 0x20, 
 0x20, 0x20, 0x20, 0x22, 0x22, 0x1c, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0xc6, 0x42, 0x42, 
 0x42, 0x42, 0x42, 0x42, 0x46, 0x39, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0xc6, 0x82, 0x82, 
 0x44, 0x44, 0x44, 0x28, 0x28, 0x10, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x92, 0x92, 0x92, 
 0x92, 0xaa, 0xaa, 0x44, 0x44, 0x44, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0xee, 0x44, 0x28, 
 0x28, 0x10, 0x28, 0x28, 0x44, 0xee, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0xe7, 0x42, 0x22, 
 0x24, 0x14, 0x08, 0x08, 0x10, 0x90, 0xa0, 0x40
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x44, 0x08, 
 0x08, 0x10, 0x10, 0x22, 0x42, 0xfe, 0x00, 0x00
 },
{0x06, 0x08, 0x08, 0x08, 0x08, 0x08, 0x10, 0x20, 
 0x10, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x06
 },
{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10
 },
{0xc0, 0x20, 0x20, 0x20, 0x20, 0x20, 0x10, 0x08, 
 0x10, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xc0
 },
{0x60, 0x92, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
};

unsigned char *codeset0data[1]={
codeset0row0[0]};

unsigned char codeset2row0[96][16] = {
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x20, 0x50, 0x50, 0x20
 },
{0x1e, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 
 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x3c
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x20, 0x20
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 
 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x7e, 0x02, 0x02, 0x02, 0x7e, 0x02, 
 0x06, 0x04, 0x0c, 0x08, 0x18, 0x30, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x02, 
 0x16, 0x14, 0x10, 0x10, 0x10, 0x10, 0x30, 0x20
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x04, 0x04, 
 0x08, 0x18, 0x68, 0x08, 0x08, 0x08, 0x08, 0x08
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x7e, 
 0x42, 0x42, 0x02, 0x04, 0x04, 0x08, 0x10, 0x20
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 
 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x7e, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 
 0x7e, 0x18, 0x18, 0x28, 0x28, 0x48, 0x08, 0x18
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x22, 0x16, 
 0x1a, 0x32, 0x48, 0x08, 0x08, 0x04, 0x04, 0x04
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 
 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x7e, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x04, 
 0x04, 0x04, 0x3c, 0x04, 0x04, 0x04, 0x3c, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x52, 
 0x4a, 0x22, 0x04, 0x04, 0x08, 0x08, 0x10, 0x20
 },
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0xff, 0x01, 0x12, 0x14, 0x14, 0x10, 
 0x10, 0x10, 0x10, 0x10, 0x20, 0x20, 0x40, 0x00
 },
{0x00, 0x02, 0x02, 0x04, 0x04, 0x08, 0x18, 0x28, 
 0xc8, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00
 },
{0x10, 0x10, 0x10, 0x7e, 0x42, 0x42, 0x42, 0x42, 
 0x02, 0x02, 0x04, 0x04, 0x08, 0x10, 0x20, 0x00
 },
{0x00, 0x00, 0x00, 0x7e, 0x10, 0x10, 0x10, 0x10, 
 0x10, 0x10, 0x10, 0x10, 0xff, 0x00, 0x00, 0x00
 },
{0x08, 0x08, 0x08, 0x08, 0xff, 0x08, 0x18, 0x18, 
 0x28, 0x28, 0x48, 0x48, 0x88, 0x08, 0x18, 0x00
 },
{0x10, 0x10, 0x10, 0x10, 0x7e, 0x12, 0x12, 0x12, 
 0x12, 0x12, 0x22, 0x22, 0x22, 0x42, 0x4e, 0x00
 },
{0x20, 0x20, 0x20, 0x16, 0x18, 0x30, 0xd0, 0x13, 
 0x0c, 0x38, 0x68, 0x08, 0x04, 0x04, 0x04, 0x04
 },
{0x10, 0x10, 0x10, 0x1e, 0x12, 0x22, 0x22, 0x44, 
 0x04, 0x04, 0x08, 0x08, 0x10, 0x20, 0x40, 0x00
 },
{0x20, 0x20, 0x20, 0x20, 0x3f, 0x44, 0x44, 0x84, 
 0x04, 0x08, 0x08, 0x08, 0x10, 0x10, 0x20, 0x00
 },
{0x00, 0x00, 0x00, 0x7e, 0x02, 0x02, 0x02, 0x02, 
 0x02, 0x02, 0x02, 0x02, 0x02, 0x7e, 0x00, 0x00
 },
{0x00, 0x24, 0x24, 0x24, 0xff, 0x24, 0x24, 0x24, 
 0x24, 0x04, 0x04, 0x08, 0x08, 0x10, 0x20, 0x00
 },
{0x00, 0x00, 0x20, 0x30, 0x10, 0x00, 0x41, 0x61, 
 0x22, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x00
 },
{0x00, 0x00, 0x7c, 0x04, 0x04, 0x04, 0x04, 0x08, 
 0x08, 0x18, 0x14, 0x24, 0x22, 0x42, 0x81, 0x00
 },
{0x00, 0x20, 0x20, 0x20, 0x2e, 0x32, 0xe2, 0x24, 
 0x20, 0x20, 0x20, 0x20, 0x20, 0x3e, 0x00, 0x00
 },
{0x00, 0x00, 0x42, 0x42, 0x42, 0x22, 0x22, 0x02, 
 0x04, 0x04, 0x04, 0x08, 0x08, 0x10, 0x20, 0x00
 },
{0x10, 0x10, 0x10, 0x1e, 0x12, 0x22, 0x32, 0x4a, 
 0x04, 0x04, 0x08, 0x08, 0x10, 0x20, 0x40, 0x00
 },
{0x00, 0x02, 0x04, 0x78, 0x08, 0x08, 0x08, 0xff, 
 0x08, 0x08, 0x10, 0x10, 0x10, 0x20, 0x40, 0x00
 },
{0x00, 0x00, 0x20, 0xa1, 0x91, 0x51, 0x52, 0x42, 
 0x02, 0x04, 0x04, 0x08, 0x08, 0x10, 0x20, 0x00
 },
{0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0xff, 0x08, 
 0x08, 0x08, 0x10, 0x10, 0x10, 0x20, 0x40, 0x00
 },
{0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x18, 0x14, 
 0x12, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00
 },
{0x00, 0x08, 0x08, 0x08, 0x08, 0xff, 0x08, 0x08, 
 0x08, 0x08, 0x10, 0x10, 0x10, 0x20, 0x40, 0x00
 },
{0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x7e, 0x02, 0x02, 0x04, 0x24, 0x28, 
 0x18, 0x08, 0x14, 0x12, 0x22, 0x20, 0x40, 0x00
 },
{0x10, 0x10, 0x10, 0x7e, 0x02, 0x04, 0x08, 0x18, 
 0x34, 0x52, 0x91, 0x10, 0x10, 0x10, 0x10, 0x00
 },
{0x00, 0x00, 0x02, 0x02, 0x04, 0x04, 0x04, 0x08, 
 0x08, 0x08, 0x10, 0x10, 0x20, 0x20, 0x40, 0x00
 },
{0x00, 0x00, 0x00, 0x24, 0x24, 0x24, 0x22, 0x22, 
 0x22, 0x22, 0x42, 0x41, 0x41, 0x81, 0x00, 0x00
 },
{0x00, 0x40, 0x40, 0x40, 0x46, 0x48, 0x70, 0x40, 
 0x40, 0x40, 0x40, 0x40, 0x20, 0x1e, 0x00, 0x00
 },
{0x00, 0x00, 0x7e, 0x02, 0x02, 0x02, 0x04, 0x04, 
 0x04, 0x08, 0x08, 0x10, 0x20, 0x40, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x20, 0x30, 0x30, 0x48, 0x48, 
 0x44, 0x84, 0x02, 0x02, 0x01, 0x00, 0x00, 0x00
 },
{0x00, 0x10, 0x10, 0x10, 0xff, 0x10, 0x10, 0x54, 
 0x52, 0x52, 0x52, 0x91, 0x91, 0x10, 0x30, 0x00
 },
{0x00, 0x00, 0x00, 0xff, 0x01, 0x02, 0x04, 0x28, 
 0x10, 0x18, 0x08, 0x08, 0x04, 0x04, 0x00, 0x00
 },
{0x00, 0x20, 0x18, 0x04, 0x00, 0x00, 0x20, 0x10, 
 0x0c, 0x00, 0x00, 0x20, 0x10, 0x08, 0x04, 0x00
 },
{0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x20, 0x24, 
 0x24, 0x24, 0x22, 0x4e, 0x72, 0x81, 0x00, 0x00
 },
{0x00, 0x02, 0x02, 0x02, 0x04, 0x24, 0x24, 0x18, 
 0x08, 0x1c, 0x14, 0x22, 0x20, 0x40, 0x80, 0x00
 },
{0x00, 0x00, 0x7e, 0x10, 0x10, 0x10, 0x10, 0xfe, 
 0x10, 0x10, 0x10, 0x10, 0x10, 0x1e, 0x00, 0x00
 },
{0x00, 0x20, 0x20, 0x23, 0x2d, 0x31, 0xd2, 0x12, 
 0x10, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00
 },
{0x00, 0x00, 0x00, 0x7c, 0x04, 0x04, 0x04, 0x04, 
 0x04, 0x04, 0x04, 0x04, 0xff, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x7e, 0x02, 0x02, 0x02, 0x02, 0x7e, 
 0x02, 0x02, 0x02, 0x02, 0x02, 0x7e, 0x00, 0x00
 },
{0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x7e, 0x02, 
 0x02, 0x02, 0x04, 0x04, 0x08, 0x10, 0x20, 0x00
 },
{0x00, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 
 0x22, 0x02, 0x04, 0x04, 0x04, 0x08, 0x10, 0x00
 },
{0x00, 0x08, 0x28, 0x28, 0x28, 0x28, 0x28, 0x29, 
 0x29, 0x29, 0x2a, 0x4a, 0x4c, 0x88, 0x00, 0x00
 },
{0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x21, 
 0x21, 0x22, 0x22, 0x24, 0x28, 0x30, 0x00, 0x00
 },
{0x00, 0x00, 0x00, 0x7e, 0x42, 0x42, 0x42, 0x42, 
 0x42, 0x42, 0x42, 0x42, 0x7e, 0x00, 0x00, 0x00
 },
{0x00, 0x00, 0x7e, 0x42, 0x42, 0x42, 0x02, 0x02, 
 0x02, 0x04, 0x04, 0x08, 0x10, 0x20, 0x00, 0x00
 },
{0x00, 0x00, 0x40, 0x20, 0x10, 0x01, 0x02, 0x02, 
 0x04, 0x04, 0x08, 0x10, 0x20, 0x40, 0x00, 0x00
 },
{0x28, 0x28, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
{0x10, 0x28, 0x28, 0x10, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 },
};

unsigned char *codeset2data[1]={
codeset2row0[0]};

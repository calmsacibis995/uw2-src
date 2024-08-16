/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/asciimap.h	1.7"
#ifndef _NET_NUC_NCP_ASCIIMAP_H
#define _NET_NUC_NCP_ASCIIMAP_H

#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/asciimap.h,v 2.51.2.1 1994/12/12 01:21:11 stevbam Exp $"

/*
 *  Netware Unix Client 
 */

/*
 *	Only two we care about currently
 */
#define DOS_CHAR		0x01
#define SYSV_UNIX_CHAR	0x02

/*
 *		ASCII character table with bit-field of compatibility
 */
static unsigned char asciiTab[128] =
{
	SYSV_UNIX_CHAR,				/* nul */
	SYSV_UNIX_CHAR,				/* soh */
	SYSV_UNIX_CHAR,				/* stx */
	SYSV_UNIX_CHAR,				/* etx */
	SYSV_UNIX_CHAR,				/* eot */
	SYSV_UNIX_CHAR,				/* enq */
	SYSV_UNIX_CHAR,				/* ack */
	SYSV_UNIX_CHAR,				/* bel */
	SYSV_UNIX_CHAR,				/* bs */
	SYSV_UNIX_CHAR,				/* ht */
	SYSV_UNIX_CHAR,				/* nl */
	SYSV_UNIX_CHAR,				/* vt */
	SYSV_UNIX_CHAR,				/* np */
	SYSV_UNIX_CHAR,				/* cr */
	SYSV_UNIX_CHAR,				/* so */
	SYSV_UNIX_CHAR,				/* si */
	SYSV_UNIX_CHAR,				/* dle */
	SYSV_UNIX_CHAR,				/* dc1 */
	SYSV_UNIX_CHAR,				/* dc2 */
	SYSV_UNIX_CHAR,				/* dc3 */
	SYSV_UNIX_CHAR,				/* dc4 */
	SYSV_UNIX_CHAR,				/* nak */
	SYSV_UNIX_CHAR,				/* syn */
	SYSV_UNIX_CHAR,				/* etb */
	SYSV_UNIX_CHAR,				/* can */
	SYSV_UNIX_CHAR,				/* em */
	SYSV_UNIX_CHAR,				/* sub */
	SYSV_UNIX_CHAR,				/* esc */
	SYSV_UNIX_CHAR,				/* fs */
	SYSV_UNIX_CHAR,				/* gs */
	SYSV_UNIX_CHAR,				/* rs */
	SYSV_UNIX_CHAR,				/* us */
	SYSV_UNIX_CHAR,				/* space */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* ! */
	SYSV_UNIX_CHAR,				/* " */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* # */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* $ */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* % */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* & */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* ' */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* ( */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* ) */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* * */	
	SYSV_UNIX_CHAR,				/* + */
	SYSV_UNIX_CHAR,				/* , */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* - */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* . */
	SYSV_UNIX_CHAR,				/* / */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* 0 */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* 1 */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* 2 */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* 3 */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* 4 */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* 5 */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* 6 */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* 7 */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* 8 */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* 9 */
	SYSV_UNIX_CHAR,				/* : */
	SYSV_UNIX_CHAR,				/* ; */
	SYSV_UNIX_CHAR,				/* < */
	SYSV_UNIX_CHAR,				/* = */
	SYSV_UNIX_CHAR,				/* > */
	SYSV_UNIX_CHAR,				/* ? */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* @ */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* A */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* B */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* C */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* D */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* E */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* F */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* G */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* H */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* I */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* J */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* K */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* L */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* M */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* N */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* O */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* P */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* Q */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* R */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* S */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* T */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* U */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* V */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* W */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* X */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* Y */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* Z */
	SYSV_UNIX_CHAR,				/* [ */
	SYSV_UNIX_CHAR,				/* \ */
	SYSV_UNIX_CHAR,				/* ] */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* ^ */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* _ */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* ` */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* a */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* b */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* c */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* d */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* e */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* f */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* g */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* h */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* i */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* j */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* k */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* l */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* m */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* n */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* o */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* p */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* q */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* r */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* s */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* t */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* u */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* v */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* w */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* x */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* y */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* z */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* { */
	SYSV_UNIX_CHAR,				/* | */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* } */
	SYSV_UNIX_CHAR|DOS_CHAR,	/* ~ */
	SYSV_UNIX_CHAR,				/* del */
};

#endif /* _NET_NUC_NCP_ASCIIMAP_H */

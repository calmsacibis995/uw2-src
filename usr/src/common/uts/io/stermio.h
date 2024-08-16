/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_STERMIO_H	/* wrapper symbol for kernel use */
#define _IO_STERMIO_H	/* subject to change without notice */

#ident	"@(#)kern:io/stermio.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * ioctl commands for control channels
 */
#define STSTART		1	/* start protocol */
#define STHALT		2	/* cease protocol */
#define STPRINT		3	/* assign device to printer */
#define STENABLE	4	/* enable polling */
#define STDISABLE	5	/* disable polling */
#define STPOLL		6	/* set polling rate */
#define STCNTRS		7	/* poke for status reports */
#define STTCHAN		8	/* set trace channel number */

/*
 * ioctl commands for terminal and printer channels
 */
#define STGET	(('Z'<<8)|0)	/* get line options */
#define STSET	(('Z'<<8)|1)	/* set line options */
#define	STTHROW	(('Z'<<8)|2)	/* throw away queued input */
#define	STWLINE	(('Z'<<8)|3)	/* get synchronous line # */
#define STTSV	(('Z'<<8)|4)	/* get all line information */

struct stio {
	unsigned short	ttyid;
	char		row;
	char		col;
	char		orow;
	char		ocol;
	char		tab;
	char		aid;
	char		ss1;
	char		ss2;
	unsigned short	imode;
	unsigned short	lmode;
	unsigned short	omode;
};

/*
**	Mode Definitions.
*/
#define	STFLUSH	00400	/* FLUSH mode; lmode */
#define	STWRAP	01000	/* WRAP mode; lmode */
#define	STAPPL	02000	/* APPLICATION mode; lmode */

struct sttsv {
	char	st_major;
	short	st_pcdnum;
	char	st_devaddr;
	int	st_csidev;
};

struct stcntrs {
	char	st_lrc;
	char	st_xnaks;
	char	st_rnaks;
	char	st_xwaks;
	char	st_rwaks;
	char	st_scc;
};

/* trace message definitions */

#define LOC	113	/* loss of carrier */


#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_STERMIO_H */

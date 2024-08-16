/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)head.usr:dial.h	1.2.6.4"

#ifndef _DIAL_H
#define _DIAL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef  _SYS_TERMIO_H
#include <sys/termio.h>
#endif

/* uucico routines need these */
/*#define DIAL */

/* The following are no longer used by dial() and may be out of date.	*/
/* They are included here only to maintain source compatibility.	*/
#define STANDALONE
#define DEVDIR	"/dev/"				/* device path		*/
#define LOCK	"/var/spool/uucp/LCK.."		/* lock file semaphore	*/
#define DVC_LEN	80	/* max NO of chars in TTY-device path name	*/
/* End of unused definitions						*/

		/* error mnemonics */

#define	TRUE	1
#define FALSE	0
#define INTRPT	(-1)	/* interrupt occured */
#define D_HUNG	(-2)	/* dialer hung (no return from write) */
#define NO_ANS	(-3)	/* no answer (caller script failed) */
#define ILL_BD	(-4)	/* illegal baud-rate */
#define A_PROB	(-5)	/* acu problem (open() failure) */
#define L_PROB	(-6)	/* line problem (open() failure) */
#define NO_Ldv	(-7)	/* can't open Devices file */
#define DV_NT_A	(-8)	/* requested device not available */
#define DV_NT_K	(-9)	/* requested device not known */
#define NO_BD_A	(-10)	/* no device available at requested baud */
#define NO_BD_K	(-11)	/* no device known at requested baud */
#define DV_NT_E (-12)	/* requested speed does not match */
#define BAD_SYS (-13)	/* system not in Systems file */
#define CS_PROB (-14)	/* connection server related error */
#define DV_W_TM	(-15)	/* wrong time to call */

typedef struct{
	int	version;	/* for future modifications to structure format */
	char	*service;	/* name of service to use(default = "cu") */
#ifndef __cplusplus
	char	*class;		/* "class" of device to use */
#else
	char	*dev_class;	/* "class" of device to use */
#endif
	char	*protocol;	/* returns the protocol string for the connection made */
	char	*reserved1;	/* reserved for future expansion */
} CALL_EXT;

typedef struct {
	struct termio *attr;	/* ptr to termio attribute struct */
	int	baud;		/* unused */
	int	speed;		/* 212A modem: low=300, high=1200 */
	char	*line;		/* device name for out-going line */
	char	*telno;		/* ptr to tel-no/system name string */
	int	modem;		/* unused */
	char	*device;	/* pointer to a CALL_EXT structure */
				/* this was unused previously */
	int	dev_len;	/* unused */
} CALL;

#if defined(__STDC__)

extern int dial(CALL);
extern void undial(int);

#else

extern int dial();
extern void undial();

#endif

#ifdef __cplusplus
}
#endif

#endif 	/* _DIAL_H */

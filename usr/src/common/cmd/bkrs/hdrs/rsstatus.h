/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/hdrs/rsstatus.h	1.4.5.3"
#ident  "$Header: rsstatus.h 1.2 91/06/21 $"

/* This file contains information pertaining to rsstatus table and command */

/* name of the table */
#define	RS_STATLOG	(unsigned char *)"rsstatus.tab"

/* FIELD names */
#define	RST_JOBID	(unsigned char *)"jobid"
#define	RST_TYPE	(unsigned char *)"type"
#define	RST_OBJECT	(unsigned char *)"object"
#define	RST_FDATE	(unsigned char *)"fdate"
#define	RST_TARGET	(unsigned char *)"target"
#define	RST_REFSNAME	(unsigned char *)"refsname"
#define	RST_REDEV	(unsigned char *)"redev"
#define	RST_MUID	(unsigned char *)"muid"
#define	RST_UID	(unsigned char *)"uid"
#define	RST_METHOD	(unsigned char *)"method"
#define	RST_MOPTION	(unsigned char *)"moption"
#define	RST_DGROUP	(unsigned char *)"dgroup"
#define	RST_DDEVICE	(unsigned char *)"ddevice"
#define	RST_DLABEL	(unsigned char *)"dlabel"
#define	RST_TLABEL	(unsigned char *)"tlabel"
#define	RST_DCHAR	(unsigned char *)"dchar"
#define	RST_ARCHDATE	(unsigned char *)"archdate"
#define	RST_TMSTATE	(unsigned char *)"tmstate"
#define	RST_TMDATE	(unsigned char *)"tmdate"
#define	RST_TMONAME	(unsigned char *)"tmoname"
#define	RST_TMODEV	(unsigned char *)"tmodev"
#define	RST_TMSTIMULUS	(unsigned char *)"tmstimulus"
#define	RST_TMSUCCEEDED	(unsigned char *)"tmsucceeded"
#define	RST_STATUS	(unsigned char *)"status"
#define	RST_EXPLANATION	(unsigned char *)"explanation"

#define	R_RSSTATUS_F (unsigned char *)\
	"jobid:type:object:fdate:target:refsname:redev:muid:uid:method:moption:dgroup:ddevice:dlabel:tlabel:dchar:archdate:tmstate:tmdate:tmoname:tmodev:tmstimulus:tmsucceeded:status:explanation"

/* Values for status field */
#define	RST_ACTIVE	(unsigned char *)"active"
#define	RST_PENDING	(unsigned char *)"pending"
#define	RST_FAILED	(unsigned char *)"not_completed"
#define	RST_SUCCESS	(unsigned char *)"completed"
#define RST_MKFSFAILED  (unsigned char *)"mkfsfailed"

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/tlitest.h	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW4U_SPX_TEST_TLI_TLITEST_H  /* wrapper symbol for kernel use */
#define _NET_NW4U_SPX_TEST_TLI_TLITEST_H  /* subject to change without notice */

#ident	"$Id: tlitest.h,v 1.3 1994/08/05 14:41:19 meb Exp $"
/*********************************************************************
*
*	Program Name:	
*	File Name:		tlitest.h
*	Date Created:	02/22/90
*	Version:			1.0
*	Programmer(s):	Rick Johnson
*	Purpose:
*	Modifications: (When, Who, What, Why)
*
*	COPYRIGHT (c) 1990 by Novell, Inc.  All Rights Reserved.
*
*********************************************************************/

/*--------------------------------------------------------------------
*	Include Files
*-------------------------------------------------------------------*/
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tiuser.h>

#ifdef NWU
#include <net/nw/nwtdr.h>
#endif

/*--------------------------------------------------------------------
*	ANSI Prototypes
*-------------------------------------------------------------------*/
extern int t_accept(int fildes, int resfd, struct t_call *call);
extern char *t_alloc(int fildes, int struct_type, int fields);
extern int t_bind(int fildes, struct t_bind *req, struct t_bind *ret);
extern int t_close(int fildes);
extern int t_connect(int fildes, struct t_call *sndcall,
            struct t_call *rcvcall);
/*
extern void t_error(char *errmsg); 
*/
extern int t_free(char *ptr, int struct_type);
extern int t_getinfo(int fildes, struct t_info *info);
extern int t_getstate(int fildes);
extern int t_listen(int fildes, struct t_call *call);
extern int t_look(int fildes);
extern int t_open(char *path, int oflag, struct t_info *info);
extern int t_optmgmt(int fildes, struct t_optmgmt *req,
            struct t_optmgmt *ret);
extern int t_rcv(int fildes, char *buf, unsigned nbytes, int *flags);
extern int t_rcvconnect(int fildes, struct t_call *call);
extern int t_rcvdis(int fildes, struct t_discon *discon);
extern int t_rcvrel(int fildes);
extern int t_rcvudata(int fildes, struct t_unitdata *unitdata, int *flags);
extern int t_rcvuderr(int fildes, struct t_uderr *uderr);
extern int t_snd(int fildes, char *buf, unsigned nbytes, int flags);
extern int t_snddis(int fildes, struct t_call *call);
extern int t_sndrel(int fildes);
extern int t_sndudata(int fildes, struct t_unitdata *unitdata);
extern int t_sync(int fildes);
extern int t_unbind(int fildes);

/*--------------------------------------------------------------------
*	Constant Definitions
*-------------------------------------------------------------------*/

#define BRANDX		"/dev/brandx"
#define IPX			"/dev/ipx"
#define SPX			"/dev/nspx"
#define SPX2		"/dev/nspx2"
#define NBDG		"/dev/nbdg"		/* NetBios Datagram */
#define NBIO		"/dev/nbio"		/* NetBios Session */
#define IP			"/dev/ip"

#define MAXSOLOMAJOR	8	/* maximum number of case statements in main */
#define MAXSRVRMAJOR	13
#define MAXCLNTMAJOR	13

#define MAXSTACKS 6	/* number of protocol stacks currently implemented */

#define ADDRSIZE	12
#define ASYNC		(O_RDWR|O_NDELAY)
#define CALLFMT		"%-13s"
#define DATA_A          "Data: 0123"
#define DATA_B          "456789"
#define DATA		"Data: 0123456789"
#define DATASIZE	80
#define ERRFMT		"%-12s"
#define EVNTFMT		"%s"
#define MAXCONN		5
#define MAXDATA		2048
#define MAXOPT		4
#define MSGSIZE		80
#define OKFMT		"%2s\n"
#define SOCKET		"\x44\x44"
#define STATEFMT	"%-11s"
#define SYNC		O_RDWR
#define T_DUMMY		0x80
#define T_NOEVENT	0
#define T_UNINIT	0
#define TNOERR		0
#define TNUMFMT		"%s\n"

#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif

#define UNINIT		0x01
#define UNBND		0x02
#define IDLE		0x04
#define OUTCON		0x08
#define INCON		0x10
#define DATAXFER	0x20
#define OUTREL		0x40
#define INREL		0x80

/*--------------------------------------------------------------------
*	Type Definitions
*-------------------------------------------------------------------*/
typedef struct t_bind		tbind;
typedef struct t_bind		*tbindptr;
typedef struct t_call		tcall;
typedef struct t_call		*tcallptr;
typedef struct t_discon		tdisc;
typedef struct t_discon		*tdiscptr;
typedef struct t_info		tinfo;
typedef struct t_info		*tinfoptr;
typedef struct t_optmgmt	toptm;
typedef struct t_optmgmt	*toptmptr;
typedef struct t_uderr		tuderr;
typedef struct t_uderr		*tuderrptr;
typedef struct t_unitdata	tudata;
typedef struct t_unitdata	*tudataptr;
typedef struct test			*testptr;
typedef struct test
{
	int testnum;
	int verbose;
	testptr next;
} testrec;


/*--------------------------------------------------------------------
*	Global Variables
*-------------------------------------------------------------------*/
char			MuchoData[MAXDATA];
testptr			Seqptr;
int				Sequenced;
uint8			Srvraddr[ADDRSIZE];
int				Testnum;
int				Verbose;
int				ProtoStk;
int				MaxMajors;
int				Repeat;
char			Net1[4];
char			Net2[4];
int				Numclnts;
int				t_errno;	/* TEB */

/*--------------------------------------------------------------------
*	Function Prototypes for major test functions
*-------------------------------------------------------------------*/
void	acceptclnt (char *protocol, int mode, int major);
void	acceptsrvr (char *ps1, char *ps2, int mode, int major);
void	connectclnt (char *ps1, char *ps2, int mode, int major);
void	connectsrvr (char *protocol, int mode, int major);
void 	eventsrvr (char *ps1, char * ps2, int mode, int major);
void 	eventclnt (char *ps1, char * ps2, int mode, int major);
void	getinfo (char *protocol, int mode, int major);
void	getstate (char *protocol, int mode, int major);
void	listenclnt (char *ps1, char *ps2, int mode, int major);
void	listensrvr (char *ps1, char *ps2, int mode, int major);
void	lookclnt (char *ps1, char *ps2, int mode, int major);
void	looksrvr (char *ps1, char *ps2, int mode, int major);
void	maxconnclnt (char *protocol, int mode,int major);
void	maxconnsrvr (char *protocol, int mode, int major);
void	optmgmtclnt (char *ps1, int mode, int major);
void	optmgmtsrvr (char *protocol, int mode, int major);
void	rcvconnectclnt (char *ps1,char *ps2,int major);
void	rcvconnectsrvr (char *protocol, int major);
void	sndrcvclnt (char *ps1, char *ps2, int mode, int major);
void	sndrcvsrvr (char *ps1, char *ps2, int mode, int major);
void	sndrcvdisclnt (char *ps1, char *ps2, int mode, int major);
void	sndrcvdissrvr (char *ps1, char *ps2, int mode, int major);
void	sndrcvrelclnt (char *ps1, char *ps2, int mode, int major);
void	sndrcvrelsrvr (char *ps1, char *ps2, int mode, int major);
void	sndrcvudataclnt (char *ps1, char *ps2, int mode, int major);
void	sndrcvudatasrvr (char *ps1, char *ps2, int mode, int major);
void	testalloc (char *protocol, int mode, int major);
void	testbind (char *protocol, int mode, int major);
void	testclose (char *protocol, int mode, int major);
void	testerror (char *protocol, int mode, int major);
void	testfree (char *protocol, int mode, int major);
void	testopen (char *protocol, int mode, int major);
void	unbindclnt (char *ps1, char *ps2, int mode, int major);
void	unbindsrvr (char *protocol, int mode, int major);


/*--------------------------------------------------------------------
*	Function prototypes for routines found in tlifunc.c
*-------------------------------------------------------------------*/
void	relinquish(void);
void	showProtoStk(char *prefix, int mode);
testptr addtest (int testnum);
void	badopt (int argc, char **argv);
void	checkstate (int fd, int expected);
void	checktbind (tbindptr bind);
void	checktcall (tcallptr call);
void	checktdisc (tdiscptr disc);
void	checktinfo (tinfoptr info);
void	checktoptm (toptmptr optm);
void	checktudata (tudataptr udata);
void	checktuderr (tuderrptr uderr);
void	cmdlineargs (int argc, char *argv[]);
void	errcheck (int fd, int status, int experr, short expstate);
int	MyCallBack(int fd, int event, void *param);
int	nexttest (void);
void	panicbutton (int fd, int state);
void	pause (void);
char *progname (char *argv0);
void	showerr (int error);
void	showmsg (char *format, char *msg);
void	showstate (short statemask);
void	showtnum (int major, int minor);
short statelookup (int fd);
void	tliaccept (int fd, int rfd, tcallptr call, int experr, int expstate,
					  int rexpstate);
char	*tlialloc (int fd, int type, int fields, int experr, int expstate);
void	tlibind (int fd, tbindptr req, tbindptr ret, int experr, int expstate);
void	tliclose (int fd, int experr, int expstate);
void	tliconnect (int fd, tcallptr sndcall, tcallptr rcvcall, int experr,
						int expstate);
void	tlierror (int fd, char *errmsg, int expstate);
void	tlifree (int fd, char *ptr, int type, int experr, int expstate);
void	tligetinfo (int fd, tinfoptr info, int experr, int expstate);
void	tligetstate (int fd, int experr, int expstate);
void	tlilisten (int fd, tcallptr call, int experr, int expstate);
void	tlilook (int fd, int experr, int expstate, int expevent);
int	tliopen (char *path, int oflag, tinfoptr info, int experr,
					int expstate);
void	tlioptmgmt (int fd, toptmptr req, toptmptr ret, int experr,
						int expstate);
void	tlircv (int fd, char *buf, unsigned nbytes, int *flags, int experr,
				  int expstate);
void	tlircvconnect (int fd, tcallptr call, int experr, int expstate);
void	tlircvdis (int fd, tdiscptr discon, int experr, int expstate);
void	tlircvrel (int fd, int experr, int expstate);
void	tlircvudata (int fd, tudataptr unitdata, int *flags, int experr,
						 int expstate);
void	tlircvuderr (int fd, tuderrptr uderr, int experr, int expstate);
void	tlisnd (int fd, char *buf, unsigned nbytes, int flags, int experr,
				  int expstate);
void	tlisnddis (int fd, tcallptr call, int experr, int expstate);
void	tlisndrel (int fd, int experr, int expstate);
void	tlisndudata (int fd, tudataptr unitdata, int experr, int expstate);
void	tlisync (int fd, int experr, int expstate);
void	tliunbind (int fd, int experr, int expstate);
int		kbhit( void );

#endif /* _NET_NW4U_SPX_TEST_TLI_TLITEST_H */

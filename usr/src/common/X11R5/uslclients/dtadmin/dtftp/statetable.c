/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:statetable.c	1.1.2.1"
#endif

#include <stdio.h>
#include "states.h"

extern void	DoDirCmd ();
extern void	StartCd ();
extern void	DeleteError ();
extern void	ResetDirFlag ();
extern void	BadIdleTime ();
extern void	GetIdleTime ();
extern void	Traverse ();
extern void	EndPut ();
extern void	NotEmpty ();
extern void	OutputDir ();
extern void	PwdError ();
extern void	DirError ();
extern void	CntPut ();
extern void	ChkCd ();
extern void	ChkDir ();
extern void	GatherNames ();
extern void	GetPutWarn ();
extern void	GetError ();
extern void	RenameOk ();
extern void	RenameError ();
extern void	PutError ();
extern void	LoginOk ();
extern void	Hash ();
extern void	Invalid ();
extern void	ChkStatus ();
extern void	XConnect ();
extern void	Reconnect ();
extern void	Connected ();
extern void	SetPwd ();
extern void	DisplayDir ();
extern void	Open ();
extern void	GetUserPassword ();
extern void	Ret500 ();
extern void	Ret400 ();

StateTable DirTable[] = {
    /* 100	200	300	400	500	any	ftp	s1   Cancel */
	 0,	 1,	 0,	 0,	 2,	 0,	 0,	 7,	 0,
/*0*/	NULL, SetPwd,	NULL,	NULL, PwdError,	NULL,	NULL,	NULL,	NULL,
	"500 Command not understood",
	 0,	 0,	 0,	 0,	 0,	 0,	 3,	 0,	 0,
/*1*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL, OutputDir,NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*2*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,ResetDirFlag,NULL,	NULL,
	NULL,
	 0,	 4,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
/*3*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
	 5,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
/*4*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
	 0,	 6,	 0,	 0,	 0,	 0,	 0,	 5,	 0,
/*5*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	"#",
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*6*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,DisplayDir,NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 2,	 0,	 0,	 8,	 0,
/*7*/	NULL,	NULL,	NULL,	NULL, PwdError,	NULL,	NULL,	NULL,	NULL,
	"PWD command not recognized",
	 0,	 1,	 0,	 0,	 2,	 0,	 0,	 0,	 0,
/*8*/	NULL, SetPwd,	NULL,	NULL, PwdError,	NULL,	NULL,	NULL,	NULL,
	NULL,
};


StateTable CdTable[] = {
    /* 100	200	300	400	500	any	ftp	s1   Cancel */
	 0,	 1,	 0,	 0,	 2,	 0,	 0,	 3,	 0,
/*0*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL, XConnect, NULL,
	"Not connected.",
	 0,	 1,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*1*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL, DoDirCmd,	NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*2*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL, DoDirCmd,	NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*3*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL, Reconnect,NULL,	NULL,
	NULL,
};

/*
 * If the type command returns:
 *
 *	421 Timeout (30 seconds): closing control connection.
 *
 * then the connection to the host has disconnected.
 */
StateTable TypeTable[] = {
    /* 100	200	300	400	500	any	ftp	s1   Cancel */
	 0,	 1,	 0,	  1,	0,	 0,	 0,	 1,	 0,
/*0*/	NULL,	NULL,	NULL, XConnect,	NULL,	NULL,	NULL, XConnect, NULL,
	"Not connected.",
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*1*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL, Reconnect,NULL,	NULL,
	NULL,
};

StateTable FtpTable[] = {
    /* 100	200	300	400	500	any	ftp	s1   Cancel */
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*0*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	Open,	NULL,	NULL,
	NULL,
};

StateTable UserTable[] = {
    /* 100	200	300	400	500	any	ftp	s1   Cancel */
	 0,	 3,	 1,	 2,	 2,	 0,	 0,	 0,	 0,
/*0*/	NULL,	NULL,	NULL,	Ret500,	Ret500,	NULL,	NULL,	NULL,	NULL,
	NULL,
	 0,	 3,	 0,	 2,	 2,	 0,	 0,	 0,	 0,
/*1*/	NULL,	NULL,	NULL,	Ret500,	Ret500,	NULL,	NULL,	NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 2,	 2,	 0,	 0,	 4,	 0,
/*2*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	"Login failed.",
	 0,	 3,	 0,	 0,	 0,	 3,	-1,	 0,	 0,
/*3*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	LoginOk,NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*4*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
};

StateTable OpenTable[] = {
    /* 100	200	300	400	500	any	ftp	s1   Cancel */
	 0,	 0,	 0,	 0,	 0,	 2,	 0,	 1,	 2,
/*0*/	NULL,	NULL,	NULL,	NULL,	NULL,	Ret400,	NULL,Connected,	NULL,
	"Connected to",
	 0,	 1,	 0,	 2,	 2,	 1,	-1,	 0,	 0,
/*1*/	NULL,	NULL,	NULL,	Ret400,	Ret400,	NULL,	GetUserPassword,NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*2*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
};

StateTable GetTable[] = {
    /* 100	200	300	400	500	any	ftp	s1   Cancel */
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 1,	 6,
/*0*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL, Invalid,
	"local: ",
	 0,	 2,	 0,	 0,	 0,	 7,	 0,	 0,	 6,
/*1*/	NULL,	NULL,	NULL,	NULL,	NULL, GetError, NULL,	NULL, Invalid,
	NULL,
	 3,	 0,	 0,	 7,	 7,	 0,	 0,	 0,	 6,
/*2*/	NULL,	NULL,	NULL, GetError,GetError,NULL,	NULL,	NULL, Invalid,
	NULL,
	 0,	 4,	 0,	 0,	 0,	 0,	 0,	 3,	 6,
/*3*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	Hash, Invalid,
	"#",
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 5,	 6,
/*4*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL, Invalid,
	" bytes received in ",
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*5*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 6,	 0,	 8,	 0,
/*6*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL, GetPutWarn,NULL,
	"?Invalid command",
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 6,
/*7*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL, Invalid,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*8*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,ChkStatus,	NULL,	NULL,
	NULL,
};

StateTable PutTable[] = {
    /* 100	200	300	400	500	any	ftp	s1   Cancel */
	 0,	 9,	 0,	 0,	10,	 0,	 0,	 0,	 6,
/*0*/	NULL, DirError,	NULL,	NULL,	ChkCd,	NULL,	NULL,	NULL, Invalid,
	NULL,
	 0,	 2,	 0,	 0,	 0,	 7,	 0,	 0,	 6,
/*1*/	NULL,	NULL,	NULL,	NULL,	NULL, PutError, NULL,	NULL, Invalid,
	NULL,
	 3,	 0,	 0,	 7,	 7,	 0,	 0,	 0,	 6,
/*2*/	NULL,	NULL,	NULL, PutError,PutError,NULL,	NULL,	NULL, Invalid,
	"#",
	 0,	 4,	 0,	 0,	 0,	 3,	 0,	 3,	 6,
/*3*/	NULL, EndPut,	NULL,	NULL,	NULL,	NULL,	NULL,	Hash, Invalid,
	"#",
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 5,	 6,
/*4*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL, Invalid,
	" bytes sent in ",
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 6,
/*5*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL, Invalid,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 6,	 0,	 8,	 0,
/*6*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL, GetPutWarn,NULL,
	"?Invalid command",
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*7*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*8*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,ChkStatus,	NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*9*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 0,	11,	 0,	-1,
/*10*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	CntPut,	NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 1,	 6,
/*11*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL, Invalid,
	"local: ",
};

StateTable DelTable[] = {
    /* 100	200	300	400	500	any	ftp	s1   Cancel */
	 0,	 0,	 0,	 0,	 1,	 1,	-1,	 0,	 0,
/*0*/	NULL,	NULL,	NULL,	NULL,DeleteError,NULL,	NULL,	NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 1,	-1,	 0,	 0,
/*1*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
};

StateTable PwdTable[] = {
    /* 100	200	300	400	500	any	ftp	s1   Cancel */
	 0,	 1,	 0,	 0,	 2,	 0,	 0,	 3,	 0,
/*0*/	NULL, SetPwd,	NULL,	NULL, PwdError,	NULL,	NULL,	NULL,	NULL,
	"500 Command not understood",
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*1*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL, StartCd,	NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*2*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 2,	 0,	 0,	 4,	 0,
/*3*/	NULL,	NULL,	NULL,	NULL, PwdError, NULL,	NULL,	NULL,	NULL,
	"PWD command not recognized",
	 0,	 1,	 0,	 0,	 2,	 0,	-1,	 0,	 0,
/*4*/	NULL, SetPwd,	NULL,	NULL, PwdError, NULL,	NULL,	NULL,	NULL,
	NULL,
};

StateTable HashTable[] = {
    /* 100	200	300	400	500	any	ftp	s1   Cancel */
	 0,	 0,	 0,	 0,	 0,	 1,	 0,	 0,	 0,
/*0*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*1*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
};

StateTable RenameTable[] = {
    /* 100	200	300	400	500	any	ftp	s1   Cancel */
	 0,	 0,	 1,	 0,	 3,	 0,	 0,	 0,	 0,
/*0*/	NULL,	NULL,	NULL,	NULL, RenameError,NULL,	NULL,	NULL,	NULL,
	NULL,
	 0,	 2,	 0,	 0,	 3,	 0,	 0,	 0,	 0,
/*1*/	NULL,	NULL,	NULL,	NULL, RenameError,NULL,	NULL,	NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*2*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL, RenameOk, NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*3*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
};

StateTable GetDirTable[] = {
    /* 100	200	300	400	500	any	ftp	s1   Cancel */
	 0,	 1,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
/*0*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
	 2,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
/*1*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
	 0,	 3,	 0,	 0,	 0,	 0,	 0,	 2,	 0,
/*2*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	"#",
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*3*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,GatherNames,NULL,	NULL,
	NULL,
};

StateTable MkDirTable[] = {
    /* 100	200	300	400	500	any	ftp	s1   Cancel */
	 0,	 1,	 0,	 0,	 2,	 0,	 0,	 3,	 0,
/*0*/	NULL,	NULL,	NULL,	NULL,	ChkDir,	NULL,	NULL,	NULL,	NULL,
	"Command not understood",
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*1*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*2*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 4,	 0,
/*3*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	"MKD",
	 0,	 1,	 0,	 0,	 2,	 0,	 0,	 0,	 0,
/*4*/	NULL,	NULL,	NULL,	NULL,	ChkDir,	NULL,	NULL,	NULL,	NULL,
	NULL,
};

StateTable InvTable[] = {
    /* 100	200	300	400	500	any	ftp	s1   Cancel */
	 0,	 0,	 0,	 0,	 0,	 1,	 0,	 1,	 0,
/*0*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	"?Invalid command",
	 0,	 0,	 0,	 0,	 0,	 1,	-1,	 1,	 0,
/*1*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	"?Invalid command",
};

StateTable RmDirTable[] = {
    /* 100	200	300	400	500	any	ftp	s1   Cancel */
	 0,	 1,	 0,	 0,	 2,	 0,	 0,	 3,	 0,
/*0*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL, NotEmpty,	NULL,
	"File exists",
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*1*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*2*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*3*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
};

StateTable ResolveTable[] = {
    /* 100	200	300	400	500	any	ftp	s1   Cancel */
	 0,	 1,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
/*0*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
	 2,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
/*1*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
	 0,	 3,	 0,	 0,	 0,	 0,	 0,	 2,	 0,
/*2*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	"#",
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*3*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL, Traverse, NULL,	NULL,
	NULL,
};

StateTable SetIdleTable[] = {
    /* 100	200	300	400	500	any	ftp	s1   Cancel */
	 0,	 1,	 0,	 0,	 2,	 0,	 0,	 0,	 0,
/*0*/	NULL,	NULL,	NULL,	NULL,BadIdleTime,NULL,	NULL,	NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*1*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*2*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
};

StateTable IdleTable[] = {
    /* 100	200	300	400	500	any	ftp	s1   Cancel */
	 0,	 1,	 0,	 0,	 2,	 0,	 0,	 0,	 0,
/*0*/	NULL,GetIdleTime,NULL,	NULL,GetIdleTime,NULL,	NULL,	NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*1*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
	 0,	 0,	 0,	 0,	 0,	 0,	-1,	 0,	 0,
/*2*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
};

static StateTable dummy[] = {
    /* 100	200	300	400	500	any	ftp	s1   Cancel */
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
/*0*/	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
	NULL,
};

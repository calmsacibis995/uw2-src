/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlist:nlist.h	1.2"
#ifndef NLIST_H
#define NLIST_H

#define ACTIVE							0x00000001
#define CONTINUOUS_LISTING				0x00000002
#define BINDERY							0x00000004
#define NAMES_ONLY						0x00000008
#define USE_DEFAULT_FS					0x00000010
#define PRINT_DETAIL					0x00000020
#define NOT_VALID_OBJECT_TYPE			0x01110001

#ifdef OLD
int nprintf( char* fmt, ... );
#endif
uint32 BinderyListObjects( char* ServerName, char* SearchName,
		char* ObjTypeString, int Flags );

#endif NLIST_H

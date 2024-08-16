/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/error.h	1.1"

#if !defined(_ERROR_H_INCLUDED_)

#define _ERROR_H_INCLUDED_

#define NEG							0x80000000L
#define ERRORS						_ERRORS_: switch (_err_)
#define CLEANUP						switch(_err_)

#if defined(DEBUG_CODE)

#define ERROR_VARS					UINT32 _cc_, _ln_, _err_
#define JUMP(eval, ecode)			{_cc_ = (UINT32)ecode; _ln_ = __LINE__ - 1; _err_ = eval; goto _ERRORS_; }

#define ERRORNZ(ecode, enumber)		if (ecode      ) JUMP(enumber, ecode)
#define ERROREZ(ecode, enumber)		if (ecode == 0L) JUMP(enumber, ecode)
#define ERRORLZ(ecode, enumber)		if (ecode & NEG) JUMP(enumber, ecode)

#define PrintError					printf("ccode of %lX on line %lu in %s\n", _cc_, _ln_, __FILE__)

#else

#define ERROR_VARS					UINT32 _err_
#define JUMP(eval)					{ _err_ = eval; goto _ERRORS_; }

#define ERRORNZ(ecode, enumber)		if ((UINT32)ecode      ) JUMP(enumber)
#define ERROREZ(ecode, enumber)		if (ecode == 0         ) JUMP(enumber)
#define ERRORLZ(ecode, enumber)		if ((UINT32)ecode & NEG) JUMP(enumber)

#define PrintError

#endif

#endif



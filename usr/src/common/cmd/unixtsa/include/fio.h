/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/fio.h	1.1"

#include <fcntl.h>
#include <stdio.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <share.h>


typedef struct {
int Handle;
long Lseek;
} HANDLE;

typedef struct {
char reserved[21];
char attrib;
unsigned short wr_time;
unsigned short wr_date;
long size;
char name[13];
} FIND_INFO;


long __close( int handle );
int __creat( char *file, int permission );
int __eof( int handle );
long __filelength( int handle );
long __lseek( int handle, long offset, int origin );
unsigned int __read( int handle, char *buf, unsigned int len );
int __sopen( char *file, int access, int share, int permission );
unsigned int __tell( int handle );
int __unlink( char *path );
unsigned int __write( int handle, char *buf, unsigned int len );


int __access( char *path, int mode );
int __chdir( char *path );
int __chmod( char *path, int mode );
char *__getcwd( char *path, int len );
int __mkdir( char *path );
int __rmdir( char *path );
int FindFirstFile( char *newpath, short att, FIND_INFO* dta );
int FindNextFile( FIND_INFO* dta );

void SetErrorno( int AX );
int GetWorkStationType();
int BuildName(char *name,char *old);

#ifndef DO_NOT_REDEFINE_FIO

#define open(a,b,c)  __sopen(a,b,0,c)
#define close(a)     __close(a)
#define creat(a,b)   __creat(a,b)
#define eof(a)       __eof(a)
#define filelength(a)  __filelength(a)
#define lseek(a,b,c)  __lseek(a,b,c)
#define read(a,b,c)   __read(a,b,c)
#define sopen(a,b,c,d) __sopen(a,b,c,d)
#define tell(a)       __tell(a)
#define unlink(a)     __unlink(a)
#define write(a,b,c)  __write(a,b,c)

#define access(a,b)   __access(a,b)
#define chdir(a)      __chdir(a)
#define chmod(a,b)    __chmod(a,b)
#define getcwd(a,b)   __getcwd(a,b)
#define mkdir(a)      __mkdir(a)
#define rmdir(a)      __rmdir(a)

#endif

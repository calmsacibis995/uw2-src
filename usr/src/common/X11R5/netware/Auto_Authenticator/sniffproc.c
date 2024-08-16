/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoauthent:sniffproc.c	1.3"
/******** SCCS/whatever stuff goes here *******/


/*
 *    Copyright Novell Inc. 1993, 1994
 *    Copyright Univel 1993, 1994
 *    (C) Unpublished Copyright of Novell, Inc. All Rights Reserved.
 *
 *    No part of this file may be duplicated, revised, translated, localized
 *    or modified in any manner or compiled, linked or uploaded or
 *    downloaded to or from any computer system without the prior written
 *    consent of Novell, Inc.
 *
 *  Netware Unix Client
 *
 *	Author:  Duck
 *	Created: 1-1-93 (really)
 *
 *	MODULE:
 *		sniffproc.c - Determine various things about another process.
 *
 *	ABSTRACT:
 *
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/fault.h>
#include <sys/syscall.h>
#include <sys/procfs.h>
#include <sys/proc.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <utmpx.h>
#include "sniffproc.h"


/* Gloabl variables */
static	char		*otherStack;
static	vaddr_t		stackUpperBound;


/* Function Prototypes */
static	char	*validPtr( char **ptr, const int stackSize);


/*
 * BEGIN_MANUAL_ENTRY( sniff_env(xxx), xxx )
 *
 * NAME
 *		sniff_env - return a pointer to another process's environment variable. 
 *
 * SYNOPSIS
 *		#include <sys/types.h>		(for pid_t)
 *		#include <sniffproc.h>
 *		char *sniff_env( const pid_t pid, const char *searchString)
 *
 * INPUT
 *		pid          - pid of the process to sniff
 *		searchString - environment string to find
 *
 * OUTPUT
 *		None.
 *
 * RETURN VALUES
 *		char *	-	A pointer to a copy of the environment string.
 *					Note that it is the responsibility of the caller to free(3C)
 *					this area when it no longer needed.
 *
 *		NULL	-	searchString not found in process's environment
 *
 *		-1		-	a problem was encountered with a system or library
 *					call used by sniff_env, e.g.: open, malloc, lseek, etc.
 *					The caller should examine errno for the exact cause.
 *
 * DESCRIPTION
 *		sniff_env uses the process file system (see proc(4)) to determine another
 *		process's environment variable settings.  A process's environment is stored
 *		at the beginning (highest addresses) of it's stack, along with the argument
 *		strings, the argv[] array and the envp[] array.  This area of the stack is
 *		laid out as follows: (from execmdep.c)

 *			The order is dependent on the direction of stack growth,
 *			and so, is machine-dependent.
 *			On 80x86 the userstack grows downwards toward
 *			low addresses; the arrangement is:
 *
 *					(low address).
 *			argc
 *			prefix ptrs (no NULL ptr)
 *			argv ptrs (with NULL ptr)
 *			     (old argv0 points to fname copy if prefix exits)
 *			     (last argv ptr points to fname copy if EMULATOR present)
 *			env ptrs (with NULL ptr)
 *			postfix values (put here later if they exist)
 *			prefix strings
 *			(fname if a prefix exists)
 *			argv strings
 *			(fname if EMULATOR exists)
 *			env strings
 *					(high address)
 *
 *		The algorithm goes like this:
 *			1.	Open the process's status file thru procfs.
 *			2.	Determine the stack upper bound (lowest address).
 *			3.	Open the process's address space file thru procfs.
 *			4.	Read it's stack.
 *			5.	Starting at the beginning (highest address) of the stack,
 *				scan backwards, looking for the last environment pointer.
 *				It should be the first valid pointer we come across.  See the
 *				comments in validPtr for the definition of "valid".
 *			6.	Continue scanning backwards thru the envp[] array, comparing
 *				the searchString with the environment strings,
 *				until we either find the one we want or we run out
 *				of environment strings.
 *			7.	If the desired string is found, strdup(3C) it for our caller.
 *			8.  clean up and return.
 *
 * SEE ALSO
 *
 *
 * END_MANUAL_ENTRY
 */

char *
sniff_env( const pid_t pid, const char *searchString)
{	
	int			fd, asFd;
	int			searchStringLength = strlen( searchString);
	int			stackSize;
	char		*tryThis = NULL;
	char		filename[64];
	char		**candidate;
	char		**stackEnd;
	char		*rc = (char *) -1;		/* assume the worst */
	pstatus_t	pstatus;
	char		dataBuf[256];


#ifdef NEED_PROCESS_FLAGS_FOR_SOME_REASON
	psinfo_t	psinfo;

	/*
	 *	Format the /proc/<pid>/psinfo filename, and open it.
	 */
	sprintf( filename, "/proc/%d/psinfo", (int)pid);

	if(( fd=open( filename, O_RDONLY)) < 0 )
		return( rc);

	if( read( fd, &psinfo, sizeof( psinfo_t)) < 0)
		return( rc);
	close( fd);
#endif NEED_PROCESS_FLAGS_FOR_SOME_REASON


	/*
	 *	Open the process's status file and read the struct
	 */
	sprintf( filename, "/proc/%d/status", (int)pid);
	if(( fd=open( filename, O_RDONLY)) < 0 )
		return( rc);					/* (errno valid)			*/

	if( read( fd, &pstatus, sizeof( pstatus_t)) < 0)
		return( rc);					/* (errno valid)			*/

	close( fd);							/* done with status file	*/


	/*
	 *	pstatus.pr_stksize is an outrageously large
	 *	number due to the way autogrowth is handled.  We're
	 *	going to assume that the environment stuff is within
	 *	the first 16K.
	 */
	stackSize = 0x4000;
	stackUpperBound = pstatus.pr_stkbase - stackSize;	/* go back this far	*/

	/*
	 *	Get enough space to hold the other task's stack.
	 */
	if( (otherStack = (char *) malloc( stackSize)) == NULL)
		return( rc);

	/*
	 *	Open the process's address space file.
	 */
	sprintf( filename, "/proc/%d/as", (int)pid);
	if( (asFd=open( filename, O_RDONLY)) < 0 )
		goto ret3;

	/*
	 *	Seek to the stack upper bound (lowest address) in the
	 *	other task's address space, and read in it's stack.
	 */
	if( lseek( asFd, stackUpperBound, SEEK_SET) == -1)
		goto ret3;

	if( read( asFd, otherStack, stackSize) < 0) {
		close( asFd);
		goto ret3;
	}



	/*
	 *	Point to the last possible pointer in the other process's
	 *	stack, but do it in our address space.
	 */
	candidate=(char **)otherStack + stackSize/sizeof( int);
	stackEnd = candidate;

	/*
	 *	Search backwards thru the stack until we find
	 *	the last environment pointer.
	 */
	while( !(tryThis=validPtr( candidate, stackSize)) )
		candidate--;
	tryThis = NULL;

	/*
	 *	Pointers to the env strings might point to the stack, heap, 
	 *	bss or * initialized data.  validPtr() returns first pointer 
	 *	that is within the stack area. So scan forward again until we 
	 *	get a NULL pointerr, then backup one. This should be the end
	 *	of envp[].
	 */
	while( *candidate != NULL && candidate < stackEnd  )
		candidate++;
	candidate--;

	/*
	 *	Now see if that string is the one we want.
	 *	If not, scan backwards thru envp[] until we find it, or
	 *	hit the beginning.  Loop is terminated when we reach a
	 *	NULL address pionter, which is the end of argv[], start of
	 *	envp[].
	 */
	do {
		if( lseek( asFd, (vaddr_t)*candidate, SEEK_SET) == -1)
			goto ret3;
		if( read( asFd, dataBuf, 256) < 0) 
			goto ret3;
		if( strncmp( dataBuf, searchString, searchStringLength) == 0)
		{
			tryThis = dataBuf;
			break;
		}
		candidate--;
	} while( *candidate != NULL && candidate < stackEnd );


	if( tryThis != NULL)			/* found it						*/
		rc = strdup( tryThis);		/* make a copy for our caller	*/
	else							/* string not found				*/
		rc = NULL;					/* indicate such				*/

ret3:
	free( otherStack);
	close( asFd);					/* done with address space	*/

	return( rc);
}



/*
 *	validPtr - Check to see if the pointer we're given is valid.
 *
 *	In order to be considered valid, it must meet 2 criteria:
 *		a.	It must point beyond where it is stored.  In other
 *			words, it's value must be greater than it's address,
 *			which is why the argument to this function is a
 *			double pointer.
 *
 *		b.	It must not point beyond the beginning (highest
 *			address) of the user's stack.
 *
 *	If the pointer passes these tests, we return a pointer
 *	which is translated from the other process's address space
 *	into ours, so that we may use it directly to access our
 *	copy of his stack.
 *
 *	If the pointer is invalid, we return NULL.
 */

char *
validPtr( char **ptr, const int stackSize)
{
	int	offset;


	offset = *ptr-(char *)stackUpperBound;	/* pointer's offset into stack	*/

	if( offset < ((char *)ptr - otherStack) )
		return NULL;						/* This chair's too low!		*/

	if( offset > stackSize )
		return NULL;						/* This chair's too high!		*/

	return otherStack+offset;				/* This chair's just right		*/
}



int
sniffinfo( const pid_t pid, const int cmd )
{	
	int			fd, rc;
	char		filename[64];
	psinfo_t	psinfo;


	/*
	 *	Format the /proc/<pid>/psinfo filename, and open it.
	 */
	sprintf( filename, "/proc/%d/psinfo", (int)pid);

	if(( fd=open( filename, O_RDONLY)) < 0 )
		return( -1);

	if( read( fd, &psinfo, sizeof( psinfo_t)) < 0) {
		close( fd);
		return( -1);
	}

	close( fd);
	
	switch( cmd) {
		case SNIFF_TTYDEV:
			rc = (int) psinfo.pr_ttydev;
			break;

		case SNIFF_PPID:
			rc = (int) psinfo.pr_ppid;
			break;

		case SNIFF_FNAME:
			rc = (int) strdup( psinfo.pr_fname);
			break;
		case SNIFF_PSARGS:
			rc = (int) strdup( psinfo.pr_psargs);
			break;

		default:
			rc =  -1;
			break;
	}

	return( rc);
}

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


#ident	"@(#)bkrs:common/cmd/bkrs/rslib.d/gethdr.c	1.1.5.3"
#ident  "$Header: gethdr.c 1.2 91/06/21 $"

#include	<time.h>
#include 	<stdio.h>
#include	<string.h>
#include	<pwd.h>
#include	<bktypes.h>
#include	<brarc.h>
#include	<bkrs.h>
#include 	"libadmIO.h"

#ifndef	FALSE
#define TRUE 1
#define FALSE 0
#endif

#define	IS_ALIAS(d)	(d && *d != '/')

extern int rsgethdr();
extern void free();
extern argv_t *s_to_argv();
extern void argv_free();
extern char *devattr();

static argv_t	*attrs;

/* 
	If dchar is given, look for the attribute in there first.
	Since these are stored as <name>=<value>, return a pointer
	to the <value> part only.
*/
static
char *
getattr( device, attribute )
char *device, *attribute;
{
	register i, size;

	if( attrs != (argv_t *)0 ) {
		size = strlen( attribute );
		for( i = 0; (*attrs)[i]; i++ ) 
			if( !strncmp( (*attrs)[i], attribute, size ) 
				&& *((*attrs)[i] + size) == '=' )
				return( (*attrs)[i] + size );
	}
	return( devattr( device, attribute ) );
}

static
void
new_dchar( dchar )
char *dchar;
{
	if( attrs != (argv_t *)0 )
		argv_free( attrs );

	attrs = s_to_argv( dchar, ":" );
}

static
void
free_dchar()
{
	if( attrs != (argv_t *)0 ) {
		argv_free( attrs );
		attrs = (argv_t *)0;
	}
}

int
get_hdr( device, dchar, label, ai, flags, fptr )
char *device, *dchar, *label;
archive_info_t *ai;
int flags;
GFILE **fptr;
{
	char *name;
	if( !IS_ALIAS( device ) )
		return( !rsgethdr( device, dchar, label, ai, flags, fptr ) );

	if( dchar )
		new_dchar( dchar );

	/* Try CDEVICE or BDEVICE */
	if( ( name = getattr( device, "cdevice" ) ) && *name ) {
		if( !rsgethdr( name, dchar, label, ai, flags, fptr ) ) {
			free_dchar();
			return( 1 );
		}

	} else if( ( name = getattr( device, "bdevice" ) ) && *name )
		if( !rsgethdr( name, dchar, label, ai, flags, fptr ) ) {
			free_dchar();
			return( 1 );
		}

	if( ( name = getattr( device, "pathname" ) ) && *name )
		if( !rsgethdr( name, dchar, label, ai, flags, fptr ) ) {
			free_dchar();
			return( 1 );
		}

	free_dchar();
        fprintf(stderr,"ERROR: %s : At least one of the cdevice or bdevice or pathname must be specified in device.tab.\n",device);                            
	return( 0 );
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)bkrs:common/cmd/bkrs/bklib.d/argv.c	1.5.5.3"
#ident  "$Header: argv.c 1.2 91/06/21 $"

#include	<string.h>
#include	<bktypes.h>

#define	ARGVINC	10

extern void *malloc();
extern void *realloc();
extern void free();
extern int sprintf();

extern void brlog();
/*
	This function takes a character string containing a series of fields
	separated by 'separators', and parses it into a malloc'd "argv" structure.
	A pointer to the "argv" structure is returned.
*/

argv_t *
s_to_argv( string, separators )
char *string, *separators;
{
	register argc = 0, argsize;
	register argv_t *argv;
	register char *ptr;

	if( !(argv = (argv_t *) malloc( ARGVINC * sizeof( char * ) ) ) )
		return( (argv_t *) 0 );
	argsize = ARGVINC;

	ptr = strtok( string, separators );
	(*argv)[ argc++ ] = (ptr != NULL) ? strdup(ptr) : NULL;
	if (!ptr) return( argv );	

	while( ptr = strtok( 0, separators ) ) {
		if( argc == argsize ) {
			if( !(argv = (argv_t *)realloc( (void *) argv,
				(argsize + ARGVINC) * sizeof( char * ))))
				return( (argv_t *) 0 );
			argsize += ARGVINC;
		}
		(*argv)[ argc++ ] = strdup( ptr );
	}

	if( argc == argsize ) {
		if( !(argv = (argv_t *)realloc( (void *) argv,
			(argsize + ARGVINC) * sizeof( char * ) ) ) )
			return( (argv_t *) 0 );
		argsize += ARGVINC;
	}
	(*argv)[ argc++ ] = (char *)0;;
	return( argv );
}

char *
argv_to_s( argv, separator )
argv_t *argv;
char separator;
{
	register size = 0, i;
	register char *buffer, *ptr;

	for( i = 0; (*argv)[i]; i++ )
		size += strlen( (*argv)[i] ) + 1;

	if( !(buffer = (char *)malloc( (unsigned int) size + 1 ) ) )
		return( (char *)0 );

	ptr = buffer;
	*ptr = '\0';

	for( i = 0; (*argv)[i]; i++ )
		ptr += sprintf( ptr, "%s%c", (*argv)[i], separator );

	if (i)
		*(ptr - 1) = '\0';

	return( buffer );
}

/* Free all memory associated with a malloc'd argv_t */
void
argv_free( argv )
argv_t *argv;
{
	register i;

	for( i = 0; (*argv)[i]; i++ )
		free( (*argv)[i] );

	free( (void *) argv );
}
/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*	Copyright (c) 1988, 1992 AT&T
 *	  All Rights Reserved
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
 *	The copyright notice above does not evidence any
 *	actual or intended publication of such source code.
 */

#ident	"@(#)ktool:common/ktool/ddicheck/flint.c	1.1"

/*
 * flint.c - filter lint output based upon regular expressions
 *
 *	usage:	flint [-n] [-r range] [-m mapfile] datafile [files]
 *
 *	flint reads data_file for sets of patterns that correspond 
 *	to fields in lint messages.  Then files are scanned for lint
 *	messages that do not match the patterns.  Also included in
 *	the pattern set are patterns for matching a previous ikwid
 *	(lint info) message, and the source line that generated the
 *	message.
 *
 * options:
 *
 *	-n reverse sense, print matching lines
 *	-r set range for ikwid processing
 *	-m search mapfile for source file paths
 *
 * overview:
 *
 *	parse args
 *
 *	read and compile pattern sets in data file
 *
 *	while get message
 *
 *	 	parse message into file, line, type, msg, id
 *
 *		if type is ikwid 
 *			queue message pending possible later action
 *			stale messages (if any) are flushed by enqueue
 *			continue
 *
 *		set status to "pass"
 *
 *		for pattern in linked list of pattern sets
 *
 *			if msg does not match msg pattern or if there
 *			is an id pattern and id does not match
 *				continue
 *
 *			if pass 2 msg and no more patterns in this set
 *				found a message that can be filtered - 
 *				set status to "filter" and break
 *
 *			if file pattern and file does not match
 *				continue
 *
 *			if ikwid pattern
 *				search queue for pending ikwids that
 *				have matching filename, line no. range, 
 *				and text that matches ikwid pattern
 *				if not found
 *					continue
 *
 *			if src pattern and src line does not match it
 *				continue
 *	
 *			found a message that can be filtered - set
 *			status to "filter" and break
 *	
 *		if message can be filtered
 *			if found pending ikwid in queue
 *				mark it "filter"
 *			
 *		place message on output queue - stale messages (if any) 
 *		are flushed, if queue is empty then message is printed
 *		
 *	flush any messages remaining on queue
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>

/*
 * define macros for rexexp.h
 */

#define INIT		register char *sp = instring;
#define GETC()		( *sp++ )
#define PEEKC()		( *sp )
#define UNGETC(c)	( --sp )
#define RETURN(c)	return( c )
#define ERROR(c)	regerr( c )

static void regerr();

#include <regexp.h>

/*
 * regexp error numbers and strings
 */
static struct regex_err {
	int errnum;
	char *errstr;
} regex_errs[] = {
	{ 11, "Range endpoint too large." 		},
	{ 16, "Bad number." 				},
	{ 25, "``\\digit'' out of range." 		},
	{ 36, "Illegal or missing delimiter."		},
	{ 41, "No remembered search string." 		},
	{ 42, "\\( \\) imbalance." 			},
	{ 43, "Too many \\(." 				},
	{ 44, "More than 2 numbers given in \\{ \\}." 	},
	{ 45, "} expected after \\."			},
	{ 46, "First number exceeds second in \\{ \\}."	},
	{ 49, "[ ] imbalance." 				},
	{ 50, "Regular expression overflow." 		},
	{  0, "Unknown regexp error code!!" 		}
};

#define NREGERRS	(sizeof(regex_errs)/sizeof(struct regex_err)-1)

/*
 * Buffer size for compiled regular expressions
 */
#define	ESIZE		256

/*
 * function to call for regular expression match
 */
#define match( string, expr )	step( string, expr )

/*
 * save argv[0] for error reporting
 */
static char *progname;

/*
 * tokens for data file parsing
 */
static struct strings {
	char	*name;
	int	len;
} strings[] = {
	{	"msg:",		4	},
	{	"id:",		3	},
	{	"ikwid:",	6	},
	{	"file:",	5	},
	{	"src:",		4	}
};

/*
 * array offsets for data file tokens, must correspond to the above
 */
#define MSG	0
#define ID	1
#define IKWID	2
#define FYLE	3
#define	SRC	4
#define NEXPR	5

/*
 * struct definition for linked list of pattern sets from data file
 *
 */
static struct msg_list {
	struct msg_list *next;
	char *expr[NEXPR];
} *root_list = NULL;

/*
 * struct definition for linked list of queued messages.
 * When an ikwid message is found, it must be enqueued
 * pending a match with a subsequent warning.  However,
 * it is possible to have multiple ikwid's and / or 
 * intervening messages which do not match the ikwid.
 * All of these messages are queued until it is determined 
 * that the queue can be flushed, so that the order of 
 * messages during output is the same as the input order.
 * As a result, all output is done through enqueue().
 * enqueue() determines whether a message needs to be queued
 * or can be output.
 */
static struct queue {
	struct queue *next;
	char *buf;		/* queued message	*/
	char *msg;		/* msg portion		*/
	char *file;		/* file name portion	*/
	char *id;		/* id portion		*/
	int line;		/* line number		*/
	int sts;		/* PENDING/PASS/FILTER	*/
	int range;		/* last line for effect	*/
} *root_queue = NULL;

/*
 * queued message status - pending determination, pass thru, filter out
 */
#define PENDING	0
#define PASS	1
#define FILTER	2

/*
 * token which lint -K prints to identify an ikwid
 */
#define IKWIDMSG	"info"

/* 
 * default ikwid range, number of lines after an ikwid which will
 * still permit a match to a warning message
 */
#define IKWID_RANGE	1

#ifdef CI5
extern void *malloc();		/* use for CI5 and SVR4.0 	*/
#else
extern char *malloc();
#endif
extern void free();
extern int atoi();
extern void exit();
extern FILE *fopen();
extern int fclose();

static void get_msg_list();	/* build list of patterns sets	*/
static void filter();		/* process lint messages	*/
static void enqueue();		/* put message on output queue	*/
static char *salloc();		/* mem allocation and init	*/
static void dequeue();		/* remove from queue and print	*/
static void parsemsg();		/* break lint msg into fields	*/
static char *srcline();		/* get src line that caused msg	*/
static int chk_file_name();	/* basename matching		*/
static char *get_file_name();	/* get file name from map file	*/

void main(argc, argv)
register argc;
register char **argv;
{
	register int c;
	register int nflag = 0;
	register int mflag = 0;
	register int rflag = 0;
	register int errflg = 0;
	register int ikwid_range = IKWID_RANGE;
	register char *map = NULL;
	extern int optind;
	extern char *optarg;
	extern int getopt();
	
	progname = argv[0];
	
	while((c=getopt(argc, argv, "nm:r:")) != EOF)
		switch(c) {
		case 'n':
			if( nflag )
				errflg++;
			nflag++;
			break;
		case 'r':
			if( rflag )
				errflg++;
			rflag++;
			if( ( ikwid_range = atoi( optarg ) ) < 0 )
				errflg++;
			break;
		case 'm':
			if( mflag )
				errflg++;
			mflag++;
			map = optarg;
			if( !map || !*map )
				errflg++;
			break;
		default:
			errflg++;
			break;
		}

	if(errflg || (optind >= argc)) {
		(void) fprintf( stderr,
		  "Usage: %s", progname );
		(void) fprintf( stderr,
		  " [-n] [-r range] [-m mapfile] datafile [files]\n");
		exit(1);
	}
/*
 *	get patterns from data file
 */
	get_msg_list( argv[optind++] );

	argc -= optind;
	argv = &argv[optind];
/*
 *	filter lint messages
 */
	if (!argc)
		filter( NULL, nflag, map, ikwid_range );
	else
		while (argc-- > 0)
			filter( *argv++, nflag, map, ikwid_range );
	exit(0);
/*
 * 	NOTREACHED 
 */
}
static void get_msg_list( file )
register char *file;
{
	register FILE *fp;
	register char *buf;
	register char *p, *b;
	register int lineno = 0;
	register struct msg_list *tmp;
	register int i, j;

	if( (fp = fopen( file, "r" )) == NULL ) {
		(void) fprintf(stderr, "%s: can't open '%s'\n", 
		  progname, file );
		exit( 1 );
	}
	if(!(buf = malloc( BUFSIZ ) ) ) {
		(void) fprintf(stderr, "%s: malloc failed\n",
		  progname);
		exit( 1 );
	}
	while( fgets( buf, BUFSIZ, fp ) ) {
		lineno++;
/*
 *		remove leading white space and trailing '\n'
 *		skip blank lines and comments
 */		
		b = buf; 
		while( *b == ' ' || *b == '\t' ) 
			b++;

		if( ( p = strchr( buf, '\n' ) ) != NULL )
			*p = '\0';
		if( !*b || *b == '#' )
			continue;

/*
 *		search list of data file tokens for a match 
 *		with this line
 */
		for( i = 0; i < NEXPR; i++ ) {
			if( !strncmp( b, strings[i].name,
			  strings[i].len ))
				break;
		}
		if( i == NEXPR ) {
			(void) fprintf( stderr, 
			  "%s: line %d in '%s' - unrecognized token\n",
			  progname, lineno, file );
			exit( 1 );
		}			
		if( i == MSG ) {
/*
 *			found new msg expression.
 *			allocate new msg_list for new pattern set, 
 *			add new msg_list to root_list, and clear
 *			expression array for this set.
 */
			if(!(tmp = (struct msg_list *) 
			  malloc( sizeof( struct msg_list )))){
				(void) fprintf(stderr, 
				  "%s: malloc failed\n", progname);
				exit( 1 );
			}
			tmp->next = root_list;
			root_list = tmp;
			for( j = 0; j < NEXPR; j++ )
				root_list->expr[j] = NULL;
		} else {
/*
 *			found qualifier for current pattern set.
 *			make sure there is a current pattern set and
 *			that this is not a duplicate qualifier.
 */
			if( !root_list ) {
				(void) fprintf( stderr,
				  "%s: error at line %d in data file,",
				   progname, lineno);
				(void) fprintf( stderr,
				  " qualifier before 'msg' pattern\n");
				exit( 1 );
			}
			if( root_list->expr[i] ) {
				(void) fprintf( stderr,
				  "%s: error at line %d in data file,",
				   progname, lineno);
				(void) fprintf( stderr,
				  " multiple '%s' qualifiers\n",
				  strings[i].name );
				exit( 1 );
			}
		}
/*		
 *		strip leading and trailing white space from expression.
 */
		b = b+strings[i].len;
		while( *b == ' ' || *b == '\t' ) 
			b++;
		p = b + strlen( b ) - 1;
		while( *p == ' ' || *p == '\t' )
			*p-- = '\0';
/*
 *		allocate space and compile expression
 */
		if( !(root_list->expr[i] = malloc( ESIZE ) ) ) {
			(void) fprintf(stderr, "%s: malloc failed\n",
			  progname);
			exit( 1 );
		}
		p = root_list->expr[i];
		(void) compile(b, p, p+ESIZE, '\0' );
	}
	free( buf );
	(void) fclose( fp );
}
static void filter(infile, nflag, map, range)
register char *infile;
register int nflag;
register char *map;
register int range;
{
	register FILE *fp;
	register struct msg_list *msgptr;
	register struct queue *qp;
	register char *buf;
	register char *p;
	register int lineno;
	register int sts;
	char *msg, *file, *msgtyp, *id;
	int line;
/*
 *	if reading from file, open file, else set fp to stdin
 */
	if (infile) {
		if( (fp = fopen( infile, "r" )) == NULL ) {
			(void) fprintf(stderr, "%s: can't open '%s'\n", 
			  progname, infile );
			exit( 1 );
		}
	} else {
		infile = "stdin";
		fp = stdin;
	}

	if( !(buf = malloc( BUFSIZ ))) {
		(void) fprintf( stderr,
		  "%s: malloc failed\n", progname);
		exit( 1 );
	}

	lineno = 0;
	while( fgets( buf, BUFSIZ, fp )) {
		lineno++;
/*
 *		remove trailing '\n' and leading white space,
 *		if nothing left, skip this line
 */
		if( ( p = strchr( buf, '\n' ) ) != NULL )
			*p = '\0';
		p = buf; 
		while( *p == ' ' || *p == '\t' ) 
			p++;
		if( !*p ) 
			continue;
/*
 *		parse the lint message into fields
 */
		parsemsg( buf, &file, &line, &msgtyp,
		  &msg, &id, infile, lineno);
/*
 *		if ikwid (info) message, enqueue pending later action
 */
		if( !strcmp( msgtyp, IKWIDMSG ) ) {
			enqueue(buf, msg, id, file, line, line+range,
			  PENDING, nflag );
			continue;
		}
/*
 *		found a candidate for filtering.
 *		search list of pattern sets for a match.
 */
		sts = PASS;
		for( msgptr=root_list; msgptr; msgptr=msgptr->next ) {
/*
 *			check for msg field match
 */
			if( !match( msg, msgptr->expr[MSG] ) ) 
				continue;
/*
 *			check for id field match
 */
			if( msgptr->expr[ID]  
			  && !match( id, msgptr->expr[ID] ) ) 
				continue;
/*
 *			check for pass 2 message. Subsequent checks 
 *			for file name match, ikwid match, and src 
 *			line match are short circuited here for 
 *			pass 2 messages, because pass 2 messages 
 *			only have a message and optional id field.  
 *			Hence checks using src file and line number 
 *			will fail.  If no checks of these fields 
 *			are in this pattern set, then this message 
 *			can be filtered.
 */
			if( !*msgtyp ) { 
				if( msgptr->expr[ FYLE ] 
				  || msgptr->expr[ IKWID ] 
				  || msgptr->expr[ SRC ] ) 
					continue;
				sts = FILTER;
				break;
			}
/*
 *			check for file name match
 */
			if( msgptr->expr[FYLE] &&
			  !match( file, msgptr->expr[FYLE] ))
				continue;
/*
 *			check for ikwid match
 */
			if( msgptr->expr[ IKWID ] ) {
/*
 *				search queue for pending ikwid messages
 *				check that line is within ikwid range
 *				check that file matches ikwid file name
 *				check that ikwid text matches pattern
 */
				for( qp=root_queue; qp; qp=qp->next ) {
					if( qp->sts == PENDING 
					  && line >= qp->line
					  && line <= qp->range
					  && !strcmp( qp->file, file )
					  && match( qp->msg, 
					  msgptr->expr[ IKWID ] ) )
						break;
				}
/*
 *				if qp is NULL, then ikwid check failed
 */
				if( !qp ) 
					continue;
				
			}
/*
 *			check line in source file, save this check
 *			for last, since it is the most expensive
 */
			if( msgptr->expr[ SRC ] 
			  && !match( srcline( file, line, map ), 
			  msgptr->expr[ SRC ] ) )
					 continue;
/*
 *			found lint message that has passed all tests,
 *			message should be filtered
 */
			sts = FILTER;
/*
 *			if there is an ikwid match, mark
 *			the queued ikwid for filtering.
 */
			if( msgptr->expr[ IKWID ] )
				qp->sts = FILTER;
			
			break;
		}
/*
 *		place the lint message on the output queue
 */
		enqueue(buf, NULL, NULL, NULL, 0, 0, sts, nflag);
	}
/*
 *	finished  processing this input, dequeue / print whatever is 
 *	still queued, close input, and free buffer.
 */
	while( root_queue )
		dequeue( nflag );
	(void) fclose( fp );
	free( buf );
}						
static void regerr(rerr)
register int rerr;
{
	register int i;
/*
 *	search regexp error list - print error message and exit
 */
	for( i = 0; i < NREGERRS; i++ )
		if( regex_errs[i].errnum == rerr ) 
			break;
	(void) fprintf( stderr,
	  "%s: regular expression error %d: %s\n",
	  progname, rerr, regex_errs[i].errstr);
	exit( 1 );
}
static void enqueue( buf, msg, id, file, line, range, sts, nflag )
register char *buf;
register char *msg;
register char *id;
register char *file;
register int line;
register int sts;
register int nflag;
{
	register struct queue *qp;
/* 
 *	add a new message to the queue.
 *	new entries are added to the tail so that they can be 
 *	dequeued in the order seen.
 */

/*
 *	remove stale messages from the head of the queue.
 *	the queue is stale up to, but not including the 
 *	first pending ikwid that has a valid file and range.
 */
	while( root_queue && ( root_queue->sts != PENDING 
	  || root_queue->range < line 
	  || strcmp( root_queue->file, file ) ) )
		dequeue( nflag );
/*
 *	run the links to get to the end of the queue.
 *	if a pending ikwid message is being queued, update 'range'
 *	for all currently pending ikwid messages.  Since 'range' is
 *	unused by other types of message, update range for all messages
 *	instead of wasting time to check to see if it's really needed.
 *
 *	note that the initial setting of qp as a pointer to root_queue 
 *	requires that the first structure element of a "struct queue" 
 *	be "next".  Moving "next" from the first position will cause
 *	the search to malfunction.
 */	
	for( qp=(struct queue *)&root_queue; qp->next; qp=qp->next )
		if( sts == PENDING )
			qp->next->range = range;
/*
 *	if there are any queue entries or if a pending ikwid is being
 *	queued, allocate a new queue entry and add it to the tail of 
 *	the queue.  Otherwise, print this message.
 */	
	if( root_queue || sts == PENDING ) {
		if(!(qp->next = 
		  (struct queue *)malloc( sizeof( struct queue )))){
			(void) fprintf(stderr, "%s: malloc failed\n", 
			  progname);
			exit( 1 );
		}
		qp = qp->next;
		qp->buf = salloc( buf );
		if( sts == PENDING ) {
			qp->msg = salloc( msg );
			qp->id = salloc( id );
			qp->file = salloc( file );
			qp->line = line;
			qp->sts = sts;
			qp->range = range;
		}
		qp->next = NULL;
	} else {
		if( (!nflag && sts != FILTER ) 
		  || (nflag && sts == FILTER ))
			(void) printf( "%s\n", buf );
	}
}
static char *salloc( str )
register char *str;
{
	register char *p;
	
	if( !str || !*str ) 
		return( NULL );
	if( !(p = malloc( (unsigned int)strlen(str) + 1 ))) {
		(void) fprintf(stderr,"%s: malloc failed\n", progname);
		exit( 1 );
	}
	(void) strcpy( p, str );
	return( p );
}
static void dequeue( nflag )
register int nflag;
{
	register struct queue *qp;
/*
 *	print and remove the message at the queue head
 */
	if( !root_queue )
		return;
	
	if( (!nflag && root_queue->sts != FILTER )
	  ||(nflag && root_queue->sts == FILTER ))
		(void) printf( "%s\n", root_queue->buf );

	qp = root_queue;
	root_queue = root_queue->next;
	if( qp->file ) 
		free( qp->file );
	if( qp->id )
		free( qp->id );
	if( qp->msg )
		free( qp->msg );
	free( qp->buf );
	free( qp );
}
static void parsemsg( buf, file, line, msgtyp, msg, id, infile, lineno )
register char *buf;
register char **file;
register int *line;
register char **msgtyp;
register char **msg;
register char **id;
register char *infile;
register int lineno;
{
	register char *p;
	static char lbuf[BUFSIZ];

/*
 *	copy to lbuf to keep original buffer intact, add '\0's
 *	after tokens in lbuf while parsing.
 */	
	(void) strcpy( lbuf, buf );	
	p = &lbuf[0];
	if( *p == '"' ) {
/*
 *		parse pass 1 message
 *
 *		"<file>", line <lineno>: <msgtyp>: <msg>[: <id>]
 */
/*
 *		parse file field
 */
		*file = ++p;
		while( *p && *p != '"' ) p++;
		if( !*p ) goto err;
		*p++ = '\0';
/*
 *		parse line number field
 */
		while( *p && !isdigit( *p ) ) p++;
		if( (*line = atoi( p )) <= 0 ) goto err;
		while( *p && *p != ':' ) p++;
/*
 *		parse message type field
 */
		if( !*p++ ) goto err;
		while( *p == ' ' || *p == '\t' ) p++;
		if( !*p ) goto err;
		*msgtyp = p;
		while( *p && *p != ':' ) p++;
		if( !*p ) goto err;
		*p++ = '\0';
/*
 *		parse message field
 */
		while( *p == ' ' || *p == '\t' ) p++;
		if( *p ) {
			*msg = p;
			while( *p && *p != ':' ) p++;
		} 
		else
			*msg = "";
/*
 *		parse id field if present, otherwise set to "".
 */
		if( *p ) {
			*p++ = '\0';
			while( *p == ' ' || *p == '\t' ) p++;
			*id = p;
		} else
			*id = "";

	} else {
/*
 *		pass 2 message
 *
 *		<msg>[:<id>]
 */
/*
 *		parse message field
 */
		while( *p == ' ' || *p == '\t' ) p++;
		if( !*p ) goto err;
		*msg = p;
		while( *p && *p != ':' ) p++;
/*
 *		parse id field if present, otherwise set to "".
 */
		if( *p ) {
 			*p++ = '\0';
			while( *p == ' ' || *p == '\t' ) p++;
			*id = p;
		} else
			*id = "";
/*
 *		clear other fields for pass 2 messages
 */
		*line = 0;
		*file = "";
		*msgtyp = "";
	}
	return;
err:
	(void) fprintf(stderr, "%s: unrecognized lint message:\n", 
	  progname );
	(void) fprintf(stderr, "%s(%d):'%s'\n", infile, lineno, buf );
	exit(1);
}
static char *srcline( file, line, map ) 
register char *file;
register int line;
register char *map;
{
	static char buf[BUFSIZ];
	register char *fyl;
	register FILE *fp;
	register int i;
	register char *p;
/*
 *	return src line at line number 'line' in 'file', if 'map'
 *	is set, search map file for 'file'.  If not found, then
 *	try current working directory.
 */
	fyl = get_file_name( map, file );
 /*
  * 	open source file, and read until appropriate line number
  */
	if( !(fp = fopen( fyl, "r" ) ) ) {
		(void) fprintf( stderr, "%s: cannot open '%s'\n", 
		  progname, fyl );
		exit( 1 );
	}
	for( i = 0 ; i < line ; i++ ) {
		if( !fgets( buf, BUFSIZ, fp )) {
			(void) fprintf( stderr,
			  "%s: unexpected eof reading '%s'\n",
			  progname, fyl );
			exit( 1 );
		}
	}
/*
 *	remove trailing '\n' and close file.
 */
	if( ( p = strchr( buf, '\n' ) ) != NULL )
		*p ='\0';
	(void) fclose( fp );
	return( buf );
	
}
static int chk_file_name( base_name, full_path )
register char *base_name;
register char *full_path;
{
	register char *p;
/*
 *	check for basename match
 */
	if( ( p = strrchr( full_path, '/' ) ) != NULL ) 
		p++;
	else
		p = full_path;
	return( !strcmp( p, base_name ) );
}
static char *get_file_name( map, file )
register char *map;
register char *file;
{
	register FILE *fp;
	static char fyl[BUFSIZ] = "";
	register int foundit;
	register char *p;
/*
 *	scan map file for matching basename
 *	if no map file, use basename
 */
	if( !map || !*map ) 
		return( file );
/*
 *	check to see if cached name matches
 */
	if( chk_file_name( file, fyl ) )
		return( fyl );
/*
 *	open map file and search for a matching file name
 */
	if( !(fp = fopen( map, "r" ) ) ) {
		(void) fprintf( stderr,"%s: cannot open '%s'\n", 
		  progname, map );
		exit( 1 );
	}
	foundit = 0;
	while( !foundit && fgets( fyl, BUFSIZ, fp ) ) {
		if( ( p = strchr( fyl, '\n' ) ) != NULL )
			*p = '\0';
		foundit = chk_file_name( file, fyl );
	}
/*
 *	if not found, use basename
 */
	if( !foundit )
		(void) strcpy( fyl, file );

	(void) fclose( fp );			
	return( fyl );
}


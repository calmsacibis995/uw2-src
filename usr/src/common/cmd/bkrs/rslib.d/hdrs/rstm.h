/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/rslib.d/hdrs/rstm.h	1.1.5.2"
#ident  "$Header: rstm.h 1.2 91/06/21 $"

/* The various stimulae types */
#define	RSTM_BEGIN	1	/* Hit beginning of history table */
#define	RSTM_END	2	/* Hit end of history table */
#define	RSTM_FARCHIVE	3	/* found full archive from acceptable method */
#define	RSTM_PARCHIVE	4	/* found partial archive from acceptable method */
#define	RSTM_RSDATE	5	/* hit restore status date in table */
#define	RSTM_OPTFARCHIVE	6	/* Optional full archive */
#define	RSTM_OPTPARCHIVE	7	/* Optional partial archive */

/* Action values */
#define	RSTM_IGNORE	1	/* merely pass over this archive */
#define	RSTM_ACCEPT	2	/* accept this archive */

/* Directions */
#define	RSTM_FORWARD 1
#define	RSTM_BACKWARD 2
#define	RSTM_STOP	3

/* Coverage */
#define	RSTM_FULL	1
#define	RSTM_PART	2

/* Structure to hold a table transition */
typedef	struct	rsstrans_s {
	char *state;
	int	stimulus;
	int action;
	char *next_state;
} rsstrans_t;

typedef	struct	rsstable_s {
	char *type;	/* restore object type of this table */
	char *start;	/* start state */
	int	stopat;	/* How many successful archives to do */
	rsstrans_t *rootp;	/* pointer to the binary table of transitions */
} rsstable_t;

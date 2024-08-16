/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)inetinst:options.c	1.2"

/*
 *  Routines for populating and parsing options.
 */

#include "inetinst.h"
#include <string.h>

static struct iopts options[IOPT_MAXOPTS];

/*
 *  Routine to parse the options file
 *	INPUT	filename containing options
 *	OUTPUT	none
 *	ACTION	sets default options
 */
void
parse_options_file(char *path)
{
	FILE	*fptr;
	char	optbuf[IBUF_SIZE];
	char	tag[IBUF_SIZE];
	char	value[IBUF_SIZE];

	if ((fptr = fopen(path, "r")) == NULL)
		return;

	while(fgets(optbuf, IBUF_SIZE, fptr) != NULL) {

		/*
		 *  If this was a comment or contained bad syntax, just
		 *  keep going.
		 */

		if (optbuf[0] == '#')
			continue;
		if (sscanf(optbuf, "%s %s", tag, value) != 2) {
			continue;
		}

		set_option(tag, value);
	}
}

/*
 *  First order of business is to assign default options.
 *	INPUT	string containing hostname of requestor
 *	OUTPUT	none
 *	ACTION	sets default options
 */
void
set_default_options(char *requestor)
{
	char optbuf[IBUF_SIZE];
	struct utsname my_uname;	/* For getting system name info */
	
	/*
	 *  Default target is requesting_host:ISPOOL_DIR
	 */
	sprintf(optbuf, "%s:%s", requestor, ISPOOL_DIR);
	options[IOPT_TARGET].tag = strdup(IOPT_TARGET_NAME);
	options[IOPT_TARGET].value = strdup(optbuf);

	/*
	 *  Default source is this_host:ISPOOL_DIR
	 */
	sprintf(optbuf, "%s:%s", get_nodename(), ISPOOL_DIR);
	options[IOPT_SOURCE].tag = strdup(IOPT_SOURCE_NAME);
	options[IOPT_SOURCE].value = strdup(optbuf);

	/*
	 *  Default package is " "
	 */
	options[IOPT_PACKAGE].tag = strdup(IOPT_PACKAGE_NAME);
	options[IOPT_PACKAGE].value = strdup(" ");

	/*
	 *  Default interactive is "yes"
	 */
	options[IOPT_INTERACTIVE].tag = strdup(IOPT_INTERACTIVE_NAME);
	options[IOPT_INTERACTIVE].value = strdup("YES");

	/*
	 *  Default verbose is "1"
	 */
	options[IOPT_VERBOSE].tag = strdup(IOPT_VERBOSE_NAME);
	options[IOPT_VERBOSE].value = strdup("1");

	/*
	 *  Default requestor is requestor
	 */
	options[IOPT_REQUESTOR].tag = strdup(IOPT_REQUESTOR_NAME);
	options[IOPT_REQUESTOR].value = strdup(requestor);

	/*
	 *  Default mail is none
	 */
	options[IOPT_MAIL].tag = strdup(IOPT_MAIL_NAME);
	options[IOPT_MAIL].value = strdup("none");

	/*
	 *  Go now to the system defaults file and see if there are
	 *  some options to be set.
	 */
	parse_options_file(IOPT_FILE);
}

/*
 *  Routines to dump out all options
 */
void
print_all_options()
{
	int	i;
	char	tmpbuf[IBUF_SIZE];

	log(catgets(mCat, MSG_SET, C_LOG_OPTS, M_LOG_OPTS));
	for (i=0; i< IOPT_MAXOPTS; i++) {
		sprintf(tmpbuf, "%s: %s\n", options[i].tag, options[i].value);
		log(tmpbuf);
	}
}

/*
 *  Routine to set an option
 *	INPUT	option tag, new value
 *	OUTPUT	none
 *	ACTION	Finds the correct option, set its value, returns 0 on
 *		success, IERR_BADOPT on failure.
 */
int
set_option(char *tag, char *value)
{
	int	i;		/* Counter for running through options list */
	char optbuf[IBUF_SIZE];	/* Tmp buffer for string manipulation */
	char netbuf[IBUF_SIZE];	/* Buffer for Network Messages */
	int	pass=0;		/* Flag for success */

	for (i = 0; i < IOPT_MAXOPTS; i++) {
		if (!strcmp(tag, options[i].tag)) {
			pass = 1;
			break;
		}
	}

	if (!pass) {
		sprintf(netbuf, catgets(mCat, MSG_SET, C_BADKEY_OPT, M_BADKEY_OPT), tag);
		netputs(netbuf, stdout);
		return(IERR_BADOPT);
	}

	/*
	 *  Now that we're at a point where we have a valid tag, 
	 *  we need to check to make sure the value given is sane.
	 */
	switch(i) {
	case IOPT_TARGET:
	case IOPT_SOURCE:
		break;
	case IOPT_PACKAGE:
		break;
	case IOPT_INTERACTIVE:
		if ((strcmp(value, "NO")) && (strcmp(value, "YES"))) {
			pass = 0;
			sprintf(netbuf, "Must be YES or NO  - ");
			netputs(netbuf, stdout);
		}
		break;
	case IOPT_VERBOSE:
		if ((strcmp(value, "0")) && (strcmp(value, "1")) && (strcmp(value, "2"))) {
			pass = 0;
			sprintf(netbuf, "Must be 0 1 or 2 - ");
			netputs(netbuf, stdout);
		}
		break;
	default:
		break;
	}

	if (!pass) {
		sprintf(netbuf, catgets(mCat, MSG_SET, C_BADOPT_VAL, M_BADOPT_VAL), value, tag);
		netputs(netbuf, stdout);
		return(IERR_BADOPT);
	}

	options[i].value = strdup(value);
	return(0);
}

/*
 *  Routine to return the value of an option.
 *	INPUT	Index of option (ie: IOPT_TARGET)
 *	OUTPUT	string containing value of the option
 */
char *
get_option(int index)
{
	return(options[index].value);
}

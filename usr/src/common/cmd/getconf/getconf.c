/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)getconf:getconf.c	1.1"

/*
 * COPYRIGHT NOTICE
 * 
 * This source code is designated as Restricted Confidential Information
 * and is subject to special restrictions in a confidential disclosure
 * agreement between HP, IBM, SUN, NOVELL and OSF.  Do not distribute
 * this source code outside your company without OSF's specific written
 * approval.  This source code, and all copies and derivative works
 * thereof, must be returned or destroyed at request. You must retain
 * this notice on any copies which you make.
 * 
 * (c) Copyright 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED 
 */
/*
 * OSF/1 1.2
 */
#if !defined(lint) && !defined(_NOIDENT)
static char rcsid[] = "@(#)$RCSfile: getconf.c,v $ $Revision: 1.2.6.2 $ (OSF) $Date: 1992/11/11 16:17:38 $";
#endif
/*
 * COMPONENT_NAME: (CMDPOSIX) new commands required by Posix 1003.2
 *
 * FUNCTIONS: getconf
 *
 *
 * (C) COPYRIGHT International Business Machines Corp. 1985, 1989
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 *
 * 1.3  com/cmd/posix/getconf.c, cmdposix, bos320, 9125320 6/5/91 09:16:59
 */


#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

/*
 *
 *  NOTE:  This table must remain sorted 
 *
 */
struct values{
	char *name;
	int kind;
#define CONSTANT	1
#define SYSCONF		2
#define PATHCONF	3
#define CONFSTR		4
	int numb;
} value[] = 
	{
	"ARG_MAX",		SYSCONF,	_SC_ARG_MAX,
	"BC_BASE_MAX",		SYSCONF,	_SC_BC_BASE_MAX,
	"BC_DIM_MAX",		SYSCONF,	_SC_BC_DIM_MAX,
	"BC_SCALE_MAX",		SYSCONF,	_SC_BC_SCALE_MAX,
	"BC_STRING_MAX",	SYSCONF,	_SC_BC_STRING_MAX,
	"CHILD_MAX",		SYSCONF,	_SC_CHILD_MAX,
	"CLK_TCK",		SYSCONF,	_SC_CLK_TCK,
	"COLL_WEIGHTS_MAX",	SYSCONF,	_SC_COLL_WEIGHTS_MAX,
	"CS_PATH",		CONFSTR,	_CS_PATH,
	"EXPR_NEST_MAX",	SYSCONF,	_SC_EXPR_NEST_MAX,
	"LINE_MAX",		SYSCONF,	_SC_LINE_MAX,
	"LINK_MAX",		PATHCONF,	_PC_LINK_MAX,
	"MAX_CANON",		PATHCONF,	_PC_MAX_CANON,
	"MAX_INPUT",		PATHCONF,	_PC_MAX_INPUT,
	"NAME_MAX",		PATHCONF,	_PC_NAME_MAX,
	"NGROUPS_MAX",		SYSCONF,	_SC_NGROUPS_MAX,
	"OPEN_MAX",		SYSCONF,	_SC_OPEN_MAX,
	"PAGE_SIZE",		SYSCONF,	_SC_PAGESIZE,
	"PASS_MAX",		SYSCONF,	_SC_PASS_MAX,
	"PATH",			CONFSTR,	_CS_PATH,
	"PATH_MAX",		PATHCONF,	_PC_PATH_MAX,
	"PIPE_BUF",		PATHCONF,	_PC_PIPE_BUF,
	"POSIX2_BC_BASE_MAX",	CONSTANT,	_POSIX2_BC_BASE_MAX,
	"POSIX2_BC_DIM_MAX",	CONSTANT,	_POSIX2_BC_DIM_MAX,
	"POSIX2_BC_SCALE_MAX",	CONSTANT,	_POSIX2_BC_SCALE_MAX,
	"POSIX2_BC_STRING_MAX",	CONSTANT,	_POSIX2_BC_STRING_MAX,
	"POSIX2_CHAR_TERM",	SYSCONF,	_SC_2_CHAR_TERM,
	"POSIX2_COLL_WEIGHTS_MAX",CONSTANT,	_POSIX2_COLL_WEIGHTS_MAX,
#ifdef _POSIX2_C_BIND
	"POSIX2_C_BIND",	CONSTANT,	1,
#else
	"POSIX2_C_BIND",	CONSTANT,	0,
#endif
	"POSIX2_C_DEV",		SYSCONF,	_SC_2_C_DEV,
	"POSIX2_C_VERSION",	CONSTANT,	_POSIX2_C_VERSION,
	"POSIX2_EXPR_NEST_MAX",	CONSTANT,	_POSIX2_EXPR_NEST_MAX,
	"POSIX2_FORT_DEV",	SYSCONF,	_SC_2_FORT_DEV,
	"POSIX2_FORT_RUN",	SYSCONF,	_SC_2_FORT_RUN,
	"POSIX2_LINE_MAX",	CONSTANT,	_POSIX2_LINE_MAX,
	"POSIX2_LOCALEDEF",	SYSCONF,	_SC_2_LOCALEDEF,
	"POSIX2_RE_DUP_MAX",	CONSTANT,	_POSIX2_RE_DUP_MAX,
	"POSIX2_SW_DEV",	SYSCONF,	_SC_2_SW_DEV,
	"POSIX2_UPE",		SYSCONF,	_SC_2_UPE,
	"POSIX2_VERSION",	SYSCONF,	_SC_2_VERSION,
#ifdef _POSIX_LOCALEDEF
	"POSIX_LOCALEDEF",	CONSTANT,	1,
#else
	"POSIX_LOCALEDEF",	CONSTANT,	0,
#endif
	"RE_DUP_MAX",		SYSCONF,	_SC_RE_DUP_MAX,
	"SSIZE_MAX",		CONSTANT,	_POSIX_SSIZE_MAX,
	"STREAM_MAX",		SYSCONF,	_SC_STREAM_MAX,
	"TZNAME_MAX",		SYSCONF,	_SC_TZNAME_MAX,
	"_POSIX_ARG_MAX",	CONSTANT,	_POSIX_ARG_MAX,
	"_POSIX_CHILD_MAX",	CONSTANT,	_POSIX_CHILD_MAX,
	"_POSIX_CHOWN_RESTRICTED",PATHCONF,	_PC_CHOWN_RESTRICTED,
	"_POSIX_JOB_CONTROL",	SYSCONF,	_SC_JOB_CONTROL,
	"_POSIX_LINK_MAX",	CONSTANT,	_POSIX_LINK_MAX,
	"_POSIX_MAX_CANON",	CONSTANT,	_POSIX_MAX_CANON,
	"_POSIX_MAX_INPUT",	CONSTANT,	_POSIX_MAX_INPUT,
	"_POSIX_NAME_MAX",	CONSTANT,	_POSIX_NAME_MAX,
	"_POSIX_NGROUPS_MAX",	CONSTANT,	_POSIX_NGROUPS_MAX,
	"_POSIX_NO_TRUNC",	PATHCONF,	_PC_NO_TRUNC,
	"_POSIX_OPEN_MAX",	CONSTANT,	_POSIX_OPEN_MAX,
	"_POSIX_PATH_MAX",	CONSTANT,	_POSIX_PATH_MAX,
	"_POSIX_PIPE_BUF",	CONSTANT,	_POSIX_PIPE_BUF,
	"_POSIX_SAVED_IDS",	SYSCONF,	_SC_SAVED_IDS,
	"_POSIX_SSIZE_MAX",	CONSTANT,	_POSIX_SSIZE_MAX,
	"_POSIX_STREAM_MAX",	CONSTANT,	_POSIX_STREAM_MAX,
	"_POSIX_TZNAME_MAX",	CONSTANT,	_POSIX_TZNAME_MAX,
	"_POSIX_VDISABLE",	PATHCONF,	_PC_VDISABLE,
	"_POSIX_VERSION",	SYSCONF,	_SC_VERSION,
	(char *)0,		  0,		0,
	};

main(int argc, char **argv)
{
	int i;		/* index */
	size_t size;    /* size of buffer necessary for confstr */
	char *buf;      /* buffer necessary for confstr */
	int ret;	/* return value */

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:getconf");

	errno = 0;

       /*  
	*  Getconf only expects the following two formats:
	*
	*      getconf system_var
	*      getconf path_var pathname
	*
	*  Give usage and exit if not correct format.
	*/

	if ((argc < 2) || (argc > 3))
		usage();


	while((i = getopt(argc, argv, "")) != EOF)
		switch(i) {

		case '?':
			usage();
			break;
		}

       /*
	* The first argument is either the system or path variable
	* to be queried.  Convert to the correct index via the table.
	*/

    	for (i = 0; i < sizeof(value)/sizeof(value[0]) - 1; i++) {
		ret = strcmp(argv[1], value[i].name);

	       /*
		*  If match is found, break the loop and call correct
		*  interpreting routine.
		*/
		if (ret == 0)
			break;

	       /*
		*  If a string bigger than the argument is found in the
		*  table (which is sorted), then we know that it is an
		*  invalid argument.  Exit with error status
		*/
		if (ret < 0)   {
			pfmt(stderr, MM_ERROR,
			":959:Specified variable is not valid on this system\n");
			exit(2);
		}
	}

	/* If did not find argument in table, exit with error status */
	if (value[i].name == (char *)0) {
		pfmt(stderr, MM_ERROR,
		    ":959:Specified variable is not valid on this system\n");
		exit(2);
	}

       /*
	*  When there is only one argument to getconf, it is assumed
	*  to be a system variable.  If the variable is valid and is
	*  defined on the system, print the associated value.  If it is 
	*  valid, but not defined, print 'undefined'. Otherwise exit
	*  with >0.
	*/

	if (argc == 2) { 
		switch (value[i].kind) {
			case CONSTANT:
				printf("%d\n", value[i].numb);
				exit(0);
				break;

			case SYSCONF:
				ret = sysconf(value[i].numb);
				if (ret != -1) {
					printf("%d\n", ret);
					exit(0);
				}
				else if (errno == 0) {
					pfmt(stderr, MM_NOSTD,
						":960:undefined\n");
					exit(0);
				}
				else 
					exit(1);
				break;

			case CONFSTR:
				size = confstr(value[i].numb, (char *)0, 0);
				if (size == 0) {  /* Argument is not defined */
					if (errno == 0) {
						pfmt(stderr, MM_NOSTD,
							":960:undefined\n");
						exit(0);
					}
					else
						exit(1);
				}
				if ((buf = malloc(size)) == NULL) 
					perror(gettxt(":961", " getconf"));
				confstr(value[i].numb, buf, size);
				printf("%s\n", buf);
				exit(0);
				break;

			default: 
				pfmt(stderr, MM_ERROR,
				  ":962:Must specify pathname with path_var\n");
			exit(2);
		} /* switch */

       /*
	*  When there are two arguments to getconf, it is assumed
	*  to be a path variable.  If the variable is valid and is
	*  defined on the system, print the associated value.  If it is 
	*  valid, but not defined, print 'undefined', else exit w/ value 1
	*/

	} else if (value[i].kind == PATHCONF) {
		ret = pathconf(argv[2], value[i].numb);
		if (ret != -1) {
			printf("%d\n", ret);
			exit(0);
		}
		else if (errno == 0) {
			pfmt(stderr, MM_NOSTD, ":960:undefined\n");
			exit(0);
                }
		else
			exit(1);
	} else {
		pfmt(stderr, MM_ERROR,
			":963:Must not specify pathname with system_var\n");
		exit(2);
	}
	
}
	
usage(void)
{
	pfmt(stderr, MM_ACTION, ":964:Usage: getconf system_var\n");
	pfmt(stderr, MM_NOSTD, ":965:\t\tgetconf path_var pathname\n");
	exit(2);
}


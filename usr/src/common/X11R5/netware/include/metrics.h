/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include <mas.h>
struct met {
	char *title;
	uint32 met;
	caddr_t met_p;	
	double intv;
	double cooked;
};

/*
 *	double long typedef - needed for freemem and freeswap
 */
struct dl {
	uint32 lo;
	uint32 hi;
};
typedef struct dl dl_t;

struct dblmet {
	char *title;
	dl_t met;
	caddr_t met_p;	
	double intv;
	double cooked;
};

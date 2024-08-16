/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lprof:libprof/common/lst_str.h	1.2"
/******STRUCTURE FOR MAINTAINING USED FUNCTION LISTS******/


/*******USED FUNCTION LIST*******/

struct caFUNCLIST
   {	char	*name;			 /*   Function Entry Name  */
	struct	caFUNCLIST   *next_func; /* Pointer to next element*/
    };

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* LINTLIBRARY */

#include "users.h"


/*	from file usermgmt.c */

int putuser ( char * user, USER * pri_s )
{
    static int _returned_value;
    return _returned_value;
}

USER * getuser ( char * user )
{
    static USER * _returned_value;
    return _returned_value;
}

int deluser ( char * user )
{
    static int _returned_value;
    return _returned_value;
}

int getdfltpri ( void )
{
    static int _returned_value;
    return _returned_value;
}

int trashusers ( void )
{
    static int _returned_value;
    return _returned_value;
}


/*	from file loadpri.c */
struct user_priority * ld_priority_file ( char * path )
{
    static struct user_priority * _returned_value;
    return _returned_value;
}

int add_user ( struct user_priority * ppri_tbl, char * user, int limit )
{
    static int _returned_value;
    return _returned_value;
}

char * next_user ( FILE * f, char * buf, char ** pp )
{
    static char * _returned_value;
    return _returned_value;
}

del_user ( struct user_priority * ppri_tbl, char * user )
{
    static  _returned_value;
    return _returned_value;
}


/*	from file storepri.c */
print_tbl ( struct user_priority * ppri_tbl )
{
    static  _returned_value;
    return _returned_value;
}

output_tbl ( FILE * f, struct user_priority * ppri_tbl )
{
    static  _returned_value;
	return _returned_value;
}

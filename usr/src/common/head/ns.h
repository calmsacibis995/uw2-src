/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head.usr:ns.h	1.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * NS functions
 */
enum nsfuncs {
	NSBYNAM   = 0, 
	NSBYNUM   = 1, 
	NSGETENT  = 2, 
	NSSETENT  = 3, 
	NSENDENT  = 4
};

/*
 * Databases
 */
enum nsdbs {
	PWDDB   = 0, 
	GRPDB   = 1, 
	NETDB   = 2, 
	HOSTDB  = 3, 
	PROTODB = 4, 
	SERVDB  = 5, 
	ETHDB   = 6, 
	RPCDB   = 7, 
	MAXDB   = 8
};
/*
 * Action codes
 */
enum nsaction {
	NS_SUCCESS  = 0, /* entry was found */
	NS_NOTFOUND = 1, /* entry was not found */
	NS_UNAVAIL  = 2, /* service not available */
	NS_TRYAGAIN = 3, /* service was busy, try again later */
};

#define NS_GETNAME(db, arg, type) \
{\
	void *(*fptr)();\
	if (fptr = (void *(*)())_nsload(db, NSBYNAM)) \
		return((type)(*fptr)(db, arg, (char *)0)); \
}

#define NS_GETNUM(db, arg, type) \
{\
	void *(*fptr)();\
	if (fptr = (void *(*)())_nsload(db, NSBYNUM)) \
		return((type)(*fptr)(db, arg, (char *)0)); \
}
#define NS_GETNAME2(db, arg1, arg2, type) \
{\
	void *(*fptr)();\
	if (fptr = (void *(*)())_nsload(db, NSBYNAM)) \
		return((type)(*fptr)(db, arg1, arg2)); \
}

#define NS_GETNUM2(db, arg1, arg2, type) \
{\
	void *(*fptr)();\
	if (fptr = (void *(*)())_nsload(db, NSBYNUM)) \
		return((type)(*fptr)(db, arg1, arg2)); \
}
#define NS_GETENT(db, type) \
{\
	void *(*fptr)();\
	if (fptr = (void *(*)())_nsload(db, NSGETENT)) \
		return((type)(*fptr)(db)); \
}
#define NS_SETENT(db, arg) \
{\
	void *(*fptr)();\
	if (fptr = (void *(*)())_nsload(db, NSSETENT)) \
		return((int)(*fptr)(db, arg)); \
}
#define NS_ENDENT(db) \
{\
	void *(*fptr)();\
	if (fptr = (void *(*)())_nsload(db, NSENDENT)) {\
		(*fptr)(db);\
		return;\
	}\
}


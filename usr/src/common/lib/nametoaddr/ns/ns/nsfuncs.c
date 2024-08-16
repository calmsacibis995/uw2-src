/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/ns/ns/nsfuncs.c	1.4"
#ident  "$Header: $"

#include <stdio.h>
#include <ns.h>
#ifdef _REENTRANT
#include "ns_mt.h"
#endif

extern char *nis_getpwnam(), *nis_getpwuid(), *nis_getpwent(), 
	*nis_getgrnam(), *nis_getgrgid(), *nis_getgrent(),
	*nis_getpwentry(), *nis_getnetbyname(), *nis_getnetbyaddr(),
	*nis_getnetent(), *nis_gethostent(), *nis_getprotobyname(), 
	*nis_getprotobynumber(), *nis_getprotoent(),
	*nis_getservbyname(), *nis_getservbyport(), 
	*nis_getservent(), *nis_ether_hostton(), *nis_ether_ntohost(),
	*nis_getrpcbyname(), *nis_getrpcbynumber(), *nis_getrpcent();

extern char *_getnetbyname(), *_getnetbyaddr(), *_getnetent(), 
	*_gethtent(), *_getprotobyname(), *_getprotobynumber(), 
	*_getprotoent(), *_getservbyname(), *_getservbyport(), 
	*_getservent(), *_getrpcbyname(), *_getrpcbynumber(), 
	*_getrpcent(), *_ether_hostton(), *_ether_ntohost();

extern int nis_initgroups(), _initgroups();

extern int nis_setpwent(), nis_setgrent(), nis_setnetent(),
	nis_sethostent(), nis_setprotoent(), nis_setservent(),
	nis_setrpcent();

extern void nis_endpwent(), nis_endgrent(), nis_endnetent(),
	nis_endhostent(), nis_endprotoent(), nis_endservent(),
	nis_endrpcent();

extern int _setnetent(), _sethtent(), _setprotoent(), 
	_setservent(), _setrpcent();

extern void _endnetent(), _endhtent(), _endprotoent(), 
	_endservent(), _endrpcent();

struct _nsdata {
	char *(*func)();
	int  db;
};

/*
 * _get_nsdata allocates thread safe data
 */
static struct _nsdata *
_get_nsdata()
{
	static struct _nsdata ndata;

#ifdef _REENTRANT
	struct _ns_tsd *key_tbl;

	/*
	 * This is the case of the initial thread
	 */
	if (FIRST_OR_NO_THREAD)
		return(&ndata);
	/*
	 * This is the case of threads other than the first.
	 */
	key_tbl = (struct _ns_tsd *)
		_mt_get_thr_specific_storage(_ns_key,_NS_KEYTBL_SIZE);
	if (key_tbl == NULL)
		return (NULL);
	if (key_tbl->ns_getent_p == NULL)
		key_tbl->ns_getent_p = 
			(struct _nsdata *)calloc(1,sizeof(struct _nsdata));
	return((struct _nsdata *)key_tbl->ns_getent_p);
#else  /* ! _REENTRANT */
	return(&ndata);
#endif
}

#ifdef _REENTRANT
void
_free_ns_getent(p)
    void *p;
{
    if (FIRST_OR_NO_THREAD)
        return;
    if (p != NULL)
        free(p);
    return;
}
#endif

/*
 * ns_getbyname is called from the NS_GETBYNAME macro which
 * retrieves a entry by the given name.  First the NIS routine
 * is tried. If it fails because NIS is down, the parse routine
 * is called which looks the local database for the entry.
 *
 * Note: arg2 is NULL in some cases.
 */
char *
ns_getbyname(db, arg1, arg2)
	int db;
	char *arg1, *arg2;
{
	register char *(*nisfunc)(), *(*parse)();
	register char *ptr;

	nisfunc = parse = 0;
	switch(db) {
	case PWDDB:
		nisfunc = nis_getpwnam;
		break;
	case GRPDB:
		nisfunc = nis_getgrnam;
		break;
	case NETDB:
		nisfunc = nis_getnetbyname;
		parse = _getnetbyname;
		break;
	case PROTODB:
		nisfunc = nis_getprotobyname;
		parse = _getprotobyname;
		break;
	case SERVDB:
		nisfunc = nis_getservbyname;
		parse = _getservbyname;
		break;
	case ETHDB:
		nisfunc = nis_ether_hostton;
		parse = _ether_hostton;
		break;
	case RPCDB:
		nisfunc = nis_getrpcbyname;
		parse = _getrpcbyname;
		break;
	case HOSTDB:
		/*
		 * gethostbyname() uses netdir interface
		 */
	/* FALLTHRU */
	default:
		return(NULL);
	}

	ptr = (*nisfunc)(arg1, arg2);

	/*
	 * If NIS is up, a non-NULL value will be return
	 * except in the case of ETHDB, which return 0 for
	 * success and -1 for failure.
	 */
	if (db == ETHDB) {
		if (!ptr)
			return(ptr);
	} else if (ptr){
		return(ptr);
	}

	/*
	 * see why NIS failed
	 */
	switch(get_nsaction()){
		case NS_UNAVAIL:
			/*
			 * NIS is down
			 */
			break;
		default:
			/*
			 * NIS was up, but the entry was not found
			 */
			return(NULL);
	}
	/*
	 * Check to see if we can parse the local database
	 */
	if (parse) 
		return((*parse)(arg1, arg2));

	return(NULL);
}
/*
 * ns_getbynum is called from the NS_GETBYNUM macro which
 * retrieves a entry by the given number or address.  First the 
 * NIS routine is tried. If it fails because NIS is down, 
 * the parse routine * is called which looks the local 
 * database for the entry.
 *
 * Note: arg2 is NULL in some cases.
 */
char *
ns_getbynum(db, arg1, arg2)
	int db;
	char *arg1, *arg2;
{
	register char *(*nisfunc)(), *(*parse)();
	register char *ptr;

	nisfunc = parse = 0;

	switch(db) {
	case PWDDB:
		nisfunc = nis_getpwuid;
		break;
	case GRPDB:
		nisfunc = nis_getgrgid;
		break;
	case NETDB:
		nisfunc = nis_getnetbyaddr;
		parse = _getnetbyaddr;
		break;
	case PROTODB:
		nisfunc = nis_getprotobynumber;
		parse = _getprotobynumber;
		break;
	case SERVDB:
		nisfunc = nis_getservbyport;
		parse = _getservbyport;
		break;
	case ETHDB:
		nisfunc = nis_ether_ntohost;
		parse = _ether_ntohost;
		break;
	case RPCDB:
		nisfunc = nis_getrpcbynumber;
		parse = _getrpcbynumber;
		break;
	case HOSTDB:
		/*
		 * gethostbyname() uses netdir interface
		 */
	/* FALLTHRU */
	default:
		return(NULL);
	}

	ptr = (*nisfunc)(arg1, arg2);

	/*
	 * If NIS is up, a non-NULL value will be return
	 * except in the case of ETHDB, which return 0 for
	 * success and -1 for failure.
	 */
	if (db == ETHDB) {
		if (!ptr)
			return(ptr);
	} else if (ptr){
		return(ptr);
	}

	/*
	 * see why NIS failed
	 */
	switch(get_nsaction()){
		case NS_UNAVAIL:
			/*
			 * NIS is down
			 */
			break;
		default:
			/*
			 * NIS was up, but the entry was not found
			 */
			return(NULL);
	}

	/*
	 * Check to see if we can parse the local database
	 */
	if (parse) {
		return((*parse)(arg1, arg2));
	}

	return(NULL);

}
/*
 * ns_getent is called from the NS_GETENT macro. It returns
 * every entry in the database. First the NIS is tried. If
 * NIS is down, the local database is used, except in the
 * case of PWDDB and GRPDB.
 */ 
char *
ns_getent(db)
int db;
{
	register struct _nsdata *ns =  _get_nsdata();
	register char *ptr;
	static char *(*func)();
	static char usednis;
	char *(*parse)();

	if (ns == NULL)
		return(NULL);

	/*
	 * First time thru, set func and db to the
	 * appropriate function and database. Once func
	 * is set, it will used until a different database
	 * is needed.
	 */
	parse = 0;
	if (ns->func == NULL || ns->db != db){
		ns->db = db;
		switch(db) {
		case PWDDB:
			ns->func = nis_getpwent;
			break;
		case GRPDB:
			ns->func = nis_getgrent;
			break;
		case NETDB:
			ns->func = nis_getnetent;
			parse = _getnetent;
			break;
		case HOSTDB:
			ns->func = nis_gethostent;
			parse = _gethtent;
			break;
		case PROTODB:
			ns->func = nis_getprotoent;
			parse = _getprotoent;
			break;
		case SERVDB:
			ns->func = nis_getservent;
			parse = _getservent;
			break;
		case RPCDB:
			ns->func = nis_getrpcent;
			parse = _getrpcent;
			break;
		case ETHDB:
		/* FALLTHRU */
		default:
			return(NULL);
		}
		/*
		 * First try the NIS database
		 */
		if (ptr = (*((ns)->func))()){
			return(ptr);
		}

		/*
		 * See why NIS failed
		 */
		switch(get_nsaction()){
		case NS_UNAVAIL:
			/*
			 * NIS is down 
			 */
			break;
		case NS_SUCCESS:
			/* 
			 * We are at the end of the database
			 */
		/* FALLTHRU */
		default:
			return(NULL);
		}
		if (parse == 0)
			return(NULL);

		/*
		 * Use the local database
		 */
		ns->func = parse;
	}
	return((*((ns)->func))());
}
/*
 * ns_setent is called from the NS_SETENT macro.
 */
int
ns_setent(db, arg)
int db;
int arg;
{
	register int (*nisfunc)(), (*parse)();

	parse = 0;

	switch(db) {
	case PWDDB:
		nisfunc = nis_setpwent;
		break;
	case GRPDB:
		nisfunc = nis_setgrent;
		break;
	case NETDB:
		nisfunc = nis_setnetent;
		parse = _setnetent;
		break;
	case HOSTDB:
		nisfunc = nis_sethostent;
		parse = _sethtent;
		break;
	case PROTODB:
		nisfunc = nis_setprotoent;
		parse = _setprotoent;
		break;
	case SERVDB:
		nisfunc = nis_setservent;
		parse = _setservent;
		break;
	case RPCDB:
		nisfunc = nis_setrpcent;
		parse = _setrpcent;
		break;
	case ETHDB:
		/* 
		 * There is no setent routine for this database
		 */
	/* FALLTHRU */
	default:
		return(NULL);
	}

	/*
	 * Call both functions, because we don't know if NIS is up 
	 * which means we don't know what routine will be used in
	 * the getent call.
	 *
	 * Note: This adds extra overhead by call doing a open which
	 *       may or may not be needed. But it is less overhead
	 *       of doing a rpc call to see if NIS is up.
	 */
	(*nisfunc)(arg);
	if (parse)
		(*parse)(arg);

	return(NULL);
}
/*
 * ns_endent is called from the NS_ENDENT macro.
 */
void
ns_endent(db)
int db;
{
	register void (*nisfunc)(), (*parse)();

	parse = 0;
	switch(db) {
	case PWDDB:
		nisfunc = nis_endgrent;
		break;
	case GRPDB:
		nisfunc = nis_endgrent;
		break;
	case NETDB:
		nisfunc = nis_endnetent;
		parse = _endnetent;
		break;
	case HOSTDB:
		nisfunc = nis_endhostent;
		parse = _endhtent;
		break;
	case PROTODB:
		nisfunc = nis_endprotoent;
		parse = _endprotoent;
		break;
	case SERVDB:
		nisfunc = nis_endservent;
		parse = _endservent;
		break;
	case RPCDB:
		nisfunc = nis_endrpcent;
		parse = _endrpcent;
		break;
	case ETHDB:
	/* FALLTHRU */
	default:
		return;
	}

	/*
	 * Call both functions, since both setent routines were
	 * called in ns_setent().
	 */
	(*nisfunc)();
	if (parse)
		(*parse)();

	return;
}

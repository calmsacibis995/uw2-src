/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/netdir/netdir.c	1.7.11.11"
#ident	"$Header: $"

/*
 *+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *              PROPRIETARY NOTICE (Combined)
 *
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's UNIX(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *
 *
 *
 *              Copyright Notice
 *
 *  Notice of copyright on this source code product does not indicate
 *  publication.
 *
 *      (c) 1986,1987,1988.1989  Sun Microsystems, Inc
 *      (c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *      (c) 1990,1991  UNIX System Laboratories, Inc.
 *                All rights reserved.
 */

/*
 * netdir.c
 *
 * This file contains the library routines that do the name-to-address
 * translation.
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <xti.h>
#include <netconfig.h>
#include <netdir.h>
#include <string.h>
#include <sys/file.h>
#include <dlfcn.h>
#include "netdir_mt.h"

#undef _nderror
int	_nderror;

static struct translator {
	struct nd_addrlist	*(*gbn)();	/* _netdir_getbyname	*/
	struct nd_hostservlist	*(*gba)();	/* _netdir_getbyaddr	*/
	int			(*opt)();	/* _netdir_options	*/
	char			*(*t2u)();	/* _taddr2uaddr		*/
	struct netbuf		*(*u2t)();	/* _uaddr2taddr		*/
	void			*tr_fd;		/* library descriptor	*/
	char			*tr_name;	/* Full path		*/
	struct translator	*next;
};


static struct translator *xlate_list = NULL;
static struct translator *load_xlate();
static char * _get_ndbuf();

/*
 * This routine is the main routine.  It resolves host/service/transport
 * triples into a bunch of netbufs that should connect you to that particular
 * service. RPC uses it to contact the binder service (rpcbind).
 */
int
netdir_getbyname(tp, serv, addrs)
	const struct netconfig	*tp;	/* The network config entry	*/
	const struct nd_hostserv *serv;	/* the service, host pair	*/
	struct nd_addrlist	**addrs; /* the answer			*/
{
	struct translator	*t0;	/* pointer to head of list	*/
	struct translator	*t;	/* pointer to member of list	*/
	struct nd_addrlist	*nn;	/* the results			*/
	char			*lr;	/* routines to try		*/
	int			nderror;/* local copy of _nderror	*/
	int			i;	/* counts the routines		*/

	/* Handle null pointers */
	if (tp == (struct netconfig *)NULL
	 || serv == (struct nd_hostserv *)NULL
	 || addrs == (struct nd_addrlist **)NULL) {
		set_nderror(ND_BADARG);
		return(ND_BADARG);
	}

	/* Handle case of no libraries in list */
	if (tp->nc_nlookups == 0) {
		set_nderror(ND_NOLIB);
		return(ND_NOLIB);
	}

	/* Try each library *lr in the netconfig entry *tp */
	for (i = 0; i < tp->nc_nlookups; i++) {
		lr = *((tp->nc_lookups) + i);
		/*
		 * Multiple threads can safely check the translator list
		 * at the same time, so for now just set a read lock while
		 * determining the head of the list (xlate_list).
		 */
		RW_RDLOCK(&_netdir_xlist_lock);
		t0 = xlate_list; /* Locate head of translator list */
		RW_UNLOCK(&_netdir_xlist_lock);
		/* Examine each member *t in the translator list */
		for (t = t0; t; t = t->next) {
			/* Does list member *t refer to the desired library? */
			if (strcmp(lr, t->tr_name) == 0) {
				/* Yes, so use its name-to-address function */
				nn = (*(t->gbn))(tp, serv);
				/* Did translation produce a result? */
				if (nn) {
					/* Return desired address */
					*addrs = nn;
					return (0);
				} else {
					/* Did translation have an error? */
					if ((nderror = get_nderror()) < 0) {
						/* ??? Should this return -l? */
						return (nderror);
					}
					/* Give up on the library *lr,
					 * exiting loop with t != 0 */
					break;
				}
			}
		}

		/* Did we find library *lr on list ? */
		if (t != NULL) {
			/* Something went wrong, so try another library */
			continue;
		}
		/* 
		 * We didn't find library *lr in the list (t == NULL), so
		 * try loading it.
		 * In the multithreaded case, we must first check that
		 * another thread hasn't added it since we last looked,
		 * so we check any members added to the list since then.
		 * Only one thread may update the translator list, so
		 * we set a write lock.  This is a no-op without
		 * multithreading.
		 */
		RW_WRLOCK(&_netdir_xlist_lock);
		if (MULTI_THREADED) {
			/* Examine new members *t in the translator list */
			for (t = xlate_list; t != t0; t = t->next){
				if (strcmp(lr, t->tr_name) == 0) {
					/* A sibling thread has added a
					 * library to the list since we checked,
					 * so use its name-to-address routine.*/
					nn = (*(t->gbn))(tp, serv);
					/* Did translation produce a result? */
					if (nn) {
						RW_UNLOCK(&_netdir_xlist_lock);
						/* Return desired address */
						*addrs = nn;
						return (0);
					} else {
						/* Error in translation? */
						if ((nderror = get_nderror())
						    < 0) {
							RW_UNLOCK(
							   &_netdir_xlist_lock);
							/* ??? Should this
							 * return -l? */
							return (nderror);
						}
						break;
					}
				}
			} /* end second pass through the list */
	
			/* Did we find library *lr on list this time? */
			if (t != t0) {
				/* Library was on list, but something went
				 * wrong, so try another library */
				RW_UNLOCK(&_netdir_xlist_lock);
				continue;
			}
		} /* End second search for multithreaded case */
		/* 
		 * No other thread has added library *lr to the list,
		 * so we can safely load it.
		 */

		if ((t = load_xlate(lr)) != NULL) {
			/* add it to the list */
			t->next = xlate_list;
			xlate_list = t;
			RW_UNLOCK(&_netdir_xlist_lock);
			nn = (*(t->gbn))(tp, serv);
			if (nn) {
				/* Successful translation! */
				*addrs = nn;
				return (0);
			} else {
				if ((nderror = get_nderror()) < 0) {
					/* Translation failed with an error */
					/* ??? Return -l? */
					return (nderror);
				}
			} /* End check of translation */
			/* Translation failed without error, so
			 * it's ok to try another netconfig entry. */

		} else
			RW_UNLOCK(&_netdir_xlist_lock);
		/* End attempt to load functions */
		/* No translator was found or translation failed without error,
		 * so try another netconfig entry. */

	} /* End loop through netconfig entries */

	/* Give up:  cannot translate (host, service) with any library
	 * in the netconfig entry *tp. */
	return (get_nderror()); /* Assume translator set _nderror */
}

/*
 * This routine is similar to the one above, except that it tries to resolve
 * the name by the address passed.
 */
int
netdir_getbyaddr(tp, serv, addr)
	const struct netconfig	*tp;	/* The netconfig entry		*/
	struct nd_hostservlist	**serv;	/* the answer(s)		*/
	const struct netbuf	*addr;	/* the address we have		*/
{
	struct translator	*t0;	/* pointer to head of list	*/
	struct translator	*t;	/* pointer to translator list	*/
	struct nd_hostservlist	*hs;	/* the results			*/
	char			*lr;	/* routines to try		*/
	int			nderror;/* local copy of _nderror	*/
	int			i;	/* counts the routines		*/

	/* Handle null pointer */
	if (tp == (struct netconfig *)NULL
	 || serv == (struct nd_hostservlist **)NULL
	 || addr == (struct netbuf *)NULL) {
		set_nderror(ND_BADARG);
		return(ND_BADARG);
	}

	/* Handle case of no libraries in list */
	if (tp->nc_nlookups == 0) {
		set_nderror(ND_NOLIB);
		return(ND_NOLIB);
	}

	/* Try each library *lr in the netconfig entry *tp */
	for (i = 0; i < tp->nc_nlookups; i++) {
		lr = *((tp->nc_lookups) + i);
		/*
		 * Multiple threads can safely check the translator list
		 * at the same time, so for now just set a read lock.
		 */
		RW_RDLOCK(&_netdir_xlist_lock);
		t0 = xlate_list;
		RW_UNLOCK(&_netdir_xlist_lock);
		/* Examine each member *t in the translator list *xlate_list */
		for (t = t0; t; t = t->next) {
			/* Is list member *t for the desired library? */
			if (strcmp(lr, t->tr_name) == 0) {
				/* Yes, so use its address-to-name function */
				hs = (*(t->gba))(tp, addr);
				/* Did translation produce a result? */
				if (hs) {
					/* Return desired name */
					*serv = hs;
					return (0);
				} else {
					/* Did translation have an error? */
					if ((nderror = get_nderror()) < 0) {
						/* ??? Should this return -l? */
						return (nderror);
					}
					/* Give up on the library *lr,
					 * exiting loop with t != 0 */
					break;
				}
			}
		}

		/* Did we find library *lr on list ? */
		if (t != NULL) {
			/* Something went wrong, so try another library */
			continue;
		}
		/* 
		 * We didn't find library *lr in the list (t == NULL), so
		 * try loading it.
		 * In the multithreaded case, we must first check that
		 * another thread hasn't added it since we last looked,
		 * so we check the new members on the list;
		 * Only one thread may update the translator list, so
		 * we set a write lock.  This is a no-op without
		 * multithreading.
		 */
		RW_WRLOCK(&_netdir_xlist_lock);
		if (MULTI_THREADED) {
			/* Examine new members *t in the translator list */
			for (t = xlate_list; t != t0; t = t->next){
				if (strcmp(lr, t->tr_name) == 0) {
					/* A sibling thread has added a
					 * library to the list since we checked,
					 * so use its address-to-name routine.*/
					hs = (*(t->gba))(tp, addr);
					/* Did translation produce a result? */
					if (hs) {
						RW_UNLOCK(&_netdir_xlist_lock);
						/* Return desired name */
						*serv = hs;
						return (0);
					} else {
						/* Error in translation? */
						if ((nderror = get_nderror())
						    < 0) {
							RW_UNLOCK(
							   &_netdir_xlist_lock);
							/* ??? Should this
							 * return -l? */
							return (nderror);
						}
						break;
					}
				}
			} /* end second pass through the list */
	
			/* Did we find library *lr on list this time? */
			if (t != t0) {
				/* Library was on list, but something went
				 * wrong, so try another library */
				RW_UNLOCK(&_netdir_xlist_lock);
				continue;
			}
		} /* End second search for multithreaded case */
		/* 
		 * No other thread has added library *lr to the list,
		 * so we can safely load it.
		 */

		if ((t = load_xlate(lr)) != NULL) {
			/* add it to the list */
			t->next = xlate_list;
			xlate_list = t;
			RW_UNLOCK(&_netdir_xlist_lock);
			hs = (*(t->gba))(tp, addr);
			if (hs) {
				/* Successful translation! */
				*serv = hs;
				return (0);
			} else {
				if ((nderror = get_nderror()) < 0) {
					/* Translation failed with an error */
					/* ??? Should this return -l? */
					return (nderror);
				}
			} /* End check of translation */
			/* Translation failed without error, so
			 * it's ok to try another netconfig entry. */
		} else 
			RW_UNLOCK(&_netdir_xlist_lock);
		/* End attempt to load functions */
		/* No translator was found or translation failed without error,
		 * so try another netconfig entry. */

	} /* End loop through netconfig entries */

	/* Give up:  cannot translate address with any library
	 * in the netconfig entry *tp. /
	/* ??? Should this return -l? */
	return (get_nderror());
}

/*
 * The original purpose of this library routine was to merge universal
 * addresses with the address of the client caller, so that an address that
 * makes sense can be returned to the client caller.  The function has
 * also been enhanced to perform transport-independent setting of options.
 * The routine uses the same code as below (uaddr2taddr)
 * to search for the appropriate translation routine. However, it doesn't
 * bother trying a whole bunch of routines, since either the transport
 * can merge it or it can't.
 * ??? Is this strategy appropriate for setting options?
 */
int
netdir_options(tp, option, fd, par)
	const struct netconfig	*tp;	/* the netconfig entry		*/
	int			option;	/* option number		*/
	int			fd;	/* open file descriptor		*/
	const char		*par;	/* parameters if any		*/
{
	struct translator	*t0;	/* pointer to head of list	*/
	struct translator	*t;	/* pointer to translator list	*/
	char			*lr,	/* routines to try		*/
				*x;	/* the answer			*/
	int			i;	/* counts the routines		*/
	int			nderror;/* local copy of _nderror	*/

	/* Handle null pointer */
	if (tp == (struct netconfig *)NULL) {
		set_nderror(ND_BADARG);
		return(ND_BADARG);
	}

	/* Try each library *lr in the netconfig entry *tp */
	for (i = 0; i < tp->nc_nlookups; i++) {
		lr = *((tp->nc_lookups) + i);
		/*
		 * Multiple threads can safely check the translator list
	 	 * at the same time, so for now just set a read lock while
		 * determining the head of the list (xlate_list).
		*/
		RW_RDLOCK(&_netdir_xlist_lock);
		t0 = xlate_list;
		RW_UNLOCK(&_netdir_xlist_lock);
		/* Examine each member *t in the translator list */
		for (t = t0; t; t = t->next) {
			/* Does list member *t refer to the desired library? */
			if (strcmp(lr, t->tr_name) == 0) {
				return ((*(t->opt))(tp, option, fd, par));
			}
		}
		/* This point is reached only if the desired library is not
		 * in the translator list.
		 * Try loading it. 
		 * In the multithreaded case, we must first check that
		 * another thread hasn't added it since we last looked,
		 * so we check the new members on the list.
		 * Only one thread may update the translator list, so
		 * we set a write lock.	 This is a no-op without
		 * multithreading.
		 */
		RW_WRLOCK(&_netdir_xlist_lock);
		if (MULTI_THREADED) {
			/* Examine new members *t in the translator list */
			for (t = xlate_list; t != t0; t = t->next){
				/* Does list member *t refer to the desired
				 *library? */
				if (strcmp(lr, t->tr_name) == 0) {
					return ((*(t->opt))
						(tp, option, fd, par));
				}
			}
		}
		/* This point is reached only if the desired library
		 * is not in the translator list.
		*/
		if ((t = load_xlate(lr)) != NULL) {
			/* add it to the list */
			t->next = xlate_list;
			xlate_list = t;
			RW_UNLOCK(&_netdir_xlist_lock);
			/* 
			 * It's OK to return unlocked, since members
			 * are only added, never deleted or changed.
			 */
			return ((*(t->opt))(tp, option, fd, par));
		} else
			RW_UNLOCK(&_netdir_xlist_lock);
		/* End attempt to load functions */
		/* At this point, no translator was found,
		 * so try another netconfig entry. */
	}

	set_nderror(ND_NOLIB);
	return (-1);	/* No library in netconfig entry could be loaded. */
}

/*
 * This is the library routine for translating universal addresses to
 * transport specific addresses. Again it uses the same code as above
 * to search for the appropriate translation routine. However, it doesn't
 * bother trying a whole bunch of routines, since either the transport
 * can translate it or it can't.
 * ??? The current implementation loops through netconfig entries until
 * it has succeeded or until every matching provider has been tried.
 */
struct netbuf *
uaddr2taddr(tp, addr)
	const struct netconfig	*tp;	/* the netconfig entry		*/
	const char		*addr;	/* The address in question	*/
{
	struct translator	*t0;	/* pointer to head of list	*/
	struct translator	*t;	/* pointer to translator list	*/
	struct netbuf		*x;	/* the answer we want		*/
	char			*lr;	/* routines to try		*/
	int			i;	/* counts the routines		*/
	int			nderror;/* local copy of _nderror	*/

	/* Handle null pointer */
	if (tp == (struct netconfig *)NULL) {
		set_nderror(ND_BADARG);
		return((struct netbuf *)NULL);
	}

	/* Handle case of no libraries in list */
	if (tp->nc_nlookups == 0) {
		set_nderror(ND_NOLIB);
		return((struct netbuf *)NULL);
	}

	for (i = 0; i < tp->nc_nlookups; i++) {
		lr = *((tp->nc_lookups) + i);
		/*
		 * Multiple threads can safely check the translator list
		 * at the same time, so for now just set a read lock while
		 * determining the head of the list (xlate_list).
		 */
		RW_RDLOCK(&_netdir_xlist_lock);
		t0 = xlate_list;
		RW_UNLOCK(&_netdir_xlist_lock);
		/* Examine each member *t in the translator list */
		for (t = t0; t; t = t->next) {
			/* Does list member *t refer to the desired library? */
			if (strcmp(lr, t->tr_name) == 0) {
				/* Yes, so use its uaddr-to-taddr function */
				x = (*(t->u2t))(tp, addr);
				/* Did translation produce a result? */
				if (x) {
					/* Return desired address */
					return (x);
				}
				/* Did translation have an error? */
				if (get_nderror() < 0) {
					return (NULL);
				}
			}
		}
		/* Did we find library *lr on list ? */
		if (t != NULL) {
			/* Something went wrong, so try another library */
			continue;
		}
		/* 
		 * We didn't find library *lr in the list (t == NULL), so
		 * try loading it.
		 * In the multithreaded case, we must first check that
		 * another thread hasn't added it since we last looked,
		 * so we check any new members in the list.
		 * Only one thread may update the translator list, so
		 * we set a write lock.  This is a no-op without
		 * multithreading.
		 */
		RW_WRLOCK(&_netdir_xlist_lock);
		if (MULTI_THREADED) {
			/* Examine new members *t in the translator list */
			for (t = xlate_list; t != t0; t = t->next){
				/* Does *t refer to the desired library? */
				if (strcmp(lr, t->tr_name) == 0) {
					/* Yes, so use its u2t function */
					x = (*(t->u2t))(tp, addr);
					/* Did translation produce a result? */
					if (x) {
						RW_UNLOCK(&_netdir_xlist_lock);
						/* Return desired address */
						return (x);
					}
					/* Did translation have an error? */
					if (get_nderror() < 0) {
						RW_UNLOCK(
						   &_netdir_xlist_lock);
						/* Translation error */
						return (NULL);
					}
				}
			} /* end second pass through the list */
			/* Did we find library *lr on list this time? */
			if (t != t0) {
				/* Library was on list, but something went
				 * wrong, so try another library */
				RW_UNLOCK(&_netdir_xlist_lock);
				continue;
			}
		} /* End second search for multithreaded case */

		/* 
		 * No other thread has added library *lr to the list,
		 * so we can safely load it.
		 */

		if ((t = load_xlate(lr)) != NULL) {
			/* add it to the list */
			t->next = xlate_list;
			xlate_list = t;
			RW_UNLOCK(&_netdir_xlist_lock);
			x = (*(t->u2t))(tp, addr);
			if (x) {
				return (x);
			}
			if (get_nderror() < 0) {
				return (NULL);
			} /* End check of translation */
			/* Translation failed without error, so
			 * it's ok to try another netconfig entry. */
		} else 
			RW_UNLOCK(&_netdir_xlist_lock);

	} /* End loop through netconfig entries */

	/* Give up:  cannot translate universal address with any library
	 * in the netconfig entry *tp. */
	/* ??? Assume translator set _nderror */
	return (NULL);	/* No one works */
}

/*
 * This is the library routine for translating transport specific
 * addresses to universal addresses. Again it uses the same code as above
 * to search for the appropriate translation routine. Only it doesn't
 * bother trying a whole bunch of routines since either the transport
 * can translate it or it can't.
 */
char *
taddr2uaddr(tp, addr)
	const struct netconfig	*tp;	/* the netconfig entry		*/
	const struct netbuf	*addr;	/* The address in question	*/
{
	struct translator	*t0;	/* pointer to first translator  */
	struct translator	*t;	/* pointer to any translator    */
	char			*lr;	/* routines to try		*/
	char			*x;	/* the answer			*/
	int			i;	/* counts the routines		*/
	int			nderror;/* local copy of _nderror	*/

	/* Handle null pointer */
	if (tp == (struct netconfig *)NULL) {
		set_nderror(ND_BADARG);
		return(NULL);
	}

	/* Handle case of no libraries in list */
	if (tp->nc_nlookups == 0) {
		set_nderror(ND_NOLIB);
		return(NULL);
	}

	/* Try each library *lr in the netconfig entry *tp */
	for (i = 0; i < tp->nc_nlookups; i++) {
		lr = *((tp->nc_lookups) + i);
		/*
		 * Multiple threads can safely check the translator list
	 	 * at the same time, so for now just set a read lock while
		 * determining the head of the list (xlate_list).
		*/
		RW_RDLOCK(&_netdir_xlist_lock);
		t0 = xlate_list;
		RW_UNLOCK(&_netdir_xlist_lock);
		/* Examine each member *t in the translator list */
		for (t = t0; t; t = t->next) {
			/* Does list member *t refer to the desired library? */
			if (strcmp(lr, t->tr_name) == 0) {
				/* Yes, so use its taddr-to-uaddr function */
				x = (*(t->t2u))(tp, addr);
				if (x) {
					return (x);
				}
				if (get_nderror() < 0) {
					return (NULL);
				}
			}
		}
		/* Did we find library *lr on list ? */
		if (t != NULL) {
			/* Something went wrong, so try another library */
			continue;
		}
		/* 
		 * We didn't find library *lr in the list (t == NULL), so
		 * try loading it.
		 * In the multithreaded case, we must first check that
		 * another thread hasn't added it since we last looked,
		 * so we check the new members in the list.
		 * Only one thread may update the translator list, so
		 * we set a write lock.  This is a no-op without
		 * multithreading.
		 */
		RW_WRLOCK(&_netdir_xlist_lock);
		if (MULTI_THREADED) {
			/* Examine new members *t in the translator list */
			for (t = xlate_list; t != t0; t = t->next){
				/* Does *t refer to the desired library? */
				if (strcmp(lr, t->tr_name) == 0) {
					/* Yes, so use its t2u function */
					x = (*(t->t2u))(tp, addr);
					/* Did translation produce a result? */	
					if (x) {
						RW_UNLOCK(&_netdir_xlist_lock);
						return (x);
					}
					/* Did translation have an error? */
					if (get_nderror() < 0) {
						RW_UNLOCK(&_netdir_xlist_lock);
						return (NULL);
					}
				}
			} /* End second pass through the list */

			/* Did we find library *lr on list this time? */
			if (t != t0) {
				/* Library was on list, but something went
				 * wrong, so try another library */
				RW_UNLOCK(&_netdir_xlist_lock);
				continue;
			}

		} /* End second search for multithreaded case */

		/* 
		 * No other thread has added library *lr to the list,
		 * so we can safely load it.
		 */

		if ((t = load_xlate(lr)) != NULL) {
			/* add it to the list */
			t->next = xlate_list;
			xlate_list = t;
			x = (*(t->t2u))(tp, addr);
			if (x) {
				RW_UNLOCK(&_netdir_xlist_lock);
				return (x);
			}
			if (get_nderror() < 0) {
				RW_UNLOCK(&_netdir_xlist_lock);
				return (NULL);
			} /* End check of translation */
			/* Translation failed without error, so
			 * it's ok to try another netconfig entry. */
		} else 
			RW_UNLOCK(&_netdir_xlist_lock);

	} /* End loop through netconfig entries */

	/* Give up:  cannot translate transport address with any library
	 * in the netconfig entry *tp. */
	/* ??? Assume translator set _nderror */

	return (NULL);	/* No one works */
}

/*
 * This is the routine that frees the objects that netdir routines allocate.
 */
void
netdir_free(ptr, type)
	void	*ptr;	/* generic pointer	*/
	int	type;	/* thing we are freeing */
{
	struct netbuf		*na;
	struct nd_addrlist	*nas;
	struct nd_hostserv	*hs;
	struct nd_hostservlist	*hss;
	int			i;
	int			nderror;/* local copy of _nderror	*/

	/* Handle null pointer */
	if (ptr == (void *)NULL) {
		set_nderror(ND_BADARG);
		return;
	}

	switch (type) {
	case ND_ADDR :
		na = (struct netbuf *) ptr;
		free(na->buf);
		free((char *)na);
		break;

	case ND_ADDRLIST :
		nas = (struct nd_addrlist *) ptr;
		for (na = nas->n_addrs, i = 0; i < nas->n_cnt; i++, na++) {
			free(na->buf);
		}
		free((char *)nas->n_addrs);
		free((char *)nas);
		break;

	case ND_HOSTSERV :
		hs = (struct nd_hostserv *) ptr;
		free(hs->h_host);
		free(hs->h_serv);
		free((char *)hs);
		break;

	case ND_HOSTSERVLIST :
		hss = (struct nd_hostservlist *) ptr;
		for (hs = hss->h_hostservs, i = 0; i < hss->h_cnt; i++, hs++) {
			free(hs->h_host);
			free(hs->h_serv);
		}
		free((char *)hss->h_hostservs);
		free((char *)hss);
		break;

	default :
		set_nderror(ND_UKNWN);
		break;
	}
}

/*
 * load_xlate is a routine that will attempt to dynamically link in the
 * file specified by the network configuration structure.
 */
static struct translator *
load_xlate(name)
	char	*name;		/* file name to load */
{
	struct translator	*t;
	int			nderror;/* local copy of _nderror	*/

	/* do a sanity check on the file ... */
	if (access(name, 00) != 0) {
		set_nderror(ND_ACCESS);
		return (NULL);
	}
	t = (struct translator *) malloc(sizeof (struct translator));
	if (!t) {
		set_nderror(ND_NOMEM);
		return (NULL);
	}
	t->tr_name = strdup(name);
	if (!t->tr_name) {
		set_nderror(ND_NOMEM);
		free((char *)t);
		return (NULL);
	}

	/* open for linking */
	t->tr_fd = _dlopen(name, RTLD_NOW);
	if (t->tr_fd == NULL) {
		set_nderror(ND_OPEN);
		goto error;
	}

	/* Resolve the getbyname symbol */
	t->gbn = (struct nd_addrlist *(*)())_dlsym(t->tr_fd,
				"_netdir_getbyname");
	if (!(t->gbn)) {
		set_nderror(ND_NOSYM);
		goto error;
	}

	/* resolve the getbyaddr symbol */
	t->gba = (struct nd_hostservlist *(*)())_dlsym(t->tr_fd,
				"_netdir_getbyaddr");
	if (!(t->gba)) {
		set_nderror(ND_NOSYM);
		goto error;
	}

	/* resolve the taddr2uaddr symbol */
	t->t2u = (char *(*)())_dlsym(t->tr_fd, "_taddr2uaddr");
	if (!(t->t2u)) {
		set_nderror(ND_NOSYM);
		goto error;
	}

	/* resolve the uaddr2taddr symbol */
	t->u2t = (struct netbuf *(*)())_dlsym(t->tr_fd, "_uaddr2taddr");
	if (!(t->u2t)) {
		set_nderror(ND_NOSYM);
		goto error;
	}

	/* resolve the netdir_options symbol */
	t->opt = (int (*)())_dlsym(t->tr_fd, "_netdir_options");
	if (!(t->opt)) {
		set_nderror(ND_NOSYM);
		goto error;
	}
	return (t);
error:
	free(t->tr_name);
	free((char *)t);
	return (NULL);
}

/*
 * This is a routine that returns a string related to the current
 * error in _nderror.
 */
char *
netdir_sperror()
{
/*
 * temporary solution until #define's get into the cross.
 * add these new errors specific to netdir_options()
 */
        /* an xti call failed; check get_t_errno() */
#ifndef ND_XTIERROR
#define ND_XTIERROR     12
#endif

        /* incorrect state to attempt t_bind() */
#ifndef ND_BADSTATE
#define ND_BADSTATE     13
#endif

	char	*str;
	int	nderror;	/* local copy of _nderror	*/

	str = _get_ndbuf();

	if (str == NULL)
		return (NULL);
	switch (nderror = get_nderror()) {
	case ND_NOMEM :
		(void) sprintf(str,
		    gettxt("uxnsl:148", "n2a: memory allocation failed"));
		break;
	case ND_OK :
		(void) sprintf(str,
		    gettxt("uxnsl:149", "n2a: successful completion"));
		break;
	case ND_NOHOST :
		(void) sprintf(str,
		    gettxt("uxnsl:150", "n2a: hostname not found"));
		break;
	case ND_NOSERV :
		(void) sprintf(str,
		    gettxt("uxnsl:151", "n2a: service name not found"));
		break;
	case ND_NOSYM :
		(void) sprintf(str,
		    gettxt("uxnsl:152",
			"n2a: symbol missing in shared object - %s"),
		    _dlerror());
		break;
	case ND_OPEN :
		(void) sprintf(str,
		    gettxt("uxnsl:153",
			"n2a: couldn't open shared object - %s"),
		    _dlerror());
		break;
	case ND_ACCESS :
		(void) sprintf(str,
		    gettxt("uxnsl:154",
			"n2a: access denied for shared object"));
		break;
	case ND_UKNWN :
		(void) sprintf(str,
		    gettxt("uxnsl:155",
			"n2a: attempt to free unknown object"));
		break;
	case ND_BADARG :
		(void) sprintf(str,
		    gettxt("uxnsl:156",
			"n2a: bad arguments passed to routine"));
		break;
	case ND_NOCTRL:
		(void) sprintf(str,
		    gettxt("uxnsl:157", "n2a: unknown option passed"));
		break;
	case ND_FAILCTRL:
		(void) sprintf(str,
		    gettxt("uxnsl:158", "n2a: control operation failed"));
		break;
	case ND_SYSTEM:
		(void) sprintf(str,
		    gettxt("uxnsl:159",
			"n2a: system error: %s"),
		    strerror(errno));
		break;
	case ND_NOERRMEM:
		(void) sprintf(str,
		    gettxt("uxnsl:160",
		       "n2a: no memory for error variable could be allocated"));
		break;
	case ND_NOLIB:
		(void) sprintf(str,
		    gettxt("uxnsl:161",
		       "n2a: no library in the netconfig list could be found"));
		break;
	case ND_XTIERROR:
		(void) sprintf(str,
		    gettxt("uxnsl:198", "n2a: XTI error: %s"),
		    t_strerror(get_t_errno()));
		break;
	case ND_BADSTATE:
		(void) sprintf(str,
		    gettxt("uxnsl:199",
		       "n2a: incorrect state to attempt t_bind"));
		break;
	default :
		(void) sprintf(str,
		    gettxt("uxnsl:162", "n2a: unknown error %d"), nderror);
		break;
	}
	return (str);
}

/*
 * This is a routine that prints out strings related to the current
 * error in _nderror. Like perror() it takes a string to print with a
 * colon first.
 */
void
netdir_perror(s)
	const char	*s;
{
	char	*err;

	err = netdir_sperror();

	/* Handle null pointer */
	if (s == NULL) {
		set_nderror(ND_BADARG);
		return;
	}

	if(strlen(s))
	    fprintf(stderr,
		"%s: %s\n", s, err ? err: gettxt("uxnsl:163", "nd: error"));
	else
	    fprintf(stderr,
		"%s\n", err ? err: gettxt("uxnsl:163", "nd: error"));
	return;
}

static char *
_get_ndbuf()
{
	static char _ndbuf[128];
#ifdef _REENTRANT
	struct _netdir_tsd *key_tbl;
	/*
	 * This is the case of the initial thread or no threads.
	 */
        if (FIRST_OR_NO_THREAD) return (&_ndbuf[0]);
	
	/*
	 * This is the case of other threads.
	 */
	key_tbl = (struct _netdir_tsd *)
		  _mt_get_thr_specific_storage(_netdir_key, NETDIR_KEYTBL_SIZE);
	if (key_tbl == NULL) return (NULL);
	if (key_tbl->errbuf_p == NULL) {
		key_tbl->errbuf_p = calloc(1,128);
	}
	return((char *)key_tbl->errbuf_p);
#else
	return (&_ndbuf[0]);
#endif /* _REENTRANT */
}

int
get_nderror()
{
#ifdef _REENTRANT
	struct _netdir_tsd *key_tbl;

	/*
	 * This is the case of the initial thread.
	 */
	if (FIRST_OR_NO_THREAD) return (_nderror);

	key_tbl = (struct _netdir_tsd *)
		  _mt_get_thr_specific_storage(_netdir_key, NETDIR_KEYTBL_SIZE);
	if (key_tbl != NULL && key_tbl->_nderror_p != NULL)
		return(*(int *)key_tbl->_nderror_p);
	return (ND_NOERRMEM); 
#else
	return (_nderror);
#endif /* _REENTRANT */
}

int
set_nderror(errcode)
	int errcode;
{
#ifdef _REENTRANT
	struct _netdir_tsd *key_tbl;

	/*
	 * This is the case of the initial thread.
	 */
	if (FIRST_OR_NO_THREAD) {
		_nderror = errcode;
		return 0;
	}

	key_tbl = (struct _netdir_tsd *)
		  _mt_get_thr_specific_storage(_netdir_key, NETDIR_KEYTBL_SIZE);
	if (key_tbl == NULL) return -1;
	if (key_tbl->_nderror_p == NULL) 
		key_tbl->_nderror_p = calloc(1, sizeof(int));
	if (key_tbl->_nderror_p == NULL) return -1;
	*(int *)key_tbl->_nderror_p = errcode;
#else
	_nderror = errcode;
#endif /* _REENTRANT */
	return 0;
}

const int *
_netdir_nderror()
{
#ifdef _REENTRANT
	struct _netdir_tsd *key_tbl;
	static int __nderror;

	/*
	 * This is the case of the initial thread.
	 */
	if (FIRST_OR_NO_THREAD) return (&_nderror);

	key_tbl = (struct _netdir_tsd *)
		  _mt_get_thr_specific_storage(_netdir_key, NETDIR_KEYTBL_SIZE);
	if (key_tbl != NULL && key_tbl->_nderror_p != NULL)
		return((int *)key_tbl->_nderror_p);
	return (&__nderror); 
#else
	return (&_nderror);
#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_netdir__nderror(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

void
_free_netdir_errbuf(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

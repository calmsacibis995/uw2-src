/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/autoconf/confmgr/confmgr.c	1.29"
#ident	"$Header: $"

/*
** Autoconfig -- Configuration Manager
**
** This is the platform independent part of the Configuration Manager
** Module for the Autoconfig feature.  It provides the interface into
** the Resource Manager database for device drivers and provides
** routines to attach/detach driver interrupt routines.
*/

/*

Things to do:

- ?? Check for some flag for VERIFY routines and return from intr routines
  without doing anything
- New way to flag DCU to run if resmgr changes (env var)
- Need to purge empty keys

*/

#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <mem/kmem.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>
#include <io/ddi.h>

/* Temporary, for debugging only */
#undef STATIC
#define STATIC

/*
** Internal Structure Definition
**
** struct cookie_list is used to save a linked list on "magic cookies"
** that are created when interrupts are attached and used when the
** interrupts are detached.
*/

struct cookie_list
{
	void			*cl_cookie;
	struct cookie_list	*cl_next;
};

/*
** Externs for symbols within this module.
*/

extern int		cm_find_match();

/*
** Externs for symbols outside this module.
*/

extern boolean_t	_cm_ro_param();

/*
** cm_init()
**
** Calling/Exit State:
**	Resmgr database is modified to reflect the changes to devices
**	represented in NVRAM.
**
** Descriptions:
**	Calls the platform specific initialization routine.
*/

void
cm_init( void )
{
	cm_init_p();
}

/*
** cm_getversion()
**
** Calling/Exit State: None
**
** Description:
**	Returns current version number of confmgr.
*/

int
cm_getversion( void )
{
	return CM_VERSION;
}

/*
** cm_getnbrd()
**
** Calling/Exit State: None
**
** Description:
**	Returns number of keys in resmgr database with MODNAME=modname.
*/

int
cm_getnbrd( char *modname )
{
	struct rm_args	rma;
	char		namebuf[ NAMESZ ];
	int		nbrds;

	if ( modname == NULL )
		return 0;

	/* Initialize rma for call to cm_find_match() */

	/*
	** I need to assume the modname was initially stored with a null
	** terminator.  If I don't assume this, and the modname was stored
	** with a null, then I will fail to find the match.
	**
	** If however, the modname was NOT stored with a null, this will
	** still CORRECTLY find the match.
	*/

	rma.rm_vallen = strlen( modname ) + 1;	/* + 1 for null terminator */

	if ( rma.rm_vallen > NAMESZ )		/* illegal modname */
		return 0;

	rma.rm_val = namebuf;
	(void)strcpy( rma.rm_param, CM_MODNAME );
	rma.rm_key = RM_NULL_KEY;
	rma.rm_n = 0;		/* I expect only 1 value for this param */

	nbrds = 0;

	while ( cm_find_match( &rma, modname ) == 0 )
		nbrds++;

	return nbrds;
}

/*
** cm_getbrdkey()
**
** Calling/Exit State: None
**
** Description:
**	Returns the key for the n-th entry in the resmgr database with
**	MODNAME=modname.
**
**	Returns RM_NULL_KEY if there is no n-th entry.
**
*/

rm_key_t
cm_getbrdkey( char *modname, int bdinst )
{
	struct rm_args	mod_rma;
	rm_key_t	ret = RM_NULL_KEY;
	char		namebuf[ NAMESZ ];

	if ( bdinst < 0  ||  modname == NULL )
		return ret;

	/* Initialize mod_rma for call to cm_find_match */

	mod_rma.rm_vallen = strlen( modname ) + 1;	/* +1 for null */

	if ( mod_rma.rm_vallen > NAMESZ )
		return ret;

	mod_rma.rm_val = namebuf;
	(void)strcpy( mod_rma.rm_param, CM_MODNAME );
	mod_rma.rm_key = RM_NULL_KEY;
	mod_rma.rm_n = 0;		/* I expect only 1 value for CM_MODNAME */

	/* Iterate through keys with matching modname */

	while ( cm_find_match( &mod_rma, modname ) == 0 )
	{
		if ( bdinst-- == 0 )
		{
			ret = mod_rma.rm_key;
			break;
		}
	}

	return ret;
}

/*
** cm_delval()
**
** Calling/Exit State:
**	For given key, specified param and all associated values are
**	deleted from the resmgr database.
**
** Descriptions:
**	Deletes values from database based on key and param.
**
**	Fails is the param is read-only.
*/

int
cm_delval( cm_args_t *cma )
{
	struct rm_args	rma;

	if ( cma == NULL  ||  cma->cm_param == NULL  ||
				_cm_ro_param( cma->cm_param ) == B_TRUE )
		return EINVAL;

	(void)strncpy( rma.rm_param, cma->cm_param, RM_MAXPARAMLEN );

	rma.rm_key = cma->cm_key;
	return rm_delval( &rma );
}

/*
** cm_addval()
**
** Calling/Exit State:
**	New param/value pair added to key in resmgr database.
**
** Descriptions:
**	Either adds a new param/value pair to key, or appends new value
**	to param if param already exists for this key.
**
**	Fails if param is read-only.
*/

int
cm_addval( cm_args_t *cma )
{
	struct rm_args	rma;

	if ( cma == NULL  ||  cma->cm_param == NULL  ||
				_cm_ro_param( cma->cm_param ) == B_TRUE )
		return EINVAL;

	(void)strncpy( rma.rm_param, cma->cm_param, RM_MAXPARAMLEN );

	rma.rm_key = cma->cm_key;
	rma.rm_val = cma->cm_val;
	rma.rm_vallen = cma->cm_vallen;
	return rm_addval( &rma, UIO_SYSSPACE );
}

/*
** cm_getval()
**
** Calling/Exit State: None
**
** Descriptions:
**	Returns n-th value for given key and param.
*/

int
cm_getval( cm_args_t *cma )
{
	struct rm_args	rma;
	int		ret;

	if ( cma == NULL  ||  cma->cm_param == NULL )
		return EINVAL;

	(void)strncpy( rma.rm_param, cma->cm_param, RM_MAXPARAMLEN );

	rma.rm_key = cma->cm_key;
	rma.rm_val = cma->cm_val;
	rma.rm_vallen = cma->cm_vallen;
	rma.rm_n = cma->cm_n;
	ret = rm_getval( &rma, UIO_SYSSPACE );
	cma->cm_vallen = rma.rm_vallen;		/* rm_getval may have updated vallen */
	return ret;
}

/*
** cm_find_match()
**
** This routine is used to iterate through all the keys in the resmgr
** looking for a particular value associated with a specific param.
** The first time you call this routine you should initialize rm_args
** to:
**
**	rm_key = RM_NULL_KEY
**	rm_param = param of interest
**	rm_vallen = exact size of match
**	rm_val = ptr to buffer of size vallen
**	rm_n = N
**
**	val should point to the value you're looking for.
**	    it MUST be of length vallen also !!!
**
** The value of rm_n determines how many values of a multi-valued param
** are checked.  Setting rm_n=0 will only check the first value.  Setting
** rm_n=N will check values 0 - N.  If you don't know how many values
** there may be and you want to check them all, set rm_n=INT_MAX.
**
** On subsequent calls, rm_args should be passed in "as it was returned."
** It will pick up the iteration where it left off until it has walked
** through all the keys in resmgr.
**
** Returns:  0 if a match was found AND rm_key is set to matching key
**	    -1 otherwise
**
** Once a -1 is returned, calling it again without first reinitializing
** rm_args will return unpredictable values.
**
** It expects the buffer to be the correct size.  Values are defined to
** be uninterpreted byte strings.  It's impossible for this routine to
** determine a vallen.
**
** Exit state:
**		rm_key and rm_val may have changed.
*/

int
cm_find_match( struct rm_args *rma, void *val )
{
	size_t		vlen = rma->rm_vallen;
	uint		n = rma->rm_n;
	int		lcv;
	int		rv;

 	for ( ;; )
	{
		if ( rm_nextkey( rma ) != 0 )
		{
			rma->rm_n = n;
			return -1;		/* No more keys */
		}

		for ( lcv = 0; lcv <= n; lcv++ )
		{
			rma->rm_n = lcv;

			/*
			** I MUST bzero the buffer before each call to rm_getval.
			** Consider this case: I want to match ABCD of len = 4
			** (remember these are just bytes of data, NOT a null
			** terminated string.)  The first time through I get
			** BBCD returned.  If I don't bzero these out before the
			** next call, a return of AB will cause the bcmp to
			** succeed ==> WRONG !!
			*/
	
			bzero( rma->rm_val, vlen );
	
			rv = rm_getval( rma, UIO_SYSSPACE );

			/*
			** Since rm_getval can change rm_vallen, I MUST
			** reinitialize it each time through the loop.
			*/

			rma->rm_vallen = vlen;

			if ( rv == ENOSPC )
				continue;

			else if ( rv != 0 )
				break;

			if ( bcmp( val, rma->rm_val, vlen ) == 0 )
			{
				rma->rm_n = n;
				return 0;
			}
		}
	}
}

/*
** int
** cm_intr_attach_all( char *modname, void (*handler)(), int *devflagp,
**		       void **intr_cookiep )
**
**	Attach all device interrupts for a module.
**
** Calling/Exit State:
**	May block, so must not be called with basic locks held.
**
**	If intr_cookiep is non-NULL, it will be filled in with a cookie
**	which can be passed to cm_intr_detach_all to detach the interrupts
**	when the driver is done with them.  If intr_cookiep is NULL,
**	the interrupts will never be detached.
*/

void
cm_intr_attach_all( char *modname, void (*handler)(), int *devflagp,
		    void **intr_cookiep )
{
	struct cookie_list	*clistp;
	rm_key_t		key;
	void			*cookie;
	int			nattached;
	int			n;

	if ( intr_cookiep != NULL )
		*(struct cookie_list **)intr_cookiep = NULL;

	n = cm_getnbrd( modname );

	while ( n-- > 0 )
	{
		key = cm_getbrdkey( modname, n );
		nattached = cm_intr_attach( key, handler, devflagp, &cookie );

		if ( nattached <= 0  ||  intr_cookiep == NULL )
			continue;

		clistp = kmem_alloc( sizeof( *clistp ), KM_SLEEP );
		clistp->cl_cookie = cookie;
		clistp->cl_next = *(struct cookie_list **)intr_cookiep;
		*(struct cookie_list **)intr_cookiep = clistp;
	}
}

/*
** void
** cm_intr_detach_all( void *intr_cookie )
**
**	Detach all device interrupts for a module.
**
** Calling/Exit State:
**	May block, so must not be called with basic locks held.
**
**	The intr_cookie must be a value passed back from a previous call
**	to cm_intr_attach_all.
*/

void
cm_intr_detach_all( void *intr_cookie )
{
	struct cookie_list *clnext;
	struct cookie_list *clistp;

	clistp = (struct cookie_list *)intr_cookie;

	while ( clistp != NULL )
	{
		clnext = clistp->cl_next;
		cm_intr_detach( clistp->cl_cookie );
		kmem_free( clistp, sizeof( *clistp ));
		clistp = clnext;
	}
}

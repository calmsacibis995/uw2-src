/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/autoconf/resmgr/resmgr.c	1.42"
#ident	"$Header: $"

/*
**	Autoconfig -- Resource Manager
**
**  This is the Resource Manager Module for the Autoconfig feature.
**  It is used to store and retrieve (param, value) pairs.  The values
**  are arbitrary bytes of data, interpreted only by the those who
**  store and retrieve them.  User level access is via ioctl's after
**  opening a pseudo-device.  Kernel-level access is via a fixed
**  set of rm_ routines.  Currently only the Configuration Manager
**  Module should call any of the kernel-level routines directly
**  because there's NO guarantee the interfaces will not change.
**
**  Kernel modules (device drivers in particular) should use the
**  cm_ routines provided by the Configuration Manager to access
**  the Resource Manager.
*/

/*

Things to do:

- Un-comment _rm_rawclose after they get the hang fixed.
- Verify rm_resmgr boot code changes get made
- Update comments based on recent changes
- Leave RM_KEY in hashtbl and remove all the special cases
- Move rm_invoke_dcu into confmgr

*/


#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/cmn_err.h>
#include <fs/file.h>
#include <fs/vnode.h>
#include <fs/memfs/memfs.h>
#include <io/open.h>
#include <io/ioctl.h>
#include <util/debug.h>
#include <proc/cred.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/seg_kvn.h>
#include <io/uio.h>
#include <util/ksynch.h>
#include <util/ipl.h>
#include <svc/time.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/conf.h>
#include <io/ddi.h>			/* MUST be last #include */

/* Temporary, for debugging only */
#undef STATIC
#define STATIC

/*
** Local defines
*/

#define NODELKEY	0
#define DELKEY		1

/*
** Internal Structure Definitions
**
**    Here's the basic overview of the structs.  Each rm_key maintains
**    a linked list of rm_params.  Each rm_param has a linked list of
**    rm_vals.
**
**
**    +------+    +--------+    +--------+             +--------+
**    |rm_key|--->|rm_param|--->|rm_param|---> ... --->|rm_param|
**    +------+    +--------+    +--------+             +--------+
**                    |                                    |
**                 +------+                             +------+
**                 |rm_val|                             |rm_val|
**                 +------+                             +------+
**                    |
**    
**                    .
**                    .
**                    .
**    
**                    |
**                 +------+
**                 |rm_val|
**                 +------+
*/

struct rm_val
{
	void		*val;
	size_t		vlen;
	struct rm_val	*vnext;
};

struct rm_param
{
	char		pname[ RM_MAXPARAMLEN ];
	struct rm_val	*pval;
	struct rm_param	*pnext;
};

struct rm_key
{
	rm_key_t	key;
	struct rm_param	*plist;
	struct rm_key	*knext;
};

/*
** Externs for symbols outside this module.
*/

extern timestruc_t	hrestime;

/*
** Externs for symbols within this module.
*/

extern int		_rm_rawopen();
extern int		_rm_rawread();
extern void		_rm_rawclose();

extern void		_rm_warn();
extern void		_rm_panic();

extern int		_rm_copyin();
extern int		_rm_copyout();

extern void		_rm_param_del();
extern boolean_t	_rm_ro_param();

extern void		_rm_add_lookup();
extern void		_rm_del_lookup();
extern rm_key_t		_rm_find_next();
extern struct rm_key	*_rm_lookup_key();

/*
** Global Variables: non-static variables may be reinitialized outside
**                   this module.
*/

/*
** Variables used to map raw resmgr data into kernel virtual address
** space and then access the raw data.
*/

STATIC void	*rm_mapcook;
STATIC vnode_t	*rm_vp;

page_t	*resmgr_obj_plist = NULL;
size_t	resmgr_rdata_size = 0;		/* Actual size of data */
size_t	resmgr_size = 0;		/* Rounded up to PAGESIZE */

STATIC caddr_t		rm_rawptr = NULL;
STATIC caddr_t		rm_rawend;

/* Variable used to store name of file from /stand used to initialize */

char	*rm_resmgr = "resmgr";		/* "resmgr" is default */

/* Boot parameter to force DCU invocation */

boolean_t	rm_invoke_dcu;		/* B_FALSE is default */

/* Prefix_devflag is required by all "device drivers" */

int	rm_devflag = D_MP;

/* Variables for creating spin locks */

STATIC LKINFO_DECL( usrlki, "rm_userlock", 0 );
STATIC LKINFO_DECL( dblki, "rm_dblock", 0 );

STATIC lock_t	*rm_userlock;
STATIC lock_t	*rm_dblock;

/* Defines and variables for hash list of resmgr keys */

#define RM_HASHSZ	16			/* MUST 2^n */
#define RM_HASHIT	(RM_HASHSZ - 1)

STATIC struct rm_key	*rm_hashtbl[ RM_HASHSZ ];

/* Key for storing resmgr specific param/values */

STATIC struct rm_key	*rm_kptr;

/*
** Variables for implementing single-writer/multiple-readers
** concurrency control protocol for user level access to resmgr.
*/

STATIC int	rm_writers = 0;
STATIC int	rm_readers = 0;

/* Stores "next" key to dole out via rm_newkey routine */

STATIC rm_key_t		rm_next_key = 1;

/* Updated each time resmgr database changes */

STATIC time_t	*rm_timestamp;

/*
** rm_init()
**
** Calling/Exit State:
**	Resmgr database is initialized and resmgr specific key has
**	been created and initialized.
**
** Descriptions:
**	Converts the raw resmgr database read in off the disk during
**	the boot into the in-core representation and it creates the
**	resmgr specific key.
*/

void
rm_init( void )
{
	struct rm_args	rma;
	size_t		valbufsz = 32;
	time_t		tmptime;

	if (( rm_userlock = LOCK_ALLOC( 32, plbase, &usrlki, KM_NOSLEEP )) == NULL )
		_rm_panic( "LOCK_ALLOC", "rm_userlock" );

	if (( rm_dblock = LOCK_ALLOC( 32, plbase, &dblki, KM_NOSLEEP )) == NULL )
		_rm_panic( "LOCK_ALLOC", "rm_dblock" );

	if (( rma.rm_val = kmem_alloc( valbufsz, KM_NOSLEEP )) == NULL )
		_rm_panic( "kmem_alloc", "rm_val" );

	/*
	** Since rm_newkey updates rm_timestamp, it better point to a
	** time_t or else we may panic right here and now !!
	*/

	rm_timestamp = &tmptime;

	/* Create the key to store the Resource Manager specific data */

	(void)rm_newkey( &rma );	/* failure will PANIC in kmem_alloc */

	/* Add TIMESTAMP param */

	(void)strcpy( rma.rm_param, RM_TIMESTAMP );

	rma.rm_vallen = sizeof( time_t );

	/*
	** rma.rm_val doesn't point to a real time at this point, but
	** we don't care.  We just want rm_addval to allocate the
	** sizeof( time_t ) at this point.
	*/

	(void)rm_addval( &rma, UIO_SYSSPACE );	/* failure will PANIC in kmem_alloc */

	/*
	** For now this key is READ-ONLY.  In order to restrict access to it,
	** I'm going to save the struct elsewhere and before I return, I'll
	** remove it from the hash table.
	*/

	rm_kptr = _rm_lookup_key( rma.rm_key );
	ASSERT( rm_kptr != NULL );

	/* NOW set rm_timestamp to the actual location and initialize */

	rm_timestamp = rm_kptr->plist->pval->val;
	*rm_timestamp = hrestime.tv_sec;

	if ( _rm_rawopen() != 0 )
	{
		_rm_warn( "missing", &rma, valbufsz, NODELKEY );
		return;
	}


	for ( ;; )
	{
		if ( _rm_rawread( (void *)&rma.rm_key, sizeof( rm_key_t )) != 0 )
		{
			_rm_warn( "corrupt", &rma, valbufsz, NODELKEY );
			return;
		}

		if ( rma.rm_key == RM_NULL_KEY )
		{
			/* Done converting to the in-core representation */

			_rm_rawclose();
			kmem_free( rma.rm_val, valbufsz );

			rma.rm_key = rm_kptr->key;
			(void)strcpy( rma.rm_param, RM_INITFILE );
			rma.rm_val = rm_resmgr;
			rma.rm_vallen = strlen( rm_resmgr ) + 1;

			(void)rm_addval( &rma, UIO_SYSSPACE );

			*rm_timestamp = 0;		/* To indicate No Change */
			_rm_del_lookup( rm_kptr );
			return;
		}

		(void)rm_newkey( &rma );	/* failure will PANIC in kmem_alloc */

		for ( ;; )
		{
			if ( _rm_rawread( rma.rm_param, RM_MAXPARAMLEN ) != 0 )
			{
				_rm_warn( "corrupt", &rma, valbufsz, DELKEY );
				return;
			}

			if ( strcmp( rma.rm_param, RM_ENDOFPARAMS ) == 0 )
				break;

			for ( ;; )
			{
				if ( _rm_rawread( &rma.rm_vallen, sizeof( size_t )) != 0 )
				{
					_rm_warn( "corrupt", &rma, valbufsz, DELKEY );
					return;
				}

				if ( rma.rm_vallen == RM_ENDOFVALS )
					break;

				if ( rma.rm_vallen > valbufsz )
				{
					kmem_free( rma.rm_val, valbufsz );
					valbufsz = rma.rm_vallen;

					if (( rma.rm_val = kmem_alloc( valbufsz,
								KM_NOSLEEP )) == NULL )
					{
						/*
						** I'm going to assume this failed
						** because vallen was WAY TOO BIG
						** (i.e. corrupted /stand/resmgr) !!
						*/

						_rm_warn( "corrupt", &rma, 0, DELKEY );
						return;
					}
				}

				if ( _rm_rawread( rma.rm_val, rma.rm_vallen ) != 0 )
				{
					_rm_warn( "corrupt", &rma, valbufsz, DELKEY );
					return;
				}

				/* failure will PANIC in kmem_alloc */

				(void)rm_addval( &rma, UIO_SYSSPACE );
			}
		}
	}
}

/*
** rm_open()
**
** Calling/Exit State:
**	rm_readers or rm_writers is updated.
**
** Descriptions:
** 	Driver open() entry point.  Enforce concurrent access using
**	the classic multiple-readers/single-writer scheme.
*/

/* ARGSUSED */

int
rm_open( const dev_t *devp, int oflag, int otyp, const cred_t *cred_p )
{
	int	wflg = oflag & FWRITE;
	int	ret = 0;
	pl_t	oldpl = LOCK( rm_userlock, plbase );

	if ( rm_writers != 0  ||  ( rm_readers > 0  &&  wflg != 0 ))
		ret = EBUSY;

	else if ( wflg != 0 )
		rm_writers++;

	else if (( oflag & FREAD ) != 0 )
		rm_readers++;

	else
		ret = EINVAL;

	UNLOCK( rm_userlock, oldpl );
	return ret;
}

/*
** rm_close()
**
** Calling/Exit State:
**	rm_readers or rm_writers is updated.
**
** Descriptions:
** 	Driver close() entry point.
*/

/* ARGSUSED */

int
rm_close( dev_t dev, int oflag, int otyp, const cred_t *cred_p )
{
	pl_t	oldpl = LOCK( rm_userlock, plbase );

	if (( oflag & FWRITE ) != 0 )
	{
		ASSERT( rm_writers == 1 );
		rm_writers = 0;
	}
	else
	{
		/*
		** Since rm_close is only called once, upon the final user
		** level close(), I have to reset rm_readers = 0 rather
		** than rm_readers--;
		*/

		ASSERT( rm_readers > 0 );
		rm_readers = 0;
	}

	UNLOCK( rm_userlock, oldpl );
	return 0;
}

/*
** rm_ioctl()
**
** Calling/Exit State:
**	Resmgr database MAY have changed.
**
** Descriptions:
**	Driver ioctl() entry point.
*/

/* ARGSUSED */

int
rm_ioctl( dev_t dev, int cmd, caddr_t arg, int oflag, const cred_t *cred_p, int *rval_p )
{
	struct rm_args	rma;
	int		wflg = oflag & FWRITE;
	int		copyitout = 0;
	int		ret_code = EACCES;

	if ( copyin( arg, (caddr_t)&rma, sizeof( struct rm_args )) != 0 )
		return EFAULT;

	switch ( cmd )
	{
		case RMIOC_DELVAL:

			if ( wflg != 0 )
				ret_code = rm_delval( &rma );

			break;

		case RMIOC_DELKEY:

			if ( wflg != 0 )
				ret_code = rm_delkey( &rma );
			
			break;

		case RMIOC_ADDVAL:

			if ( wflg != 0 )
				ret_code = rm_addval( &rma, UIO_USERSPACE );

			break;

		case RMIOC_GETVAL:

			ret_code = rm_getval( &rma, UIO_USERSPACE );
			copyitout = 1;
			break;

		case RMIOC_NEXTKEY:

			if (( ret_code = rm_nextkey( &rma )) == 0 )
				copyitout = 1;

			break;

		case RMIOC_NEWKEY:

			if ( wflg != 0  &&  ( ret_code = rm_newkey( &rma )) == 0 )
				copyitout = 1;

			break;

		case RMIOC_NEXTPARAM:

			if (( ret_code = rm_nextparam( &rma )) == 0 )
				copyitout = 1;

			break;
	
		default:

			ret_code = EINVAL;
	}

	if ( copyitout != 0  &&  copyout( (caddr_t)&rma, arg,
					sizeof( struct rm_args )) != 0 )
		ret_code = EFAULT;

	return ret_code;
}

/*
** rm_newkey()
**
** Calling/Exit State:
**	New key has been added to the hash table.
**
** Descriptions:
**	This routine creates and returns a new key.
*/

int
rm_newkey( struct rm_args *rma )
{
	struct rm_key	*kptr;
	pl_t		oldpl;

	kptr = kmem_alloc( sizeof( struct rm_key ), KM_SLEEP );

	kptr->plist = NULL;

	oldpl = LOCK( rm_dblock, plbase );
	rma->rm_key = kptr->key = rm_next_key++;
	_rm_add_lookup( kptr );
	*rm_timestamp = hrestime.tv_sec;
	UNLOCK( rm_dblock, oldpl );

	return 0;
}

/*
** rm_delkey()
**
** Calling/Exit State:
**	Key is removed from hash table and all param/values for that key
**	have been deleted.
**
** Descriptions:
**	Deletes a key from the resmgr database.
*/

int
rm_delkey( struct rm_args *rma )
{
	struct rm_key	*kptr;
	struct rm_param	*plist;
	struct rm_param	*tmpplist;
	pl_t		oldpl = LOCK( rm_dblock, plbase );

	if (( kptr = _rm_lookup_key( rma->rm_key )) == NULL )
	{
		UNLOCK( rm_dblock, oldpl );
		return EINVAL;
	}

	plist = kptr->plist;

	/* First delete all the params associated with key */

	while ( plist != NULL )
	{
		tmpplist = plist;
		plist = plist->pnext;
		_rm_param_del( tmpplist );
	}

	/* Remove it from hash table and free memory */

	_rm_del_lookup( kptr );
	kmem_free( kptr, sizeof( struct rm_key ));
	*rm_timestamp = hrestime.tv_sec;

	UNLOCK( rm_dblock, oldpl );
	return 0;
}

/*
** rm_nextkey()
**
** Calling/Exit State: None
**
** Descriptions:
**	Returns "next" logical key from database.  The "next" key is
**	relative to the key being passed in.  To initiate iterating
**	through the keys set rma->rm_key = RM_NULL_KEY.
*/

int
rm_nextkey( struct rm_args *rma )
{
	rm_key_t 	key = rma->rm_key;
	rm_key_t 	nkey;
	int		ret = 0;
	pl_t		oldpl = LOCK( rm_dblock, plbase );

	if ( key >= rm_next_key )
		ret = EINVAL;

	else if (( nkey = _rm_find_next( key )) == rm_next_key )
		ret = ENOENT;

	else
		rma->rm_key = nkey;

	UNLOCK( rm_dblock, oldpl );
	return ret;
}

/*
** rm_nextparam()
**
** Calling/Exit State: None
**
** Descriptions:
**	Returns n-th param for given key.
*/

int
rm_nextparam( struct rm_args *rma )
{
	struct rm_key	*kptr;
	struct rm_param	*plist;
	uint_t		n = rma->rm_n;
	int		ret = ENOENT;
	pl_t		oldpl = LOCK( rm_dblock, plbase );

	/* The first param will start at n = 0 */

	if (( kptr = _rm_lookup_key( rma->rm_key )) == NULL )
	{
		UNLOCK( rm_dblock, oldpl );
		return EINVAL;
	}

	plist = kptr->plist;

	while ( n-- > 0  &&  plist != NULL )
		plist = plist->pnext;

	if ( plist != NULL )
	{
		(void)strncpy( rma->rm_param, plist->pname, RM_MAXPARAMLEN );
		ret = 0;
	}

	UNLOCK( rm_dblock, oldpl );
	return ret;
}

/*
** rm_addval()
**
** Calling/Exit State:
**	New param/value pair added to key.
**
** Descriptions:
**	Either adds a new param/value pair to key, or appends new value
**	to param if param already exists for this key.
**
**	Set uio_seg to UIO_SYSSPACE when called by another kernel module.
*/

int
rm_addval( struct rm_args *rma, uio_seg_t uio_seg )
{
	struct rm_key	*kptr = NULL;
	struct rm_param	*plist;
	struct rm_param	*findp;
	struct rm_val	*vlast;
	struct rm_val	*val;
	boolean_t	skiptime = B_FALSE;
	size_t		vallen;
	caddr_t		param;
	pl_t		oldpl;

	if (( vallen = rma->rm_vallen ) == 0 )
		return EINVAL;

	plist = kmem_alloc( sizeof( struct rm_param ), KM_SLEEP );

	/*
	** I may as well alloc enough for both the struct
	** and value at the same time to save a call to
	** kmem_alloc.
	*/

	val = kmem_alloc( sizeof( struct rm_val ) + vallen, KM_SLEEP );

	/* Initialize val */

	val->val = (caddr_t)val + sizeof( struct rm_val );

	if ( _rm_copyin( rma->rm_val, val->val, vallen, uio_seg ) != 0 )
	{
		kmem_free( plist, sizeof( struct rm_param ));
		kmem_free( val, sizeof( struct rm_val ) + vallen );
		return EFAULT;
	}

	val->vlen = vallen;
	val->vnext = NULL;

	oldpl = LOCK( rm_dblock, plbase );

	if ( rma->rm_key == RM_KEY )
	{
		if ( _rm_ro_param( rma->rm_param ) != B_TRUE )
		{
			kptr = rm_kptr;
			skiptime = B_TRUE;
		}
	}
	else
		kptr = _rm_lookup_key( rma->rm_key );

	if ( kptr == NULL )
	{
		kmem_free( plist, sizeof( struct rm_param ));
		kmem_free( val, sizeof( struct rm_val ) + vallen );
		UNLOCK( rm_dblock, oldpl );
		return EINVAL;
	}

	param = rma->rm_param;
	findp = kptr->plist;

	if ( skiptime != B_TRUE )
		*rm_timestamp = hrestime.tv_sec;

	/* First let's see if this param already exists */

	while ( findp != NULL )
	{
		if ( strncmp( param, findp->pname, RM_MAXPARAMLEN ) != 0 )
		{
			findp = findp->pnext;
			continue;
		}

		kmem_free( plist, sizeof( struct rm_param ));

		vlast = findp->pval;

		while ( vlast->vnext != NULL )
			vlast = vlast->vnext;

		vlast->vnext = val;

		UNLOCK( rm_dblock, oldpl );
		return 0;
	}

	/* Initialize new param */

	(void)strncpy( plist->pname, param, RM_MAXPARAMLEN );
	plist->pval = val;

	/* Attach param to key */

	plist->pnext = kptr->plist;
	kptr->plist = plist;

	UNLOCK( rm_dblock, oldpl );
	return 0;
}

/*
** rm_getval()
**
** Calling/Exit State: None
**
** Descriptions:
**	Returns n-th value for given key and param.
**
**	Set uio_seg to UIO_SYSSPACE when called by another kernel module.
*/

int
rm_getval( struct rm_args *rma, uio_seg_t uio_seg )
{
	struct rm_key	*kptr;
	struct rm_param	*plist;
	struct rm_val	*val;
	boolean_t	unlock_it = B_TRUE;
	size_t		reqlen;
	caddr_t		param;
	uint_t		n = rma->rm_n;
	size_t		tvallen;
	size_t		tvlen;
	void		*tval;
	int		ret = ENOENT;
	pl_t		oldpl;

	if ( uio_seg == UIO_USERSPACE )
	{
		/*
		** We'll be doing a real copyout in the _rm_copyout routine
		** below, and we can't be holding rm_dblock at the time, so
		** we must release it beforehand.  If we don't save the
		** value we're returning via the copyout in a local buffer,
		** we'd leave a small hole where the existing value might
		** be free'd before the copyout completed.  A No, No !!
		**
		** Since I'm only doing this when the rm_getval is coming
		** from user space, I can safely KM_SLEEP below.
		*/

		tvallen = rma->rm_vallen;
		tval = kmem_alloc( tvallen, KM_SLEEP );
	}

	oldpl = LOCK( rm_dblock, plbase );

	/* First check to see if they're requesting Resmgr specific info */

	if ( rma->rm_key == RM_KEY )
		kptr = rm_kptr;

	else if (( kptr = _rm_lookup_key( rma->rm_key )) == NULL )
	{
		UNLOCK( rm_dblock, oldpl );

		if ( uio_seg == UIO_USERSPACE )
			kmem_free( tval, tvallen );

		return EINVAL;
	}

	param = rma->rm_param;
	plist = kptr->plist;

	while ( plist != NULL )
	{
		if ( strncmp( param, plist->pname, RM_MAXPARAMLEN ) == 0 )
		{
			val = plist->pval;

			/* The first val will start at n = 0 */

			while ( n-- > 0  &&  val != NULL )
				val = val->vnext;

			if ( val == NULL )
				break;			/* return ENOENT */

			reqlen = rma->rm_vallen;
			tvlen = rma->rm_vallen = val->vlen;

			if ( tvlen > reqlen )
			{
				ret = ENOSPC;
				break;
			}

			if ( uio_seg == UIO_USERSPACE )
			{
				bcopy( val->val, tval, tvlen );
				UNLOCK( rm_dblock, oldpl );
				unlock_it = B_FALSE;
			}
			else
				tval = val->val;

			if ( _rm_copyout( tval, rma->rm_val, tvlen, uio_seg ) != 0 )
			{
				ret = EFAULT;
				break;
			}

			ret = 0;
			break;
		}

		plist = plist->pnext;
	}

	if ( unlock_it == B_TRUE )
		UNLOCK( rm_dblock, oldpl );

	if ( uio_seg == UIO_USERSPACE )
		kmem_free( tval, tvallen );

	return ret;
}

/*
** rm_delval()
**
** Calling/Exit State:
**	For given key, specified param and all associated values are deleted.
**
** Descriptions:
**	Deletes values from database based on key and param.
*/

int
rm_delval( struct rm_args *rma )
{
	struct rm_param	**plistp;
	struct rm_param	*tmpplist;
	struct rm_key	*kptr = NULL;
	boolean_t	skiptime = B_FALSE;
	caddr_t		param = rma->rm_param;
	pl_t		oldpl = LOCK( rm_dblock, plbase );

	if ( rma->rm_key == RM_KEY )
	{
		if ( _rm_ro_param( rma->rm_param ) != B_TRUE )
		{
			kptr = rm_kptr;
			skiptime = B_TRUE;
		}
	}
	else
		kptr = _rm_lookup_key( rma->rm_key );

	if ( kptr == NULL )
	{
		UNLOCK( rm_dblock, oldpl );
		return EINVAL;
	}

	plistp = &kptr->plist;

	while ( *plistp != NULL  &&  strncmp( param, (*plistp)->pname,
							RM_MAXPARAMLEN ) != 0 )
		plistp = &(*plistp)->pnext;

	if ( *plistp != NULL )
	{
		tmpplist = *plistp;
		*plistp = (*plistp)->pnext;
		_rm_param_del( tmpplist );

		if ( skiptime != B_TRUE )
			*rm_timestamp = hrestime.tv_sec;
	}

	UNLOCK( rm_dblock, oldpl );
	return 0;
}

/*
** _rm_ro_param()
**
** Calling/Exit State: None
**
** Description:
**	Returns B_TRUE if param is read-only, B_FALSE otherwise.
*/

STATIC boolean_t
_rm_ro_param( const char *param )
{
	static const char *ro_params[] = {
					    RM_TIMESTAMP,
					    RM_INITFILE,
					    NULL
				   	 };

	const char	**rop = ro_params;

	while ( *rop != NULL )
		if ( strcmp( *rop++, param ) == 0 )
			return B_TRUE;

	return B_FALSE;
}

/*
** _rm_lookup_key()
**
** Calling/Exit State: None
**
** Descriptions:
**	Returns the pointer to the data for key.
*/

STATIC struct rm_key *
_rm_lookup_key( rm_key_t key )
{
	struct rm_key	*kptr = rm_hashtbl[ key & RM_HASHIT ];

	while ( kptr != NULL  &&  kptr->key != key )
		kptr = kptr->knext;

	return kptr;
}

/*
** _rm_add_lookup()
**
** Calling/Exit State:
**	New entry in hash table.
**
** Descriptions:
**	Adds entry to hash table.
*/

STATIC void
_rm_add_lookup( struct rm_key *kptr )
{
	int	offset = kptr->key & RM_HASHIT;

	kptr->knext = rm_hashtbl[ offset ];
	rm_hashtbl[ offset ] = kptr;
}

/*
** _rm_del_lookup()
**
** Calling/Exit State:
**	Entry removed from hash table.
**
** Descriptions:
**	Deletes an entry from the hash table.
*/

STATIC void
_rm_del_lookup( const struct rm_key *kptr )
{
	struct rm_key	**kptrp = &rm_hashtbl[ kptr->key & RM_HASHIT ];

	/*
	** Since kptr was retrieved from the hash table before being passed
	** into this routine I don't need to check for a NULL ptr because
	** I'm guaranteed I'll find it.
	*/

	while ( *kptrp != kptr )
		kptrp = &(*kptrp)->knext;

	*kptrp = (*kptrp)->knext;
}

/*
** _rm_find_next()
**
** Calling/Exit State: None
**
** Descriptions:
**	Given a key, finds the next logical key in database.
**	Returns rm_next_key if there is NO more "next" key.
*/

STATIC rm_key_t
_rm_find_next( rm_key_t key )
{
	for ( key++; key < rm_next_key; key++ )
		if ( _rm_lookup_key( key ) != NULL )
			break;

	return key;
}

/*
** _rm_param_del()
**
** Calling/Exit State: None
**
** Descriptions:
**	Free's up a param and all associated values.
*/

STATIC void
_rm_param_del( struct rm_param *param )
{
	struct rm_val	*val = param->pval;
	struct rm_val	*tmpval;

	kmem_free( param, sizeof( struct rm_param ));

	while ( val != NULL )
	{
		/*
		** Recall I alloc'ed enough for both the struct and
		** value in one chunk to save calls to kmem_alloc/free.
		*/

		tmpval = val;
		val = val->vnext;
		ASSERT( tmpval->vlen > 0 );
		kmem_free( tmpval, sizeof( struct rm_val ) + tmpval->vlen );
	}
}

/*
** _rm_rawopen()
**
** Calling/Exit State:
**	rm_rawptr and rm_rawend initialized to beginning and end of raw
**	resmgr data.
**
** Descriptions:
**	Maps raw resmgr data into kernel virtual address space.
*/

STATIC int
_rm_rawopen( void )
{
	if ( resmgr_rdata_size == 0 )		/* Empty /stand/resmgr */
		return -1;

	ASSERT( resmgr_size > 0 );

	if (( rm_vp = memfs_create_unnamed( resmgr_size, MEMFS_NORESV )) == NULL )
		return -1;

	memfs_bind( rm_vp, resmgr_size, resmgr_obj_plist );

	/*
	** If either segkvn_vp_mapin or segkvn_lock fail, we could "undo"
	** the previous calls, but they should NOT fail and if they do,
	** it likely the system is in big trouble anyhow, so we're not
	** going to bother "undo'ing" anything.  The worst thing that will
	** happen is a little wasted space from some mem_alloc's.
	*/

	if (( rm_rawptr = (caddr_t)segkvn_vp_mapin( 0, resmgr_size, 0, rm_vp, 0,
					SEGKVN_NOFLAGS, &rm_mapcook )) == NULL )
		return -1;

	if ( segkvn_lock( rm_mapcook, SEGKVN_MEM_LOCK ) != 0 )
	{
		rm_rawptr = NULL;
		return -1;
	}

	rm_rawend = rm_rawptr + resmgr_rdata_size;
	return 0;
}

/*
** _rm_rawread()
**
** Calling/Exit State: None
**
** Descriptions:
**	Copies chunk of raw data to local buffer.
*/

STATIC int
_rm_rawread( void *buf, size_t len )
{
	/*
	** For robustness, check that I don't read past the end.
	*/

	if ( rm_rawptr + len > rm_rawend )
		return -1;

	bcopy( rm_rawptr, buf, len );
	rm_rawptr += len;
	return 0;
}

/*
** _rm_rawclose()
**
** Calling/Exit State: None
**
** Descriptions:
**	Returns pages containing raw resmgr data back to the system.
*/

STATIC void
_rm_rawclose( void )
{
#ifdef LESTER
	/* Free up the space used by the raw rmdb data */

	segkvn_unlock( rm_mapcook, SEGKVN_MEM_LOCK | SEGKVN_DONTNEED |
							SEGKVN_DISCARD );
	segkvn_mapout( rm_mapcook );
	VN_RELE_CRED( rm_vp, NULL );
#endif /* LESTER */
}

/*
** _rm_copyin()
**
** Calling/Exit State: None
**
** Descriptions:
**	Move bytes into local variable.
*/


STATIC int
_rm_copyin( const void *from, void *to, size_t count, uio_seg_t uio_seg )
{
	int		ret = 0;

	if ( uio_seg == UIO_SYSSPACE )
		bcopy( from, to, count );
	else
		ret = copyin( from, to, count );

	return ret;
}

/*
** _rm_copyout()
**
** Calling/Exit State: None
**
** Descriptions:
**	Moves bytes out of local variable.
*/

STATIC int
_rm_copyout( const void *from, void *to, size_t count, uio_seg_t uio_seg )
{
	int		ret = 0;

	if ( uio_seg == UIO_SYSSPACE )
		bcopy( from, to, count );
	else
		ret = copyout( from, to, count );

	return ret;
}

/*
** _rm_panic()
**
** Calling/Exit State: None
**
** Descriptions:
**	Deals with fatal error.
*/

STATIC void
_rm_panic( const caddr_t str1, const caddr_t str2 )
{
	cmn_err( CE_PANIC, "rm_init: %s failure--%s", str1, str2 );
}

/*
** _rm_warn()
**
** Calling/Exit State:
**
** Descriptions:
**	Deals with non-fatal error.
*/

STATIC void
_rm_warn( const caddr_t str, struct rm_args *rma, size_t bufsz, int delflg )
{
	static const caddr_t	corrupt = "corrupt";
	struct rm_args		tmprma;

	cmn_err( CE_WARN, "rm_init: %s in-core resmgr database", str );

	if ( bufsz != 0 )
		kmem_free( rma->rm_val, bufsz );

	if ( delflg == DELKEY )
		(void)rm_delkey( rma );

	if ( rm_rawptr != NULL )
		_rm_rawclose();

	tmprma.rm_key = rm_kptr->key;
	(void)strcpy( tmprma.rm_param, RM_INITFILE );
	tmprma.rm_val = corrupt;
	tmprma.rm_vallen = strlen( corrupt ) + 1;

	(void)rm_addval( &tmprma, UIO_SYSSPACE );
	_rm_del_lookup( rm_kptr );
}

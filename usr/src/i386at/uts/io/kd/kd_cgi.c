/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/kd/kd_cgi.c	1.4"
#ident	"$Header: $"

/*
 * Enhanced Application Compatibility Support 
 */

#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <mem/immu.h>
#include <proc/proc.h>	
#include <proc/signal.h>
#include <svc/errno.h>
#include <proc/user.h>
#include <util/inline.h>
#include <mem/kmem.h>
#include <util/cmn_err.h>
#include <io/ws/vt.h>
#include <io/ansi/at_ansi.h>
#include <io/uio.h>
#include <io/kd/kd.h>
#include <io/xque/xque.h>
#include <io/stream.h>
#include <io/termios.h>
#include <io/strtty.h>
#include <io/stropts.h>
#include <io/ws/ws.h>
#include <io/ws/chan.h>
#include <io/gvid/vid.h>
#include <io/gvid/vdc.h>
#include <proc/cred.h>
#include <proc/mman.h>
#include <proc/iobitmap.h>
#include <io/kd/kd_cgi.h>
#include <util/debug.h>

#include <io/ddi.h>	/* Must come last. */


#define MAXCLN		64

STATIC faddr_t	cgi_umapinit(channel_t *, long, long);
STATIC int	cgi_ioprivl(int, struct portrange *);

extern void	map_addr(vaddr_t *, uint_t, off_t, int);
extern int	kdvm_map(void *, channel_t *, struct map_info *, struct kd_memloc *);

extern wstation_t  Kdws;


/*
 * int
 * cgi_mapclass(channel_t *, int, int *)
 *
 * Calling/Exit State:
 *	- Called from kdvmstr_doioctl()
 *	- No locks are held on entry/exit.
 */
int
cgi_mapclass(channel_t *chp, int arg, int *rvalp)
{
	int	i, rv = 0;
	struct cgi_class *vcp;
	char	name[MAXCLN];
	extern struct cgi_class cgi_classlist[];/* defined in Space.c */
	pl_t	pl;


	for (i = 0; i < MAXCLN; i++) {
		if (0 == (name[i] = fubyte(arg++)))
			break;
		if (-1 == name[i]) {
			return(EFAULT);
		}
	}

	if (MAXCLN == i) {
		/* name is garbage */
		return(EINVAL);
	}

	for (vcp = cgi_classlist; vcp->name; vcp++)
		if (!strcmp(name, vcp->name))		/* S018 */
			break;

	if (!vcp->name) {
		/* name is not found */
		return(ENXIO);
	}

	pl = RW_WRLOCK(Kdws.w_rwlock, plstr);

	if ((*rvalp = (int) cgi_umapinit(chp, vcp->base, vcp->size)) != (int)NULL) {
		rv = cgi_ioprivl(1, vcp->ports);
		RW_UNLOCK(Kdws.w_rwlock, pl);
		return(rv);
	}

	RW_UNLOCK(Kdws.w_rwlock, pl);

	return(EIO);
}


/*
 * STATIC char * 
 * cgi_umapinit(channel_t *, long, long)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode on entry through exit.
 */  
STATIC faddr_t
cgi_umapinit(channel_t *chp, long base, long size)
{
	struct kd_memloc memloc;
	void		*procp;
	faddr_t		vaddr;
	struct map_info	*map_p = &Kdws.w_map;


        if (chp != (channel_t *)ws_activechan(&Kdws))
		return(NULL);

	procp = proc_ref();
	if (map_p->m_procp && map_p->m_procp != procp ||
					 map_p->m_cnt == CH_MAPMX) {
		proc_unref(procp);
		return(NULL);
	}

	/*
	 * release the workstations reader/writer lock
	 */
	RW_UNLOCK(Kdws.w_rwlock, plbase);

	map_addr((vaddr_t *)&vaddr, size, (off_t)0, 1);

	(void) RW_WRLOCK(Kdws.w_rwlock, plstr);

	if (vaddr == NULL)
		return(vaddr);

	memloc.vaddr = vaddr;
	memloc.physaddr = (caddr_t)base;
	memloc.length = size;
	memloc.ioflg = 0;

	ws_mapavail(chp, map_p);
	
	if (!kdvm_map(procp, chp, map_p, &memloc)) {
		drv_munmap((vaddr_t)vaddr, size);
		proc_unref(procp);
		return(NULL);
	}

	return(vaddr);
}


/*
 * STATIC int
 * cgi_ioprivl(int, struct portrange *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exlusive mode on entry through exit.
 *
 * Description:
 *	Grant or revoke permission to do direct OUTs from
 *	user space.
 *
 */
STATIC int
cgi_ioprivl(int arg, struct portrange *ports)
{
	ushort_t maxport, curport, ioports[2];	/* keep it simple for now */
	

	ioports[1] = 0;		/* delimit the (very short) list */

	for ( ; ports->count; ports++) {
		maxport = ports->first + ports->count;
		for (curport = ports->first; curport < maxport; curport++) {
			ioports[0] = curport;
			arg ? iobitmapctl(IOB_ENABLE, ioports) : iobitmapctl(IOB_DISABLE, ioports);
		}
	}

	return 0;
}


/*
 * End Enhanced Application Compatibility Support 
 */

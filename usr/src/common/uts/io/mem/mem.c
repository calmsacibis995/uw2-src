/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/mem/mem.c	1.15"
#ident	"$Header: $"

#include <fs/file.h>
#include <io/conf.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <mem/seg_kmem.h>
#include <mem/vmparam.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/mman.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/mod/ksym.h>
#include <util/mod/mod_obj.h>
#include <util/param.h>
#include <util/types.h>

#include <io/ddi.h>	/* must come last */

#define	M_MEM		0	/* /dev/mem - physical main memory */
#define	M_KMEM		1	/* /dev/kmem - virtual kernel memory & I/O */
#define M_SYSDAT	2	/* /dev/sysdat - timer and possibly other */
				/* 		 system data */

int mmdevflag = D_MP;

STATIC int mmrw(dev_t, uio_t *, uio_rw_t);
int mmmmap(dev_t, off_t, uint_t);


/*
 * int mmopen(dev_t *, int, int, cred_t *)
 *	Open routine for /dev/[k]mem.
 *
 * Calling/Exit State: 
 *	None.
 */
/* ARGSUSED */
int
mmopen(dev_t *devp, int flag, int type, cred_t *cr)
{
	switch (getminor(*devp)) {
	case M_MEM:
	case M_KMEM:
	case M_SYSDAT:
		return 0;

	default:
		return ENODEV;
	}
}

/*
 * int mmclose(dev_t, int, cred_t *)
 *      Close routine for /dev/[k]mem.
 *
 * Calling/Exit State:
 *      None.
 */
/* ARGSUSED */
int
mmclose(dev_t dev, int flag, cred_t *cr)
{
        return 0;
}

/*
 * int mmioctl(dev_t, int, int, int, cred_t *, int *)
 *      ioctl routine for /dev/[k]mem.
 *
 * Calling/Exit State:
 *      None.
 */
/* ARGSUSED */
int
mmioctl(dev_t dev, int cmd, int arg, int flag, cred_t *cr, int *rvalp)
{
	struct mioc_rksym rks;
	int error;
	char *kname;
	vaddr_t kaddr;
	Elf32_Sym ksp;
	struct modctl *mcp;
	uio_t uio;
	iovec_t iov;
	uio_rw_t rw;

	switch(cmd) {
	case MIOC_READKSYM:
	case MIOC_IREADKSYM:
		if(!(flag & FREAD))
			return EBADF;
		rw = UIO_READ;
		break;
	case MIOC_WRITEKSYM:
	case MIOC_IWRITEKSYM:
		if(!(flag & FWRITE))
			return(EBADF);
		rw = UIO_WRITE;
		break;
	default:
		return EINVAL;
	}

	if (getminor(dev) != M_KMEM)
		return ENXIO;

	if ((error = ucopyin((const void *)arg, &rks,
			     sizeof(struct mioc_rksym), 0)) != 0)
		return error;
	

	kname = kmem_alloc(MAXSYMNMLEN, KM_SLEEP);

	if ((error = copyinstr(rks.mirk_symname, kname, MAXSYMNMLEN, NULL)) 
			!= 0) {
		kmem_free(kname, MAXSYMNMLEN);
		return (error);
	}

	/*
	 * The module contents the symbol will be held when return
	 * from mod_obj_getsym_mcp().
	 */
	mcp = mod_obj_getsym_mcp(kname, B_FALSE, SLEEP, &ksp);
	if (mcp == NULL) {
		kmem_free(kname, MAXSYMNMLEN);
		return (ENOMATCH);
	}

	kaddr = (vaddr_t)ksp.st_value;
	if (cmd == MIOC_IREADKSYM || cmd == MIOC_IWRITEKSYM) {
		if ((kaddr % sizeof(void *)) != 0) {
			error = EINVAL;
			goto done;
		}
		kaddr = *(vaddr_t *)kaddr;
		if (!KADDR(kaddr)) {
			error = EINVAL;
			goto done;
		}
	}

	iov.iov_base = rks.mirk_buf;
	iov.iov_len = rks.mirk_buflen;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = (off_t)kaddr;
	uio.uio_segflg = UIO_USERSPACE;
	uio.uio_resid = iov.iov_len;
	uio.uio_fmode = (short)flag;

	error = mmrw(dev, &uio, rw);

done:
	kmem_free(kname, MAXSYMNMLEN);
	/* release the hold of the module */
	mod_obj_modrele(mcp);
	return error;
}

/*
 * int mmread(dev_t,uio_t *, cred_t *, int *)
 *      read routine for /dev/[k]mem.
 *
 * Calling/Exit State:
 *      None.
 */
/* ARGSUSED */
int
mmread(dev_t dev, uio_t *uiop, cred_t *cr)
{
	return mmrw(dev, uiop, UIO_READ);
}

/*
 * int mmwrite(dev_t, uio_t *, cred_t *, int *)
 *      write routine for /dev/[k]mem.
 *
 * Calling/Exit State:
 *      None.
 */
/* ARGSUSED */
int
mmwrite(dev_t dev, uio_t *uiop, cred_t *cr)
{
	return mmrw(dev, uiop, UIO_WRITE);
}

/*
 * STATIC int mmrw(dev_t, uio_t *, uio_rw_t)
 *      read routine for /dev/[k]mem.
 *
 * Calling/Exit State:
 *      None.
 */
STATIC int
mmrw(dev_t dev, uio_t *uiop, uio_rw_t rw)
{
	off_t off;
	size_t n;
	ppid_t ppid;
	void *map;
	char *addr;
	int error;

        while (uiop->uio_resid != 0) {
		/*
		 * Don't cross page boundaries, since vtop might change.
		 * Allow for "negative" uio_offset values.
		 */
		n = PAGESIZE - ((vaddr_t)(off = uiop->uio_offset) & PAGEOFFSET);

		ppid = (ppid_t)mmmmap(dev, off, 0);

		if (ppid == NOPAGE)
			return ENXIO;

		map = kpg_ppid_mapin(1, ppid, PROT_ALL, KM_SLEEP);

		addr = (char *)map + ((vaddr_t)off & PAGEOFFSET);

		error = uiomove_catch(addr, n, rw, uiop, CATCH_ALL_FAULTS);

		kpg_mapout(map, 1);

		if (error)
			return error;
	}

	return 0;
}

/*
 * int mmmmap(dev_t, off_t, uint_t)
 *      mmap routine for /dev/[k]mem.
 *
 * Calling/Exit State:
 *     None.
 */
/* ARGSUSED */
int
mmmmap(dev_t dev, off_t off, uint_t prot)
{
	ASSERT((getminor(dev) == M_MEM) || (getminor(dev) == M_KMEM) ||
			(getminor(dev) == M_SYSDAT));

	switch (getminor(dev)) {

	case M_MEM:
		return phystoppid((paddr_t)off);

	case M_KMEM:
		if (KADDR(off))
			return kvtoppid((caddr_t)off);
		break;

	case M_SYSDAT:
		/*
		 * check that the offset is in the valid range for sysdat pages
		 */
		if ((off < ((SYSDAT_PAGES * PAGESIZE) + KVSYSDAT)) &&
				(off >= KVSYSDAT)) {
			return kvtoppid((caddr_t)off);
		}
		break;
	}

	return NOPAGE;
}

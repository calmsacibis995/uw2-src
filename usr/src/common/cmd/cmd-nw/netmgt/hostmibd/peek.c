/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/netmgt/hostmibd/peek.c	1.1"
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <nlist.h>

#include <sys/resource.h>
#include <sys/param.h>
#include <sys/mman.h>

#include <sys/sysmacros.h>

#include <signal.h>
#include <sys/siginfo.h>
#include <sys/ucontext.h>
#include <sys/vmparam.h>
#include <sys/immu.h>

#include "peek.h"


caddr_t     pagingBase = (caddr_t) 0;

static int  kmem_fd = -1;

static void PageHandler (int sig, siginfo_t *sinfo, ucontext_t *uctx)
{
    caddr_t     page;
    caddr_t     pa;

    if (kmem_fd < 0 || sinfo == (siginfo_t *) 0 || !SI_FROMKERNEL (sinfo))
/*	LeaveText ("Unknown signal passed to PageHandler()", 1)*/;

/*    if (UTKT (sinfo->si_addr, caddr_t) < (caddr_t) KERNEL_VS_MIN
	|| UTKT (sinfo->si_addr, caddr_t) > (caddr_t) KERNEL_VS_MAX
    )
	LeaveText ("SIGSEGV on no-kernel address", 1)*/;

    page = (caddr_t) (UTKT(sinfo->si_addr, unsigned long)
		& ~((unsigned long) NBPP - 1));

    pa = mmap (KTUT (page, caddr_t),
	NBPP, PROT_READ, MAP_FIXED | MAP_SHARED, kmem_fd, (off_t) page
    );

    if (pa == (caddr_t) -1)
/*	LeaveText ("mmap() failed", 255)*/;
}


void peek_kernel_init(void)
{
  static struct sigaction     new_act;
  static struct nlist kmemRange[] = {
	{ "_start" },
	{ "_end"   },
	{ 0        },
    };

    /*
    **  for now, we cannot use _end as max kernel VM address, because we
    **  needn't only know the max data address, we have to know the max
    **  dynamic memory address. So we reset the max address here.
    */
    if (kmem_fd < 0)
	if ((kmem_fd = open (KMEM_FILE, O_RDONLY, 0)) < 0)
/*	    LeaveText ("Cannot open kmem", 255)*/;

    new_act.sa_handler = PageHandler;
    new_act.sa_flags   = SA_RESTART | SA_SIGINFO;
    sigemptyset (&new_act.sa_mask);

    if (sigaction (SIGSEGV, &new_act, 0) == -1)
/*	LeaveText ("sigaction:", 255)*/;

    if (pagingBase == (caddr_t) 0)
    {
	pagingBase = mmap (
	    0, NBPP, PROT_READ, MAP_SHARED, kmem_fd, (off_t) KVBASE
	);

	if (pagingBase == (caddr_t) -1)
/*	    LeaveText ("SetupPaging (mmap)", 255)*/;

	(void) munmap (pagingBase, NBPP);
    }
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/autoconf.c	1.11"
#ident	"$Header: $"

#include <util/cmn_err.h>
#include <util/engine.h>
#include <util/param.h>
#include <util/types.h>

static void conf_proc(void);

struct  engine  *engine;                /* base of engine array */
struct  engine  *engine_Nengine;        /* end of engine array */
int Nengine;
extern int cpurate;

/*
 * void
 * configure(void)
 *
 *      Initial sys config
 *
 * Calling/Exit State:
 *
 *      Assumes system is only running on the boot processor.
 *      No return value.
 */
void
configure(void)
{
	/*
	 * Do any platform-specific configuration. On an AT-MP class
	 * of platforms, this may determine the number of engines,
	 * the interrupt distribution mode etc.
	 */
	psm_configure();
	conf_proc();
	conf_mem();
}


/*
 * static void
 * conf_proc(void)
 *
 *	Configure processors.
 *
 * Calling/Exit State:
 *
 *	Allocate engine table for all possible processors, but only remember
 *	alive and configured processors.
 *
 *	We also set `Nengine' here to the # of processors.
 */
static void
conf_proc(void)
{
        struct engine *eng;
        int	i;
	int	engcnt = 1;
        int     flags = 0;
	extern int psm_numeng(void);


	engcnt = psm_numeng();
	if (engcnt > MAXNUMCPU) {
		/*
		 *+ More CPUs are present than supported by the operating
		 *+ system as it has been built.  The extra CPUs will not
		 *+ be used.
		 */
		cmn_err(CE_NOTE, "System supports only %d processor(s).\n"
				 "\t%d not used\n",
				 MAXNUMCPU, engcnt - MAXNUMCPU);
		engcnt = MAXNUMCPU;
	}

        engine = (struct engine *)calloc((engcnt) * sizeof(struct engine));
	if (engine == NULL) {
		/*
		 *+ Not enough memory available to allocate space
		 *+ for engine structures. Check the total memory 
		 *+ configured in your system.
		 */
		cmn_err(CE_PANIC, "Can't alloc engine struct\n");
		/* NOTREACHED */
	}

#ifdef DEBUG
        cmn_err(CE_CONT, "# %d processor(s).\n", engcnt);
#endif

        for (i = 0; i < engcnt; i++) {

                eng = &engine[Nengine++];

                /*
                 * Set the engine rate and find the fastest processor.
                 */

                if (eng->e_cpu_speed == 0) {
                        eng->e_cpu_speed = cpurate;
                } else if (eng->e_cpu_speed > cpurate) {
                        cpurate = eng->e_cpu_speed;
                }

        }

        engine_Nengine = &engine[Nengine];

}


/*
 * void
 * io_kmadv(void)
 *	Call kmem_advise for data structures present in IO.
 *
 * Calling/Exit State:
 *	Called at sysinit time while still single threaded.
 */
void
io_kmadv(void)
{
	extern void str_kmadv(void);

	str_kmadv();
}

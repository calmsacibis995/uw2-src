/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/ims/ims.cf/Space.c	1.3"


/*
** ident @(#) @(#) space.c.ims 1.1 1 12/6/93 09:36:49 
**
** sccs_id[] = {"@(#) 1.1 space.c.ims "}
*/

/*
***************************************************************************
**
**      MODULE NAME:  space.c.ims 
**
**      PURPOSE:  IMS device driver configurables.
**
**      DEPENDENCIES:
**
**
**      REVISION HISTORY:
**      FPR/CRN     Date    Author      Description
**                  08/01/92    K. Conner     Initial Release
**
**      COPYRIGHT:
**
**          (c) Tricord systems, Inc. 1992
**               All rights reserved.
**
**
***************************************************************************
*/

/*
***************************************************************************
**                      System Include Files
***************************************************************************
*/

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/cmn_err.h"
#include "sys/sysinfo.h"

/*
***************************************************************************
**                  Module Related Include Files
***************************************************************************
*/

#include "sys/ims_mrp.h"
#include "sys/ims.h"
/* #include "config.h" */
#include "sys/triccs.h"

/*
***************************************************************************
**                                Defines
***************************************************************************
*/
#define IMS_DEBUG_MSG        0 /* Usage: 0 Turn off debug messages.        
                                         1 Turn on debug messages.         */
#define IMS_NMI              1 /* Usage: 0 Driver NMI processiong off 
                                         1 Driver NMI processing on        */
#define IMS_MPIC_REDIRECTION 1 /* Usage: 0 I/O MPIC re-direction off.
                                         1 I/O MPIC re-direction on.       */
/* REMOTE CONSOLE FLAGS */
#define IMS_REMOTE_CONSOLE_OFF   0
#define IMS_REMOTE_CONSOLE_ON    1

#define IMS_REMOTE_CON_KIN   1 /* Bit 0 Usage: 0 Kernel input processing off.
                                         1 Kernel input  processing on.    */
#define IMS_REMOTE_CON_KOUT  2 /* Bit 1 Usage: 0 Kernel output processing off.
                                         1 Remote console processing on.    */
#define IMS_REMOTE_CON_UIN   4 /* Bit 2 Usage: 0 User mode input processing off.
                                         1 User mode input processing on.    */
#define IMS_REMOTE_CON_UOUT  8 /* Bit 3 Usage: 0 User mode out. processing off.
                                         1 User mode output processing on.    */
#define IMS_MIN         60000  /* Timeout of 1 minute in Hertz.             */
#define IMS_TUNE_MRPS   5      /* Tunable number of mrp's data sets         */
#define IMS_CPU_UPDATE_RATE  2000 /* Time in seconds at which  
                                    the master CPU's watchdog counter will
                                    be updated by an IMS driver
                                    timeout function.                       */
#define IMS_CPU_DEAD_TIMEOUT 10000 /* Time in seconds during
                                      which the UNIX O/S must update
                                      a CPU I'm alive counter before
                                      the IMS considers the CPU
                                      deceased!!!                        */

/*
** Log buffer size
*/
#define IMS_LOG_SIZE    4096    /* Log buffer size                            */

/*
***************************************************************************
**                                Typedefs
***************************************************************************
*/

/*
***************************************************************************
**                Externals
****************************************************************************
*/

extern void tri_mpic_int_7();  /* Interrupts originating from the MPIC 
                                  that this driver is to detect            
                                  These interrupt routine addresses are
                                  stuffed into the "idt" table in UNIX.
                                  The re-direction table associated with the 
                                  I/O MPIC on the base processor is
                                  modified to cause these interrupts,
                                  if configured to do so.                   */ 
extern void tri_mpic_int_8();
extern void tri_mpic_int_9();
extern void tri_mpic_int_10();
extern void tri_mpic_int_11();
extern void tri_mpic_int_12();
extern void tri_mpic_int_13();
extern void tri_mpic_int_14();
extern void tri_mpic_int_15();
extern void tri_mpic_int_spurious();

extern struct sysinfo sysinfo;  /* Kernel counters and such                */


/*
***************************************************************************
**                      Global Storage
***************************************************************************
*/

/*
 **  Log buffer
 */
char    tri_log_buf[IMS_LOG_SIZE];   /* Log message buffer                */
int        ims_log_size = IMS_LOG_SIZE;

/*
 ** Driver device structure
 */
IMS_DEVICE ims_device[] =
{
 /*
 ** IMS device 0. - NOTE: This structure is bigger than what
 **                       is initialized because of IMS_DEVICE typedef
 */
 {(int) 0, (int) 0, (int) 0, (paddr_t) IMS_DUAL_PORT_ADDR}
};

/*
***************************************************************************
**                          Configuration information
***************************************************************************
*/
int ims_debug_msg = IMS_DEBUG_MSG;  /* 1 then debug messages on.           */
int ims_nmi = IMS_NMI;              /* Usage: 0 Driver NMI processiong off 
                                              1 Driver NMI processing on   */
int ims_mpic_redirection = IMS_MPIC_REDIRECTION; /* Usage: 0 I/O MPIC
                                                            redirection off
                                                          1 I/O MPIC
                                                            redirection on */
int ims_remote_console = IMS_REMOTE_CON_KIN      |
                         IMS_REMOTE_CON_KOUT     |
                         IMS_REMOTE_CON_UIN      |
                         IMS_REMOTE_CON_UOUT;

int ims_rc_status = IMS_REMOTE_CONSOLE_OFF;

int ims_tune_dpm = IMS_DP_PHYSICAL;
                                    /* Place some mrp d.s. in dual-port RAM */
int ims_tune_mrps = IMS_TUNE_MRPS;
                                    /* The number of physical MRP d.s.
                                       to allocate.
                                                                            */
int ims_intr_ipl = IMS_INTR_PRI;  /* Interrupt Priority Level.                 */
int ims_cpu_dead_timeout = IMS_CPU_DEAD_TIMEOUT; /*ms till CPU considered dead
                                                   by IMS                    */
int ims_cpu_update_rate = IMS_CPU_UPDATE_RATE;   /* ms during which, master cpu
                                                   in an MP environment will
                                                   its "I'm alive
                                                   counter updated.          */



/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/astm/astioctl.c	1.4"

/*
 * astioctl.c -- routines for user level front panel support
 *	for AST Manhattan UP OS (in standard PIC mode)
 *
 * Modifications:
 *
 * M001:  If PWROFF_IOCTL is defined then power will not be turned
 * 	  of on shutdown unless EBI_SET_PWR_OFF ioctl is called first.
 *	  Otherwise, "shutdown" or "init 0" command will turn off power.
 *		
 * M002:  Added IOCTL case to clear event flags.  Will be called by
 *	  astmonitor at initialization time.
 *
 * M003:  Added two variable to UP kernel only to handle power shutoff.
 *	  astm_shutdown_count keeps track of how many times the shutdown
 *	  button has been pressed since astmonitor was initialized.
 *	  astm_monitor_running keeps track of whether the ioctl has 
 *	  ever been called.  These are used in UP kernel to determine if
 *	  second shutdown button press will cause power shutoff.
 *	  Controller by ATUP_POWER_OFF.
 *
 * M004:  Added ioctl to enable/disable RAM cache.  Note:  both primary
 *	  (internal) and secondary (external) cache is affected.
 */

/* for UnixWare 2.0 multiprocessor or uniprocessor build */
/* UNIPROC must be defined for uniprocessor builds */

#ifdef _KERNEL_HEADERS		/* if building in source hierarchy */

#include <util/types.h>
#include <util/engine.h>
#include <util/metrics.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/processor.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/kdb/xdebug.h>
#include <sys/conf.h>
#include <psm/ast/ebi.h>
#include <psm/ast/ast.h>
#include <io/f_ddi.h>
#include <io/ddi.h>

#else

#include <sys/types.h>
#include <sys/engine.h>
#include <sys/metrics.h>
#include <sys/param.h>
#include <sys/plocal.h>
#include <sys/processor.h>
#include <sys/errno.h>
#include <sys/cmn_err.h>
#include <sys/debug.h>
#include <sys/kdb/xdebug.h>
#include <sys/ebi.h>
#include <sys/ast.h>
#include <sys/f_ddi.h>
#include <sys/ddi.h>

#endif

#define ATUP_POWER_OFF  /* M003 */

#ifdef UNIPROC
/* These things defined in Q4 load's ddi_f.h */
#ifndef NMI_ATTACH
#define NMI_ATTACH		1
#define NMI_DETACH		2
#define NMI_UNKNOWN		0x00
#define NMI_FATAL		0x01
#define NMI_BENIGN		0x02
#define NMI_BUS_TIMEOUT 0x04
#define NMI_REBOOT		0x10
#endif	/* NMI_ATTACH */
#endif	/* UNIPROC */

#define START	0
#define STOP	1

/*
 *  EXTERNALS -- from psm in MP version and astm.c in UP.
 */
extern event_t			ast_event_sv;
extern int			ast_event_code;
extern EBI_II			ast_calltab;
extern void			**MMIOTable;

#ifdef PWROFF_IOCTL	/* M001 */
extern int			ast_shutdown_pwr;
#endif

extern struct dispinfo 		ast_display;

#ifdef UNIPROC
extern void 			ast_intr_init(void);
#endif

/*
 * Following are in Space.c, so that defaults can be modified
 */
extern int			astm_util_rate;
extern int 			astm_alpha_util_mode;
extern int 			astm_graph_mode;

/*
 * GLOBAL DATA
 */
int 			astm_devflag = D_NEW;

#ifdef UNIPROC
int			astm_monitor_running = 0;	/* M003 */

#ifdef ATUP_POWER_OFF
int			astm_shutdown_count = 0;	/* M003 */
#endif	/* ifdef ATUP_POWEROFF */

#endif  /* ifdef UNIPROC */

/* 
 * FUNCTION PROTOTYPES
 */
	void 	astm_init(void);
	int	astm_open(dev_t *dev, int oflag, int otype, cred_t *crp);
	int	astm_close(dev_t dev, int oflag, int otype, cred_t *crp);
	int  	astm_ioctl(dev_t dev, int cmd, void *arg, int mode, cred_t *crp, int *rvalp);
	int	astm_nmi_handler(void);
	void 	astm_update_util(void);
STATIC	void astm_control_util(int);
extern	void astm_update_graph(void);

/*
 * Routine: astm_init
 * Purpose: intialize Extended Bios Support in UP system
 * Calling/Exit Stat: none
 * Notes: none
 */
void
astm_init(void)
{
#ifdef UNIPROC
	/* map in EBI and init associated data structures */
	ast_intr_init();

	/* register NMI handler, which will use EBI */
	drv_callback(NMI_ATTACH,  astm_nmi_handler, NULL);
#endif
	if (astm_alpha_util_mode) 
	{
		/* Set timeout to update utilization alpha display */
		astm_control_util(START);
	}	
	if (astm_graph_mode == PANEL_MODE_ONLINE) 
	{
		(ast_calltab.SetPanelProcGraphMode)(MMIOTable, PANEL_MODE_OVERRIDE);
		astm_update_graph();	/* update display */
	}
}

STATIC void
astm_control_util(int start)
{
	static int 		ast_alpha_tout_id = 0;

	if (start == START)
	{
		/* Set timeout to update utilization alpha display */
		if (! ast_alpha_tout_id)
		{
			ast_alpha_tout_id = 
				timeout(astm_update_util, NULL, astm_util_rate | TO_PERIODIC);
		}
	}
	else
	{
		/* Cancel utilization updates */
		untimeout(ast_alpha_tout_id);
		ast_alpha_tout_id = 0;
	}
}

/*
 * Routine: ast_open/ast_close
 * Purpose: stub routine to allow user programs access to the ioctl
 *          routine for control of various aspects of the AST Manhattan
 *          hardware.
 * Calling/Exit State: none
 * Notes: None
 */
/*ARGSUSED*/
int
astm_open(dev_t *dev, int oflag, int otype, cred_t *crp)
{
#ifdef AST_DEBUG
	cmn_err(CE_NOTE, "ast_open called\n");
#endif
	return(0);
}

/*ARGSUSED*/
int
astm_close(dev_t dev, int oflag, int otype, cred_t *crp)
{
#ifdef AST_DEBUG
	cmn_err(CE_NOTE, "ast_close called\n");
#endif
	return(0);
}

/*
 * Routine: astm_ioctl
 * Purpose: to allow user programs to control the following aspects of
 *          the AST Manhattan hardware:
 *
 *             1. Front Panel alpha/numeric display
 *             2. Front Panel UPS led
 *             3. Front Panel key switch (events are caught in spi_intr())
 *             4. Front Panel attention switch (events are caught in spi_intr())
 *             5. Power fail interrupts (events are caught in spi_intr())
 *             6. Front Panel histogram display
 *             7. Multiple power supply status reporting
 *             8. Thermal state sensing
 *             9. Software control of main power
 *
 * Calling/Exit State: none
 * Notes: May need locking if kernel does not serialize access to device
 *        open/close/ioctl routines.
 */
/*ARGSUSED*/
int
astm_ioctl(dev_t dev, int cmd, void *arg, int mode, cred_t *crp, int *rvalp)
{
	unsigned int  it;      /* temporary for int data       */
	unsigned char cta[16]; /* temporary for character data */

	if (arg == NULL && cmd != EBI_GET_EVENT)
	{
		cmn_err(CE_NOTE, "ast_ioctl: bad argument!\n");
		return(EINVAL);
	}

#ifdef AST_DEBUG
	cmn_err(CE_NOTE, "ast_ioctl: cmd=0x%x!\n", cmd);
#endif
	switch (cmd)
	{
		case EBI_GET_ALPHA_INFO:
#ifdef AST_DEBUG
			cmn_err(CE_NOTE, "ast_ioctl: EBI_GET_ALPHA_INFO\n");
#endif
			if (copyout(&ast_display, arg, sizeof(struct dispinfo)))
			{
				return(EFAULT);
			}
			break;
		case EBI_SET_ALPHA_DISP:
#ifdef AST_DEBUG
			cmn_err(CE_NOTE, "ast_ioctl: EBI_SET_ALPHA_DISP\n");
#endif
			if ( astm_alpha_util_mode )
				return(EINVAL);
			if (copyin(arg, cta, ast_display.size)) 
			{
				return(EFAULT);
			}
			if ( (ast_calltab.SetPanelAlphaNum)(MMIOTable, cta) != OK )
				return(ENXIO);
			break;
		case EBI_GET_ALPHA_DISP:
#ifdef AST_DEBUG
			cmn_err(CE_NOTE, "ast_ioctl: EBI_GET_ALPHA_DISP\n");
#endif
			if ( (ast_calltab.GetPanelAlphaNum)(MMIOTable, cta) != OK)
				return(ENXIO);
			cta[ast_display.size >= sizeof(cta) ? 
				sizeof(cta) - 1 : ast_display.size] = '\0';
			if (copyout(cta, arg, sizeof(struct dispinfo))) 
			{
				return(EFAULT);
			}
			break;
		case EBI_SET_UPS_LED:
#ifdef AST_DEBUG
			cmn_err(CE_NOTE, "ast_ioctl: EBI_SET_UPS_LED\n");
#endif
			if (copyin(arg, &it, sizeof(int))) 
			{
				return(EFAULT);
			}
			if ((ast_calltab.SetPanelUPS)(MMIOTable, it))
			{
				return(EINVAL);
			}
			break;
		case EBI_GET_UPS_LED:
#ifdef AST_DEBUG
			cmn_err(CE_NOTE, "ast_ioctl: EBI_GET_UPS_LED\n");
#endif
			(ast_calltab.GetPanelUPS)(MMIOTable, &it);
			if (copyout(&it, arg, sizeof(int))) 
			{
				return(EFAULT);
			}
			break;
		case EBI_GET_BAR_GRAPH_MODE:
#ifdef AST_DEBUG
			cmn_err(CE_NOTE, "ast_ioctl: EBI_GET_BAR_GRAPH_MODE\n");
#endif
			it = astm_graph_mode;
			if (copyout(&it, arg, sizeof(int))) 
			{
				return(EFAULT);
			}
			break;
		case EBI_SET_BAR_GRAPH_MODE:
#ifdef AST_DEBUG
			cmn_err(CE_NOTE, "ast_ioctl: EBI_SET_BAR_GRAPH_MODE\n");
#endif
			if (copyin(arg, &it, sizeof(int))) 
			{
				return(EFAULT);
			}
			switch(it)
			{
				case PANEL_MODE_HISTOGRAM:
					astm_graph_mode = it;
#ifdef UNIPROC
					it = PANEL_MODE_OVERRIDE;	/* turn off the autopilot */
					astm_control_util(START);
#endif
					break;
				case PANEL_MODE_OVERRIDE:
					astm_graph_mode = it;
					if (! astm_alpha_util_mode )
						astm_control_util(STOP);
					break;
				case PANEL_MODE_ONLINE:
					if (! astm_alpha_util_mode )
						astm_control_util(STOP);
					astm_graph_mode = it;
					it = PANEL_MODE_OVERRIDE;	/* turn off the autopilot */
					break;
				case PANEL_MODE_STATUS:
#ifdef UNIPROC
					/* By special agreement, this means "not supported
					 * by current OS"
					 */
					return(ENOMSG);
#else
					astm_graph_mode = it;
					break;
#endif
				default:
					return(EINVAL);
					/*NOTREACHED*/
					break;
			}
			if ((ast_calltab.SetPanelProcGraphMode)(MMIOTable, it))
			{
				return(EINVAL);
			}
			if (astm_graph_mode == PANEL_MODE_ONLINE)
				astm_update_graph();	/* update display */
			break;
		case EBI_SET_BAR_CONTENTS:
#ifdef AST_DEBUG
			cmn_err(CE_NOTE, "ast_ioctl: EBI_SET_BAR_CONTENTS\n");
#endif
			if (copyin(arg, &it, sizeof(int))) 
			{
				return(EFAULT);
			}
			if ((ast_calltab.SetPanelProcGraphValue)(MMIOTable, it))
			{
				return(EINVAL);
			}
			break;
		case EBI_GET_BAR_CONTENTS:
#ifdef AST_DEBUG
			cmn_err(CE_NOTE, "ast_ioctl: EBI_SET_BAR_CONTENTS\n");
#endif
			(ast_calltab.GetPanelProcGraphValue)(MMIOTable, &it);
			if (copyout(&it, arg, sizeof(int))) 
			{
				return(EFAULT);
			}
			break;
		case EBI_GET_THERMAL_STATE:
#ifdef AST_DEBUG
			cmn_err(CE_NOTE, "ast_ioctl: EBI_GET_THERMAL_STATE\n");
#endif
			(ast_calltab.GetThermalState)(MMIOTable, &it);
			if (copyout(&it, arg, sizeof(int))) 
			{
				return(EFAULT);
			}
			break;
		case EBI_GET_NUM_PWR_SUPPLIES:
#ifdef AST_DEBUG
			cmn_err(CE_NOTE, "ast_ioctl:EBI_GET_NUM_PWR_SUPPLIES\n");
#endif
			(ast_calltab.GetNumPowerSupplies)(MMIOTable, &it);
			if (copyout(&it, arg, sizeof(int))) 
			{
				return(EFAULT);
			}
			break;
		case EBI_GET_PWR_INFO:
#ifdef AST_DEBUG
			cmn_err(CE_NOTE, "ast_ioctl:EBI_GET_PWR_INFO\n");
#endif
			{
				powerSupplyInfo ps;

				int psnum = ((struct pwrinfo *)arg)->ps_num;
				if ((ast_calltab.GetPowerSupplyInfo)(MMIOTable, psnum, &ps))
				{
					return(EINVAL);
				}
				if (copyout(&ps, (caddr_t)((struct pwrinfo *)arg)->ps_info, sizeof(ps))) 
				{
					return(EFAULT);
				}
			}
			break;
#ifdef PWROFF_IOCTL	/* M001 */
		case EBI_SET_PWR_OFF:
#ifdef AST_DEBUG
			cmn_err(CE_NOTE, "ast_ioctl: EBI_SET_PWR_OFF\n");
#endif
			ast_shutdown_pwr = TRUE;
			break;
#endif	/* ifdef PWROFF_IOCTL */
		case EBI_GET_EVENT:
#ifdef AST_DEBUG
			cmn_err(CE_NOTE, "ast_ioctl: EBI_GET_EVENT\n");
#endif
			if (EVENT_WAIT_SIG(&ast_event_sv, primed) == B_FALSE)
			{
				return(EINTR);
			}
#ifdef UNIPROC
#ifdef ATUP_POWER_OFF
			astm_shutdown_count = 0;	/* M003 */
#endif
#endif
			*rvalp = ast_event_code;
			break;
		case EBI_SET_ALPHA_MODE:
#ifdef AST_DEBUG
			cmn_err(CE_NOTE, "ast_ioctl: EBI_SET_ALPHA_MODE\n");
#endif
			if (copyin(arg, &it, sizeof(int))) 
			{
				return(EFAULT);
			}
			switch(it)
			{
				case ALPHA_MODE_TEXT:
					astm_alpha_util_mode = 0;
					if (astm_graph_mode != PANEL_MODE_HISTOGRAM)
						astm_control_util(STOP);
					break;
				case ALPHA_MODE_UTIL:	
					astm_alpha_util_mode = 1;
					astm_control_util(START);
					break;
				case UTIL_REFRESH_SLOW:
					astm_util_rate = UTIL_SLOW_RATE;
					astm_control_util(STOP);
					astm_control_util(START);
					break;
				case UTIL_REFRESH_MEDIUM:
					astm_util_rate = UTIL_MEDIUM_RATE;
					astm_control_util(STOP);
					astm_control_util(START);
					break;
				case UTIL_REFRESH_FAST:
					astm_util_rate = UTIL_FAST_RATE;
					astm_control_util(STOP);
					astm_control_util(START);
					break;
				default:
					return(EINVAL);
					/*NOTREACHED*/
					break;
			}
			break;
		case EBI_GET_ALPHA_MODE:
#ifdef AST_DEBUG
			cmn_err(CE_NOTE, "ast_ioctl: EBI_GET_ALPHA_MODE\n");
#endif
			if (copyout(&astm_alpha_util_mode, arg, sizeof(int))) 
			{
				return(EFAULT);
			}
			break;


		case EBI_CLEAR_EVENTS:	/* M002 */
#ifdef AST_DEBUG
			cmn_err(CE_NOTE, "ast_ioctl: EBI_CLEAR_EVENTS\n");
#endif
			ast_event_code = 0;
#ifdef UNIPROC
			astm_monitor_running++;		/* M003 */
#ifdef ATUP_POWER_OFF
			astm_shutdown_count = 0;	/* M003 */
#endif /* ifdef ATUP_POWER_OFF */
#endif /* ifdef UNIPROC */
			(ast_calltab.NonMaskableIntEOI)(MMIOTable);
			break;

		case EBI_SET_RAM_CACHE:	/* M002 */
#ifdef AST_DEBUG
			cmn_err(CE_NOTE, "ast_ioctl: EBI_SET_RAM_CACHE\n");
#endif
			if (copyin(arg, &it, sizeof(int))) 
			{
				return(EFAULT);
			}
			switch (it) {
				case RAM_CACHE_DISABLE:
					if ((ast_calltab.DisableRAMCache)(MMIOTable))
					{
						return(ENODEV);
					}
					break;
				case RAM_CACHE_ENABLE:
					if ((ast_calltab.EnableRAMCache)(MMIOTable))
					{
						return(ENODEV);
					}
					break;
				default:	return(EINVAL);
			}
			break;

		default:
			cmn_err(CE_WARN, "ast_ioctl: unknown ioctl number\n");
			return(EINVAL);
	}
	return(0);
}


#ifdef UNIPROC
/*
 * void 
 * astm_nmi_handler(void)
 * 		AST Manhattan Non-Maskable Interrupt (NMI) handler.
 * 		>>>>> for Uniprocessor OS (standard PIC mode) only.
 *
 * Calling/Exit State:
 *
 * Description:
 *		Handles additional NMIs produced by AST Manhattan hardware in
 *		the standard (8259) interrrupt mode.  These NMI are associated
 *		with the front panel related events listed below.  When the PIC
 *		is programmed to the advanced interrupt mode (as in the MP OS),
 *		these events generate SPI interrupts, not NMIs.
 *
 * 	    This module gets called when an NMI occurs.
 *
 *	    This implementation is specific to the AST Manhattan in standard
 *	    interrupt (8259) mode.
 *
 */
int
astm_nmi_handler(void)
{
	memoryErrorInfo mei;
	unsigned int source;
	char *s;
	int ret_val = NMI_BENIGN;

	if ((ast_calltab.GetNMISource)(MMIOTable, &source) == ERR_UNKNOWN_INT) 
    {
		cmn_err(CE_NOTE, "AST NMI: cannot identify NMI");
		(ast_calltab.NonMaskableIntEOI)(MMIOTable); 
		return(NMI_UNKNOWN);
	}

	switch (source) 
	{
		/* these will already be handled by nmi.c in MP kernel */
		case INT_IO_ERROR:
			s = "system I/O error";
			break;
		case INT_MEMORY_ERROR:
			s = "uncorrectable ECC error";
			if ((ast_calltab.GetMemErrorInfo)(MMIOTable, &mei) == 
				MEMORY_ERROR_FOUND)
			{
				cmn_err(CE_CONT, "       AST NMI: memory error at 0x%x\n", 
						mei.location.low);
				cmn_err(CE_CONT, "                slot number     0x%x\n", 
						mei.slotNumber);
				cmn_err(CE_CONT, "                module number   0x%x\n", 
						mei.moduleNumber);
			}
			cmn_err(CE_PANIC, "AST NMI: uncorrectable ECC error");
			break;
		case INT_CPU_ERROR:
			s = "CPU error";
			break;
		case INT_BUS_ERR:
			s = "system bus address/parity error";
			break;
		case INT_BUS_TIMEOUT:
			s = "system bus timeout";
			break;

		/* These are definitely not handled by nmi.c: */
		case INT_POWER_FAIL:
			s = "!!!Power Failure Detected!!!";
			ast_event_code = EBI_EVENT_PWR_FAIL;
			EVENT_BROADCAST(&ast_event_sv, 0);
			break;

		case INT_SHUTDOWN:
			s = "!!!System Shutdown!!!";
			ast_event_code = EBI_EVENT_SHUTDOWN;
			EVENT_BROADCAST(&ast_event_sv, 0);

#ifdef UNIPROC			
#ifdef ATUP_POWER_OFF
			astm_shutdown_count++;
#endif
			/* M003
			 * If the monitor process is not running, 
 			 * print warning and break.
			 * In UP kernel, this skips power chop.
			 */
			if (! astm_monitor_running) {
				cmn_err(CE_WARN, 
			"astm: astmonitor not running: run panel -m\n");
				break;
			}
#ifdef ATUP_POWER_OFF
			/* M003
			 * In the UP kernel, If this is not the first 
			 * button push, then chop the power
			 */
			if (astm_shutdown_count > 1) {
				cmn_err(CE_CONT, "Shutting off power\n");
				/*
		 		 *  Give user time to read messages
		 		 */
				drv_usecwait(1000000);
	
				(ast_calltab.ShutdownPowerSupply)(MMIOTable);
				/*
		 		 * That's all, folks!
		 		 */
			}
#endif /* ATUP_POWER_OFF */
#endif /* UNIPROC */
			break;

		case INT_ATTENTION:
			s = "Attention button pressed";
			ast_event_code = EBI_EVENT_ATTENTION;
			EVENT_BROADCAST(&ast_event_sv, 0);
			break;

		default:
			ret_val = NMI_UNKNOWN;
			break;
	}
	if (ret_val != NMI_UNKNOWN)
	{
#ifdef AST_DEBUG
	/* 
	 * USG says not safe to have cmn_err() calls in NMI handler! 
	 */
		cmn_err(CE_WARN, "AST NMI: source is %d: %s.", source, s);
#endif
		(ast_calltab.NonMaskableIntEOI)(MMIOTable);
	}

	return(ret_val);
}
#endif

/*
 * Routine: astm_update_util
 * Purpose: update CPU utilization display on either the alpha display 
 *			(alpha utilization mode) or bar graph (histogram mode). 
 *			Called in UP version only.
 *
 * Calling/Exit State: none
 */
void 
astm_update_util(void)
{
		int leds, eng;
		static ulong_t old_idle, old_wait, old_user, old_sys;
		ulong_t new_idle, new_wait, new_user, new_sys;
		ulong_t idle, wait, user, sys, sum, utilpercent, tens;
		char str[5];

		/* This routine is only needed if utilization data is being displayed */
		if (!astm_alpha_util_mode && (astm_graph_mode != PANEL_MODE_HISTOGRAM) )
			return;


		new_idle = new_wait = new_user = new_sys = 0;

		for (eng=0; eng<Nengine; eng++)
		{

		new_idle += ENGINE_PLOCALMET_PTR(eng)->metp_cpu.mpc_cpu[MET_CPU_IDLE];
		new_wait += ENGINE_PLOCALMET_PTR(eng)->metp_cpu.mpc_cpu[MET_CPU_WAIT];
		new_user += ENGINE_PLOCALMET_PTR(eng)->metp_cpu.mpc_cpu[MET_CPU_USER];
		new_sys += ENGINE_PLOCALMET_PTR(eng)->metp_cpu.mpc_cpu[MET_CPU_SYS];
		}

		idle = (new_idle - old_idle) / Nengine;
		old_idle = new_idle;

		wait = (new_wait - old_wait) / Nengine;
		old_wait = new_wait;

		user = (new_user - old_user) / Nengine;
		old_user = new_user;

		sys = (new_sys - old_sys) / Nengine;
		old_sys = new_sys;

		sum = (idle + wait + user + sys);

		if (sum == 0)
		{
#ifdef DEBUG
			cmn_err(CE_WARN, "Cannot calculate CPU Utilization: bad sum\n");
#endif
			return;
		}

		utilpercent = ((user + sys) * 100) / sum;
		if ( utilpercent > 100 )
		{
#ifdef DEBUG
			cmn_err(CE_WARN, "Cannot calculate CPU Utilization: too big\n");
#endif
			return;
		}

#ifdef AST_DEBUG
		cmn_err(CE_NOTE, 
			"CPU %d: idle = %d, wait = %d, user = %d, sys = %d \n", 
			i, idle, wait, user, sys);
		cmn_err(CE_NOTE, "CPU Utilization = %d percent\n", utilpercent);
#endif
		if (astm_alpha_util_mode)
		{
			if (utilpercent == 100)
			{
				leds = 0xff;
				str[0] = '1';
				str[1] = '0';
				str[2] = '0';
			} else {
				leds = 0xff >> ((100 - utilpercent) / 12);
				tens = utilpercent / 10;
				str[0] =  ' ';
				if (tens == 0)
					str[1] = ' ';
				else
					str[1] = tens + '0';
				str[2] = (utilpercent - (tens * 10)) + '0';
			}
			str[3] = '%';;
			str[4] = '\0';;
			if ( (ast_calltab.SetPanelAlphaNum)(MMIOTable, 
				(unsigned char *) &str) != OK )
				cmn_err(CE_NOTE, 
					"ast_alpha_update: Unable to update alpha display\n");
		}
		if (astm_graph_mode == PANEL_MODE_HISTOGRAM)
		{
				leds = 0xff >> ((100 - utilpercent) / 12);
				if ((ast_calltab.SetPanelProcGraphValue)(MMIOTable, leds))
					cmn_err(CE_WARN, "Unable to set panel graph value\n");
		}
}


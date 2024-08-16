/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_IO_SSM_SSM_MISC_H	/* wrapper symbol for kernel use */
#define	_IO_SSM_SSM_MISC_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:io/ssm/ssm_misc.h	1.14"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ksynch.h>	/* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/ksynch.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * ssm_misc.h
 * 	Miscellaneous definitions for the SSM 
 *	firmware command interfaces.
 */

#define	BNAMESIZE	80		/* 80 bytes of boot string allowed */

struct ssm_misc {

	unchar	sm_who;			/* Who the sm_cmd is for */
	unchar	sm_cmd;			/* Command to be executed */

	unchar sm_unused;		/* placeholder */

	union ssm_misc_un {
		ulong	smu_max[40];	/* Max-sized entry in sm_un */

		ulong	smu_fpst;	/* State of frontpanel lights */

		struct vme_imap {	/* Map a VME interrupt vector */
			unchar	vi_vflags;	/* see flag defines */
			unchar	vi_vlev;	/* VME interrupt level */
			unchar	vi_vvec;	/* VME interrupt vector */
			unchar	vi_dest;	/* SLIC dest for this vec */
			unchar	vi_svec;	/* SLIC vector to use */
			unchar	vi_cmd;		/* SLIC command to send */
						/* following is a misc op */
						/* not used in Intr mapping */
			unchar  vme_window;	/* SSM VME window number */
						/* 0 window = IO Base + 32Mb */
						/* 1 window = Base + 64Mb, etc*/
		}	smu_vme_imap;

		struct vme_genintr {	/* Generate an interrupt on VME */
			unchar	vg_irq;		/* VME IRQ number (1-6) */
			unchar	vg_vvec;	/* VME intr vector */
		}	smu_vme_gen;

		struct wdtst {		/* Watchdog timer state */
			ulong	wdt_addr;	/* Addr of word that changes */
			ulong	wdt_intvl;	/* Interval in milliseconds */
		}	smu_wdtst;

		ulong	smu_tod;	/* Current time-of-day */

		struct todfreq {	/* Time-of-day state */
			ulong	tod_freq;	/* Clock frequency */
			unchar	tod_dest;	/* SLIC dest for TOD intrs */
			unchar	tod_vec;	/* SLIC vector for TOD intrs */
			unchar	tod_cmd;	/* SLIC cmd for tod intrs */
		}	smu_todfreq;

		struct errst {		/* Error-reporting state */
			ulong	err_time;	/* Milliseconds between polls */
			unchar	err_dest;	/* Dest to report errors to */
			unchar	err_vecbase;	/* Base SLIC vector */
			unchar	err_cmd;	/* Cmd (e.g. mIntr) to report */
		}	smu_errst;

		struct powerst { 	/* Power supply state */
			unchar	pow_dest;	/* Dest to report errors to */
			unchar	pow_vec;	/* SLIC vector to report errors */
			unchar	pow_cmd;	/* Cmd (e.g. mIntr) to report */
			ulong	pow_flags;	/* Power supply status flags */
		}	smu_powerst;	

		struct ssm_boot {
			unchar	boot_which;	/* Which boot string */
			ushort	boot_size;	/* Size of boot info */
			ushort	boot_flags;	/* Boot flags */
			char	boot_str[BNAMESIZE];	/* Boot string */
		}	smu_boot;

		struct ssm_log {
			char 	*log_buf;	/* Location of log buffer */
			char	*log_next;	/* Next free location in log */
			ushort	log_size;	/* Size of log buffer */
		}	smu_log;

		struct ssm_ram {
			char	*ram_loc;	/* Location of RAM to get/set */
			char	*ram_buf;	/* Buffer to copy to/from */
			ushort	ram_size;	/* Amount of RAM involved */
		}	smu_ram;

		struct ssm_nvram {
			ushort	nv_size;	/* Size of nvram buffer */
			char	*nv_buf;	/* Buffer to extract nvram */
		}	smu_nvram;

		struct ssm_scanst {
			unchar sic;		/* sic address */
			unchar tap;		/* chip on sic */
			unchar chain;		/* chain on sic */
			unchar ver;		/* OS version */
			char  *buf;		/* buffer receiving information */
			ushort size;		/* size of buffer/information available */
		} smu_scanst;
	}	sm_un;

	unchar	sm_dest;		/* slic destination for interrupt */
	unchar	sm_bin;			/* slic bit for interrupt */
	unchar	sm_mesg;		/* interrupt vector */
	unchar	sm_pri;			/* priority: 0 = high, 1 = low */
	lock_t	*sm_lockp;		/* Lock for access */
	ulong	sm_sw[4];		/* Reserved for s/w use */
	unchar	sm_pad2[3];
	volatile unchar	sm_stat;	/* Return value */
};

/* Abbreviations to union entries */
#define	sm_fpst		sm_un.smu_fpst
#define	sm_vme_imap	sm_un.smu_vme_imap
#define	sm_vme_gen	sm_un.smu_vme_gen
#define	sm_wdtst	sm_un.smu_wdtst
#define	sm_todval	sm_un.smu_tod
#define	sm_todfreq	sm_un.smu_todfreq
#define	sm_errst	sm_un.smu_errst
#define	sm_powerst	sm_un.smu_powerst
#define	sm_boot		sm_un.smu_boot
#define	sm_log		sm_un.smu_log
#define	sm_ram		sm_un.smu_ram
#define	sm_nvram	sm_un.smu_nvram
#define	sm_scanst	sm_un.smu_scanst

/*
 * mIntr bin for notifying SSM.
 */
#define	SM_BIN	2			/* Bin for SLIC interrupts to SSM */

/*
 * Destinations (for sm_who).
 */
#define	SM_ADDRVEC	0x0		/* mIntrs being sent for address */
#define	SM_FP		0x1		/* Cmd being sent to front panel */
#define	SM_VME		0x2		/* Cmd being sent to VME intr mapper */
#define	SM_WDT		0x3		/* Cmd being sent to watchdog timer */
#define	SM_ERR		0x4		/* Cmd being sent to err handling */
#define	SM_TOD		0x5		/* Cmd being sent to real-time clock */
#define	SM_POWER	0x6		/* Cmd being sent to power supply */
#define	SM_BOOT		0x7		/* Cmd being sent to boot info */
#define	SM_LOG		0x8		/* Cmd being sent to console log */
#define	SM_NVRAM	0x9		/* Cmd being sent to non-volatile RAM */
#define	SM_RAM		0x0a		/* Cmd being sent to regular RAM */
#define	SM_SOMETHING	0x0b		/* something else, not sure what though */
#define	SM_SCANLIB	0x0c		/* Cmd being sent to the SCAN library */

/*
 * Commands (sm_cmd).
 */
#define	SM_SET		0x01		/* Set associated data */
#define	SM_GET		0x02		/* Get associated data */
#define	SM_DS_BASE	0x10		/* Base of device-specific commands */
#define	SM_INTR		0x80		/* interrupt on completion */

/*
 * Common status definitions (sm_stat).
 */
#define	SM_BUSY		0x00		/* No status yet */
#define	SM_OK		0x01		/* Command completed OK */
#define	SM_BAD_ARGS	0xFF		/* Command rejected: bad args */

/*
 * Front panel messages.
 */
/*
 * Front panel state data (sm_fpst).
 * Obviously, not all of these are writable,
 * as they correspond to physical switches
 * on the front panel.
 */
#define	FPST_REM_ENA	0x01		/* REMOTE is enabled (ro) */
#define	FPST_SECURE	0x02		/* Power switch is in SECURE (ro) */
#define	FPST_AUTO	0x02		/* AUTO light is on (rw) */
#define	FPST_LOCAL	0x04		/* Console is LOCAL port (rw) */
#define	FPST_ERR	0x08		/* ERROR light is on (rw) */
#define	FPST_DCOK	0x10		/* DC OK light is on (ro) */

/*
 * VME interrupt mapping (sm_vme_imap) and memory accessing control requests
 */
/* Special VME commands (for sm_cmd) */
#define	SM_CLR_MAP	(SM_DS_BASE)	/* Clear all VME interrupt mapping */
#define	SM_GEN_VME	(SM_DS_BASE+1)	/* Generate a VME interrupt */
#define	SM_MAP_RDY	(SM_DS_BASE+2)	/* VME mapping/init complete */
#define SM_SET_WINDOW	(SM_DS_BASE+3)	/* Set VME Memory Window of SSM */
#define SM_SET_OFF	(SM_DS_BASE+4)	/* turn off the VME memory via the PIC */

/* Defines for vi_vflags */
#define SM_VME_NO_IACK	0x01		/* do not generrate interrupt 
					   acknowlege cycles */

/*
 * Time-of-Day Clock (sm_tod).
 */
/* special TOD commands (for sm_cmd) */
#define	SM_SET_FREQ	(SM_DS_BASE)	/* Get frequency of intrs in Hz */

/*
 * Error reporting state data (sm_errst).
 */
/* err_time */
#define	ERRST_OFF	0x00		/* Bus polling is on */

/* err_cmd */
#define	ERRST_REP_OFF	0x00		/* Error reporting is off */

/* err_vecbase must account for ERRST_NERRS vectors */
#define	ERRST_NERRS	4		/* Max number of possible errors */
#define	ERRST_BUS_LOCK	0		/* SSM believes the bus is locked up */
#define	ERRST_DEVFLT	1		/* SSM believes fatal h/w err exists,
					 * e.g. cache parity error on proc */
#define	ERRST_SSM_SSMFLT	2		/* SSM itself has fatal h/w err */
#define	ERRST_VME_BERR	3		/* SSM detected VMEbus Bus Error */


/*
 * mIntr bin for Power Supply state.
 */
#define	PSST_BIN	7		/* Bin for SLIC interrupts to SSM */
#define	SCAN_BIN	7		/* scan library interrupts from SSM */
/*
 * Power Supply state (sm_powerst).
 */
/* state flags (sm_powerst.pow_flags) */
/* special OEM only bits */
#define	PSST_P5_FAULT	0x0001		/* Fault on +5 from power supply */
#define	PSST_M5_FAULT	0x0002		/* Fault on -5 from power supply */
#define	PSST_12V_FAULT	0x0004		/* Fault on +12/-12 from power supply */
#define	PSST_LOAD_FAULT	0x0008		/* Load alarm from power supply */
/* SSM bits */
#define	PSST_AC_FAIL	0x0010		/* AC power failure */
#define	PSST_UPS_ON	0x0020		/* UPS is on */
#define	PSST_DC_FAIL	0x0040		/* DC power failure */

/*
 * Boot info (sm_bootinfo).
 */
/* Special boot info commands */
#define	SM_GET_DFT	(SM_DS_BASE)	/* Get default boot info */
#define	SM_SET_DFT	(SM_DS_BASE+1)	/* Set default boot info */
#define	SM_REBOOT	(SM_DS_BASE+2)	/* Reboot with this info */

/* boot_which values */
#define	SM_DYNIX	0x00		/* Dynix boot string */
#define	SM_DUMPER	0x01		/* Default dumper boot string */

/*
 * Console log info (sm_log).
 */
/* special log info commands */
#define	SM_GET_LOGSIZ	(SM_DS_BASE)	/* Get log size */

/*
 * Non-volatile RAM info (sm_nvram).
 */
/* special log info commands */
#define	SM_GET_NVRAMSIZ	(SM_DS_BASE)	/* Get NVRAM size */

#if defined(_KERNEL)

/*
 * Data structures and definitions used by
 * DYNIX operating system.
 */
#define	SM_NOLOCK	0x00		/* Context doesn't require locking */
#define	SM_LOCK		0x01		/* Context requires locking */
#define SM_SPL		SPLHI		/* SPL for locking ssm_misc structure */

extern void	ssm_send_misc_addr(const struct ssm_misc *, unchar);
extern ulong	ssm_get_tod(int);
extern void	ssm_set_tod(ulong_t), ssm_tod_freq(int, int, unchar, unchar, unchar);
extern void	ssm_reboot(uint_t, ushort_t, const char *);
extern void	ssm_get_boot(int, int, ushort_t *, uint_t, char *);
extern void	ssm_set_boot(int, int, ushort_t, uint_t, const char *);
extern void	scan_init(int);
extern void	*ssm_alloc(uint_t, uint_t, uint_t, int);
extern void	ssm_free(void *, uint_t);
extern void	*ssm_calloc(uint_t, uint_t, uint_t);
extern struct cons_cb * init_ssm_cons(unchar);
extern struct print_cb *init_ssm_prnt(unchar);
extern void ssm_init_wdt(ulong_t);
extern void ssm_disable_wdt(void);
extern void ssm_reenable_wdt(void);
extern void ssm_poke_wdt(void);
extern void ssm_set_fpst(ulong_t, int);
extern ulong ssm_get_fpst(int);
extern void ssm_set_vme_mem_window(struct ssm_misc *, unchar, ulong);
extern void ssm_clr_vme_imap(struct ssm_misc *, unchar);
extern void ssm_set_vme_imap(struct ssm_misc *, unchar, unchar, 
				unchar, unchar, unchar, unchar);
extern void ssm_vme_imap_ready(struct ssm_misc *, unchar);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_SSM_SSM_MISC_H_ */

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:io/xs/xs.c	1.5"
#ident	"$Header: $"

/*
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991, 1992
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/*
 * xs.c
 * 	Xylogics 781 16-port RS-232 terminal mux. STREAMS driver (xs).
 */

#include <util/types.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <util/ksynch.h>
#include <util/ipl.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <mem/kmem.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <io/autoconf.h>
#include <io/cfg.h>
#include <io/ssm/ssm_vme.h>
#include <io/stream.h>
#include <io/strmdep.h>
#include <io/stropts.h>
#include <io/termios.h>
#include <io/termio.h>
#include <io/xs/xs.h>
#include <io/ddi.h>

/*
 * The following definitions are driver specific and not
 * shared with any other modules.  Therefore they have been
 * placed here instead of in a header file.
 *
 * Misc. definitions.
 */
#define XS_OBUF_DEC	(XS_OBUF_SZ - 1)/* Mask for wrapping PUT index */
#define XS_MAXINPUTTIME	(1000U / 5U)	/* 1 sec max input timeout */
#define XS_MAXINPUTMIN	255		/* Max minimum input count */
#define XS_BUF_WAIT	10000		/* 10 millisec wait for streams buf */

/* VMEbus data xfer and CSR modifiers for address space and byte swap */
#define XS_DMA_AMOD	SSMVME_MODIFIER(SSM_VME_PARS_ROTATE, 0x0d) 
#define XS_CSR_AMOD	SSMVME_MODIFIER(SSM_VME_PARS_ROTATE, 0x2d) 

/* 
 * Mapping of 781 registers during reset and initialization,
 * which are functionally different than this at runtime, 
 * although in the same location. 
 */
typedef volatile struct xs_hw_regs {
	unchar f0;
	unchar low_rd_active;		/* Read active for ports 0-7 */
	unchar f1;
	unchar hi_rd_active;		/* Read active for ports 8-15 */
	unchar f2;
	unchar low_wr_active;		/* Write active for ports 0-7 */
	unchar f3;
	unchar hi_wr_active;		/* Write active for ports 8-15*/
	unchar f4;
	unchar cmd_report;		/* Command/report register */
	unchar f5;
 	unchar cntl_status;		/* 781 cntl/status register */
	unchar f6;
 	unchar port;			/* Port register */
} xsregs_t;

/*
 * The 781 hardware registers have different functions at init time
 * than at runtime.  The following definitions map the init registers 
 * into the runtime register structure.
 */
#define idb_addr0	low_rd_active
#define idb_addr1	hi_rd_active
#define idb_addr2	low_wr_active
#define idb_addr3	hi_wr_active
#define idb_addr_mod	cmd_report
#define fatal_error	port

/*
 * 781 control register commands.  (Write only)
 */
#define XSCR_CRS	0x02		/* Clear Register Status */
#define XSCR_ARC	0x04		/* Add Register Control  */
#define XSCR_EMM 	0x20		/* Enter Maintenance Mode */
#define XSCR_MRST	0x40 		/* Module Reset */

/*
 * 781 status register values. (Read only)
 */
#define XSSR_FERR	0x01		/* Fatal Error */
#define XSSR_RSP	0x02		/* Register Status Pending */
#define XSSR_ARCP	0x04		/* Add Register Control Pending */
#define XSSR_MMA	0x20		/* Maint. Mode Active */
#define XSSR_RSTA	0x40		/* Reset Active */

/*
 * Command/Report register bits
 */
#define XSCR_RAC	0x01		/* Read Active/Complete */
#define XSCR_WAC	0x02		/* Write Active/Complete */
#define	XSCR_SYNC	0x04		/* Synchronous Active/Complete */

/*
 * Command/Report register report codes (non read/write) (781 to host)
 */
#define XSCR_PARITY	0x10		/* Parity Error */
#define XSCR_FRAME	0x20		/* Framing Error/Break */
#define XSCR_MODEM	0x30		/* Modem Change */
#define XSCR_INOVRR	0x40		/* Input Overrun */
#define XSCR_CMDERR	0x50		/* Command Error */
#define XSCR_ACCERR	0x60		/* Data Access Error */
#define XSCR_ACCTIME    0x70		/* Data Access Timeout Error */
#define XSCR_UARTOVF	0x80		/* Uart Overflow */
#define XSCR_INTQOVF	0x90		/* Internal Queue Overflow */
#define XSCR_RMASK	0xF0		/* Report code mask */
/*
 * Command/Report register commands (non read/write) (host to 781)
 */
#define XSCR_UINIT	0x10		/* Uart Init */
#define XSCR_FLUSH	0x20 		/* Flush Buffers */
#define XSCR_RESET	0x30		/* Reset Port */
#define XSCR_MSET	0x40		/* Modem Set Parameters */
#define XSCR_MGET	0x50		/* Modem Get Parameters */
#define XSCR_BREAK	0x60		/* Send Break */
#define XSCR_ELGET	0x70		/* Error Log Get */
#define XSCR_XON	0x90		/* Unsuspend output */
#define XSCR_XOFF	0xA0		/* Suspend output */
#define XSCR_BREAKON	0xB0		/* Start Break */
#define XSCR_BREAKOFF	0xC0		/* Stop Break */

/*
 * 781 uart parameter blocks (1 per port)    Pointed to by idb.
 */
typedef volatile struct xs_uart_parms {
	ushort serial_modes;		/* Port stop bits, parity, bits, flow */
	ushort baud_rates;		/* Output/Input baud rates */
	ushort line_state;		/* Modem line state info */
	ushort line_mask;		/* Ignore status lines, dis/enable RX */
	ushort input_timeout;		/* Input timeout value */
	ushort input_min;		/* Minimum # of chars expected by host*/
	char   xon_char;		/* XON character */
	char   xoff_char;		/* XOFF character */
	ushort low_water;		/* Output buffer low water mark */
	ushort break_time;		/* Duration of break (in 5ms ticks) */
	ushort reserved3;		/* Reserved space */
	ushort reserved4;		/* Reserved space */
	ushort parerr_cnt;		/* Parity error count */
	ushort framerr_cnt;		/* Framing error count */
	ushort ovflow_cnt;		/* Overflow error count */
	ushort reserved5;		/* Reserved space */
	ushort reserved6;		/* Reserved space */
} xs_uparm_t;

/* For setting xs_uparam_t.breaktime must convert msecs to 5msec ticks */
#define XS_BRKTICKS(ms)		((ms) / 5)
/*
 * Values used to set uart parameters
 */

/* Line mode values for uparms.serial_modes */
#define XS_STOP1		0x4000	/* 1 stop bit */
#define XS_STOP1P5		0x8000	/* 1.5 stop bits */
#define XS_STOP2		0xC000	/* 2 stop bits */
#define XS_NO_PARITY		0x0000 	/* No parity */
#define XS_ODD_PARITY		0x1000	/* Odd parity */
#define XS_EVEN_PARITY		0x3000	/* Even parity */
#define XS_BITS5                0x0000 	/* 5 bits per character */
#define XS_BITS6		0x0400	/* 6 bits per character */
#define XS_BITS7		0x0800 	/* 7 bits per character */
#define XS_BITS8		0x0C00 	/* 8 bits per character */
#define XS_NO_FLOW		0x0000	/* No flow control */
#define XS_HW_FLOW		0x0100	/* Hardware flow control */
#define XS_SW_FLOW		0x0200	/* XON/XOFF flow control */

/* Baud rate values for uparms.baud_rates */
#define XSB_50			0x00	/* 50 baud */
#define XSB_75			0x01	/* 75 baud */
#define XSB_110			0x02	/* 110 baud */
#define XSB_134P5		0x03	/* 134.5 baud */
#define XSB_150			0x04	/* 150 baud */
#define XSB_300			0x05	/* 300 baud */
#define XSB_600			0x06	/* 600 baud */
#define XSB_1200		0x07	/* 1200 baud */
#define XSB_1800 		0x08	/* 1800 baud */
#define XSB_2000		0x09	/* 2000 baud */
#define XSB_2400		0x0A	/* 2400 baud */
#define XSB_3600		0x0B    /* 3600 baud */
#define XSB_4800		0x0C	/* 4800 baud */
#define XSB_7200		0x0D	/* 7200 baud */
#define XSB_9600		0x0E	/* 9600 baud */
#define XSB_19200		0x0F	/* 19.2K baud */


/* Values for setting line state with uparms.line_state and uparms.line_mask */
#define XSLINE_RTS		0x0100	/* RTS */
#define XSLINE_DTR		0x0200  /* DTR */
#define XSLINE_DCD		0x2000  /* DCD */
#define XSLINE_CTS		0x4000  /* CTS */
#define XSLINE_DSR		0x8000  /* DSR */

#define XS_IENABLE		0x0100  /* Input Enable bit */
#define XS_UNI			0x1000	/* Uni-directional handshake */
#define XS_MCU			0x0200	/* Modem change update ON */


/*
 * 781 Initialization Descriptor Block structure
 */
typedef volatile struct xs_idb {
	ushort icba_hi;			/* Input char buffer array addr msb */
	ushort icba_lo;			/* Input char buffer array addr lsb */
	ushort inbuf_len;		/* Length of input data buffers */
	ushort ocrba_hi;		/* Output char ring buf array addr msb*/
	ushort ocrba_lo;		/* Output char ring buf array addr lsb*/
	ushort outbuf_len;		/* Length of output data buffers */
	ushort addr_mod;		/* Addr modifier for both buffers */
	ushort inp_intvec;		/* Input interrupt vector */
	ushort inp_intlev;		/* Input interrupt level */
	ushort out_intvec;		/* Output interrupt vector */
	ushort out_intlev;		/* Output interrupt level */
	ushort stat_intvec;		/* Status interrupt vector */
	ushort stat_intlev;		/* Status interrupt level */
	xs_uparm_t uart_parms[XS_MAXPORTS];/* Per port uart parameter blocks */
} xsidb_t;

/*
 * 781 Output Character Ring Buffer Element (1 per port)
 */
typedef volatile struct outbuf_781 {
	short get;			/* GET index pointer */
	short put;			/* PUT index pointer */
	char buffer[XS_OBUF_SZ];	/* DATA area */
} xs_obuf_t;
/*
 * Output Character Ring Buffer Element array (1 per 781 board)
 */
typedef struct outbuf_array_781 {
	xs_obuf_t obufs[XS_MAXPORTS];
} xs_obuf_array_t;


/*
 * 781 Input Character Buffer Element (1 per port)
 */
typedef volatile struct inbuf_781 {
	short	count;
	char	data[XS_IBUF_SZ];
} xs_inbuf_t;

/*
 * Input Character Buffer Element array (1 per 781 board)
 */
typedef struct xs_ibuf_array {
	xs_inbuf_t ibufs[XS_MAXPORTS];
} xs_ibuf_array_t;

/*
 * Per-port structure for maintaining port and
 * stream state information.
 *
 * Note: the ttylock guarantees protected access to the uart parameters
 *       and other per-port info structures.  The only requirement is that
 *       the 781 MUST BE IDLE before writing locations such as the uart
 *       params.  A previous process may have written to the uart params
 *	 area but the 781 may still be reading the data.
 *
 * Note: bufwait is used as a wait mechanism whenever an allocb() has
 *	failed and a bufcall() or itimeout() has been scheduled.  This 
 *	occurs basically in two distinct instances: during open and 
 *	during reception of input.  The bufcall or itimeout schedules a 
 *	different routine to be awakened later in both these cases, 
 *	which should be okay since they are are disjoint events and 
 *	always retry the allocb.
 */

typedef struct xs_str_tty {
	queue_t	*qptr;			/* Local streams queue(wr)*/
	uint	flags;			/* Flags from driver space.c file */
	dev_t	devno;			/* Device number */
	lock_t	*ttylock;		/* Line structure lock */
	sv_t	*drainwait;		/* Output drain wait synch. variable */
	sv_t	*carrwait;		/* Carrier wait synch. variable */
	sv_t	*bufwait;		/* Bufcall wait synch. variable */
	uint	tty_state;		/* Current state of device */
	tcflag_t cflags;		/* Flags from termio(s) cflags */
	tcflag_t iflags;		/* Flags from termio(s) iflags */
	cc_t	cstart;			/* XOFF start character */
	cc_t	cstop;			/* XOFF stop character */
	cc_t	flow;			/* Delayed stop/start character */
	mblk_t	*msg;			/* Current output message */
	toid_t	owaitid;		/* Bufcall or timeout ID if a wait for
					 * a buffer occurs during open. Use the
					 * same ID for either since they are not
					 * undone later. */
	toid_t	bufcallid;		/* Bufcall ID */
	toid_t	timeoutid;		/* Timeout ID, used if bufcall fails */
	toid_t	delayid;		/* Output delay timeout id */
	toid_t	breakid;		/* Break duration timeout id */
	ushort	inputtime;		/* Input timeout value in millisecs */
	ushort	inputmin;		/* Minimum # of chars expected by host*/
} xstty_t;

/*
 * State bits for xstty_t.tty_state.
 */
#define XS_TXDELAY 	0x001
#define XS_ISOPEN	0x002
#define XS_WOPEN	0x004
#define XS_WCLOSE  	0x008
#define XS_CARR_ON	0x010
#define XS_TX_FULL	0x020
#define XS_DRAINTIME 	0x040
#define XS_SUSPOUT 	0x080
#define XS_WIOCTL  	0x100
#define XS_TXBREAK	0x200
#define XS_TX_BUSY	0x400
#define XS_DATASUSP	0x800

/*
 * Flag bits for xstty_t.flags.
 */
#define XS_DISCARDIN    0x001
#define XS_SYNCREQ	0x002
#define XS_RDBLOCKED	0x004
#define XS_FLOW		0x008

/*
 * 781 driver per-board info structure
 *
 * note: xs_lock allows protected access to the 781 board and its registers.
 *       However, the user process must be sure to check the ARCP bit before
 *       writing to the 781 registers.  This is because another process may
 *       have started a command to the 781, released the lock, and the 781
 *       is still in the process of latching the command.  So, MAKE SURE THE
 *       BOARD IS IDLE BEFORE WRITING TO ITS REGISTERS.
 */
typedef struct xsinfo {
	int 			boardnum;	/* Logical unit of this dev */
	lock_t			*cntlr_lock;	/* Mutex synch. access to this
						 * this structure and its CSR */
	struct xs_conf		*config;	/* Static config params */
	xsregs_t    		*csr;		/* Kernel virtual addr of CSR */
	void			*csr_map;	/* Handle returned from I/O 
						 * bus mapping the CSR. */
	paddr_t			msg_blk;	/* I/O bus base address for the
						 * idb, inbufs, and outbufs */
	void			*msg_map;	/* Handle returned from I/O 
						 * bus mapping the idb, 
						 * inbufs, and outbufs. */
	int			vector;		/* I/O bus interrupt vector
						 * assigned to this board. */
	xstty_t			tty[XS_MAXPORTS];/* Per port state info */

	/*
	 * NOTE: the DMA mapping that occurs in xs_init() 
	 * 	 of xs.c depends upon the idb, inbufs, and
	 *	 outbufs members of this structure to be
	 *	 declared last, in that sequence, within
	 * 	 this structure.  The idb must be short-
	 *	 aligned and the i/o bufs must be long-
	 *	 aligned.  Do not move or rearrange them 
	 * 	 without also changing xs_init.
	 */
	xs_ibuf_array_t 	inbufs;		/* Per port input structures */
	xs_obuf_array_t		outbufs;	/* Per port output structures */
	xsidb_t			idb;		/* Initialization description
						 * block for this 781 board */
} xsinfo_t;

/*
 * Macro for clearing 781 hardware register holding structure
 */
#define XS_CLEAR(xshw)					\
	{						\
	(xshw).low_rd_active = 0;		\
	(xshw).hi_rd_active  = 0;		\
	(xshw).low_wr_active = 0;		\
	(xshw).hi_wr_active  = 0;		\
	(xshw).cmd_report    = 0;		\
	(xshw).port          = 0;		\
	}
/*
 * Macro for reading values in 781 hardware registers
 */
#define XS_LOAD(ip, xsregs)				\
	{						\
	xsregs.low_rd_active = (ip)->csr->low_rd_active; \
	xsregs.hi_rd_active =  (ip)->csr->hi_rd_active; \
	xsregs.low_wr_active = (ip)->csr->low_wr_active; \
	xsregs.hi_wr_active  = (ip)->csr->hi_wr_active; \
	xsregs.cmd_report    = (ip)->csr->cmd_report; \
	xsregs.port          = (ip)->csr->port; \
	}
/*
 * Macros for determining state of 781 output buffers
 */

/* Access output buffer PUT pointer for a given port */
#define XS_PUT(ip, port)  ((ip)->outbufs.obufs[(port)].put)

/* Access output buffer GET pointer for a given port */
#define XS_GET(ip, port)  ((ip)->outbufs.obufs[(port)].get)

/* How much space (in chars) is left in the output buffer (allows wrapping) */
#define XS_OUTBUF_SPACE(get, put)	\
	(((((get) - (put) - 1) >= 0) && ((put) != 0)) ? (get) - (put) - 1 : \
		(get) - (put) - 1 + XS_OBUF_SZ)

/* Is the output buffer empty? */
#define XS_OUTBUF_EMPTY(ip, port)  \
	(((vtoss(XS_PUT((ip), (port))) + 1) & XS_OBUF_DEC) == \
		vtoss(XS_GET((ip), (port))))

/* Is the output buffer full? */
#define XS_OUTBUF_FULL(ip, port)   \
	(((vtoss(XS_PUT((ip),(port))) + 2) & XS_OBUF_DEC) == \
		vtoss(XS_GET((ip),(port))))

/* Move the PUT pointer forward and report on whether the buffer was empty */
#define XS_OUTBUF_ADD(ip, port, count)	\
	(XS_PUT((ip), (port)) = \
		((XS_PUT((ip), (port)) + (count)) & XS_OBUF_DEC));

/* Return address of output buffer indexed by put + 1 (next write location) */
#define XS_PUT_ADDR(ip, port)	\
	(&(ip)->outbufs.obufs[(port)].buffer[ \
		((vtoss(XS_PUT((ip),(port))) + 1) & XS_OBUF_DEC)])

/* Mark a port as active for reading */                
#define XS_SET_RD_ACTIVE(regs, port)	\
	{ \
		if ((port) <= 7) \
                        (regs).low_rd_active |= 1 << (port); \
                else \
                        (regs).hi_rd_active |= 1 << ((port) - 8); \
	}

/* Mark a port as active for writing */                
#define XS_SET_WR_ACTIVE(regs, port)	\
	{ \
		if ((port) <= 7) \
                        (regs).low_wr_active |= 1 << (port); \
                else \
                        (regs).hi_wr_active |= 1 << ((port) - 8); \
	}

/* Mark a port as inactive for reading */                
#define XS_CLR_RD_ACTIVE(regs, port)	\
	{ \
		if ((port) <= 7) \
                        (regs).low_rd_active &= ~(1 << (port)); \
                else \
                        (regs).hi_rd_active &= ~(1 << ((port) - 8)); \
	}

/* Mark a port as inactive for writing */                
#define XS_CLR_WR_ACTIVE(regs, port)	\
	{ \
		if ((port) <= 7) \
                        (regs).low_wr_active &= ~(1 << (port)); \
                else \
                        (regs).hi_wr_active &= ~(1 << ((port) - 8)); \
	}

/* Determine if a port was actively reading data */                
#define XS_TST_RD_ACTIVE(regs, port)					\
		( ((port) <= 7) ? 					\
                	((regs).low_rd_active & (1 << (port))) :	\
                        ((regs).hi_rd_active & (1 << ((port) - 8))) )

/* Determine if a port was actively writing data */                
#define XS_TST_WR_ACTIVE(regs, port)					\
		( ((port) <= 7) ? 					\
                	((regs).low_wr_active & (1 << (port))) :	\
                        ((regs).hi_wr_active & (1 << ((port) - 8))) )

/* Restart blocked input for the specified port */
#define XS_RESTART_RD(infop, portno, flags, regs)	\
		{					\
                (flags) &= ~XS_RDBLOCKED;		\
                XS_CLEAR(regs);				\
                XS_SET_RD_ACTIVE(regs, portno);		\
                (regs).port = (uchar_t)(portno);	\
                (regs).cmd_report = XSCR_RAC;		\
                xs_cmd(infop, &(regs));			\
		}

#define XS_AVGCNT	5		/* Size of typical xmit packets; used
                                         * in calculating waiting times for
                                         * data to siphon out during close(). */
/*
 * Below are the lock hierarchy values for intializing 
 * the locks within the per-port and per-controller
 * structures.  Note that whenever both must be held
 * simultaneously, that the per-controller lock must
 * be aquired first.
 */
#define XS_TTYLOCK_HIER		1	/* Hierarchy for xstty_t.ttylock */
#define XS_CNTLRLOCK_HIER	2	/* Hierarchy for xsinfo_t.cntlr_lock */

/* 
 * Macro stovs does a AB to BA byte swap on a 16-bit value.
 * this is required since the 781 swaps 16-bit values.. We
 * must compensate for this. macro vtoss does the same thing
 * as stovs, but is included so the code reads easily.
 */
#define vtoss(x) stovs(x)
#define stovs(arg) (((arg) & 0xff) << 8 | ((arg) >> 8 & 0xff))

/*
 * Forward function references.
 */
STATIC int xsopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int xsclose(queue_t *, int, cred_t *);
STATIC int xsrsrv(queue_t *);
STATIC int xswsrv(queue_t *);
STATIC void 	xs_intr(int);
STATIC void	xs_read_complete(xsinfo_t *, int, ushort, xsregs_t *);
STATIC void	xs_write_complete(xsinfo_t *, int, ushort_t);
STATIC void	xs_exc_complete(xsinfo_t *, int, ushort_t);
STATIC void 	xs_next_msg(xstty_t *, mblk_t *);
STATIC void 	xs_draintime(queue_t *);
STATIC void 	xs_delaytime(xstty_t *);
STATIC void 	xs_breaktime(xstty_t *);
STATIC int 	xs_param(xs_uparm_t *, xstty_t *);
STATIC void	xs_cmd(xsinfo_t *, xsregs_t *);
STATIC int	xs_wait(xsinfo_t *, uchar_t);
STATIC int	xs_contig_space(xsinfo_t *, int);
STATIC int	xs_bufptr_move(xsinfo_t *, int, int);
STATIC int	xs_drain(xsinfo_t *, int);

/*
 * Xs-driver specific global and local
 * variable declarations.
 */

int xsdevflag = D_MP;       		/* Required for DDI binding */

STATIC xsinfo_t **xs_info;		/* Per board state information data */
STATIC int	xs_base_vector;		/* Base VME interrupt vector */
STATIC LKINFO_DECL(xs_cntlr_lkinfo, "io:xs:unit info lock", 0);
STATIC LKINFO_DECL(xs_tty_lkinfo,   "io:xs:tty  info lock", 0);

/*
 * Streams-drivers declarations must include
 * the following set of standardish declarations
 * for externalizing their service entry-points
 * and parameters.
 */
#define STRID_XS	9		/* Streams module i.d. for xs-driver */
static struct module_info xsm_info = {
/*  module ID	module name	min pkt size	max pkt size	hiwat	lowat */
     STRID_XS,	"xs", 		0, 		XS_OBUF_SZ, 	150, 	50
};

static struct qinit xsrinit = {
	NULL,  xsrsrv,  xsopen,  xsclose,  NULL,  &xsm_info,  NULL
};

static struct qinit xswinit = {
	putq,  xswsrv,  NULL,  NULL,  NULL, &xsm_info,  NULL
};

struct streamtab xsinfo = { &xsrinit,  &xswinit,  NULL,  NULL  };

/* 
 * Table of ms-per-byte serial output rate 
 * (indexed by baud B0-EXTA).
 */
static uchar_t xsbaud_time[] = {
	0, 200, 143, 91, 77, 67, 50, 33, 17, 8, 6, 4, 2, 1, 1 
};

/*
 * This table defines the XS command bits for setting asynchronous
 * baud rates in xs_param(). '-1' is the signal to turn off DTR* output.
 * '-2' indicates that a baud rate is not available on the 781.
 */
#define XSBHANGUP         ((uchar_t)0xff)         /* hangup */
#define XSBNOSUPP         ((uchar_t)0xfe)         /* baud rate not supported */


int xs_mrates[] = {
	XSBHANGUP,		/* B0	  (hangup)	*/
	XSB_50,			/* B50			*/
	XSB_75,			/* B75			*/
	XSB_110,		/* B110			*/
	XSBNOSUPP,		/* B134.5 (not avail.)	*/
	XSB_150,		/* B150			*/
	XSBNOSUPP,		/* B200	  (not avail.)	*/
	XSB_300,		/* B300			*/
	XSB_600,		/* B600			*/
	XSB_1200,		/* B1200		*/
	XSB_1800,		/* B1800		*/
	XSB_2400,		/* B2400		*/
	XSB_4800,		/* B4800		*/
	XSB_9600,		/* B9600		*/
	XSB_19200,		/* B19200		*/
	XSBNOSUPP		/* EXTB	  (not used)	*/
};

/*
 * xsmsg() return status
 */
#define XS_MSGDONE	0		/* Message executed or started */
#define XS_PRIPUTBACK	1		/* Retry a high priority msg later */
#define XS_STDPUTBACK	2		/* Retry a normal priority msg later */

/*
 * xs_probe(xsregs_t *)
 *	 Determine if a Xylogics 781 board is installed.  
 *
 * Calling/Exit State:
 *      Basic locks may be held upon entry/exit; does not block.
 *
 *	The specified Control/Status register base virtual
 *	address must be memory mapped with the VMEbus controller
 *	upon entry/exit.  It must also be owned by the caller.
 *
 *	The caller must be prepared to handle an NMI, bus I/O access 
 *	error, if the board is not present.  Otherwise the system
 *	may panic.
 *	
 *	Returns 1 if a 781 board is present.  Return zero if
 *	another board type is present.  Results in an NMI if
 *	no board is present to respond to the attempted access.
 *
 * Description:
 *	Very Simple: attempt to read one of the device registers.
 *	Assume a 781 board is present and return 1 if the read 
 *	succeeds. Currently, the board type cannot be confirmed,
 *	so assume its a 781.  If a board is not present then
 *	the attempted read generates an NMI, bus I/O access error,
 *	and the operation will never complete.  
 */
STATIC int
xs_probe(xsregs_t *xs)
{
	/*LINTED*/
	xs->cntl_status;		/* Attempt to read the board */
	return(1);			/* No NMI - Success! */
}

/*
 * void
 * xsinit(void)
 * 	Initialize 781 hardware and driver structures.
 *
 * Calling/Exit State:
 *      Basic locks may be held upon entry/exit, although
 *      this function is normally called while still running 
 *	single threaded and does not block.  It is anticipated
 *	that this would only be the case if the driver were
 *	made dynamically loadable, such that its "load" entry
 *	point invoked xsinit() directly.
 *
 *	xs_info is allocated if any boards are configured in
 *	xs_conf[].
 *	
 *	Each board configured in xs_conf[] that is found 
 *	is reset and has a state information structure 
 *	allocated/initialized in xs_info[board], where 'board'
 *	is its corresponding index in xs_conf[].  Initialization
 *	also consumes I/O bus transfer and SLIC interrupt mapping
 *	resources, shared among all devices in the system.
 *
 * Description:
 *	First allocates the driver's xs_info array and reserves
 *	SLIC interrupt resources used in the management of any
 *	781 controllers which are configured and usable. 
 *	
 *      Then determine which, if any, configured Xylogics 781 boards 
 *	are present and usable.  For each configured board, invoke 
 *	xs_probe() via ssm_vme_probe() to determine if the expected
 *	781 is present. Prior to this, the board's CSR configuration 
 *	must be registered/mapped with the I/O bus adapter.
 *
 * 	If the board is not found then unmap the configuration
 * 	and mark the configuration as unavailable for use by
 * 	leaving its pointer in xs_info[] unallocated.
 *
 * 	Otherwise, complete that controller's initialization and 
 *	make the controller available for use as follows:
 *
 *		allocate controller/port state and messge passing structs,
 *	
 *		establish an I/O bus adapter mapping for controller initiated
 *		DMA access to the message passing structures,
 *
 *		establish hardware interrupt mapping and install an 
 *		interrupt handling for the board,
 *	
 *		initialize uart parameters for all ports, generate an
 *		initialization descriptor block (idb), then reset the
 *		controller so that it recieves them.
 *
 *		complete initialization of controller and port state
 *		structures, then announce the valid board configuration.
 *
 *	If any aspect of the controller's initialization fails, then 
 *	release all resources previously acquired for it and leave it
 *	unusable/deconfigured.  A board is presumed usable if it has
 *	a non-null ptr to its info-structs in the xs_info array.
 */
void
xsinit(void)
{
	int board, port;
	struct xs_conf *vconf;
	xsinfo_t *ip;
	void *csr_map;
	xsregs_t *csr; 
	paddr_t idb, inbuf, outbuf;
	xstty_t *tp;
	xs_uparm_t *up;

	if (xs_nconf == 0)
		return;			/* Kernel configuration has no 781s */

	xs_info = (xsinfo_t **)
		kmem_zalloc(sizeof (xsinfo_t *) * xs_nconf, KM_NOSLEEP);
	if (xs_info == NULL) {
		/*
		 *+ The xs-driver could not allocate system memory 
		 *+ for data structures required to manage Xylogics-
		 *+ 781 controllers.  All xs-devices have been 
		 *+ deconfigured as a result of this failure.
		 *+ Reconfiguring the kernel to use less memory or 
		 *+ adding more memory to the system may correct this.
		 */
		cmn_err(CE_WARN, 
			"xs: No memory for info structure; deconfigured.");
		return;
	}

	/* 
	 * Reserve a sequential group of SLIC interrupt vector, 
 	 * one per controller configuration.  This allows 
	 * the interrupt handler, xs_intr(), to easily determine
	 * which controller requested service:
	 * 	board# = handler_vector_arg - xs_base_vector.
	 */
	xs_base_vector = ivec_alloc_group(xs_global.bin, xs_nconf);
	if (xs_base_vector < 0) {
		/*
		 *+ The xs-driver could not allocate SLIC
		 *+ interrupt vectors for use with its devices;
		 *+ all xs-devices have been deconfigured as a
		 *+ result.  To correct this problem, reconfigure 
		 *+ the kernel so that some of the drivers sharing 
		 *+ the same SLIC bin as the xs-driver use other bins.
		 */
		cmn_err(CE_WARN, 
			"xs: Unable to assign SLIC interrupts; deconfigured.");
		return;
	}


	/*
	 * Probe for each of the board configurations.
	 * If present then begin allocating structures
	 * for the board and initialize them.
	 */
	for (board = 0; board < xs_nconf; board++) {
		vconf = xs_conf[board];
		ASSERT(vconf->tag == board);
		if (vconf->configured == 0)
			continue;		/* Do not use this config */

                if ( !( SSM_EXISTS ( vconf->ssmno ) && SSM_desc[vconf->ssmno].ssm_vme_alive))
                {
                        /*
                        *+ either ssm or ssm/vme is missing
                        */
                        cmn_err ( CE_CONT, "SSM/vme %d doesn't exist \n", vconf->ssmno ) ;
                        continue ;
		}



		/*
		 * Generate an adapter mapping for the board's
		 * CSR configuration and probe for a 781 there.
		 */
		ip = NULL;			/* Must be initialized */
		csr_map = ssm_vme_mdesc_alloc(vconf->ssmno, 
						XS_CSR_AMOD, KM_NOSLEEP);
		if (csr_map == NULL) {
			csr = NULL;
		} else {
			csr = (xsregs_t *)ssm_s2v_map(csr_map, vconf->csr, 
				  sizeof (xsregs_t), KM_NOSLEEP);
		}
		if (csr == NULL) {
			/*
			 *+ The xs-driver could not generate a mapping 
			 *+ for the control/status registers of the specified
			 *+ 781 controller, based on its configuration
			 *+ parameters.  Probable causes are that its 
			 *+ register configuration overlaps that of
			 *+ another configured controller on the same I/O bus,
			 *+ or that all I/O bus mapping resources have been
			 *+ exhausted, or that the driver is now using an
			 *+ invalid mapping modifier (unlikely).  Reconfiguring
			 *+ the system might correct this condition.
			 */
			cmn_err(CE_WARN, 
			"xs%d: I/O bus register mapping failed; deconfigured.",
				board);
			goto deconfig_board;	/* Cleanup allocations */
		}
		
		if (ssm_vme_probe(vconf->ssmno, xs_probe, (void *)csr) == 0) {
			goto deconfig_board;	/* 781-controller not found */
		}

		/*
		 * The board is present.  Attempt to complete
		 * its hardware and driver state initialization.
		 * If any of this fails, release all resources
		 * acquired for it and leave it unusable.
		 */
		ip = (xsinfo_t *) kmem_zalloc(sizeof (xsinfo_t), KM_NOSLEEP);
		if (!ip) {
			/*
			 *+ The xs-driver could not allocate system memory 
			 *+ for data structures required to manage the 
			 *+ specified Xylogics-781 controller which was
			 *+ specified in the kernel configuration and had
			 *+ been found on the I/O bus.  The specified
		 	 *+ board has been deconfigured as a result of 
			 *+ this failure.  To correct this condition,
			 *+ reconfigure the system.
		 	 */
			cmn_err(CE_WARN, 
			"xs%d: No memory for info structure; deconfigured.", 
				board);
			goto deconfig_board;	/* Cleanup allocations */
		}
		ip->boardnum = board;
		ip->config = vconf;
		ip->csr_map = csr_map;
		ip->csr = csr;

		/* 
		 * CAUTION:
		 *	The input buffer, outbput buffer, and idb
		 *	structures for communicating with the 781 
		 *	must be declared/allocated in the 
	 	 * 	afforementioned order for the mapping code 
		 * 	below to work.  The idb must be short-aligned
		 * 	and i/o buffers must be long-aligned. A 
		 *	change to xsinfo_t may require changes below.
		 */
		ASSERT((((ulong_t)&ip->inbufs) & 3) == 0);
		ASSERT((((ulong_t)&ip->outbufs) & 3) == 0);
		ASSERT((((ulong_t)&ip->idb) & 1) == 0);
		ASSERT( ((ulong_t)&ip->inbufs) < ((ulong_t)&ip->outbufs) );
		ASSERT( ((ulong_t)&ip->outbufs) < ((ulong_t)&ip->idb) );

		/* 
		 * Set up VMEbus initiator to Sequent bus transfer 
		 * mapping for input buffer, output buffer, and idb.  
		 * Just do this as one conglomerant since they are 
		 * allocated sequentially in the info structure and 
		 * use the same address modifiers.
		 */
		ip->msg_map = ssm_vme_mdesc_alloc(vconf->ssmno, 
						XS_DMA_AMOD, KM_NOSLEEP);
		if (ip->msg_map == NULL) {
			ip->msg_blk = VME_INVALID_ADDR;
		} else {
			ip->msg_blk = ssm_v2s_map(ip->msg_map, 
				(caddr_t)&ip->inbufs, sizeof (xsinfo_t) - 
				(int) &((xsinfo_t *)0)->inbufs, KM_NOSLEEP);
		}
		if (ip->msg_blk == VME_INVALID_ADDR) {
			/*
			 *+ The xs-driver could not generate a mapping 
			 *+ for the message blocks of the specified
			 *+ 781 controller.  Probable causes are that
			 *+ all I/O bus mapping resources have been
			 *+ exhausted or that the driver is now using an
			 *+ invalid mapping modifier (unlikely).  Reconfiguring
			 *+ the system might correct this condition.
			 */
			cmn_err(CE_WARN, 
			"xs%d: I/O bus msg-blk mapping failed; deconfigured.",
				board);
			goto deconfig_board;	/* Cleanup allocations */
		}
		inbuf = ip->msg_blk;
		outbuf = inbuf + (((paddr_t) &((xsinfo_t *)0)->outbufs) -
			((paddr_t) &((xsinfo_t *)0)->inbufs));
		idb = inbuf + (((paddr_t) &((xsinfo_t *)0)->idb) -
			((paddr_t) &((xsinfo_t *)0)->inbufs));
		
		/* 
		 * For the 781 to interrupt the driver, a I/O bus
		 * to SLIC interrupt mapping must be created.
		 * The SLIC interrupt bin and vector have already
		 * been allocated for this device, and its I/O bus 
		 * interrupt level was specified in the kernel
		 * configuration, but its vector is software 
		 * programable.  The code which follows will attempt
		 * to select an I/O bus vector for this board and
		 * then finish setting up the interrupt mapping.
		 */ 
		ip->vector = ssm_vme_assign_vec(vconf->ssmno, vconf->intlvl,
				xs_global.bin, xs_base_vector + board);
		if (ip->vector < 0) {
			/* 
			 *+ The xs-driver could not generate a mapping
			 *+ from an I/O bus interrupt to a SLIC interrupt
			 *+ for the specified 781 controller.  Possible 
			 *+ causes are an invalidly configured I/O bus 
			 *+ interrupt level, an I/O bus/adapter problem,
			 *+ or that all possible I/O bus interrupt vectors
			 *+ have been used up, leaving none for this board.
			 *+ Recheck the system configuration and hardware
			 *+ installation in an attempt to correct this.
			 */
			cmn_err(CE_WARN, "xs%d: I/O bus interrupt ", board);
			cmn_err(CE_CONT, "vectors alloc failed; deconfigured.");
			goto deconfig_board;	/* Cleanup allocations */
		}
		ivecinit(xs_global.bin, xs_base_vector + board, xs_intr);

		/* 
		 * Prior to resetting the board, initialize
		 * its output buffers, uart parameter blocks,
		 * and idb.  Then load the board's CSR with
		 * the idb address and reset the board.
		 *
		 * First init the output buffers and uart 
		 * parameter blocks...
		 */
		for (port = 0; port < XS_MAXPORTS; port++) {
			ip->outbufs.obufs[port].get = stovs(1);
			up = &ip->idb.uart_parms[port];
			up->serial_modes =
				XS_STOP1 | XS_BITS8 | XS_NO_PARITY | XS_HW_FLOW;
			up->baud_rates = XSB_9600 << 12 | XSB_9600 << 8;
			up->line_mask = XSLINE_DSR | XSLINE_CTS | XSLINE_DCD;
 			up->input_timeout = stovs(xs_global.rxtime);
 			up->input_min = stovs(xs_global.ibufsize);
			up->low_water = stovs(xs_global.lowwater);
			up->break_time = stovs(XS_BRKTICKS(xs_global.brktime));
		}

		/* ...then initialize idb:  interrupts and buffer addressing */
		ip->idb.inp_intlev = ip->idb.out_intlev = 
			ip->idb.stat_intlev= stovs(vconf->intlvl);
		ip->idb.inp_intvec  = ip->idb.out_intvec = 
			ip->idb.stat_intvec = stovs(ip->vector);
		ip->idb.addr_mod = stovs(SSMVME_AMOD(XS_DMA_AMOD));
		ip->idb.icba_hi = stovs(inbuf >> 16);
		ip->idb.icba_lo = stovs(inbuf);
		ip->idb.inbuf_len = stovs(XS_IBUF_SZ);
		ip->idb.ocrba_hi = stovs(outbuf >> 16);
		ip->idb.ocrba_lo = stovs(outbuf);
		ip->idb.outbuf_len = stovs(XS_OBUF_SZ);
		csr->idb_addr3 = (ushort)(idb >> 24);
		csr->idb_addr2 = (ushort)(idb >> 16);
		csr->idb_addr1 = (ushort)(idb >> 8);
		csr->idb_addr0 = (ushort)(idb);
		csr->idb_addr_mod = SSMVME_AMOD(XS_DMA_AMOD);

		/* 
		 * Now instruct the board to reset itself,
		 * then busy-wait a limited time for it to complete.
		 * If the reset fails or times out, deallocate
		 * resources for it and leave it not configured.
		 */
		csr->cntl_status = XSCR_MRST;
		if (xs_wait(ip, XSSR_RSTA) != 0) {
			/*
			 *+ The specified Xylogics 781 controller failed 
			 *+ to execute a "Module Reset" command; its
			 *+ termination status code was also specified.
			 *+ Recheck its hardware installation and 
			 *+ software configuration.
			 */
			cmn_err(CE_WARN, "xs%d: Board failed reset, ", board);
			cmn_err(CE_CONT, "code %d; deconfigured.", 
				csr->cntl_status);
			goto deconfig_board;	/* Cleanup allocations */
		}

		/* 
		 * The board reset successfully.  What remains is to
		 * finish initializing the controller lock and state 
		 * information/synch. structs for each port.
		 */
		ip->cntlr_lock = LOCK_ALLOC(XS_CNTLRLOCK_HIER, 
			plstr, &xs_cntlr_lkinfo, KM_NOSLEEP);
		if (ip->cntlr_lock == NULL) {
			port = -1;		/* So test below fails also */
		} else {
			/* Init per-port state information. */
			for (tp = ip->tty, port = 0; 
			     port < XS_MAXPORTS; tp++,port++) {
				tp->ttylock = LOCK_ALLOC(XS_TTYLOCK_HIER, plstr,
						&xs_tty_lkinfo, KM_NOSLEEP);
				tp->drainwait = SV_ALLOC(KM_NOSLEEP);
				tp->carrwait = SV_ALLOC(KM_NOSLEEP);
				tp->bufwait = SV_ALLOC(KM_NOSLEEP);
				if (tp->ttylock == NULL || tp->drainwait == NULL
				||  tp->carrwait == NULL || tp->bufwait == NULL)
					break;	/* Failed port initialization */

				tp->devno = makedevice(xs_global.c_major, 
							XSMINOR(board, port));
			}
		}

		if (port == XS_MAXPORTS) {
			/*
			 * Reaching this code implies that the
			 * board initialization has been successful;
			 * make it available for use then move along
			 * to the next board configuration. 
			 */
			xs_info[board] = ip;
			/*
			 *+ The xs-driver is announcing that it has
			 *+ succeeded at locating and initializing
			 *+ one of its configured controllers, which
			 *+ is now available for use.  No corrective
			 *+ action is required.
			 */
			cmn_err(CE_CONT, "xs %d found on ssm %d ",
				board, vconf->ssmno);
			cmn_err(CE_CONT, "csr 0x%x lvl %d vec %d\n",
				vconf->csr, vconf->intlvl, ip->vector);
			continue;		/* Try the next board/config */
		}

		/* 
		 * Reaching this code implies that the controller
		 * lock allocation or per port initialization failed.  
		 * After undoing the per port allocations flow into 
		 * common code which undoes other per-controller allocations.
		 */

		/*
		 *+ The xs-driver was unable to allocate/initialize
		 *+ synchronization structures for managing the 
		 *+ specified 781 board.  This most likely is due to 
		 *+ exhaustion of allocatable kernel memory, which may 
		 *+ be corrected by reconfiguring the kernel or adding 
		 *+ more memory to this system.
		 */
		cmn_err(CE_WARN, 
			"xs%d: Sync structure allocs failed; deconfigured.", 
				board);
		for( ; port >= 0; port--, tp--) {
			if (tp->ttylock != NULL)
				LOCK_DEALLOC(tp->ttylock);
			if (tp->drainwait != NULL)
				SV_DEALLOC(tp->drainwait);
			if (tp->carrwait != NULL)
				SV_DEALLOC(tp->carrwait);
			if (tp->bufwait != NULL)
				SV_DEALLOC(tp->bufwait);
		}
deconfig_board:
		/* 
		 * Common code for unwinding the allocations
		 * and I/O bus transfer mappings that occured
		 * prior to an initialization failure for this
		 * controller configuration.
		 */
		if (ip != NULL) {
			if (ip->msg_map != NULL) {
				if (ip->cntlr_lock != NULL)
					LOCK_DEALLOC(ip->cntlr_lock);
				ssm_vme_mdesc_free(ip->msg_map);
			}
			kmem_free(ip, sizeof (xsinfo_t));
		}
		ssm_vme_mdesc_free(csr_map);
	}				/* Try the next board configuration */
}

/*
 * STATIC void
 * xsopen_wakeup(xstty_t *)
 * 	Wakeup open()s waiting on tp->bufwait for a
 *	stream buffers to become available.
 *
 * Calling/Exit State:
 *	The the specified tty's ttylock and controller's lock
 *	must not be held upon entry or exit, although other 
 *	basic locks may.
 *
 *	Has no process context and does not block.
 *
 *	Clears tp->owaitid to indicate that a bufcall() 
 *	or itimeout() is no longer pending.  The itimeout()
 *	is used as a backup if bufcall() fails.  This
 *	duel usage works, since they are never undone once
 *	started.
 *	
 *	Broadcasts a wakeup to all processes blocked
 *		on the tp->bufwait synch variable.
 *	
 *	No return value.
 */
STATIC void
xsopen_wakeup(xstty_t *tp)
{
	pl_t ttypl = LOCK(tp->ttylock, plstr);

	tp->owaitid = 0;
	SV_BROADCAST(tp->bufwait, 0);
	UNLOCK(tp->ttylock, ttypl);
}

/*
 * STATIC int
 * xsopen(queue_t *, dev_t *, int, int, cred_t *)
 * 	Open a stream for the specified 781 board/serial port.
 *
 * Calling/Exit State:
 *	Basic locks must not be held upon entry or exit.
 *
 *	Upon successful completion, the port's xstty_t 
 *	structure is attached to the queue descriptor.
 *
 *	Possible returns:
 *	EAGAIN  unable to wait for resource allocation.
 *	EBUSY	the queue for this device appears to already 
 *		be in use, although this is the device's first open.
 *	EINVAL	attempt to initialize the port parameters failed.
 *	EINTR	interrupted by a signal while waiting for streams
 *		message buffer or carrier.
 *	ENOSR	if streams resources are unavailable to completed
 *		the open.
 *	ENXIO	sflag not DRVOPEN.	
 *	ENXIO	minor device specified is invalid or unusable.
 *	0	open succeeded.
 *
 * Description:
 *	Verify that the minor device is valid prior to locating
 *	driver state information for the port.  If not already
 *	open, determine initial port parameters and communicate
 *	them to the 781 board, flush any transient data the 781
 *	may have for this port, and initiate input solicitation
 * 	if opened for reads.  If already open, just supply a DTR 
 *	setting and assume a session leader will otherwise init
 *	port parameters if necessary.
 *
 *	All opens wait for carrier, if non-blocking, then inform 
 *	the stream head that this device can serve as a ctty.  
 *	Lastly, enable the queue' service routines if the device 
 *	is not already open.
 */
/*ARGSUSED*/
STATIC int
xsopen(queue_t *rd_q, dev_t *devp, int oflag, int sflag, cred_t *crp)
{
	xstty_t *tp;
	xsinfo_t *ip;
	xsregs_t xsregs;
	pl_t ttypl;
	mblk_t *bp;
	struct stroptions *sop;
	int port = XSPORT(*devp);
	int board = XSBOARD(*devp);
	int error;
	uchar_t firstopen;

	/* Reject non-driver opens or invalid/unusable device specification */
	if (sflag != DRVOPEN || xs_info == NULL || board >= xs_nconf 
	||  (ip = xs_info[board]) == NULL) {
		return (ENXIO);
	}

	XS_CLEAR(xsregs);		 	/* Start w/ clean copy of CSR */
	tp = &ip->tty[port];
	ttypl = LOCK(tp->ttylock, plstr);

	/*
	 * If the port is not already open, then 
	 * initialize it to the default state.
	 */
	if ((tp->tty_state & XS_ISOPEN) == 0) {
		if (rd_q->q_ptr != NULL) {
			UNLOCK(tp->ttylock, ttypl);
			/*
			 *+ The xs-driver open routine has been passed a
			 *+ pointer to a stream that appears to be in use
			 *+ by another device or that was not cleaned up
			 *+ correctly by its previous owner.
			 */
			cmn_err(CE_WARN, 
				"xs%d: first open found queue already in use.", 
				ip->boardnum);
			return (EBUSY);
		}

		/* Set default driver state information about the port */
		tp->qptr = WR(rd_q);
		rd_q->q_ptr = (char *)tp;
		WR(rd_q)->q_ptr = (char *)tp;
		tp->flags = 0;
		tp->tty_state = XS_ISOPEN;
		tp->cflags = xs_global.cflags;
		tp->iflags = xs_global.iflags;
		tp->cstart = CSTART;
		tp->cstop = CSTOP;
		tp->flow = 0;
		tp->msg = NULL;
		tp->bufcallid = 0;
		tp->timeoutid = 0;
		tp->delayid = 0;
		tp->breakid = 0;
		tp->inputtime = 0;
		tp->inputmin = 0;


		/* Execute a READ-FLUSH to discard any transient data */
		xsregs.port = (uchar_t)port;
		xsregs.cmd_report = XSCR_FLUSH;
		xs_cmd(ip, &xsregs);

		/* 
		 * Attempt to set port parmeters to the board.
		 * If this fails, then clear the port and
		 * stream state info prior to failing the open.
		 */
		if (xs_param(&ip->idb.uart_parms[port], tp) != 0) {
			error = EINVAL;
			goto first_open_fail;
		}

		/* 
		 * Finish initializing I/O buffer and uart
		 * parameters for this port, then execute
		 * a UART-INIT command.
		 */
		ip->inbufs.ibufs[port].count = 0;
		if ((tp->cflags & CREAD) != 0) {
			/* Enable RX interrupts */
			ip->idb.uart_parms[port].line_mask |= XS_IENABLE;
		} else {
			ip->idb.uart_parms[port].line_mask &= ~XS_IENABLE;
		}
		ip->idb.uart_parms[port].line_mask &= ~(XS_UNI | XSLINE_DCD);
		ip->idb.uart_parms[port].line_mask |= XS_MCU;
		ip->idb.uart_parms[port].line_state |= XSLINE_DTR;
		xsregs.cmd_report = XSCR_UINIT;
		xs_cmd(ip, &xsregs);

		/* Do another READ-FLUSH to discard any transient data */
		xsregs.cmd_report = XSCR_FLUSH;
		xs_cmd(ip, &xsregs);
		XS_CLEAR(xsregs);

		/* Clear internal flow conditions by executing a WRITE-RESET */
		xsregs.cmd_report = XSCR_RESET;
		xsregs.port = (uchar_t)port;
		xs_cmd(ip, &xsregs);
		XS_CLEAR(xsregs);

		/* Since transient data was flushed, a read can be issued */
		if (tp->cflags & CREAD) {
			XS_SET_RD_ACTIVE(xsregs, port);
			xsregs.cmd_report = XSCR_RAC;
			xs_cmd(ip, &xsregs);
		}
		firstopen = 1;			/* Denote this as first open */
	} else {
		/*
		 * The device is already open.  In case a
		 * session leader does not get the "firstopen" 
		 * case because a process is still running from 
		 * the previous session, supply a basic setting 
		 * for DTR if required.  But the new session 
		 * leader perform all other initialization.
		 *
		 * ttymon sends an intervening TCSET after a B0 then
		 * does a re-open to turn on DTR. so we can't use
		 * b0 to decide if should turn on DTR. in this case
		 * we always turn on DTR
		 * 
	 	 */	
		if ((tp->cflags & CBAUD) == 0) {
			tp->cflags |= xs_global.cflags & CBAUD;
		}
		if (xs_param(&ip->idb.uart_parms[port], tp) != 0) {
			UNLOCK(tp->ttylock, ttypl);
			return (EINVAL);
		}
		ip->idb.uart_parms[port].line_state |= XSLINE_DTR;
		xsregs.cmd_report = XSCR_UINIT;
		xsregs.port = (uchar_t)port;
		xs_cmd(ip, &xsregs);
		firstopen = 0;		/* Denote that device is already open */
	}

	if (tp->cflags & CLOCAL)
		tp->tty_state |= XS_CARR_ON;
	else
		tp->tty_state &= ~XS_CARR_ON;

	if ((oflag & (FNDELAY | FNONBLOCK)) == 0) {
		/* 
		 * Read the modem line status and
		 * wait for carrier if not set.
		 */
		XS_CLEAR(xsregs);
		xsregs.cmd_report = XSCR_MGET;
		xsregs.port = (uchar_t)port;
		xs_cmd(ip, &xsregs);
		(void) xs_wait(ip, XSSR_ARCP);
		ssm_vme_dma_flush(ip->config->ssmno);

		if (ip->idb.uart_parms[port].line_state & XSLINE_DCD)
			tp->tty_state |= XS_CARR_ON;	/* Carrier is set */

		while ((tp->tty_state & XS_CARR_ON) == 0) {
			/*
			 * Wait for carrier.  If we get signalled out of the
			 * wait and we are the first openers of this port,
		 	 * then clear the port and stream state info prior 
			 * to failing the open.
			 */
			tp->tty_state |= XS_WOPEN;
			if (SV_WAIT_SIG(tp->carrwait,
					pritty, tp->ttylock) == FALSE) {
				error = EINTR;	/* Signalled out of wait */
				goto open_fail;
			}
			ttypl = LOCK(tp->ttylock, plstr);
		}
		tp->tty_state &= ~XS_WOPEN;
	}
	UNLOCK(tp->ttylock, ttypl);

	/* 
	 * Inform the stream head that this device can be 
	 * a ctty.  In the event of multiple opens, let it 
	 * sort out which process is the session leader and
	 * actually register the ctty.  
	 */
	while ((bp = allocb(sizeof (struct stroptions), BPRI_MED)) == NULL) {
 		if (oflag & (FNDELAY | FNONBLOCK)) {
			error = EAGAIN;		/* Cannot wait for allocation */
			goto open_fail;
                }
		/*
		 * Wait for the resources to become available
		 * and try again to aquire them.  If a bufcall() 
		 * wait is already in progress join it.  If the
		 * bufcall() fails, try scheduling a timeout to
		 * do the same thing.  If that fails, then give 
		 * up and fail the open as well.
		 */
		ttypl = LOCK(tp->ttylock, plstr);
		if (! tp->owaitid) {
			tp->owaitid = 
				bufcall((uint_t)sizeof (struct stroptions), 
					BPRI_MED, xsopen_wakeup, (long)tp);
			if (tp->owaitid == 0) {
				tp->owaitid = itimeout(xsopen_wakeup,
					(caddr_t)tp, drv_usectohz(XS_BUF_WAIT),
					plstr);
				if (tp->owaitid == 0) {
					UNLOCK(tp->ttylock, ttypl);
					error = ENOSR;
					goto open_fail;
				}
			}
		}
		if (SV_WAIT_SIG(tp->bufwait, pritty, tp->ttylock) == FALSE) {
			error = EINTR;		/* Signalled out of wait */
			goto open_fail;
		}
        }
        bp->b_datap->db_type = M_SETOPTS;
        bp->b_wptr += sizeof (struct stroptions);
	/*LINTED*/
        sop = (struct stroptions *)bp->b_rptr;
        sop->so_flags = SO_ISTTY;
        putnext(rd_q, bp);

	if (firstopen)
		qprocson(rd_q);		/* Enable the queue service routines */

	return(0);			/* Open succeeded */

open_fail:
	/* 
	 * Shared code for error exits once the tty 
	 * has been partially initialized.  
	 * Note: only failure during firstopen code
	 * already has the ttylock held.
	 */
	if (firstopen) {
		ttypl = LOCK(tp->ttylock, plstr);
first_open_fail:
		tp->tty_state = 0;
		tp->flags = 0;
		tp->qptr = NULL;
		rd_q->q_ptr = NULL;
		WR(rd_q)->q_ptr = NULL;
		UNLOCK(tp->ttylock, ttypl);
	}
        return (error);
}

/*
 * STATIC int
 * xsclose(queue_t *, int, cred_t *)
 * 	Close the specified 781 board/serial port.
 *
 * Calling/Exit State:
 *	Basic locks must not be held upon entry or exit.
 *
 *	The queue is emptied of all input and output, then
 *	detached from the port's xstty_t structure.
 *
 *	Always returns zero.
 *
 * Description:
 *	Stop solicitation of input from the 781 for that port, 
 *	pushing buffered input upstream or flushing it.  If 
 *	output remains buffered, allow it a reasonable amount 
 *	of time to drain.  If unacceptable to wait or it doesn't
 *	drain fast enough, abort the remainder.  Then diable 
 *	service routines for the queue, deallocate remaining 
 *	message blocks, and perform disconnect prior to returning.
 *
 * Remarks: 
 *	In calculating a reasonable draintime, the amount of
 *	data currently queued in the output buffer is unknown,
 *	since the driver does not count each byte going into it
 *	and uncount them when it estimates they have xferred.
 *	Instead, it uses the output buffer size as a "worst case" 
 *	estimate.
 *
 *	The 781 has a bug where it may have an internal 
 *	pointer that allows old data in its onboard buffer 
 *	to be transmitted again when the output buffer empties.  
 *	Issuing a RESET clears the problem.
 */
/*ARGSUSED*/
STATIC int
xsclose(queue_t *rd_q, int flag, cred_t *crp)
{
	xstty_t *tp = (xstty_t *) rd_q->q_ptr;
	xsinfo_t *ip = xs_info[XSBOARD(tp->devno)];
	int port = XSPORT(tp->devno);
	xsregs_t xsregs;
	pl_t ttypl;
	short wait, numq;
	toid_t bid, drainid;
	bool_t status;

	ttypl = LOCK(tp->ttylock, plstr);
	XS_CLEAR(xsregs);

	/*
	 * If the carrier is still active and NDELAY 
	 * is not set, try to process all queued output.
	 */
	if ((tp->tty_state & XS_CARR_ON) && ((flag & FNDELAY) == 0)) {
		tp->flags |= XS_DISCARDIN;	/* Logically disable input */

		if ((numq = qsize(WR(rd_q))) > 0 || xs_drain(ip, port) != 0) {
			/* 
			 * Estimate a reasonable time in which 
			 * remaining output should complete, and
			 * wait until either it drains or times out.
 			 * If a timeout cannot be scheduled, then
			 * just complete the close now without waiting.
			 */
			tp->tty_state |= XS_WCLOSE;
			tp->tty_state &= ~XS_DRAINTIME;
			wait = ( (short)(((numq * XS_AVGCNT) + XS_OBUF_SZ)
				* (short)xsbaud_time[(tp->cflags & CBAUD)])
				/ (short)1000) + (short)xs_global.addwait;
			drainid = itimeout(xs_draintime, 
					  (caddr_t)rd_q, wait * HZ, plstr);
			if (drainid == 0) {
				/*
				 *+ The xs-driver encountered internal condition
				 *+ during an attempt to close the indicated 
				 *+ Xylogics 781 board and port in which it was
				 *+ unable to wait for remaining output to be
				 *+ performed.  The extraneous data has been 
				 *+ flushed in order to recover the port.
				 */
				cmn_err(CE_WARN, 
				"xs%d: Output lost on close, port %d.", 
						ip->boardnum, port);
				tp->tty_state &= ~XS_WCLOSE;
			}

			while (tp->tty_state & XS_WCLOSE) {
				status = SV_WAIT_SIG(tp->drainwait,
						pritty, tp->ttylock);
				ttypl = LOCK(tp->ttylock, plstr);

				/* 
				 * A timeout clears WCLOSE and exits the loop.
				 * For a job control signal, continue loop.
				 * When either output draines or the wait was 
				 * interrupted by non-job control signal(s),
				 * then cancel the pending timeout and exit
				 * the wait loop.
				 */
				if ((tp->tty_state & XS_DRAINTIME) == 0
				&& ((tp->tty_state & XS_WCLOSE) == 0
				    || status == FALSE)) {
					UNLOCK(tp->ttylock, ttypl);
					untimeout(drainid);
					ttypl = LOCK(tp->ttylock, plstr);
					tp->tty_state &= ~XS_WCLOSE;
				}
			} 
		}
	} 
	/*
	 * Normally, output at the board is only flushed
	 * at this point if the carrier is off or NDELAY 
	 * is set, or the wait timed out.  But the port 
	 * also needs to be reset to workaround the 781 
	 * bug mentioned earlier, so always do it here.
	 */
	xsregs.port = (uchar_t)port;
	xsregs.cmd_report = XSCR_RESET;
	xs_cmd(ip, &xsregs);

	/* 
	 * Now release all allocated messages on the
	 * queue. Then disable receiver data transfers
	 * at the board and discard leftover data. 
	 * Also, perform a hangup if configured to do so.
	 */
	flushq(WR(rd_q), FLUSHALL);
	freemsg(tp->msg);
	tp->msg = NULL;

	(void) xs_wait(ip, XSSR_ARCP);	
	ip->outbufs.obufs[port].get = stovs(1);
	ip->outbufs.obufs[port].put = 0;
	ip->idb.uart_parms[port].line_mask &= ~XS_IENABLE;
	ip->idb.uart_parms[port].line_mask |= XSLINE_DCD;
	if (tp->cflags & HUPCL)
		ip->idb.uart_parms[port].line_state &= ~XSLINE_DTR;

	XS_CLEAR(xsregs);
	xsregs.port = (uchar_t)port;
	xsregs.cmd_report = XSCR_FLUSH;	/* Discard transient data */
	xs_cmd(ip, &xsregs);
	xsregs.cmd_report = XSCR_UINIT; /* Reset uart to new settings */
	xs_cmd(ip, &xsregs);
	xsregs.cmd_report = XSCR_FLUSH; /* Again discard any transient data */
	xs_cmd(ip, &xsregs);

	/*
	 * In order to finish clearing the queue, we must
	 * inhibit further bufcalls, disable the queue service
	 * routines, and then cancel any pending bufcalls
	 * for this port once service routines are stopped.
	 */
	bid = tp->bufcallid;
	tp->bufcallid = 1;		/* Inhibit further bufcalls */
	tp->tty_state &= ~XS_ISOPEN;
	UNLOCK(tp->ttylock, ttypl);

	/* 
	 * Finish clearing the port state, including
	 * cancelling any pending timeouts and bufcalls.  
	 * Then disconnect the port info from this 
 	 * stream prior to returning.
	 */
	if (bid != 0)
		unbufcall(bid);
	if (tp->timeoutid != 0)
		untimeout(tp->timeoutid);
	if (tp->delayid != 0)
		untimeout(tp->delayid);
	if (tp->breakid != 0)
		untimeout(tp->breakid);

	qprocsoff(rd_q);

	(void)LOCK(tp->ttylock, plstr);
	tp->tty_state = 0;
	tp->flags = 0;
	tp->qptr = NULL;
	UNLOCK(tp->ttylock, ttypl);
	rd_q->q_ptr = NULL;
	WR(rd_q)->q_ptr = NULL;
	return (0);
}

/*
 * STATIC void
 * xsrecover(xstty_t *)
 * 	Input bufcall recovery interface.
 *
 * Calling/Exit State:
 *	Has no process context and does not block.
 *
 *	The controller lock for the specified port and the
 *	port's tty lock must not be held upon entry/exit.
 *
 *	Other basic locks may be held upon entry/exit.
 *
 *	Input solicitation from the 781 for this port has
 *	been suspended until data already in the board's 
 *	input buffer can be passed upstream.  It is resumed
 *	if data is successfully passed upstream.
 *
 *	If a stream buffer is now available, tp->bufcallid
 *	and tp->timeoutid are cleared to indicate that no 
 *	bufcall()s or itimeout()s are pending for the port
 *	awaiting a new buffer.  Resets it with another bufcall()
 *	or itimeout(), if bufcall() fails,  otherwise.
 *
 *	No return value.
 *
 * Remarks:
 *	If the input buffer count has somehow been corrupted 
 * 	from the time the 781 interrupted us until now, discard 
 *	the data and restart the read anyway.
 *
 * 	Try to get a streams buffer for the input data.  If we 
 *	*still* cannot get a buffer, the only thing to do is
 *	bufcall again.  Otherwise, load the buffer with input 
 *	data and send it upstream.  If the bufcall fails, try
 *	an itimeout().  If that fails then bail out, flushing
 *	the data and allowing reads to be reenabled.  This is
 *	drastic action, but there seems no other way to 
 *	reschedule a wakeup to process the data.
 */
STATIC void
xsrecover(xstty_t *tp)
{
	mblk_t	*mptr;
	int port = XSPORT(tp->devno);
	int board = XSBOARD(tp->devno);
	xsinfo_t *ip = xs_info[board];
	xsregs_t xsregs;
	uint_t cnt;
	pl_t ttypl;

	ttypl = LOCK(tp->ttylock, plstr);
	if ((tp->tty_state & (XS_ISOPEN | XS_WCLOSE)) != XS_ISOPEN) {
		UNLOCK(tp->ttylock, ttypl);
		return;				/* Ignore this invocation */
	}

	/* Calculate and validate the amount of pending input */
	cnt = vtoss(ip->inbufs.ibufs[port].count);
	if (cnt > XS_IBUF_SZ) {
		XS_RESTART_RD(ip, port, tp->flags, xsregs);
		UNLOCK(tp->ttylock, ttypl);
		(void)putctl(WR(tp->qptr), M_STARTI);
		/*
		 *+ The xs-driver has detected some an unexpected
		 *+ input block-size from the specified 781 board 
		 *+ and port.  The data located within will be 
		 *+ discarded and new input will be solicited.
	 	 */
		cmn_err(CE_WARN, 
			"xs%d: illegal block input count %d port %d rec.",
  			board, cnt, port);
		return;
	}

	/* Try to send the data upstream */
	if ((mptr = allocb(cnt, BPRI_MED)) == NULL) {
		tp->bufcallid = bufcall(cnt, BPRI_MED, xsrecover, (long)tp);
		if (tp->bufcallid == 0) {
			tp->timeoutid = itimeout(xsrecover, (caddr_t)tp,
				drv_usectohz(XS_BUF_WAIT), plstr);
			if (tp->timeoutid == 0) {
				XS_RESTART_RD(ip, port, tp->flags, xsregs);
				UNLOCK(tp->ttylock, ttypl);
				(void)putctl(WR(tp->qptr), M_STARTI);
				/*
				 *+ The xs-driver encountered an input-overrun
				 *+ condition internally on the indicated
				 *+ Xylogics 781 board and port.  The extraneous
				 *+ data has been lost and input restarted on
				 *+ that port in an attempt to recover.
				 */
				cmn_err(CE_WARN, 
				"xs%d: Input overrun - data lost, port %d.", 
					ip->boardnum, port);
				return;
			}
		} else {
			tp->timeoutid = 0;
		}
	} else {
		tp->bufcallid = 0;
		tp->timeoutid = 0;
		bcopy((caddr_t)&ip->inbufs.ibufs[port].data[0],
		      (caddr_t) mptr->b_wptr, cnt);
		mptr->b_wptr += cnt;
		if (canputnext(RD(tp->qptr))) {
			UNLOCK(tp->ttylock, ttypl);
			putnext(RD(tp->qptr), mptr);

			ttypl = LOCK(tp->ttylock, plstr);
			XS_RESTART_RD(ip, port, tp->flags, xsregs);
			UNLOCK(tp->ttylock, ttypl);
			(void)putctl(WR(tp->qptr), M_STARTI);
			return;
		}

		putq(RD(tp->qptr), mptr);
	}
	UNLOCK(tp->ttylock, ttypl);
}

/*
 * STATIC void
 * xs_intr(int)
 * 	Xylogics-781 driver interrupt service routine.
 *
 * Calling/Exit State:
 *	Has no process context and does not block.
 *
 *	Basic locks may not be held upon entry/exit.
 *
 * 	Depending on the basis of the interrupt, may
 *	queue incoming data upstream or release message
 *	buffers for outgoing requests.
 *
 *	No return value.
 *
 * Description:
 *	Verify that the specified interrupt vector corresponds to 
 * 	a functional xs-device; discard the interrupt otherwise.  
 *	The interrupt vector is translated into a 781 controller
 *	number, from which the correct controller state information
 *	structure is located.  From there, the boards control 
 *	structures are accessible.
 *
 *	The general flow of this function is:
 *
 *		1) Read the hardware control registers and flush 
 *		   the DMA controller's readahead buffer.
 *
 *		2) Process data received on all ports of the associated
 *		   board.  Note that for framing and parity errors, a 
 *		   read complete is dummied up for the port with the 
 *		   so all data prior to the error is passed along.
 *
 *		3) Process all ports for which their is a write 
 *		   operations that passed the ports' low-water mark.
 *		   Normally, this consists of queuing more data to
 *		   the 781 for output if there is some pending.
 *
 *		4) Check the controller status register and attempt
 *		   reasonable recovery from any reported exceptions
 *		   or error conditions.  
 *
 *		5) Lastly, clear the interrupt at the 781 board which
 *		   re-enables it for further processing.  Then return.
 *
 * Remarks:
 *	While this function loops attempting to determine which
 *	ports status is being reported for, the 781 generating
 *	this interrupt will not update the status and data buffers, 
 *	nor issue another interrupt, until the driver sets the
 *	Clear Register Status (CRS) bit in the control register.	
 *	Since this is the only place in the driver CRS is set,
 *	the driver does not need to lock ip->cntlr_lock here.
 *	Since many operations called from here, such as putnext(),
 *	require locks not be held, this is an essential interaction.
 *
 */
STATIC void
xs_intr(int vector)
{
	xsinfo_t *ip;
	int board = vector - xs_base_vector;
	ushort report, rcode;
	int port;
	xsregs_t xsregs;

	if ((unsigned) board >= xs_nconf 
	||  (ip = xs_info[board]) == NULL) {
		/*
		 *+ An interrupt was delivered to the xs-driver which
		 *+ appears to be for a deconfigured, non-functional
		 *+ unit.  The interrupt has been discarded.
		 */
		cmn_err(CE_NOTE, 
			"xs: stray interrupt, vector %d. board %d not alive.",
			vector, board);
		return;
	}

	/* Read the hardware control registers and flush the DMA controller */
	XS_LOAD(ip, xsregs);
	ssm_vme_dma_flush(ip->config->ssmno);
	report = xsregs.cmd_report;
	rcode = report & XSCR_RMASK;

	/*
	 * If this is a FRAME or PARITY error, setup a dummy 
	 * read complete for the port with the error.  If data
	 * really was returned with the parity error, assume it
	 * was the last byte of the data; xs_read_complete() will
	 * take it from there.
	 */
	if (((report & XSCR_RAC) == 0) &&
	    ((rcode == (ushort)XSCR_PARITY) ||
	    (rcode == (ushort)XSCR_FRAME))) {
		report |= XSCR_RAC;
		port = xsregs.port;
		XS_SET_RD_ACTIVE(xsregs, port);
	}

 	/* Check for read completions */
	if (report & XSCR_RAC) {
	        drv_setparm(SYSRINT, 1);        /* Keep statistics */
		for (port = 0; port < XS_MAXPORTS; port++) {
			if (XS_TST_RD_ACTIVE(xsregs, port)) {
				xs_read_complete(ip, port, rcode, &xsregs);
			}
		}
		xsregs.cmd_report = XSCR_RAC;	/* Restart reads */
	}

	/* 
	 * Next check for write operations that passed 
	 * the low-water mark of their write queue
	 */
	if ((report & (XSCR_WAC | XSCR_SYNC)) != 0) {
	        drv_setparm(SYSXINT, 1);        /* Keep statistics */
		for (port = 0; port < XS_MAXPORTS; port++) {
			if (XS_TST_WR_ACTIVE(xsregs, port)) {
				xs_write_complete(ip, port, report);
			}
		}
	}

	/*
	 * Now check for other reported errors.
	 */
	if (rcode) {
		xs_exc_complete(ip, xsregs.port, rcode);
	}

	if (xsregs.cmd_report == XSCR_RAC) {
		/*
		 * Clear write active bits.  781 looks at them
		 * even though the WAC bit is not set.
		 */
		xsregs.low_wr_active = 0;
		xsregs.hi_wr_active = 0;
		xs_cmd(ip, &xsregs);
	}
	ip->csr->cntl_status = XSCR_CRS;		/* Clear interrupt */
}

/*
 * STATIC void
 * xs_read_complete(xsinfo_t *, int, ushort, xsregs_t *)
 *	Data reception termination handling.
 *
 * Calling/Exit State:
 *      Basic locks must not be held upon entry/exit.
 *
 *      Input data from the 781 for this port is in the 
 *      board's input buffer upon entry
 *
 *      If a streams buffer is not available, input soliciation
 *	is suspended and a bufcall() to xs_recover() is scheduled.
 *	Otherwise the data is copied the streams buffer and queued
 *	upstream, if possible, or to this device's queue.
 *
 *      No return value.
 *	input buffer.
 *
 * Remarks:
 *      If the input buffer count has somehow been corrupted
 *      from the time the 781 interrupted us until now, discard
 *      the data.
 *
 *	If a parity error is encountered, but there is not space in
 *	the buffer for extra parity marking characters, then the
 *	marks will be forgotten and a NULL placed into the data 
 *	stream.
 */
STATIC void
xs_read_complete(xsinfo_t *ip, int port, ushort rcode, xsregs_t *regs)
{
	xstty_t *tp;
	uint_t cnt;
	pl_t ttypl;
	mblk_t	*mptr;
	char culprit;

	tp = &ip->tty[port];
	ttypl = LOCK(tp->ttylock, plstr);

	if ((tp->tty_state & XS_ISOPEN) == 0 
	||  (tp->tty_state & XS_WCLOSE) != 0 
	||  (tp->flags & XS_DISCARDIN)  != 0) {
		XS_CLR_RD_ACTIVE(*regs, port);
		UNLOCK(tp->ttylock, ttypl);
		return;		/* Ignore this input */
	}

	cnt = vtoss(ip->inbufs.ibufs[port].count);
	if (cnt > XS_IBUF_SZ) {
		UNLOCK(tp->ttylock, ttypl);
                /*
                 *+ The xs-driver has detected some an unexpected
                 *+ input block-size from the specified 781 board
                 *+ and port.  The data located within will be
                 *+ discarded.
                 */
		cmn_err(CE_WARN, "xs%d: illegal block input count %d port %d.",
			ip->boardnum, cnt, port);
		return;		
	}

	if ((rcode == (ushort)XSCR_PARITY || rcode == (ushort)XSCR_FRAME) 
	&&  port == regs->port) {
		/*
                 * If a parity error occurred, then determine if 
		 * anything needs to be passed upstream for it or 
		 * if it is to be ignored.  Otherwise, delete the
		 * offending byte from the data buffer so it is
		 * not passed upstream, including for framing errors.
                 */
		if (rcode == XSCR_PARITY && (tp->iflags & IGNPAR) == 0) {
                        if ((tp->iflags & PARMRK) != 0
			&& cnt <= XS_IBUF_SZ - 2) {
                                /*
                                 * Insert a parity mark along with the
                                 * culprit byte into the data stream.
                                 */
				if (cnt == 0) {
					cnt = 1;
					culprit = '\0';
				} else {
					culprit = ip->inbufs.ibufs[port].
							data[cnt - 1];
				}

				ip->inbufs.ibufs[port].data[cnt - 1] = '\377';
				ip->inbufs.ibufs[port].data[cnt]     = '\0';
				ip->inbufs.ibufs[port].data[cnt + 1] = culprit;
				cnt += 2;
                        } else {
                                /*
                                 * Just insert a NUL byte into the stream.
                                 */
				if (cnt == 0)
					cnt = 1;
				ip->inbufs.ibufs[port].data[cnt - 1] ='\0';
                        }
                } else if (cnt != 0) {
			cnt--;		/* Ignore the error byte */
		}
	}

	if (cnt == 0) {
		UNLOCK(tp->ttylock, ttypl);
		return;		/* No data to process */
	}

	drv_setparm(SYSRAWC, cnt);		/* Keep statistics */
	if ((mptr = allocb(cnt, BPRI_MED)) == NULL) {
		if (xs_global.printallocfail) {
			/*
			 *+ A driver attempt to allocate a
			 *+ stream buffer for incoming data
			 *+ has failed.
			 */
			cmn_err(CE_WARN, 
				"xs%d: port %d unable to allocb %d bytes.",
				ip->boardnum, port, cnt);
		}
		/* 
		 * bufcall() to wait for streams buffer.
		 * If that fails, try to schedule a itimeout()
		 * to perform a similar action.  If that fails
		 * as well, then we have little choice but to
		 * pitch the data and continue, since there is
		 * no apparent way to reschedule a callback to
		 * restart input later.
		 * If bufcall() succeeds, then also attempt to 
		 * control flow the channel to stop more input 
		 * from being received.  To do this, we must 
		 * send a message to our output service routine
		 * to perform control flow.  So, yes, we
		 * really do mean to talk to ourselves!
		 */
		XS_CLR_RD_ACTIVE(*regs, port);
		tp->flags |= XS_RDBLOCKED;
		if (tp->bufcallid == 0 && tp->timeoutid == 0) {
			tp->bufcallid = 
				bufcall(cnt, BPRI_MED, xsrecover, (long)tp);
			if (tp->bufcallid == 0) {
				tp->timeoutid = itimeout(xsrecover, (caddr_t)tp,
					drv_usectohz(XS_BUF_WAIT), plstr);
				if (tp->timeoutid == 0) {
					/*
					 *+ The xs-driver encountered an input-
					 *+ overrun condition internally on the 
					 *+ indicated Xylogics 781 board and 
					 *+ port.  The extraneous data has been 
					 *+ lost and input restarted on that 
					 *+ port in an attempt to recover.
					 */
					cmn_err(CE_WARN, 
					"xs%d: Input overrun - data lost, port %d.", 
						ip->boardnum, port);
					tp->flags &= ~XS_RDBLOCKED;
					XS_SET_RD_ACTIVE(*regs, port);
				}
			}
		}
		if ((tp->flags & XS_RDBLOCKED) != 0) {
			UNLOCK(tp->ttylock, ttypl);
			(void)putctl(WR(tp->qptr), M_STOPI);
		} else {
			UNLOCK(tp->ttylock, ttypl); /* Overrun recovery */
		}
	} else {
		/* 
		 * Copy the port's input into the streams buf 
		 * and put the message upstream if possible.  
		 * Otherwise, put the message on the local 
		 * queue, block interrupts on the correct port 
		 * and set a flag to indicate the we are 
		 * blocked.  The message on the local queue 
		 * will be processed by the service procedure, 
		 * xsrsrv().
		 */
		bcopy((caddr_t)&ip->inbufs.ibufs[port].data[0],
			(caddr_t)mptr->b_wptr, cnt);
		mptr->b_wptr += cnt;
		if (canputnext(RD(tp->qptr))) {
			UNLOCK(tp->ttylock, ttypl);
			putnext(RD(tp->qptr), mptr);
		} else {
			putq(RD(tp->qptr), mptr);
			XS_CLR_RD_ACTIVE(*regs, port);
			tp->flags |= XS_RDBLOCKED;
			UNLOCK(tp->ttylock, ttypl);
		 	/*
			 * Yes, we really do mean to talk to 
			 * ourselves here in order to control 
			 * flow the channel!
			 */
			(void)putctl(WR(tp->qptr), M_STOPI);
		}
	}
	return;
}

/*
 * STATIC void
 * xs_write_complete(xsinfo_t *, int, ushort_t)
 *	Transmit request termination handling.
 *
 * Calling/Exit State:
 *      The port's tty lock must not be held upon entry/exit.
 *
 *      Other basic locks may be held upon entry/exit.
 *
 *	May start service of the next message queued for ports
 *	that completed output, including deferred flow controls
 *	and ioctls.
 *
 *	No return value.
 *
 * Remarks:
 * 	The output buffer for a port that reports completion
 *	should not be full, but may be due to a 781 bug.
 *	If this occurs, just ignore this call and return.  
 *
 *	Port terminations when SYNC is set, the output buffer
 *	is now empty and we are waiting to either:
 *	
 * 		- an ioctl needs transmission of all data queued
 *		  to the port to be completed, afterwhich normal 
 *		  writes are reenabled and the SYNC condition is 
 *		  cleared.  The next queued message for should be 
 *		  the ioctl itself, or
 *
 * 		- a port is waiting for close.  All of the data is 
 *		  out of the write queue, but some may still be on 
 *		  the 781.  This SYNC interrupt indicates that all 
 *		  of the data has left the 781 buffers and the close
 *		  can be awakened to complete.
 */
STATIC void
xs_write_complete(xsinfo_t *ip, int port, ushort_t report)
{
	xstty_t *tp;
	pl_t ttypl;
	xsregs_t xsflow;

	tp = &ip->tty[port];
	ttypl = LOCK(tp->ttylock, plstr);

	if ((tp->tty_state & XS_ISOPEN) == 0 || XS_OUTBUF_FULL(ip, port)) {
		UNLOCK(tp->ttylock, ttypl);
		return;			/* Ignore this completion. */
	}

	/*
	 * If a flow control request was deferred, send it now.
	 */
	if (tp->flags & XS_FLOW) {
		tp->flags &= ~XS_FLOW;
		*(XS_PUT_ADDR(ip, port)) = tp->flow;
		if (xs_bufptr_move(ip, port, 1)) {
			XS_CLEAR(xsflow);
			XS_SET_WR_ACTIVE(xsflow, port);
			xsflow.cmd_report = XSCR_WAC;
			xs_cmd(ip, &xsflow);
		} else {
			/* 
			 * Set state to full if the one-byte flow control
			 * write happened to fill the output buffer.
			 */
			if (XS_OUTBUF_FULL(ip, port)) {
				tp->tty_state |= XS_TX_FULL;
				UNLOCK(tp->ttylock, ttypl);
				return;
			}
		}
	}

	tp->tty_state &= ~XS_TX_FULL; 	/* Port xfer buffer not full anymore */

	if (report & XSCR_SYNC) {
		/*
		 * The output buffer is empty; clear the output 
		 * buffer busy bit and the SYNC request flag.
		 */
		tp->tty_state &= ~XS_TX_BUSY;
		tp->flags &= ~XS_SYNCREQ;
		if (tp->tty_state & XS_WIOCTL) {
			tp->tty_state &= ~XS_WIOCTL; /* Start pending ioctl */
			qenable(tp->qptr);
		} else if (tp->tty_state & XS_WCLOSE) {
			tp->tty_state &= ~XS_WCLOSE;	
			SV_SIGNAL(tp->drainwait, 0);/* Awaken waiting close() */
		}
		UNLOCK(tp->ttylock, ttypl);
		return;
	}

	/*
	 * If we are still sending a data message
	 * out to the 781, call xs_next_msg() to put
	 * more data on the output queue.
	 */
	if (tp->msg != NULL) {
		xs_next_msg(tp, tp->msg);
	} else {
		/* 
		 * The queue must be reenabled because
		 * there had been a message put back 
		 * using noenable().
		 */
		qenable(tp->qptr);
	}

	/*
	 * If there is a close() or ioctl() waiting for
 	 * output to drain, then try to queue a SYNC request.
	 */
	if ((tp->tty_state & (XS_WCLOSE | XS_WIOCTL)) != 0 
	&&  (report & XSCR_SYNC) == 0) {
		(void)xs_drain(ip,port);
	}

	UNLOCK(tp->ttylock, ttypl);
}

/*
 * STATIC void
 * xs_exc_complete(xsinfo_t *, int, ushort_t)
 *	Interrupt handling of reported port execptions/errors.
 *
 * Calling/Exit State:
 *      No basic locks may be held upon entry/exit.
 *
 *	Upon entry rcode contains the reported status code.
 *	Also, port is the tty of the specified xsinfo_t the
 *	error is associated with, if its port specific.
 *
 *	No return value.
 *
 * Description:
 *	Many conditions handled here merely result in an
 *	error message being logged, since recovery actions
 *	their occurance is likely to be catostrophic to the
 * 	associated port(s).  However, framing errors and
 *	modem carrier transitions are propogated when appropriate.
 */
STATIC void
xs_exc_complete(xsinfo_t *ip, int port, ushort_t rcode)
{
	xstty_t *tp;
	pl_t ttypl;

        if (rcode == XSCR_INTQOVF) {
                /*
                 *+ The specified Xylogics 781 board encountered an internal 
		 *+ queue overflow condition.  This situation occurs only if 
		 *+ the host system does not respond to a 781 interrupt for 
		 *+ a long time.
                 */
                cmn_err(CE_WARN, 
			"xs%d: Internal Queue Overflow.", ip->boardnum);
		return;			/* Nothing further we can do here */
	}

	tp = &ip->tty[port];
	ttypl = LOCK(tp->ttylock, plstr);

	if ((tp->tty_state & XS_ISOPEN) == 0) {
		UNLOCK(tp->ttylock, ttypl);
		return;			/* Ignore this completion. */
	}

	switch (rcode) {
	case XSCR_PARITY:
		break; 		/* xs_read_complete() takes care of this.  */

	case XSCR_FRAME:	
		/* 
		 * Handle framing errors by attempting to
		 * send a M_BREAK message upstream.  
		 */
		UNLOCK(tp->ttylock, ttypl);
		if (!putnextctl(RD(tp->qptr), M_BREAK)) {
			/* 
			 *+ The xs-driver attempted to propagate
			 *+ a break on the specified board/port's
			 *+ stream, but failed for unknown reasons.
			 *+ The break has been discarded.
			 */
			cmn_err(CE_WARN, 
				"xs%d: break detect report failed, port %d.",
				ip->boardnum, port);
		}
		return;

	case XSCR_MODEM:
		/* 
		 * Always note when DCD is raised to awaken
		 * open's that may have been delayed.  But ignore
		 * DCD dropping if CLOCAL is set.  Otherwise when
		 * DCD drops send a hangup notification upstream if
		 * close() is occurring and always not the new line 
		 * state for DCD.
		 */
		drv_setparm(SYSRINT, 1);        /* Keep statistics */

		if (ip->idb.uart_parms[port].line_state & XSLINE_DCD) {
			tp->tty_state |= XS_CARR_ON;
			if (tp->tty_state & (XS_WOPEN | XS_WCLOSE)) {
				tp->tty_state &= ~XS_WCLOSE;
				SV_BROADCAST(tp->carrwait, 0);
			}
		} else if ((tp->cflags & CLOCAL) == 0) {
			tp->tty_state &= ~XS_CARR_ON;
			if (!(tp->tty_state & XS_WOPEN)) {
				UNLOCK(tp->ttylock, ttypl);
				if (!putnextctl(RD(tp->qptr), M_HANGUP)) {
					/* 
					 *+ The xs-driver has failed in its 
					 *+ attempt to report a carrier-drop 
					 *+ upstream for the specifed 781-board
					 *+ and port.
					 */ 
					cmn_err(CE_WARN, 
					"xs%d: M_HANGUP failure, port %d.", 
						ip->boardnum, port);
				}
				return;		/* All done */
			}
		}
		break;

	case XSCR_INOVRR:
		/*
		 *+ The specified Xylogics 781 board encountered 
		 *+ an input-overrun condition on the indicated
		 *+ port.  The condition is otherwise being ignored.
		 */
		cmn_err(CE_WARN, 
			"xs%d: Input overrun, port %d.", ip->boardnum, port);
			break;

	case XSCR_ACCERR:
		/*
		 *+ The specified Xylogics 781 board encountered 
		 *+ an I/O bus error while accessing data for the
		 *+ indicated port.  The condition is otherwise 
		 *+ being ignored by the xs-driver.
		 */
		cmn_err(CE_WARN, 
			"xs%d: 781 VME data access error, port %d.", 
			ip->boardnum, port);
			break;

	case XSCR_ACCTIME:
		/*
		 *+ The specified Xylogics 781 board encountered 
		 *+ an I/O bus timeout error while accessing data 
		 *+ for the indicated port.  The condition is otherwise 
		 *+ being ignored by the xs-driver.
		 */
		cmn_err(CE_WARN, 
			"xs%d: 781 VME data access timeout error, port %d.", 
			ip->boardnum, port);
			break;

	case XSCR_UARTOVF:
		if (xs_global.printoverflow) {
			/*
			 *+ The specified Xylogics 781 board encountered 
			 *+ an uart-overflow condition for the indicated port.  
			 *+ The condition is otherwise being ignored by the 
			 *+ xs-driver.
			 */
			cmn_err(CE_WARN, 
				"xs%d: 781 octart overflow, port %d.", 
				ip->boardnum, port);
		}
		break;

	case XSCR_CMDERR:
		/*
		 *+ The specified Xylogics 781 board reported 
		 *+ an command-error condition for the indicated port.  
		 *+ The condition is otherwise being ignored by the 
		 *+ xs-driver.
		 */
		cmn_err(CE_WARN, "xs%d: Command error, port %d.",
			ip->boardnum, port);
		break;

	default:
		/*
		 *+ The specified Xylogics 781 board reported 
		 *+ an unknown condition code to the xs-driver, 
		 *+ which is also displayed.  The condition is 
		 *+ otherwise being ignored by the xs-driver.
		 */
		cmn_err(CE_WARN, "xs%d: unexpected report code 0x%x\n", 
			ip->boardnum, rcode);
		break;
	}

	UNLOCK(tp->ttylock, ttypl);
}

/*
 * STATIC int
 * xsrsrv(queue_t *)
 * 	Read service procedure for queued messages.
 *
 * Calling/Exit State:
 *	Does not block.
 *
 *	The controller lock for the specified port and the
 *	port's tty lock must not be held upon entry/exit.
 *
 *	Other basic locks may be held upon entry/exit.
 *
 *	Returns zero although it is expected to be ignored.
 *
 * Description:
 *	Takes messages off the queue and attempts to pass them
 *	up the stream.  If it cannot, then it requeues the request
 *	and exits.  It really expects to find only one message
 *	queued, but is prepared for more just in case.
 *
 *	Once the queue is emptied, and since it likely was invoked
 *	after messages were blocked from being passed up the stream, 
 *	this function must also restart suspended solicitation of 
 *	new input from the 781 controller for the port.
 */
STATIC int
xsrsrv(queue_t *rd_q)
{
	pl_t ttypl;
	mblk_t *bp;
	xstty_t *tp;
	xsregs_t xsregs;
	int port;

	ASSERT(rd_q->q_ptr != NULL);

	while ((bp = getq(rd_q)) != NULL) {
		if (canputnext(rd_q)) {
			putnext(rd_q, bp);
		} else {
			putbq(rd_q, bp);
			return (0);
		}
	}

	tp = (xstty_t *)rd_q->q_ptr;
	ttypl = LOCK(tp->ttylock, plstr);
	if (tp->flags & XS_RDBLOCKED) {
		port = XSPORT(tp->devno);
		XS_RESTART_RD(xs_info[XSBOARD(tp->devno)], 
				port, tp->flags, xsregs);
		UNLOCK(tp->ttylock, ttypl);
		(void)putctl(WR(tp->qptr), M_STARTI);
	} else {
		UNLOCK(tp->ttylock, ttypl);
	}
	return (0);
}

/*
 * STATIC int
 * xs_doioctl(xstty_t *, mblk_t *)
 *	Process the specified M_IOCTL message.
 *
 * Calling/Exit State:
 *	Does not block.
 *
 *	The port's ttylock must not be held upon entry or exit.
 *
 *	Returns one if the port currently is busy and the command
 *	could not be executed at this time, zero otherwise.
 *	
 * Description:
 * 	Process the message by calling xs_param() to set up hardware
 * 	port parameters.  Acknowlege the message, setting its completion 
 *	status accordingly, and send it back upstream with qreply().  
 *	If we do not recognize the request, set its completion status 
 *	to M_IOCNAK.
 *
 * Remarks:
 *	In the code sequence which follows, the first switch-statement
 *	is meant as an optimization of common code for the cases
 *	it encapsulates.  The second switch-statement performs their
 *	different semantics once the tty is locked.
 */
STATIC int
xs_doioctl(xstty_t *tp, mblk_t *mp)
{
	queue_t *wr_q = (queue_t *) tp->qptr;
	int port = XSPORT(tp->devno);
	int board = XSBOARD(tp->devno);
	xsinfo_t *ip = xs_info[board];
	struct iocblk *iocp;
	struct termio *tptr;
	struct termios *tsptr;
	mblk_t *bp;
	xsregs_t xsregs;
	tcflag_t oldcflags;
	pl_t ttypl;

	/* 
	 * Validate the parameter block arguments.
	 */
	if (mp->b_wptr - mp->b_rptr < sizeof (struct iocblk)) {
		/*
		 *+ The xs-driver put service routine received a stream
		 *+ message of type M_IOCTL or M_PCIOCTL, but the message did
		 *+ not include a data block containing subcommands and
		 *+ parameters.  
		 */
		cmn_err(CE_WARN, 
		"xs%d: IOCTL message received w/ NULL data ptr, port %d.",
			board, port);
		mp->b_datap->db_type = M_IOCNAK;
		qreply(wr_q, mp);
		return (0);
	}
	/*LINTED*/
	iocp = (struct iocblk *) mp->b_rptr;

	switch (iocp->ioc_cmd) {
	case TCSETA:
	case TCSETAF:
	case TCSETAW:
	case TCSETS:
	case TCSETSF:
	case TCSETSW:
	case TCSBRK:
		if (mp->b_cont == NULL) {
			/*
			 *+ The xs-driver put service routine received a 
			 *+ stream message of type M_IOCTL or M_PCIOCTL, 
			 *+ but the message did not include a continuation 
			 *+ block for its parameters.  
			 */
			cmn_err(CE_WARN, 
			"xs%d: IOCTL message received w/ NULL continuation ptr, port %d.",
				board, port);
			mp->b_datap->db_type = M_IOCNAK;
			qreply(wr_q, mp);
			return (0);
		}
		break;

	case TCGETA:
	case TCGETS:
	default:
		/* mp->b_cont doesn't need to be set in these cases */
		break;
	}

	/*
	 * Now perform the specified ioctl.
	 */
	ttypl = LOCK(tp->ttylock, plstr);

	switch (iocp->ioc_cmd) {
	case TCSETAF:			/* Set params w/ read flush (termio) */
	case TCSETAW:			/* Set params w/o flush (termio) */
		/*
		 * Wait for all current output to
		 * drain before setting port params.
		 */
		if (xs_drain(ip, port)) {
			tp->tty_state |= XS_WIOCTL;
			UNLOCK(tp->ttylock, ttypl);
			return(1);
		}
		/*FALLTHROUGH*/

	case TCSETA:			/* Set parameters (termio) */
		XS_CLEAR(xsregs);
		/*LINTED*/
		tptr = (struct termio *) mp->b_cont->b_rptr;
		oldcflags = tp->cflags;			/* Save old flags */
		tp->cflags = (tp->cflags & 0xffff0000) | tptr->c_cflag;
		iocp->ioc_error = 
			xs_param(&ip->idb.uart_parms[port], tp);

		if (iocp->ioc_error != 0) {
			tp->cflags = oldcflags;		/* Restore old flags */
			mp->b_datap->db_type = M_IOCNAK;
		} else {
			mp->b_datap->db_type = M_IOCACK;
			tp->iflags = (tp->iflags & 0xffff0000) | tptr->c_iflag;
			if (tp->cflags & CLOCAL)
				tp->tty_state |= XS_CARR_ON;

			xsregs.cmd_report = XSCR_UINIT;
			xsregs.port = (uchar_t)port;
			xs_cmd(ip, &xsregs);
		}
	
		if (iocp->ioc_cmd == TCSETAF) {
			/* Flush ouput for this port at the board */
			xsregs.cmd_report = XSCR_FLUSH;	
			xsregs.port = (uchar_t)port;
			xs_cmd(ip, &xsregs);

			UNLOCK(tp->ttylock, ttypl);
			if (putnextctl1(RD(wr_q), M_FLUSH, FLUSHR) == 0) {
				/*
				 *+ The xs-driver attempt to send a read-flush 
				 *+ message upstream after processing a TCSETAF 
				 *+ message has failed for the specified 781-
				 *+ board/port.
				 */
				cmn_err(CE_WARN, 
				"xs%d: TCSETAF flush msg failed, port %d.", 
					board, port);
			}
			ttypl = LOCK(tp->ttylock, plstr);
		}
		iocp->ioc_count = 0;
		break;

	case TCSETSF:			/* Set params w/ read flush (termios) */
	case TCSETSW:			/* Set params w/o flush (termios) */
		/*
		 * Wait for all current output to
		 * drain before setting port params.
		 */
		if (xs_drain(ip, port)) {
			tp->tty_state |= XS_WIOCTL;
			UNLOCK(tp->ttylock, ttypl);
			return(1);
		}
		/*FALLTHROUGH*/

	case TCSETS:			/* Set parameters (termios) */
		XS_CLEAR(xsregs);
		/*LINTED*/
		tsptr = (struct termios *) mp->b_cont->b_rptr;
		oldcflags = tp->cflags;			/* Save old flags */
		tp->cflags = tsptr->c_cflag;
		iocp->ioc_error = 
			xs_param(&ip->idb.uart_parms[port], tp);

		if (iocp->ioc_error != 0) {
			tp->cflags = oldcflags;		/* Restore old flags */
			mp->b_datap->db_type = M_IOCNAK;
		} else {
			mp->b_datap->db_type = M_IOCACK;
			tp->iflags = tsptr->c_iflag;
			tp->cstart = tsptr->c_cc[VSTART];
			tp->cstop = tsptr->c_cc[VSTOP];
			if (tp->cflags & CLOCAL)
				tp->tty_state |= XS_CARR_ON;

			xsregs.cmd_report = XSCR_UINIT;
			xsregs.port = (uchar_t)port;
			xs_cmd(ip, &xsregs);
		}
	
		if (iocp->ioc_cmd == TCSETSF) {
			/* Flush ouput for this port at the board */
			xsregs.cmd_report = XSCR_FLUSH;	
			xsregs.port = (uchar_t)port;
			xs_cmd(ip, &xsregs);

			UNLOCK(tp->ttylock, ttypl);
			if (putnextctl1(RD(wr_q), M_FLUSH, FLUSHR) == 0) {
				/*
				 *+ The xs-driver attempt to send a read-flush 
				 *+ message upstream, after processing a TCSETSF
				 *+ message, has failed for the specified 781-
				 *+ board/port.
				 */
				cmn_err(CE_WARN, 
				"xs%d: TCSETSF flush msg failed, port %d.", 
					board, port);
			}
			ttypl = LOCK(tp->ttylock, plstr);
		}
		iocp->ioc_count = 0;
		break;

	case TCSBRK:
		/*
		 * Send break but wait until all current output
		 * is drained before doing anything.
		 */
		if (xs_drain(ip, port)) {
			tp->tty_state |= XS_WIOCTL;
			UNLOCK(tp->ttylock, ttypl);
			return (1);
		}

		/* LINTED */
		if (*(int *) mp->b_cont->b_rptr == 0) {
			tp->breakid = itimeout(xs_breaktime, (caddr_t) tp,
				(xs_global.brktime * HZ) / 1000, plstr);
			/* If itimeout() fails, forget the break... */
			if (tp->breakid != 0) {
				XS_CLEAR(xsregs);
				tp->tty_state |= XS_TXBREAK;
				xsregs.cmd_report = XSCR_BREAKON;
				xsregs.port = (uchar_t)port;
				xs_cmd(ip, &xsregs);
			}
		}
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_count = 0;
		break;

	case TCGETA:			/* Get parameters (termio version) */
		if (mp->b_cont == NULL) {
			bp = allocb(sizeof (struct termio), BPRI_MED);
			if (bp == NULL) {
				/*
				 *+ An attempt to allocate a streams data 
				 *+ buffer for TCGETA parameters has failed.
				 */
				cmn_err(CE_WARN, 
					"xs%d: TCGETA allocb failed, port %d.",
					board, port);
				mp->b_datap->db_type = M_IOCNAK;
				break;
			}
			/* clear unset fields, then link the data buffer */
			bzero((caddr_t)bp->b_rptr, sizeof (struct termio));
			mp->b_cont = bp;
		} else if (mp->b_cont->b_datap->db_lim - 
			   mp->b_cont->b_datap->db_base < 
			   sizeof (struct termio)) {
			/*
			 *+ A TCGETA ioctl with a preallocated buffer
			 *+ does not have enough allocated space to
			 *+ store the requested information.
			 */
			cmn_err(CE_WARN, 
				"xs%d: TCGETA bad data block size, port %d.",
				board, port);
			mp->b_datap->db_type = M_IOCNAK;
			break;
		} else {
			mp->b_cont->b_rptr = mp->b_cont->b_datap->db_base;
			mp->b_cont->b_wptr = mp->b_cont->b_rptr;
		}
		/*LINTED*/
		tptr = (struct termio *) mp->b_cont->b_wptr;
		tptr->c_cflag = (ushort)tp->cflags;	/* Load cflags */
		tptr->c_iflag = (ushort)tp->iflags;	/* Load iflags */
		mp->b_cont->b_wptr += sizeof (struct termio);
		iocp->ioc_count = sizeof (struct termio);
		mp->b_datap->db_type = M_IOCACK;
		break;

	case TCGETS:			/* Get parameters (termios version) */
		if (mp->b_cont == NULL) {
			bp = allocb(sizeof (struct termios), BPRI_MED);
			if (bp == NULL) {
				/*
				 *+ An attempt to allocate a streams data 
				 *+ buffer for TCGETS parameters has failed.
				 */
				cmn_err(CE_WARN, 
					"xs%d: TCGETS allocb failed, port %d.",
					board, port);
				mp->b_datap->db_type = M_IOCNAK;
				break;
			}
			/* clear unset fields, then link the data buffer */
			bzero((caddr_t)bp->b_rptr, sizeof (struct termios));
			mp->b_cont = bp;
		} else if (mp->b_cont->b_datap->db_lim - 
			   mp->b_cont->b_datap->db_base < 
			   sizeof (struct termios)) {
			/*
			 *+ A TCGETS ioctl with a preallocated buffer
			 *+ does not have enough allocated space to
			 *+ store the requested information.
			 */
			cmn_err(CE_WARN, 
				"xs%d: TCGETS bad data block size, port %d.",
				board, port);
			mp->b_datap->db_type = M_IOCNAK;
			break;
		} else {
			mp->b_cont->b_rptr = mp->b_cont->b_datap->db_base;
			mp->b_cont->b_wptr = mp->b_cont->b_rptr;
		}
		/*LINTED*/
		tsptr = (struct termios *) mp->b_cont->b_wptr;
		tsptr->c_cflag = tp->cflags;
		tsptr->c_iflag = tp->iflags;
		tsptr->c_cc[VSTART] = tp->cstart;
		tsptr->c_cc[VSTOP] = tp->cstop;
		mp->b_cont->b_wptr += sizeof (struct termios);
		iocp->ioc_count = sizeof (struct termios);
		mp->b_datap->db_type = M_IOCACK;
		break;

	default:
		/* nak the ioctl */
		mp->b_datap->db_type = M_IOCNAK;
	}

	UNLOCK(tp->ttylock, ttypl);
	qreply(wr_q, mp);
	return(0);
}

/*
 * STATIC int
 * xsmsg(queue_t *, mblk_t *)
 * 	write-side stream message handler.
 *
 * Calling/Exit State:
 *	Does not block.
 *
 *	The port's ttylock must not be held upon entry or exit.
 *
 *	Possible return values:
 *
 *	XS_MSGDONE 	the message completed successfully and has 
 *			been release or acknowledged accordingly.
 *
 *	XS_STDPUTBACK	the message could not be executed. Return it
 *			to the queue and reschedule its execution.
 *
 *	XS_PRIPUTBACK	a control flow stop message could not be 
 *			executed.  Return it to the head of the
 *			queue, but be aware that an external event
 *			is required to restart its execution; do
 *			not reschedule it right now.
 *
 * Remarks:
 *  	If the message heading downstream is of type M_FLUSH,
 *  	flush the appropriate queues (read or write.)  If the
 *	flush message is NOT for the upstream path (read flush),
 *	it is released.  Otherwise, the message is sent upstream.
 * 
 *	xsmsg() may be called when a 781 port's output side
 *	is busy.  It is up to the code to decide whether a message
 *	can be processed if the output side is busy.  
 */
STATIC int
xsmsg(queue_t *wr_q, mblk_t *mp)
{
	xstty_t *tp = (xstty_t *) wr_q->q_ptr;
	int port = XSPORT(tp->devno);
	int board = XSBOARD(tp->devno);
	xsinfo_t *ip;
	xsregs_t xsregs;
	pl_t ttypl;
	cc_t flow_char;

	ASSERT((unsigned) board < xs_nconf);
	ASSERT(xs_info[board] != NULL);

	switch (mp->b_datap->db_type) {
	case M_STOP:		
	case M_START:
	case M_STOPI:
	case M_STARTI:
		/*
		 * Lock the port info struct and determine
		 * if we are sending a break out of this
		 * port.  If so, wait until it is done before 
		 * doing anything else on this port.
		 */
		ttypl = LOCK(tp->ttylock, plstr);
		if (tp->tty_state & XS_TXBREAK) {
			UNLOCK(tp->ttylock, ttypl);
			return(XS_PRIPUTBACK);
		}

		XS_CLEAR(xsregs);
		ip = xs_info[board];

		/* Handle the specific flow control message type */
		switch (mp->b_datap->db_type) {
		case M_STOP:		/* Suspend data to device */
			tp->tty_state |= XS_SUSPOUT;
			xsregs.cmd_report = XSCR_XOFF;
			xsregs.port = (uchar_t)port;
			xs_cmd(ip, &xsregs);
			break;

		case M_START:		/* Unsuspend data to device */
			if (tp->tty_state & XS_SUSPOUT) {
				tp->tty_state &= ~XS_SUSPOUT;
				xsregs.cmd_report = XSCR_XON;
				xsregs.port = (uchar_t)port;
				xs_cmd(ip, &xsregs);
			}
			break;

		case M_STOPI:		/* Suspend data from device */
		case M_STARTI:		/* Unsuspend data from device */
			if ((tp->iflags & IXOFF) != 0) {
				/*
				 * If the output buffer is in a full state 
				 * (buffer was filled and we have not yet 
				 * received a write interrupt from the 781), 
				 * save the flow control character and set the 
				 * XS_FLOW flag so the interrupt routine will 
				 * transmit the character when there is space 
				 * in the output buffer.  This is done rather 
			 	 * than putting the message back on the queue 
				 * to prevent deadlock.
				 */
				flow_char = (mp->b_datap->db_type == M_STOPI) 
					? tp->cstop : tp->cstart;

				if ((tp->tty_state & XS_TX_FULL) 
				||  (tp->flags & XS_SYNCREQ)) {
					tp->flags |= XS_FLOW;
					tp->flow = flow_char;
					break;
				}

				tp->flags &= ~XS_FLOW;
				*(XS_PUT_ADDR(ip, port)) = flow_char;
				ssm_vme_dma_flush(ip->config->ssmno);

				if (xs_bufptr_move(ip, port, 1)) {
					XS_SET_WR_ACTIVE(xsregs, port);
					xsregs.cmd_report = XSCR_WAC;
					xs_cmd(ip, &xsregs);
				} else {
					/*
					 * Set full state if the one-byte flow 
					 * control write happened to fill the 
					 * output buffer.
					 */
					ssm_vme_dma_flush(ip->config->ssmno);
					if (XS_OUTBUF_FULL(ip, port)) {
						tp->tty_state |= XS_TX_FULL;
					}
				}
			}
			break;
		}

		/* Complete processing of M_STOP/M_START/M_STOPI/M_STARTI */
		UNLOCK(tp->ttylock, ttypl);
		freemsg(mp);
		break;	

	case M_FLUSH:		
	case M_IOCTL:
		/*
		 * Lock the port info struct and determine
		 * if we are sending a break out of this
		 * port.  If so, wait until it is done before 
		 * doing anything else on this port.
		 */
		ttypl = LOCK(tp->ttylock, plstr);
		if (tp->tty_state & XS_TXBREAK) {
			UNLOCK(tp->ttylock, ttypl);
			return(XS_STDPUTBACK);
		}
		UNLOCK(tp->ttylock, ttypl);

		/* Otherwise, handle the specific message type */
		switch (mp->b_datap->db_type) {
		case M_IOCTL:
			if (xs_doioctl(tp, mp) != 0)
				return(XS_STDPUTBACK);
			break;

		case M_FLUSH:			/* Flush data queue(s) */
			XS_CLEAR(xsregs);
			ip = xs_info[board];

			if (*mp->b_rptr & FLUSHW) {
				ttypl = LOCK(tp->ttylock, plstr);
				flushq(wr_q, FLUSHDATA);

				/* tell 781 to stop output and flush */
				xsregs.cmd_report = XSCR_RESET;
				xsregs.port = (uchar_t)port;
				xs_cmd(ip, &xsregs);
				(void) xs_wait(ip, XSSR_ARCP);
				ip->outbufs.obufs[port].get = stovs(1);
				ip->outbufs.obufs[port].put = 0;
				tp->tty_state &= ~(XS_TX_FULL | XS_DATASUSP);
				if (tp->msg) {
					freemsg(tp->msg);
					tp->msg = NULL;
				}
				UNLOCK(tp->ttylock, ttypl);
			}
			if (*mp->b_rptr & FLUSHR) {
				ttypl = LOCK(tp->ttylock, plstr);
				flushq(RD(wr_q), FLUSHDATA);

				/* tell 781 to discard any buffered data*/
				xsregs.cmd_report = XSCR_FLUSH;
				xsregs.port = (uchar_t)port;
				xs_cmd(ip, &xsregs);
	
				UNLOCK(tp->ttylock, ttypl);
				*mp->b_rptr &= ~FLUSHW;
				qreply(wr_q, mp);
			} else {
				freemsg(mp);
			}
			break;
		}
		break;	

	case M_DELAY:				/* Delay output for n ticks */
	case M_DATA:				/* Std. data or ioctl msgs */
		ttypl = LOCK(tp->ttylock, plstr);
		if ((tp->tty_state & (XS_TXBREAK | XS_TX_FULL | XS_TXDELAY |
				XS_SUSPOUT | XS_WIOCTL)) == 0) {
			if (mp->b_datap->db_type == M_DATA)
				tp->tty_state &= ~XS_DATASUSP;
			xs_next_msg(tp, mp);
			UNLOCK(tp->ttylock, ttypl);
		} else {
			if (mp->b_datap->db_type == M_DATA)
				tp->tty_state |= XS_DATASUSP;
			UNLOCK(tp->ttylock, ttypl);
			return(XS_STDPUTBACK);
		}
		break;

	default:
		/* Just release these messages */
		freemsg(mp);
		break;
	}

	return(XS_MSGDONE);
}

/*
 * STATIC int
 * xswsrv(queue_t *)
 * 	Write service procedure for queued messages.
 *
 * Calling/Exit State:
 *	Does not block.
 *
 *	The port's ttylock must not be held upon entry or exit.
 *
 *	Returns zero althought it is expected to be ignored.
 *
 * Description:
 *	Takes messages off the queue and invokes xsmsg() to process 
 *	them.  If it cannot complete the message at this time, it 
 *	requeues the request, scheduling it for later execution,
 *	and exits.
 */
STATIC int 
xswsrv(queue_t *wr_q)
{
	mblk_t *bp, *mb;
	pl_t ttypl;

	ASSERT(wr_q->q_ptr != NULL);

	while ((bp = getq(wr_q)) != NULL) {
		switch (xsmsg(wr_q, bp)) {
		case XS_STDPUTBACK:
			putbq(wr_q, bp);
			return (0);
		case XS_PRIPUTBACK:
			noenable(wr_q);
			ttypl = freezestr(wr_q);
			strqget(wr_q, QFIRST, QNORM, (long *)&mb);
			insq(wr_q, mb, bp);
			unfreezestr(wr_q, ttypl);
			enableok(wr_q);
			return (0);
		case XS_MSGDONE:
			break;
#ifdef DEBUG
		default:
			/*
			 *+ An internal driver error has occurred;
			 *+ the value returned from one internal
			 *+ function was unexpected by its caller.
			 */
			cmn_err(CE_PANIC,"xswsrv: unexpected xsmsg() return");
			/*NOTREACHED*/
#endif
		}
 	}
}

/*
 * STATIC int
 * xs_param(xs_uparm_t *, xstty_t *)
 *	Interpret cflags and load modem structure w/ corresponding settings.
 *
 * Calling/Exit State:
 *	The port's ttylock must be held upon entry/exit.
 *
 *	Other basic locks may be held upon entry and exit.
 *
 *	Returns EINVAL if baud rate not supported, zero otherwise.
 *
 * Description:
 *      Translates the current settings withing tp->cflags  
 *	and globally defined 781-timeouts into a format that
 *      to the 781, then store them in the 'modem' structure.
 *	When finally transmitted to the 781 for a particular
 *	port, the current baud rate, character size, stop bit 
 *	mask, parity, reciever character delays will be reset.
 */
STATIC int
xs_param(xs_uparm_t *modem, xstty_t *tp)
{
	tcflag_t cflags = tp->cflags;
	int speed = xs_mrates[(cflags & CBAUD)];
	ushort par, len, stopb, itime, imin;

	if (speed == XSBNOSUPP) {
		/*
		 *+ Xylogics 781 mux boards do not support EXTB/38,400,
		 *+ 134.5, or 200 baud.
		 */
		cmn_err(CE_NOTE, 
			"xs%d: illegal baud rate requested, port %d.", 
			XSBOARD(tp->devno), XSPORT(tp->devno));
		return (EINVAL);
	}

	if (speed == XSBHANGUP) {	
		modem->line_state &= ~XSLINE_DTR;	/* Hang up the line */
		return(0);
	}

	switch (cflags & CSIZE) {
	case CS5:
		len = XS_BITS5;
		break;
	case CS6:
		len = XS_BITS6;
		break;
	case CS7:
		len = XS_BITS7;
		break;
	default:
		len = XS_BITS8;
		break;
	}

	stopb = (cflags & CSTOPB) ? XS_STOP2 : XS_STOP1;

	if (cflags & PARENB) {		
		par = (cflags & PARODD) ? XS_ODD_PARITY : XS_EVEN_PARITY;
	} else {
		par = XS_NO_PARITY;
	}

	itime = (tp->inputtime + 4) / 5U;	/* Convert to 5ms granularity */
	if (itime == 0 || itime > XS_MAXINPUTTIME) {
		tp->inputtime = xs_global.rxtime * 5;
		itime = xs_global.rxtime;
	}

	imin = tp->inputmin;
	if (imin == 0 || imin > XS_MAXINPUTMIN) {
		tp->inputmin = xs_global.ibufsize;
		imin = xs_global.ibufsize;
	}

	modem->serial_modes = par | len | stopb | XS_HW_FLOW;
	modem->baud_rates = speed << 12 | speed << 8;
	modem->input_timeout = stovs(itime);
	modem->input_min = stovs(imin);
	modem->xoff_char = 0;
	modem->xon_char  = 0;
	modem->low_water = stovs(xs_global.lowwater);
	modem->break_time = stovs(XS_BRKTICKS(xs_global.brktime));

	return(0);
}

/*
 * STATIC void
 * xs_next_msg(xstty_t *, mblk_t *)
 * 	Send characters to a serial port or delay its output.
 *
 * Calling/Exit State:
 *	The port's ttylock must be held and its output
 *	must be inactive upon entry.
 *
 *	Other basic locks may be held upon entry/exit.
 *
 *	No return value.
 *	
 * Description:
 *	If the message is of type M_DELAY then the data must
 *	be a struct specifying the delay period.  Locate it,
 *	then with output stopped, schedule a call into xs_delaytime()
 *	to restart output after the specified time period.
 *
 *	For messages of type M_DATA, try to put as much data as 
 *	possible on the specified 781 port's output queue.  Compute 
 *	the amount of free space in the output queue (allowing wrap).  
 * 	Then attempt to fill that number of free spaces.  
 *
 *	If the port's output buffer might still be draining while 
 *	we are attempting to fill its remaining space.  This will be
 * 	managed by taking a "snapshot" of the buffer at this time to 
 *	come up with a number of characters that will fit and start
 *	filling those in.  If it goes empty during this time, note it
 *	since we will be checking for empty several times during the
 *	fill process.
 *
 * Remarks:
 *	tp->tty_state will have its XS_TX_FULL flag set if the 
 *	output queue is completely full.  This flag will not be 
 *	set if the message passed to this function will fit into 
 *	the output queue with space remaining.
 */
STATIC void
xs_next_msg(xstty_t *tp, mblk_t *bp)
{
	xsinfo_t *ip = xs_info[ XSBOARD(tp->devno) ];
	xsregs_t xsregs;
	int count, nch, xfercnt;
	uchar_t newdata, empty, moremsgs;
	queue_t *wr_q = tp->qptr;
	int port = XSPORT(tp->devno);
	mblk_t *mp;

	switch (bp->b_datap->db_type) {

	case M_DELAY:
		ASSERT(bp->b_rptr != NULL);
		ASSERT(tp->delayid == 0);
		tp->delayid = itimeout(xs_delaytime, (caddr_t) tp,
					/*LINTED*/
					*(int *) bp->b_rptr, plstr);
		/* If itimeout() fails, forget the delay... */
		if (tp->delayid != 0) {
			tp->tty_state |= XS_TXDELAY;
		}
		freemsg(bp);
		return;

	case M_DATA:

		newdata = FALSE;			/* init state */
		empty = FALSE;
		moremsgs = TRUE;

		while ((count = xs_contig_space(ip, port)) > 0 && moremsgs) {
			while (count) {
				nch = bp->b_wptr - bp->b_rptr;
				while (nch == 0) {	
					/* If msg block empty, get next one */
					mp = bp;
					bp = mp->b_cont;
					mp->b_cont = NULL;
					freeb(mp);
					if (bp == NULL) {
						moremsgs = FALSE;
						count = 0;
						break;
					}
					nch = bp->b_wptr - bp->b_rptr;
				}
				if (count > 0) {
					xfercnt = min((uint) count, (uint) nch);
					newdata = TRUE;
					bcopy((caddr_t) bp->b_rptr,
						(caddr_t)XS_PUT_ADDR(ip, port),
						(unsigned) xfercnt);
					ssm_vme_dma_flush(ip->config->ssmno);
					if (xs_bufptr_move(ip, port, xfercnt))
						empty = 1;
					bp->b_rptr += xfercnt;
					count -= xfercnt;
				}
			}
		}

		/* We have loaded the outbuf.  If bp is NULL, we have fit the
		 * entire message into the outbuf with space left.  If bp is
		 * not null, and data is left in the message, the outbuf must
		 * be full.  Set the busy flag and point tp->msg to the
		 * message for later loading.  If bp is not null and no data
		 * is left * in the message, the message just filled the
		 * outbuf.  Set tp->msg to null.
		 */
		if (bp == NULL) {
			tp->tty_state &= ~XS_TX_FULL;
			tp->msg = NULL;
			qenable(wr_q);
		} else {
			tp->tty_state |= XS_TX_FULL;
			if ((bp->b_wptr - bp->b_rptr) == 0) {
				if (bp->b_cont != NULL) {
					mp = bp->b_cont;
					bp->b_cont = NULL;
					freeb(bp);
					tp->msg = mp;
				} else {
					freeb(bp);
					tp->msg = NULL;
				}
			} else
				tp->msg = bp;
		}

		/*
		 * If the output buffer was previously empty
		 * and we added more data, tell the 781 about it.
	 	 */
		if (newdata && empty) {
			tp->tty_state |= XS_TX_BUSY;
			XS_CLEAR(xsregs);
			xsregs.cmd_report = XSCR_WAC;
			XS_SET_WR_ACTIVE(xsregs, port);
			xs_cmd(ip, &xsregs);
		}
	}
}

/*
 * STATIC void
 * xs_cmd(xsinfo_t *, xsregs_t *)
 * 	Send a command to the specified 781 board.
 *
 * Calling/Exit State:
 *	The board's cntlr_lock must not be held upon entry/exit.
 *
 *	Other basic locks may be held upon entry/exit.
 *
 *	No return value.
 *	
 * Description:
 * 	Wait for ARCP bit in 781 status register to clear.  
 *	Then move the specified in-core copy of the desired 
 *	register settings to the hardware registers, and 
 *	setting the the ARC bit to interrupt the 781 once 
 *	everything is loaded.  Do not wait for command completion.
 *
 * Remarks:
 *	Until the ARCP bit is cleared, this command will not 
 *	be latched into the 781.
 */
STATIC void
xs_cmd(xsinfo_t *ip, xsregs_t *xsregs)
{
	pl_t cplock = LOCK(ip->cntlr_lock, plstr);

	if (xs_wait(ip, XSSR_ARCP)) {
		 UNLOCK(ip->cntlr_lock, cplock);
		/*
		 *+ The specified Xylogics 781 board failed to signal, 
		 *+ within a reasonable amount of time, that it was 
		 *+ ready to accept a new command.  Its possible that
		 *+ value of xs_global.arcptime in the xs-driver's 
		 *+ Space.c file is defined to small; try increasing it.
		 */
		cmn_err(CE_WARN, 
			"xs%d: 781 timeout waiting for ARCP to clear.",
			ip->boardnum);
		return;
	}

	ip->csr->low_rd_active = xsregs->low_rd_active; /* Load the CSR */
	ip->csr->hi_rd_active  = xsregs->hi_rd_active;
	ip->csr->low_wr_active = xsregs->low_wr_active;
	ip->csr->hi_wr_active  = xsregs->hi_wr_active;
	ip->csr->cmd_report    = xsregs->cmd_report;
	ip->csr->port          = xsregs->port;
	ip->csr->cntl_status = XSCR_ARC;		/* Notify the 781 */
	UNLOCK(ip->cntlr_lock, cplock);
}

/*
 * STATIC int
 * xs_wait(xsinfo_t *, uchar_t)
 * 	Busy-wait for the specified bit(s) in 781 
 *	status register to clear.   
 *
 * Calling/Exit State:
 *	Basic locks may be held upon entry/exit.
 *
 *	The 781-board associated with 'ip' is presumed
 *	to be executing a recently started command.
 *
 *	Return 1 if the bit(s) don't clear in the required 
 *	time period.  Otherwise return 0.
 *
 * Description:
 *	Periodically poll the specified 781 board's status
 *	register to determine the specified bit pattern
 *	has been completely cleared by the 781 board, indicating
 *	it is ready for another operation or has accepted 
 *	the most recent one.  In order to use drv_usecwait()
 *	to busy-wait, we must establish for it and then poll
 *	after each one until the global time limit is reached
 *	or bit pattern finally clears.
 */
STATIC int
xs_wait(xsinfo_t *ip, uchar_t bits)
{
	clock_t temp;
	int i;

	i = 10;
	temp = xs_global.arcptime / 10;

	while ((ip->csr->cntl_status & bits) != 0) {
		if (i-- <= 0) {
			return (1);
		}
		drv_usecwait(temp);
	}
	return (0);
}

/*
 * STATIC void
 * xs_delaytime(xstty_t *)
 * 	Re-enables the queue service procedures once a scheduled
 *	delay has expired, resulting from an M_DELAY message. 
 *
 * Calling/Exit State:
 *	Does not block.
 *
 *	The port's ttylock must not be held upon entry or exit.
 *
 *	Clears tp->delayid and the TXDELAY device state flag, 
 *	indicating the delay is no longer active and transmissions
 *	may resume.
 *
 *	Re-enables the queue service procedures, allowing message
 *	servicing to be rescheduled/resumed.
 *
 *	No return value.
 */
STATIC void
xs_delaytime(xstty_t *tp)
{
	pl_t ttypl;

	ttypl = LOCK(tp->ttylock, plstr);
	tp->tty_state &= ~XS_TXDELAY;
	tp->delayid = 0;
	qenable(tp->qptr);
	UNLOCK(tp->ttylock, ttypl);
}

/*
 * STATIC void
 * xs_breaktime(xstty_t *)
 * 	Called when timer expires after a break interval 
 *	is started to re-enable queue service procedures.
 *
 * Calling/Exit State:
 *	Does not block.
 *
 *	The port's ttylock must not be held upon entry or exit.
 *
 *	Clears tp->breakid and the TXBREAK device state flag, 
 *	indicating the line-break is no longer active and 
 *	transmissions may resume.  Also informs the impacted
 *	781 port of the same.
 *
 *	Re-enables the queue service procedures, allowing message
 *	servicing to be rescheduled/resumed.
 *
 *	No return value.
 */
STATIC void
xs_breaktime(xstty_t *tp)
{
	pl_t ttypl;
	xsregs_t xsregs;
	xsinfo_t *ip = xs_info[XSBOARD(tp->devno)];

	XS_CLEAR(xsregs);
	ttypl = LOCK(tp->ttylock, plstr);
	tp->tty_state &= ~XS_TXBREAK;
	tp->breakid = 0;
	xsregs.cmd_report = XSCR_BREAKOFF;
	xsregs.port = XSPORT(tp->devno);
	xs_cmd(ip, &xsregs);
	qenable(tp->qptr);
	UNLOCK(tp->ttylock, ttypl);
}

/*
 * STATIC void
 * xs_draintime(queue_t *)
 *	Awakens close() operation waiting for output to drain.
 *
 * Calling/Exit State:
 *	Does not block.
 *
 *	The port's ttylock must not be held upon entry or exit.
 *
 *	Sets DRAINTIME in tp->flags if output queue is not empty.
 *
 *	No return value.
 *
 * Description:
 * 	Called when a timer expires while waiting for output 
 *	to drain during a close() operation.  If the output
 *	has not completely drained, set the DRAINTIME flag
 *	to notify the blocked process when it is awakened.
 *	In either case, awaken the sleeping process.
 */
STATIC void
xs_draintime(queue_t *rd_q)
{
	pl_t ttypl;
	xstty_t *tp = (xstty_t *) rd_q->q_ptr;

	ttypl = LOCK(tp->ttylock, plstr);
	if (tp->tty_state & XS_WCLOSE) {
		if (qsize(WR(rd_q)))
			tp->tty_state |= XS_DRAINTIME;
		tp->tty_state &= ~XS_WCLOSE;
		SV_SIGNAL(tp->drainwait, 0); 	/* Awaken waiting close() */
	}
	UNLOCK(tp->ttylock, ttypl);
}

/*
 * STATIC int
 * xs_contig_space(xsinfo_t *, int)
 * 	Calculate the number of contiguous (non-wrapped) bytes
 *	available in the specified 781-port's output buffer.
 *
 * Calling/Exit State:
 *	Basic locks may be held upon entry or exit.
 *
 *	Returns the number of contiguous bytes in the 
 *	output buffer available for new output.
 *
 * Description:
 * 	Flush the I/O bus DMA controller for the specified 
 *	controller to insure that integrity of the values
 *	read for the producer and consumer indexes of the
 *	specified board/port output data buffer.  Retrieve
 *	those indexes and calculate the contiguous space
 *	between them, returning the resulting value.
 */
STATIC int
xs_contig_space(xsinfo_t *ip, int port)
{
	int put, get;

	ssm_vme_dma_flush(ip->config->ssmno);
	if (XS_OUTBUF_FULL(ip, port))
		return(0);

	put = vtoss(XS_PUT(ip,port));
	put = (put + 1) & XS_OBUF_DEC;
	get = vtoss(XS_GET(ip,port));

	if (put >= get) {
		return ((get == 0) ? (XS_OBUF_DEC - put) : (XS_OBUF_SZ - put));
	} else {
		return(get - 1 - put);
	}
}

/*
 * STATIC int
 * xs_bufptr_move(xsinfo_t *, int, int)
 * 	Adjust the specified 781-board/port's output buffer
 *	producer index 'count' bytes.
 *
 * Calling/Exit State:
 *	Basic locks may be held upon entry or exit.
 *
 *	Returns one if the output buffer had been
 *	entirely consumed prior to adjusting the 
 *	producer index (it was empty). Returns zero
 *	otherwise.
 *
 * Description:
 * 	Flush the I/O bus DMA controller for the specified 
 *	controller to insure that integrity of the values
 *	read and written for the producer and consumer indexes 
 *	of the specified board/port output data buffer.  Retrieve
 *	those indexes, then rewrite the producer index after
 *	adjusting it by 'count' and factoring in queue wrap-
 *	around.
 */
STATIC int
xs_bufptr_move(xsinfo_t *ip, int port, int count)
{
	short oldput = vtoss(XS_PUT(ip, port));
	int empty = 0;
	short temp_put;

	if (XS_OUTBUF_EMPTY(ip, port))
		empty = 1;
	ssm_vme_dma_flush(ip->config->ssmno);
	temp_put = (vtoss(XS_PUT(ip,port)) + count) & XS_OBUF_DEC;
	XS_PUT(ip,port) = stovs(temp_put);
	if (vtoss(XS_GET(ip,port)) == ((oldput + 1) & XS_OBUF_DEC))
		empty = 1;
	return(empty);
}

/*
 * STATIC int
 * xs_drain(xsinfo_t *, int)
 * 	Attmpt to drain the output buffer for the specified 781 port.
 *
 * Calling/Exit State:
 *	The controller lock for the specified port must 
 *	not be held upon entry/exit because xs_cmd() 
 *	is invoked.
 *
 *	The specified port's ttylock must be held upon
 *	entry/exit.
 *
 *	Other basic locks may be held upon entry/exit.
 *
 *	Return 1 if output is draining, zero 0 otherwise.
 *
 * Description:
 * 	If data is suspended on the write queue  or a break 
 *	is in progress then indicate the buffer is draining.
 *
 *	If there are no remaining messages to post to the
 *	device then there may be data queued in the board. 
 *	In that case issue a synchronous-write request so 
 *	the 781 will only generate an interrupt when the 
 *	output buffer is empty, instead of when its low 
 * 	water mark is reached.
 *
 *	If messages remain, assume the completion of existing
 *	output posted to the device will cause them to be
 *	initiated.
 *
 * Remarks:
 *	The 781 has a bug where it may have an internal 
 *	pointer that allows old data in its onboard buffer 
 *	to be transmitted again when the buffer is empty.  
 *	Issuing a RESET clears the problem.
 */
STATIC int
xs_drain(xsinfo_t *ip, int port)
{
	xstty_t *tp = &ip->tty[port];
	xsregs_t xsregs;

	if (tp->tty_state & (XS_TXBREAK | XS_DATASUSP))
		return (1);		/* Break or suspended output */

	if (tp->msg == NULL) {
		if ((tp->tty_state & XS_TX_BUSY) == 0) {
			XS_CLEAR(xsregs);
			xsregs.cmd_report = XSCR_RESET;/* Work around 781 bug */
			xsregs.port = (uchar_t)port;
			xs_cmd(ip, &xsregs);
			return (0);			/* All output done */
		}

		if ((tp->flags & XS_SYNCREQ) == 0) {
		 	/* Schedule a SYNC request to empty all output */
			tp->flags |= XS_SYNCREQ;
			XS_CLEAR(xsregs);
 			xsregs.cmd_report = XSCR_SYNC;
			XS_SET_WR_ACTIVE(xsregs, port);
			xs_cmd(ip,&xsregs);
		}
	}
	return (1);
}

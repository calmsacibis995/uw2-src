/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* SCCSID(@(#)merge.h	2.133 LCC) Modified 09:42:49 5/3/93 */

/***************************************************************************

       Copyright (c) 1986-92 Locus Computing Corporation.
       All rights reserved.
       This is an unpublished work containing CONFIDENTIAL INFORMATION
       that is the property of Locus Computing Corporation.
       Any unauthorized use, duplication or disclosure is prohibited.

***************************************************************************/

/*
**	merge.h is no longer supported.  Its definitions have been redistributed
**		to vpippi.h, gen_def.h, kern_misc.h, svr_misc.h, vm86.h,
**		 vm86sys.h and to a few other files.  Use those files instead.
*/

/*
**  merge.h
**
**  The most common Merge defines and structures.
**
**  Initial version:  8-19-86 tjw
**  Merged several small files into this file: 7/8/87 ckp
**  Changed merge ioctl defines to the MIOC scheme: 5-Nov-87 jdp
**  Moved stuff needed for hooks to <sys/merge386.h> 15-Nov-88 jdp
**  For SVR4.0, moved contents of <sys/merge386.h> back. 15-June-90 jdp
*/

/* 
** Other include files needed before this one:
**	merge386.h
*/

#if	defined(SVR4) && ! defined(MAX_ROM_OFFSET)
/* Start of merge386.h definitions for SVR4.
** This is here because SVR4 does not come with "/usr/include/sys/merge386.h".
*/

/* Constants used for validation of offsets for mapped rom files. */
#define	MIN_ROM_OFFSET	0x000c0000
#define	MAX_ROM_OFFSET	0x000f0000
#define	REGION_OFFSET	0x00ffffff

#define	DOSSPSZ	0x100000	/* size of DOS space = 1 meg */
#define	NPPEMS 4		/* number of standard pages per EMS page */
#define	EMSPGSZ	0x4000		/* number of bytes in EMS page */
#define	NEMS_1MEG (DOSSPSZ/EMSPGSZ) /* number of EMS pages in 1 meg */


typedef struct	mproc {
	struct	vm86	*p_vm86p;
} mproc_t;

/* max number of files allowed to be mapped into a vm region */
#define	VMF	10

typedef struct mregion {
	struct	vmipage	*mr_vm86pgp;	/* ptr to information page */
	struct	inode	*mr_ip[VMF+1];	/* list of inode ptrs to mapped files*/
					/* Add one null ptr at end to make */
					/* searches fast. */
	int		mr_Bva[VMF];	/* offset into region file begins at */
	int		mr_Eva[VMF];	/* offset into region file ends at */
	int		mr_filesz[VMF];	/* our alternate for rp->r_filesz */
					/* i.e. the size in bytes of section */
					/* of file from which this region is */
					/* loaded. */
	int		linkcount[NEMS_1MEG];/* link count for each 16K page */
					/* in a 1M DOS space */
} mreg_t;


/*
** devfunc allows us to find the device handler
** related to a virtual machine port.
*/

struct	devfunc	{
	int	df_loport;		/* Lowest port assoc with that	*/
					/* device handler.		*/
	int	df_hiport;		/* Highest port.		*/
	struct vpistruct *df_vpistr;	/* Pointer to vpistruct.	*/
};
/* End of merge386.h definitions for SVR4 */

#else /* !SVR4 */

struct vmfile {
	struct	inode	*ip;		/* inode ptr to a mapped file        */
	int		Bva;		/* offset into region file begins at */
	int		Eva;		/* offset into region file ends at   */
	int		filesz; 	/* size of file section from which   */
					/* this region is loaded.            */
	struct vmfile  *next_file;	/* next mapped file on the list      */
};

#endif	/*  SVR4 and not MAX_ROM_OFFSET */


#define KEY_LENGTH	7	/* key len to identify option string */
#define KEY_STRING	{'D','O','S','O','P','T', 0xff}

/*
**	This declaration is used to declare variables and functions local
**	to a given file.  Logically a LOCAL can be made a true "static",
**	but normally we want it to be actually global.  The only reason
**	for this is so that the label can be used when debugging.
**	Every once in a while, as a test, we should compile everything
**	with LOCAL defined as static, to make sure that LOCAL is being
**	properly used.
*/
#undef	LOCAL
#define	LOCAL			/**/
/*#define	LOCAL	static	/**/

/*
**	Generic pointer type defined in SVR4 but not elsewhere.  This type
**	contains a virtual address
*/
#if	defined(SVR32) || defined(SOLARIS)
typedef char * addr_t;
#endif	/* not SVR4 */

#if	defined(CTMERGE)
extern	struct	mproc	*mproc;
#elif	defined(SVR4)
/* do not define anything */
#else	/* not CTMERGE and not SVR4 */
extern	struct	mproc	mproc[];
#endif	/* not CTMERGE and not SVR4 */
extern  struct  vm86   *vmonstate;

/* ---------- Merge system call defines and structure --------- */

#define		MAKEVMSVR	0
#define		LOAD_VM		1
#define		START_VM	2 
#define		TOVM86		3
#define		ATTACH_VM	4
#define		MAKEVMPRC	5 
#define		SETSYNC		6
#define		WAITSYNC	7
#define		GROW_VM		8
#define		MERGEVERSION	9
#define		CLEARSYNC	0x0a
#define		READSYNC	0x0b
#define		IOBMAP		0x0c
#define		NOT_DEFINED	0x0d
#define		VM_TRACE	0x0e
#define		GET_VM_INFO	0x0f
#define		MERGE_TEST	0x10
#define		FREE_CTTY	0x11
#define		GET_FLOPINFO	0x12
#define		ISDEVSTREAM	0x13
#define		DEVINFO_DONE	0x14
#define		SETUP_EM	0x15
#define		LOCK_PAGE	0x16
#define		SET_TRAP_GATE	0x17
#define		MAKE_LDTDSCR	0x18
#define		SET_GDT_DSCR	0x19
#define		EM_SNAPIMG	0x20
#define		EM_UNFREEZE	0x21
#define		SET_MRG_LIMIT	0x22
#define		GET_MRG_LIMIT	0x23
#define		MAP_WRAP	0x24
#define		EM_X_MAP	0x25
#define		CLR_MRG_LIMIT	0x26
#define		GET_MRG_SECURITY	0x27
#define		REPLACE_LDT_DSCR	0x28

/* The vm86 system call argument structure. */
typedef struct dosargs {
    int req;                        /* request type */
    int val;                        /* argument to request */
    int val2;                       /* argument 2 to request */
    int val3;                       /* argument 3 to request */
    int val4;                       /* argument 4 to request */
} dosargs_t;

/* ---------- ioctl cmd define -------------------------------- */

#ifdef	SVR4
#define GIOC		(0xaf<<8)
#else	/* not SVR4 */
#define GIOC		('M'<<8)	/* Merge IOCTL type		*/
#endif	/* not SVR4 */

#define MIOC_DINIT	    (GIOC|1)	/* Device initialize.		*/
#define MIOC_DREMOVE	    (GIOC|2)	/* Remove a device from the VM.	*/
#define MIOC_VKBDINIT	    (GIOC|3)	/* Attatch and init a VKbd.	*/
#define MIOC_VKBD_NDLY	    (GIOC|4)	/* No sleep on write.		*/
#define MIOC_WAITVM86	    (GIOC|5)	/* Sleep waiting for vm86.	*/
#define MIOC_WAKEVM86	    (GIOC|6)	/* Wake up vm86.		*/
#define MIOC_NOPOLLING	    (GIOC|7)	/* Use sleep/wakeup, don't poll */
#define MIOC_SETCMOS	    (GIOC|8)	/* Set virtual cmos.		*/
#define MIOC_GETCMOS	    (GIOC|9)	/* Get the cmos.		*/
#define MIOC_GETPPIFUNC	    (GIOC|0xa)	/* Get PPI Function.		*/
#define MIOC_GETKBPPIFUNC   (GIOC|0xb)	/* Get KB PPI Function.		*/
#define MIOC_GETMSPPIFUNC   (GIOC|0xc)	/* Get KB PPI Function.		*/
#define MIOC_SNAPSDSP	    (GIOC|0xd)	/* Get a snapshot of display.	*/
#define MIOC_MSEWRITE	    (GIOC|0xe)	/* Write a mouse event.		*/
#define MIOC_EV_SUSPEND	    (GIOC|0xf)  /* Supsend the event queue	*/
#define MIOC_EV_RESUME	    (GIOC|0x10) /* Resume the event queue	*/
#define MIOC_SCRN_SW_DONE   (GIOC|0x11) /* Screen switching done	*/
#define MIOC_DISABLE_DIRTY  (GIOC|0x12) /* Disable dirty screen processing */
#define MIOC_ENABLE_DIRTY   (GIOC|0x13) /* Enable dirty screen processing */
#define MIOC_GETLED	    (GIOC|0x14)	/* Get LED state.		*/
#define MIOC_ACQUIRE	    (GIOC|0x15)	/* Load screen.			*/
#define MIOC_RELEASE	    (GIOC|0x16)	/* Save screen.			*/
#define MIOC_GETVTNUM	    (GIOC|0x17)	/* Return vt number. (does what
					** VT_GETVTNUM is supposed to do)
					*/
#ifndef	KDVDCTYPE
#define KDVDCTYPE	    (GIOC|0x18)	/* Get display type.		*/
#endif
#define MIOC_SETSWTCH	    (GIOC|0x19)	/* Set switch screen sequence.	*/
#define MIOC_GETSWTCH	    (GIOC|0x1a)	/* Get switch screen sequence.	*/
#define MIOC_INTR	    (GIOC|0x1b) /* Generate an interrupt	*/
#define MIOC_PPI_ENABLE	    (GIOC|0x1c)	/* Enable the PPI.		*/
#define MIOC_PPI_DISABLE    (GIOC|0x1d)	/* Disable the PPI.		*/
#define MIOC_SETZOOM	    (GIOC|0x1e)	/* Set xcrt unzoom sequence.	*/
#define MIOC_VWO	    (GIOC|0x1f) /* Send an I/O to the VW driver */
#define MIOC_SETPPIFUNC	    (GIOC|0x20)	/* Set the PPI func, for a VPI.	*/
#define MIOC_PPI_DISCONNECT (GIOC|0x21)	/* Disconnect a PPI from a VPI.	*/
#define	MIOC_GETCODEPAGE    (GIOC|0x22)	/* Get code page from vm86.	*/
#if !defined (MIOC_MULT_CON_INIT)
#define	MIOC_MULT_CON_INIT  (GIOC|0x23)	/* Initialize Multi Console.	*/
#endif
#define	MIOC_GET_DEVINFO    (GIOC|0x24)	/* Get the device information.  */

/* When adding new MIOC defines, please re-use one of the "NOTUSED"
** numbers.  Thanks.
*/


#ifndef	VT_GETVTNUM
struct vt_vtnum {
	int	v_thisvt;
	int	v_activevt;
};
#endif	/* not VT_GETVTNUM */

#define DSP_ATTACHABLE	0x0001	/* Can we attach the keyboard? */
#define DSP_ATTACH	0x0002	/* May we attach the keyboard? */

#define SCR_ATTACHABLE	0x0001	/* Can we attach the screen? */
#define SCR_ATTACH	0x0002	/* May we attach the screen? */
#if	defined(MONOBOARD) || defined(CGABOARD) || defined(EGABOARD) || defined(HERCBOARD) || defined(PGABOARD) || defined(VGABOARD)
#if	MONOBOARD != 0x0020
	The above must be true
#endif
#if	CGABOARD != 0x0040
	The above must be true
#endif
#if	EGABOARD != 0x0080
	The above must be true
#endif
#if	HERCBOARD != 0x0100
	The above must be true
#endif
#if	PGABOARD != 0x0200
	The above must be true
#endif
#if	VGABOARD != 0x0400
	The above must be true
#endif
#else	/* not defined */
#define	MONOBOARD	0x0020
#define	CGABOARD	0x0040
#define	EGABOARD	0x0080
#define	HERCBOARD	0x0100
#define	PGABOARD	0x0200
#define VGABOARD	0x0400
#endif

#define KBD_ATTACHABLE	0x0001	/* Can we attach the keyboard? */
#define KBD_ATTACH	0x0002	/* May we attach the keyboard? */
#define KBD_SCANCODE	0x0004	/* Should we deliver scancodes? */
#define KBD_ISSCANCODE	0x0008	/* Is this a scancode terminal? */
#define	KBD_ISATTACHED	0x0010	/* We are now attached.		*/
#define VIRGIN_DRIVER	0x0020	/* This is an unmodified driver */

struct devattr {
	unsigned dspattr;
	unsigned kbdattr;
};



/* ---- vmono_flags, vcga_flags, vega_flags vvga_flags defines ---- */

	/*
	**	 The first 16 bits are reserved for display dirty bits.
	*/

/* Used in display vdevs */
#define	MONODIRTY	(0x00000001)
#define	HERCDIRTY	(0x0000ffff)
#define	HERCLOWDIRTY	(0x000000ff)
#define	CGADIRTY	(0x0000000f)
#define	EGADIRTY	(0x0000ffff)
#define	VGADIRTY	(0x0000ffff)
#define	D_6845		(0x00010000)	/* Virtual registers are dirty */
#define	GRX_DIRT	(0x00020000)	/* Going into graphics mode */
#define	VM86WAIT	(0x00100000)	/* VM86 is waiting (VM86_WAITCRT) */
#define	NOTIFIED	(0x00200000)	/* xcrt has been notified */

/* Used in a number of devs not vdevs */
#define	VISOPEN		(0x00020000)
#define	VISINIT		(0x00040000)


/*
**  Defines for v6845
*/

#define M6845_NUMREG	18

struct	v6845	{
	unsigned char	v_index;		/* Index register.	*/
	unsigned char	v_data;			/* Data register.	*/
	unsigned char	v_regs[M6845_NUMREG];	/* array of registers	*/
};

typedef	struct v6845	v6845_t;

#define	M6845_HTOT		0x00	/* Horizontal Total		*/
#define	M6845_HDR		0x01	/* Horizontal Display		*/
#define	M6845_HSPR		0x02	/* Horizontal Sync Pos		*/
#define	M6845_SWR		0x03	/* Sync Width			*/
#define	M6845_VTR		0x04	/* Vertical Total		*/
#define	M6845_VTAR		0x05	/* Vertical Total Adjust	*/
#define	M6845_VDR		0x06	/* Vertical Display		*/
#define	M6845_VSP		0x07	/* Vertical Sync position	*/
#define	M6845_IMSR		0x08	/* Interlace Mode		*/
#define	M6845_MXSCLN		0x09	/* Max scan line address	*/
#define M6845_CURSOR_START	0x0a	/* Cursor Start			*/
#define	M6845_CURSOR_END	0x0b	/* Cursor End			*/

#define	M6845_START_ADDR_HIGH	0x0c	/* Start Address High		*/
#define	M6845_START_ADDR_LOW	0x0d	/* Start Address Low		*/

#define	M6845_CURSOR_HIGH	0x0e	/* Cursor Address High		*/
#define	M6845_CURSOR_LOW	0x0f	/* Cursor Address Low		*/

#define	M6845_LPEN_HIGH		0x10	/* Light Pen High		*/
#define	M6845_LPEN_LOW		0x11	/* Light Pen Low		*/

#define	CRTRDY	2		/* number of times into intr code	*/


/* Bitmap struct define */

struct	BITMAP	{
	char	*map;
	int	len;		/* length in bit	*/
};


/*
**	The ppi struct is typically contained in a Virtual
**	device data structure and it is the means by which a VPI
**	is coupled to a PPI.
**
*/


#define	PPI_STRUCT	0xfefeefef	/* PPI magic number.	*/

struct	ppistruct	{
	int		ppi_magic;	/* A check.			*/
	int		(*ppi_func)();	/* The pointer to the function.	*/
	unsigned char	*ppi_data;	/* PPI data ptr.		*/
};

/*
**	The vpistruct is passed to the PPI 
**	at initialization. 
**
*/

#define	VPI_STRUCT	0xfafafafa	/* Magic Number.	*/

struct	vpistruct {
    struct proc *vm86_OLD_pp;		 /* Proc ptr.			*/	
    int			(*v_vpivmfunc)();/* Pointer to emulator.	*/
    int			(*v_vpiofunc)(); /* VPI function called from PPI.*/
    unsigned char	*v_vdev;	 /* Pointer to device virt data.*/
    struct vpistruct	*swtch_vpi;	 /* VPI data when switching.	*/
    int			(*ppi_sw_func)();/* Call when switching.	*/
    struct ppistruct	ppistruct;	 /* For ppi.			*/
    unsigned char	*ppi_dptr;	 /* PPI possible data pointer.	*/
    long		vpi_flags;	 /* Bit flags.			*/
    struct vm86		*vpi_vm86p;	 /* vm86 structure */
};

/* Bit flags for "vpi_flags" */

#define		VPIF_SWTCH	0x0001	/* switchable VPI	*/
#define		VPIF_KBD	0x0010	/* I am KB VPI		*/
#define		VPIF_MONO	0x0020	/* I am MONO VPI	*/
#define		VPIF_CGA	0x0040	/* I am CGA VPI		*/
#define		VPIF_EGA	0x0080	/* I am EGA VPI		*/
#define		VPIF_COM	0x0100	/* I am COM VPI		*/
#define		VPIF_LPT	0x0200	/* I am LPT VPI		*/
#define		VPIF_HERC	0x0400	/* I am HERC VPI	*/
#define		VPIF_VGA	0x0800	/* I am VGA VPI		*/
#define		VPIF_PPIATT	0x1000	/* VPI is currently attach to PPI */
#define		VPIF_UNIX	0x2000	/* This is a fake VPI used by normal
					** unix driver which wanna share PPI
					** with other VPI
					*/
#define		VPIF_DISPLAY	0x2ce0	/* display flag mask	==
					** MONO | HERC | CGA | EGA | VGA | UNIX
					*/
#define		VPIF_AUX	0x4000	/* Aux kbd device on PS2*/
#define		VPIF_RELEGA	0x8000  /* I am a reliable EGA VPI */
#define		VPIF_MSE       0x10000	/* I am mouse VPI	*/


/*
**	The "device request" structure.
**	This is passed to a VPI to service an IO trap.
*/

struct	devrequest	{
	unsigned long	dv_cmd;		/* Command IN/OUT	*/
	unsigned char	*dv_ioaddr;	/* IO to / from.	*/
	unsigned long	dv_port;	/* Port num - Base port.*/
	unsigned long	dv_size;	/* Word,Byte , DWORD.	*/
	unsigned long	dv_count;	/* Count to xfer.	*/
	unsigned long	dv_devflags;	/* Flags ie DIR flag	*/
	struct vpistruct *dv_vpistr;	/* VPI device info.	*/
};

/* Device commands (for the "dv_cmd" field) */
#define	DV_IN		0		/* In instruction from a port	*/
#define	DV_OUT		1		/* Out instruction to a port	*/
#define	DV_VPIEXIT	2		/* Exit device emulation	*/


/*
** Device infomation struct used to describe generic device 
*/
struct	gendev_info {
		/* io port info						*/
	int	vloport;	/* virtual port range, low		*/
	int	vhiport;	/* virtual port range, hi		*/
	int	p_off;		/* physical port range =		*/
				/* (vloport + p_off,vhiport + p_off)	*/
		/* interrupt info					*/
	int	v_intr;		/* virtual intr #. GNOTUSED mean no v_intr*/
	int	p_intr;		/* physical intr #			*/

		/* memory mapped io info				*/
	char	*vlomem;
	char	*vhimem;
	char	*mem_off;	/* physical memory io range =		*/
				/* vlomem + mem_off, vhimem + mem_off	*/
		/* dma info						*/
	int	vdma;		/* virtual dma channel			*/
	int	pdma;		/* physical dma channel			*/
	int	dev_flags;	/* reserve to use			*/
};

/*
** If the device didn't use io_port(or ivect , mem_io , dma) 
** assign value GNOTUSED
*/
#define	GNOTUSED	-1
/* ------- dev_flags define ------ */

/*
**	dinit struct. Used to initialize a VPI.
**
*/
struct	dinit	{
	long	vm86pid;		/* Pid of vm86 task.		*/
	long	d_flags;		/* Flags.			*/
	long	dataptr;		/* Dosexec supplied init data.	*/
	long	d_rsrvd;		/* For future use.		*/
	struct	ppistruct ppistruct;	/* The ppi to attach to.	*/
};

/* The lower 16 bits of d_flags are for general use.
** These are generic for all virtual devices.
*/
#define DO_VASSIGN	(0x00000001)
#define DO_SWTCH	(0x00000002)

/*
** Defines for the upper half of the d_flags word.  These are specific
** to the particular virtual devices, so it does not matter if the same
** bits are used for different purposed by different devices.
*/

/***** For virtual screen dinit ******/
#define IS_CRT		(0x00010000)
/* Defines to set virtual display type. */
#define DINIT_VGA	(0x10000000)
#define DINIT_EGA	(0x08000000)
#define DINIT_CGA	(0x04000000)
#define DINIT_HERC	(0x02000000)
#define DINIT_MONO	(0x01000000)
#define RELIABLE_EGA	(0x00100000)

/***** For keyboard dinit ******/
#define ASCII_KEYBOARD	(0x00010000)	/* Attach an ascii keyboard */

/***** For virtual screen dinit and virtual keyboard setppifunc ******/
#define IS_XCRT		(0x00020000)	/* Process doing ioctl is "xcrt" */

/* Clock event structure */
struct clkevent {
	void		(*c_clk_func)();
	int		c_current_cnt;
	int		c_resetval;
	struct clkevent	*c_clk_nxt;
};

struct mcallout {
	void	(*fun)();		/* Function to call some time later.*/
	void	*arg;			/* Argument to pass to funtion.	    */
	int	delay;			/* Delay in UNIX clock ticks.	    */
	int	class;			/* Class of events triggering wakeup.*/
	int	id;			/* Uniq id used to cancel callout   */
};

#define MAX_XCRT_CALLOUTS	4	/* Includes Windows or virtual screen.*/
#define MAX_CLOCK_CALLOUTS	1	/* Used for clock virtual ints.*/
#define MAXMCALLOUTS 		MAX_XCRT_CALLOUTS+MAX_CLOCK_CALLOUTS
#define MRG_TIME_LD		2*1000	/* Used on SLEEP or SWTCH class.  */

/* Structure defining a place to store a far pointer.  */
struct farptr {
	short	sel;
	long	*offset;
};
	
#define MAXRIDT	16	/* maximum number of redirected idt gates. */
#define MAXRGDT	64	/* maximum number of redirected gdt descriptors. */

/* Modeled after seg_desc in seg.h but defined here to reduce dependencies */
struct desc {
	unsigned long l1;
	unsigned long l2;
};

/*
** The vm86 structure.
** One allocated per vm86 process.
*/

typedef struct vm86 {
	struct proc	*v_svrp;	/* pointer to server process */
	struct proc	*v_vm86p;	/* pointer to vm86 process */
	struct vmipage	*v_vm86pgp;	/* ptr to information page */
#ifdef SVR32
	struct region	*v_reg;		/* ptr to this vm86's region */
#endif	/* SVR32 */
	char		*v_saddr;	/* linear addr of screen */
	int		 v_ssize;	/* screen size */
	int		 v_stype;	/* screen type, char/bitmapped */
	unsigned int	 hmaBaseAddr;	/* Base to attach high memory area */
	char		*v_romaddr;	/* ROM linear address */
	unsigned long	 v_pckt_val;	/* Packet value to send the server */
	unsigned long	 v_pckt_pval;	/* Pending packet from vm86 process */
	char		 v_pckt_sending;/* Packet type being sent */
	char		 v_pckt_pending;/* Packet types needing to be sent */
	char		 v_detect_sig;	/* Signal detection during tovm86 */
	char		 v_detect_retry;/* Detect retry of vm86 packet send */
	struct exit_list *v_exit_list;	/* list of exit ftns to call */
	char		 v_state;	/* state of vm86 setup */
	char		 v_unused1;
	ushort		 v_codepage;	/* Codepage value for crt/tkbd */
	int		 v_sync;	/* used by setready/waitready calls */
	struct vpistruct *vkbd_vptr;	/* current kbd ptr to vpistruct */
	struct vpistruct *mono_vptr;	/* current mono ptr to vpistruct */
	struct vpistruct *cga_vptr;	/* current cga ptr to vpistruct */
	struct vpistruct *ega_vptr;	/* current ega ptr to vpistruct */
	struct vpistruct *vga_vptr;	/* current vga ptr to vpistruct */
	struct vpistruct *v8259_vptr;	/* current 8259 ptr to vmdesw entry */
	struct vpistruct *v8253_vptr;	/* current 8253 ptr to vmdesw entry */
	int		 v_irbits;	/* interrupt request flag for VMM */
	ushort		 v_flags;	/* virtual 8086 flags for VMM */
	struct farptr	 pm_emulator;	/* emulator for protected mode oper */
	struct farptr	 pm_emstack;	/* emulator, ring 1? stack ss:sp  */
	struct farptr	 pm_emstk2;	/* ring 2 stack			    */
	struct farptr	 pm_inthdlr;	/* fault handler for PM oper        */
	unsigned short	 v_msw;		/* virtual machine status word	    */
	unsigned short	 pm_em_base;	/* base page of the PM emulator  */
 	int		 pm_emvflag;	/* Location of emu's virtual flags */
 	int		 pm_emidtr;	/* Location of emu's idt psuedodesc */
	unsigned char	 v_eqp_byte;	/* equipment byte */
	int		 vm86_flags;	/* misc. flags */
	struct vmtrace	*v_tracestuff;	/* When not NULL, ptr to trace buffer */
	int	       (*v_cursor_fn)();/* Function which handles fast cursor */
	void	       (*v_dirty_fn)();	/* Function which handles dirty bits */
	int	       (*v_crt_poll_fn)();/*Function for crt poll wakeups */
	struct devfunc	*devsw_p;	/* Pointer to the "vmdevsw" table */
	int		 devsw_max;	/* Max valid index in "vmdevsw" table */
	int		 devsw_next;	/* Next free index in "vmdevsw" table */
	unsigned int	 v_patch;	/*if((op<<16)+ip==v_patch)emvmflt();*/
	struct desc      ldt_sel;	/* Used when relocating the LDT */
	struct desc      ldt_save;	/* Original Unix LDT used to restore */
	struct desc     *gdtp[MAXRGDT+1];/* The ptrs to gdt descriptors   */
	struct desc      gdtd[MAXRGDT];  /* places to put descriptors     */
	struct desc     *idtp[MAXRIDT+1];/* The ptrs to idt gates         */
	struct desc      idtd[MAXRIDT];  /* places to put descriptors     */
	int		 ints_inservice;/* counter for interrupts in service */
	struct mcallout mrg_co[MAXMCALLOUTS];/* Callout structs, timer events.*/
	int		callout_count;	/* Callouts currently in use.	*/
	int		timeout_id;	/* Current UNIX timeout id.	*/
	unsigned long	v_last_lbolt;	/* Last lbolt we looked at.	*/
#ifdef	CR4_VME
	int		 v_cr4;		/* virtual cr4 register	*/
#endif /* CR4_VME */
} vm86_t;


/* State for the v_state field in the vm86 structure */
#define	V_CREATED	0
#define	V_LOADED	10
#define	V_SERVERBLOCKED	20
#define	V_STARTSLEEP	30
#define	V_INTWAIT	40
#define	V_RUNNING	50
#define	V_EXITING	60
#define	V_ERROR		70

/* v_irbits bit definitions in the vm86 structure */
#define	VI_IRQPND	 0x0001	/* Interrupt pending		      */
#define	VI_SUSPEND_VM86	 0x0002	/* Set this flag will make vm86 sleep.*/
#define	VI_FLP_DEASSIGN	 0x0004	/* Deassign floppy (AT386).	      */
#define	VI_VEGA_DEASSIGN 0x0008	/* Deassign the VGA ports	      */
#define	VI_VEGA_ASSIGN	 0x0010	/* Assign the VGA ports		      */
#define	VI_SCR_SW_PAUSE	 0x0020	/* Pause the vm86 while screen switch */
#define	VI_SLEEP_EVENT	 0x0040	/* A sleep event is on the callout.   */
#define	VI_SWTCH_EVENT	 0x0080	/* A switch event is on the callout.  */
#define	VI_TIMER_EVENT	 0x0100	/* A timer event is on the callout.   */
#define	VI_TIMER_OUT	 0x0200	/* A timer callout is posted.	      */
#define	VI_HOLD_INT	 0x0400	/* Hold interrupts for catch up.      */
#define	VI_VCLK_CATCHUP	 0x0800	/* Need to catchup clock interrupts.  */

/* vm86_flags bit definitions in the vm86 structure */
#define	VF_PS2_VM	0x0001	/* Use PS/2 Virtual Machine model	*/
#define	VM86_HAS_CRT	0x0002	/* VM86 has crt process.  This is used
				** to indicate when "screen dirty" events
				** need to be sent to crt (or xcrt).
				*/
#define	VM86_WAITCRT	0x0004	/* Waiting for crt to process screen	*/
#define	CRT_WAITDIRTY	0x0008	/* Crt waiting for dirty bits		*/
#define	VM86_SW_PAUSE	0x0010	/* Need to pause vm86 process during
				** screen switching.
				*/


/* v_flag parameters to vmiobmap() */
#define	VMIO_ENABLE   0		/* Enable direct access to I/O port */
#define	VMIO_DISABLE  1		/* Disable direct access to I/O port */

/* Packet types (bit flag) that get sent to the vm86 server process.
** These bit flags are used in the v_pckt_sending and v_pckt_pending fields.
*/
#define	PCKT_TYPE_MASK	0x0ff	/* Must fit in a byte. */
#define	PCKT_TYPE_MRG	1
#define	PCKT_TYPE_LED	2
#define	PCKT_TYPE_DONE	3
#define	PCKT_TYPE_INFO	4
#define	INFORM_XCRT	1
#define	RESUME_DOS	2
#define	CRT_DOWORK	3
#define	CRT_WORKDONE	4
#define	DOS_FREEZE	5
#define	DOS_UNFREEZE	6

/*
**	This macro should be the only way that a process derives the 
**	vm86pointer.  This isolates merge from details of the proc structure
**	and eliminates all explicit references to mproc.
*/
#ifdef	SVR4

#define PROCP_TO_VM86P(procp)  ((procp)->p_vm86p)

#else	/* not SVR4 */

#define PROCP_TO_VM86P(procp)  (mproc[(procp)-(proc)].p_vm86p)
#define GET_INDEX(procp)  ((procp)-(proc))
#define PREEMPT() qswtch()

#endif	/* not SVR4 */

/* Macro to fix a bug in non-SCO Unix */
#ifdef	SCO
#define FTOP		ftop
#else
#define	FTOP(x)		(ftop((unsigned long) x))
#endif

/* Merge limitation definitions. */
#define MULTI_USER	(0x1)

struct user_list {
	ushort	uid;
	int	count;
};


/*
** Code page struct is used to get the current code page
** from the vkbd. Upon the entry (ioctl call) the pid field
** is set the vm86 (dos) pid (the parent of tkbd).
*/
struct	code_page_ioctl {
	long	pid;
	ushort	code_page;
};

#ifdef	pnum
#if	CTMERGE && SVR32
#undef	pnum
#define	pnum(X)	(((uint)(X) >> PNUMSHFT) & PTOFFMASK)
#endif	/* CTMERGE and SVR32 */
#endif	/* pnum */

/* Parameters for the screen dirty functions */
#define	DIRTY_CHECK_CLOCK	0
#define	DIRTY_CHECK_FINAL	1
#define DIRTY_CLEAR		2
#define	DIRTY_FORCE_UPDATE	3


/* This structure contains the device information of the attached
 * devices.  It has three fields representing the device type, the
 * device name, and the device information.  For example, the device
 * type such as "LPT1", the device name such as "Epson lx-800", the
 * device information such as "port=378;irq=7;...".
 */
struct device_info {
	char	dev_type[10];
	char	dev_name[132];
	char	dev_info[132];
};

/* This address must correspond to the load address used */
/* to build the emulator in em/vuifile */
/* NOTE: SCO kernel has bugs which don't allow a user process */
/* to be loaded above 0x80000000. */
#define EM_LOAD_ADDR	0x70000000


#ifndef NULL
#define NULL 0
#endif

/* return values from mrgWhichProcess */
#define MRG_OTHER 	1	/* some server process */
#define MRG_DOSEXEC	2	/* dosexec */
#define MRG_VM86P	3	/* the dos task */

/*  Useful for passing to getvm86vaddr */
#define	NIL_PROCP	(struct proc *) 0

/* externs for now, was function prototypes and will soon be */
extern struct devfunc *mrgAddVpi();
extern char *mrgGetMem();
extern struct vpistruct *mrgConnectIrq();
extern int mrgConnectDma();
extern struct vm86 *mrgGetV86P();
extern struct vm86 *mrgPidToV86P();
extern void mrgForceDump();
extern struct proc *mrgCurProc();
extern int mrgGetTssStart();

#ifdef SVR32
#define VMI_ALLOC_SIZE	NBPC
#else
#define VMI_ALLOC_SIZE	4096	/* TBD - actual value should be tuned */
#endif

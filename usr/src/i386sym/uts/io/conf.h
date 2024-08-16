/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_CONF_H	/* wrapper symbol for kernel use */
#define _IO_CONF_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:io/conf.h	1.37"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Configuration specific defines and structures.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Device flags.
 *
 * Bit 0 to bit 15 are reserved for kernel.
 * Bit 16 to bit 31 are reserved for different machines.
 */
#define D_NEW		0x00	/* new-style driver */
#define	D_OLD		0x01	/* old-style driver; no longer supported */
#define D_DMA		0x02	/* driver does DMA */
#define D_BLKOFF	0x400	/* driver understands b_blkoff; */
				/* i.e. is byte addressable */
/*
 * Added for UFS.
 */
#define D_SEEKNEG       0x04    /* negative seek offsets are OK */
#define D_TAPE          0x08    /* magtape device (no bdwrite when cooked) */
/*
 * Added for ease of transition from pre-DDI drivers.  By not setting this
 * flag, the driver only sees pre-SVR4-style buffers.
 */
#define D_NOBRKUP	0x10	/* no breakup needed for new drivers */
/*
 * Security additions, for drivers requiring special MAC access policies.
 */
#define D_INITPUB	0x20	/* device is public in system setting */
#define D_NOSPECMACDATA	0x40	/* no MAC access check for data transfer */ 
				/* and no inode access time change */ 
#define D_RDWEQ		0x80	/* destructive reads, read equal, write eq */
#define SECMASK		(D_INITPUB|D_NOSPECMACDATA|D_RDWEQ)
				/* mask of all security flags */
/*
 * MP-related flags.
 */
#define D_MP		0x100	/* driver/module is MP */
#define D_UPF		0x200	/* mux is UP-friendly */
/*
 * End of devflag definitions.
 */

#define ROOTFS_NAMESZ	7	/* Maximum length of root fstype name */

#define	FMNAMESZ	8	/* max length of streams module name */

#if defined _KERNEL || defined _KMEMUSER

/*
 * Firmware interface switch 
 */
struct firmwaresw {
	void	(*fw_boot)();
	void	(*fw_return)();
	void	(*fw_inittodr)();
	void	(*fw_resettodr)();
	void	(*fw_wdtinit)();
	void	(*fw_wdtdisable)();
	void	(*fw_wdtreenable)();
};

#define FW_BOOT		(*firmwaresw->fw_boot)
#define FW_RETURN_FW	(*firmwaresw->fw_return)
#define FW_INITTODR	(*firmwaresw->fw_inittodr)
#define FW_RESETTODR	(*firmwaresw->fw_resettodr)
#define FW_WDTINIT	(*firmwaresw->fw_wdtinit)
#define FW_WDTDISABLE	(*firmwaresw->fw_wdtdisable)
#define FW_WDTREENABLE	(*firmwaresw->fw_wdtreenable)

/*
 * Declaration of block device switch. Each entry (row) is
 * the only link between the main unix code and the driver.
 * The initialization of the device switches is in the file conf.c.
 */
struct bdevsw {
	int	(*d_open)();
	int	(*d_close)();
	int	(*d_strategy)();
	int	(*d_size)();
	int	(*d_devinfo)();
	char	*d_name;
	struct iobuf	*d_tab;
	int	*d_flag;
	int	d_cpu;
	struct module	*d_modp;
};

/*
 * Character device switch.
 */
struct cdevsw {
	int	(*d_open)();
	int	(*d_close)();
	int	(*d_read)();
	int	(*d_write)();
	int	(*d_ioctl)();
	int	(*d_mmap)();
	int	(*d_segmap)();
	int	(*d_poll)();
	int	(*d_msgio)();
	int	(*d_devinfo)();
	struct tty *d_ttys;
	struct streamtab *d_str;
	char	*d_name;
	int	*d_flag;
	int	d_cpu;
	struct module *d_modp;
};

/*
 * STREAMS module information
 */

struct fmodsw {
	char	f_name[FMNAMESZ+1];
	struct streamtab *f_str;
	int	*f_flag;		/* same as device flag */
	struct module	*f_modp;
};

/*
 * Console-capable device table
 */

struct constab {
	char	*cn_name;
	struct conssw *cn_consswp;
	int	cn_cpu;
};

/* Parameter types for d_devinfo() */
typedef enum di_parm {
	DI_BCBP,
	DI_MEDIA
} di_parm_t;

#endif /* _KERNEL || _KMEMUSER */

#if defined _KERNEL

extern struct firmwaresw *firmwaresw;
extern struct bdevsw bdevsw[];
extern struct cdevsw cdevsw[];
extern struct fmodsw fmodsw[];
extern struct constab constab[];

extern int	bdevcnt;
extern int	cdevcnt;
extern int	fmodcnt;
extern int	conscnt;

extern int bdevswsz;
extern int cdevswsz;
extern int fmodswsz;

#endif /* _KERNEL */

#if defined(__cplusplus)
        }
#endif
#endif /* _IO_CONF_H */

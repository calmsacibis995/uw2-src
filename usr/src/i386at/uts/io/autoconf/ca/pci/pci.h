/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_AUTOCONF_CA_PCI_H
#define _IO_AUTOCONF_CA_PCI_H

#ident	"@(#)kern-i386at:io/autoconf/ca/pci/pci.h	1.5"
#ident  "$Header: $"

#pragma pack(1)
/*
 *
 * PCI header file
 * 
 * contains defines for PCI bus driver code, and externs usable by
 * drivers (pci_read/write devconfig, find_pci_id)
*/
#define PCI_FUNCTION_ID		0xb1
#define PCI_FAILURE		-1
#define PCI_BIOS_PRESENT	0x1
#define PCI_FIND_PCI_DEVICE	0x2

#define PCI_READ_CONFIG_BYTE	0x8
#define PCI_READ_CONFIG_WORD	0x9
#define PCI_READ_CONFIG_DWORD	0xa
#define PCI_WRITE_CONFIG_BYTE	0xb
#define PCI_WRITE_CONFIG_WORD	0xc
#define PCI_WRITE_CONFIG_DWORD	0xd

#define PCI_VENDOR_ID_OFFSET        0x00
#define PCI_DEVICE_ID_OFFSET        0x02
#define PCI_HEADER_OFFSET	0x0E
#define PCI_COMMAND_OFFSET          0x04
#define	PCI_BASE_REGISTER_OFFSET    0x10
#define PCI_E_ROM_BASE_ADDR_OFFSET  0x30
#define PCI_INTERPT_LINE_OFFSET     0x3C
#define PCI_INTERPT_PIN_OFFSET      0x3D

#define PCI_INVALID_VENDOR_ID       0xFFFF

#define	MULTI_FUNCTION_MASK	0x80

/* Class code info */
#define PCI_BASE_CLASS_OFFSET		0x0B
#define PCI_SUB_CLASS_OFFSET		0x0A
#define PCI_PROG_INTF_OFFSET		0x09
#define PCI_CLASS_TYPE_PCC		0x00
#define PCI_CLASS_TYPE_MASS_STORAGE	0x01
#define PCI_CLASS_TYPE_NETWORK		0x02
#define PCI_CLASS_TYPE_DISPLAY		0x03
#define PCI_CLASS_TYPE_MULTIMEDIA	0x04
#define PCI_CLASS_TYPE_MEMORY		0x05
#define PCI_CLASS_TYPE_BRIDGE		0x06


/* Base Registers in PCI Config space */
#define	MAX_BASE_REGISTERS		6


/* PCI ERROR CODES */

#define PCI_SUCCESS                     0x00
#define PCI_UNSUPPORTED_FUNCT           0x81
#define PCI_BAD_VENDOR_ID               0x83
#define PCI_DEVICE_NOT_FOUND            0x86
#define PCI_BAD_REGISTER_NUMBER         0x87

#define BIOS_32_START			0xE0000
#define BIOS_32_SIZE			(0xFFFF0 - 0xE0000)
#define MAX_PCI_DEVICES			32
#define MAX_PCI_FUNCTIONS		8

/* miscellaneous defines, specific to
 * our PCI ca code
*/

/* a guess as to the size of the PCI bios*/
#define PCI_BIOS_SIZE			0x10000

/* string we search the BIOS32 directory for: */
#define BIOS_32_ID			(caddr_t) "_32_"

/* BIOS 32 directory string for the PCI BIOS */
#define PCI_BIOS_ID			(caddr_t) "$PCI"

/* PCI BIOS ID string */
#define PCI_ID				(caddr_t) "PCI "

/* Number of bytes of config space */
#define MAX_PCI_REGISTERS		256

/* writing the following to devices command register disables it */
#define PCI_COMMAND_DISABLE		(unsigned short) 0xFFFC
/* expansion rom sizes and header offsets */
#define PCI_EXP_ROM_START_ADDR		0xC0000
#define PCI_EXP_ROM_SIZE		0x30000
#define PCI_EXP_ROM_HDR_CHUNK		0x200
#define PCI_EXP_ROM_HDR_SIG		0xAA55
#define PCI_EXP_ROM_DATA_SIG		"PCIR"
#define PCI_EXP_ROM_PCC_CODE		0
#ifdef _KERNEL_HEADERS
#include <util/types.h>
#elif defined(_KERNEL)
#include <sys/types.h>
#endif
struct pci_rom_header {
	ushort_t	sig;			/* 0x55AA */
	uchar_t		run_length;		/* run-time length */
	uint_t		init_fun;		/* pointer to init fun */
	uchar_t		reserved[17];		/* unique to app */
	ushort_t	offset; 		/* offset to pci_rom_data */
};
struct pci_rom_data {
	uchar_t		signature[4];		/* "PCIR" */
	ushort_t	v_id;			/* vendor id*/
	ushort_t	d_id;			/* device id*/
	ushort_t	vpd_offset;		/* vital product data offset*/
	ushort_t	len;			/* length of this struct */
	uchar_t		rev_code;		/* revision code... 0 */
	uchar_t		base_class[3];		/* base class */
	ushort_t	sub_class;		/* sub class */
	ushort_t	imagelength;		/* run time image length */
	ushort_t	rev_level;		/* rom revision level */
	uchar_t		code_type;		/* type of rom */
	uchar_t		cont_ind;		/* last rom image? */
	uchar_t		pad[2];			/* pad to 16 bytes */
};

struct pci_rom_signature_data {
	ushort_t	vendor_id;
	ushort_t	device_id;
	uint_t		addr;
	uint_t		length;
	uint_t		used;
};
/* visible functions */
#if defined(_KERNEL)
extern int _pci_bios_call(regs *, char *);
extern int find_pci_id(uchar_t *);
extern int pci_read_devconfig(uchar_t, uchar_t, uchar_t *, int, int);
extern int pci_write_devconfig(uchar_t, uchar_t, uchar_t *, int, int);
#endif
#endif /* _IO_AUTOCONF_CA_PCI_H */

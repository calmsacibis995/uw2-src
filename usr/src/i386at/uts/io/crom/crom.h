/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_IO_CROM_CROM_H		/* wrapper symbol for kernel use */
#define	_IO_CROM_CROM_H		/* subject to change without notice */

#ident	"@(#)kern-i386at:io/crom/crom.h	1.3"
#ident	"$Header: $"

/*
 *	Copyright (C) Compaq Computer Corporation, 1990.
 *	All Rights Reserved.
 *	This Module contains Proprietary Information of Compaq
 *	Computer Corporation, and should be treated as Confidential.
 */

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * This mask is scanned from bit 0 to bit 7. If a bit is set, it means
 * that an argument of the type corresponding to the bit position in
 * the key mask is next in the argument list. 
 */
typedef struct key_mask {
	unsigned int
		slot	 : 1,	/* EISA Slot Number. */
		function : 1,	/* Function Record Number within a EISA slot. */
		board_id : 1,	/* EISA readable Board ID. */
		revision : 1,	/* EISA Board Revision Number. */
		checksum : 1,	/* EISA Board Firmware Checksum. */
		type     : 1,	/* EISA Board Type String. */
		sub_type : 1,	/* EISA Board Sub-type String. */
		resources : 1,	/* EISA Board Resource list. */
			 : 24;
} KEY_MASK;


typedef struct {
	char *data;
	int length;
} eisanvm;


#ifdef ORIGINAL

#pragma pack(1)

typedef struct {
	union {
		unsigned int eax;
		struct {
			unsigned short ax;
		} word;
		struct {
			unsigned char al;
			unsigned char ah;
		} byte;
	} eax;

	union {
		unsigned int ebx;
		struct {
			unsigned short bx;
		} word;
		struct {
			unsigned char bl;
			unsigned char bh;
		} byte;
	} ebx;

	union {
		unsigned int ecx;
		struct {
			unsigned short cx;
		} word;
		struct {
			unsigned char cl;
			unsigned char ch;
		} byte;
	} ecx;

	union {
		unsigned int edx;
		struct {
			unsigned short dx;
		} word;
		struct {
			unsigned char dl;
			unsigned char dh;
		} byte;
	} edx;

	union {
		unsigned int edi;
		struct {
			unsigned short di;
		} word;
	} edi;

	union {
		unsigned int esi;
		struct {
			unsigned short si;
		} word;
	} esi;

} regs;

#endif /* ORIGINAL */


#pragma pack()

#define EISA_IOCTL		('E'<<8)
#define EISA_CMOS_QUERY		(EISA_IOCTL|1)
#define EISA_SYSTEM_MEMORY	(EISA_IOCTL|2)
#define EISA_ELCR_GET		(EISA_IOCTL|3)
#define EISA_ELCR_SET		(EISA_IOCTL|4)
#define EISA_DMAEMR_SET		(EISA_IOCTL|5)
#define EISA_TOTAL_FUNCTIONS	(EISA_IOCTL|6)
#define ROM_CALL		(EISA_IOCTL|7)

#define EISA_SLOT		0x01
#define EISA_FUNCTION		0x02
#define EISA_BOARD_ID		0x04
#define EISA_REVISION		0x08
#define EISA_CHECKSUM		0x10
#define	EISA_TYPE		0x20
#define EISA_SUB_TYPE		0x40
#define EISA_RESOURCES		0x80
#define EISA_INTEGER		(EISA_SLOT | EISA_FUNCTION | \
				 EISA_BOARD_ID | EISA_REVISION | \
				 EISA_CHECKSUM | EISA_RESOURCES)

/* DMA Addressing Modes */

#define DMA_ADDRESSING		0x0c
#define DMA_8_BYTE		0x00	/* 8-bit i/o, count by bytes. */
#define DMA_16_WORD		0x04	/* 16-bit i/o, count by words. */
#define DMA_32_BYTE		0x08	/* 32-bit i/o, count by bytes. */
#define DMA_16_BYTE		0x0c	/* 16-bit i/o, count by bytes. */

/* DMA Cycle Timing Modes */

#define DMA_TIMING		0x30
#define DMA_ISA			0x00
#define DMA_TYPE_A		0x10
#define DMA_TYPE_B		0x20
#define DMA_BURST		0x30

/* DMA Terminal Count modes. */

#define DMA_T_C			0x40
#define DMA_T_C_OUT		0x00
#define DMA_T_C_IN		0x40

/* DMA Stop Register modes. */

#define DMA_S_R			0x80
#define DMA_S_R_ON		0x00
#define DMA_S_R_OFF		0x80

/* DMA Defaults. */

#define DMA_DEF_0_3	(DMA_S_R_OFF | DMA_T_C_OUT | DMA_ISA | DMA_8_BYTE)
#define DMA_DEF_4_7	(DMA_S_R_OFF | DMA_T_C_OUT | DMA_ISA | DMA_16_WORD)

#define ARGUMENTS		8
#define LAST_ARG		(1 << (ARGUMENTS - 1))
#define SYSTEM_MEMORY		0
#define MEMORY_ADDRESS_UNITS	256
#define MEMORY_SIZE_UNITS	1024
#define EISA_READ_SLOT_CONFIG	0xd880		/* reg ax value */
#define EISA_READ_FUNC_CONFIG	0xd881		/* reg ax value */
#define EISA_CALL_ADDRESS	0xff859
#define EISA_STRING_ADDRESS	0xfffd9
#define EISA_STRING		'EISA'
#define EISA_SLOTS		16

#define EISA_BUF_SIZE		0x5000  /* buffer reserved for BIOS info */

#define EISA_ID0(z)		(z * 0x1000 + 0xc80)
#define EISA_ID1(z)		(z * 0x1000 + 0xc81)
#define EISA_ID2(z)		(z * 0x1000 + 0xc82)
#define EISA_ID3(z)		(z * 0x1000 + 0xc83)

typedef struct {
	long address;
	long length;
} MEMORY_BLOCK;


typedef struct {
	unsigned char ints_7_0;
	unsigned char ints_15_8;
} ELCR_IO;


typedef struct {
	unsigned char channel;
	unsigned char mode;
} DMAEMR_IO;

#endif	/* _IO_CROM_CROM_H */

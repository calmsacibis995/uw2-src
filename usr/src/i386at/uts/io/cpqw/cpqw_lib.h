/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/cpqw/cpqw_lib.h	1.4"

/********************************************************
 * Copyright 1993, COMPAQ Computer Corporation
 ********************************************************/

/*
 *  Generic ROM call header file.
 */


/* opcode types for the register variable */
#define NONE		0	/* the register value only contains data */
#define POINTER		1	/* the register is an address for data */

/*
#define	LOCK		1	/* lock virtual address */
/*
#define	UNLOCK		0	/* unlock virtual address */

#define DEBUG		1	/* set debug level */

#define GET_EV			0xD8A4	/* get EV from EISA NVM */
#define SET_EV			0xD8A5	/* set EV in EISA NVM */
#define ROM_CALL_ERROR		0x01	/* ROM call error bit */
#define CALL_NOT_SUPPORTED	0x86	/* ROM call not supported */
#define ERROR_NOT_LOGGED	0x87	/* ROM call not logged */
#define EV_NOT_FOUND		0x88	/* EV not found with ROM call */

#pragma pack(1)

/*
 * Registers to pass to the kernel from user space.  If the register value
 * is an address of a buffer, set the opcode flag stating this fact.  Also,
 * tell the kernel how long the data buffer is.
 *
 * eax.lword		- eax
 * eax.word		- ax
 * eax.byte.low		- al
 * eax.byte.high	- ah
 */
typedef struct {
	union {
		unsigned long lword;		/* eax */
		unsigned short word;		/* ax */

		struct {
			unsigned char low;	/* al */
			unsigned char high;	/* ah */
		} byte;
	} data;
/*
	unsigned char opcode;	/* is the register an address for data */
/*
	unsigned long length;	/* if the reg. is a pointer, how much data */
} register_t;


/* This is all registers contained in one structure */
typedef struct {
	register_t eax;
	register_t ebx;
	register_t ecx;
	register_t edx;
	register_t edi;
	register_t esi;
	register_t eflags;
} all_reg_t;

#pragma pack()


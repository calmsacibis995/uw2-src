/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-pdi:io/hba/adsa/him_code/seq_01.h	1.1"

/* $Header:   V:\source\code\aic-7770\him\common\seq_01.hv_   1.7   12 May 1994 12:31:42   FANNIN  $ */

/* Swapping D/E part sequencer */

#define  SEQ_01   0x55555555

UBYTE E_Seq_01[] = {
0x00,0x65,0xAC,0x11,0x00,0x65,0x6B,0x10,0x00,0x65,0x82,0x10,0x00,0x65,0x99,0x10,
0x00,0x65,0x72,0x11,0x5A,0x6A,0x00,0x00,0xF7,0x11,0x11,0x02,0x08,0x1F,0x1F,0x04,
0x00,0x6A,0x90,0x02,0xFF,0x3B,0x3C,0x02,0x08,0x1F,0x0C,0x1E,0xFF,0x3D,0x3C,0x02,
0x40,0x0B,0x64,0x1A,0x20,0x0B,0x2C,0x1A,0x40,0x00,0x07,0x1A,0x08,0x1F,0x1F,0x04,
0x40,0x00,0x09,0x1A,0x47,0x6A,0x65,0x00,0x80,0x6C,0x27,0x1A,0x57,0x65,0x12,0x18,
0xFF,0x42,0x09,0x1E,0x01,0x6A,0x39,0x00,0x3C,0x6A,0x64,0x00,0x43,0x6A,0x83,0x17,
0x01,0x43,0x43,0x06,0xFF,0x42,0x42,0x06,0xFF,0x3C,0x09,0x1C,0x00,0x3C,0x95,0x17,
0x08,0xBB,0xDE,0x1A,0x20,0xA0,0x23,0x1A,0x00,0xA1,0x9C,0x17,0xFF,0x34,0x23,0x1C,
0x00,0x65,0xA4,0x17,0xFF,0x3C,0x6D,0x02,0x00,0x65,0x09,0x10,0xF7,0x1F,0x1F,0x02,
0x08,0xA1,0x64,0x02,0x00,0x1F,0x1F,0x00,0x00,0x65,0x57,0x10,0xFF,0x65,0x65,0x06,
0xFF,0x65,0x66,0x02,0x7F,0x6C,0x3C,0x02,0x7F,0x6A,0x6D,0x00,0x00,0x65,0x1B,0x10,
0xFF,0x6A,0x3C,0x00,0x08,0x6A,0x0C,0x00,0x08,0x11,0x11,0x00,0x20,0x0B,0x06,0x1E,
0x20,0x6A,0x0B,0x00,0x00,0x65,0x61,0x17,0xE0,0x64,0x36,0x1C,0xA0,0x64,0x6B,0x19,
0x21,0x6A,0x91,0x00,0x00,0x65,0x31,0x10,0x80,0x12,0x56,0x1E,0x07,0x12,0x64,0x02,
0x08,0x1F,0x3A,0x02,0x00,0x19,0x64,0x00,0x00,0x3A,0x3A,0x00,0x00,0x3A,0x9C,0x17,
0xFF,0x34,0x44,0x1C,0x00,0x34,0x98,0x17,0x20,0xA0,0x44,0x1A,0xFF,0x34,0x3C,0x02,
0x00,0x3C,0x95,0x17,0x08,0xBB,0x43,0x1E,0xF0,0x6A,0x03,0x00,0x00,0x3A,0x3E,0x17,
0x00,0x65,0x60,0x17,0xE0,0x64,0x4A,0x1C,0xA0,0x64,0x4B,0x18,0xFF,0x3C,0x4B,0x1C,
0x08,0xBB,0xE5,0x1A,0x00,0x65,0x4B,0x10,0x20,0x12,0x4E,0x1C,0xFF,0x3C,0x55,0x1C,
0xFF,0xBA,0x82,0x1E,0x01,0xBA,0xAC,0x1C,0x00,0x65,0x60,0x17,0xE0,0x64,0x6B,0x19,
0xFF,0x12,0x34,0x02,0x00,0x34,0x98,0x17,0x20,0xA0,0x55,0x1E,0xFF,0x3A,0x64,0x02,
0x00,0xA1,0x3F,0x1C,0x81,0x6A,0x91,0x00,0x91,0x6A,0x91,0x00,0x5B,0x6A,0xA0,0x17,
0x08,0x1F,0x5B,0x1E,0xFF,0x3C,0x3D,0x02,0x00,0x65,0x5C,0x10,0xFF,0x3C,0x3B,0x02,
0xF0,0xA1,0x64,0x02,0x0F,0x05,0x05,0x02,0x00,0x05,0x05,0x00,0x00,0x6A,0x93,0x02,
0x18,0x6A,0x01,0x00,0x20,0x6A,0x0C,0x00,0x5A,0x6A,0x00,0x00,0x40,0x0B,0x07,0x1E,
0x00,0x3C,0x95,0x17,0xFF,0xA2,0xB2,0x02,0x08,0x6A,0x0C,0x00,0x12,0x6A,0x00,0x00,
0x08,0x11,0x11,0x00,0x40,0x0B,0x05,0x1E,0x40,0x6A,0x0B,0x00,0x00,0x65,0x61,0x17,
0xA0,0x64,0x8A,0x18,0x08,0xBB,0xE5,0x1A,0x07,0xA1,0x39,0x02,0x40,0xA0,0x73,0x1E,
0x32,0x6A,0x39,0x17,0x00,0x6E,0x73,0x1A,0x40,0x39,0x39,0x00,0x80,0x39,0x3A,0x00,
0xFF,0x3A,0x39,0x02,0x20,0xA0,0x7E,0x1E,0xFF,0x39,0x06,0x02,0x00,0x65,0x61,0x17,
0xA0,0x64,0x8A,0x18,0x23,0xA0,0x3A,0x02,0xFF,0x3A,0x06,0x02,0x00,0x65,0x61,0x17,
0xA0,0x64,0x8A,0x18,0xFF,0x3C,0x39,0x02,0x00,0xA1,0x3F,0x17,0x8F,0x04,0x81,0x1C,
0x40,0x6A,0x0C,0x00,0xFF,0x39,0x06,0x02,0x00,0x65,0x61,0x17,0x80,0x64,0x8E,0x1C,
0xE0,0x64,0x88,0x1C,0xC0,0x64,0xCE,0x1C,0xA0,0x64,0x6B,0x19,0x01,0x6A,0x91,0x00,
0x00,0x6A,0x03,0x17,0x00,0x65,0x82,0x10,0x40,0x6A,0x0C,0x00,0xC0,0x64,0xCE,0x1C,
0x80,0x64,0x6B,0x19,0x20,0xA0,0x6B,0x1B,0x40,0x6A,0x0C,0x00,0x00,0xA1,0x3E,0x17,
0x8F,0x04,0x87,0x1C,0xFF,0xAB,0x39,0x02,0xA7,0x6A,0xA8,0x17,0xFF,0xAB,0x08,0x02,
0x3D,0x6A,0x93,0x00,0x04,0x0B,0x9C,0x1A,0x10,0x0C,0x95,0x1E,0x04,0x0B,0x9C,0x1A,
0x00,0x6A,0x93,0x02,0x00,0x65,0x61,0x17,0xC0,0x64,0xCE,0x1C,0x11,0x6A,0x91,0x00,
0x34,0x6A,0x66,0x00,0xA2,0x6A,0x46,0x17,0x00,0x65,0x4F,0x17,0xB3,0x6A,0x66,0x00,
0x88,0x6A,0x44,0x17,0xA2,0x6A,0x66,0x00,0x34,0x6A,0x46,0x17,0x00,0x65,0xB2,0x10,
0x04,0x93,0xA6,0x1A,0x01,0x94,0xA5,0x1E,0xC7,0x93,0x93,0x02,0x08,0x93,0xA7,0x1A,
0xAF,0x6A,0x66,0x00,0x08,0x6A,0x48,0x17,0xFF,0x34,0xB2,0x02,0x00,0x65,0x66,0x17,
0x34,0x6A,0x66,0x00,0xA2,0x6A,0x46,0x17,0x88,0x6A,0x66,0x00,0xB3,0x6A,0x44,0x17,
0x08,0x6A,0x66,0x00,0x8C,0x6A,0x48,0x17,0xFF,0xB7,0xB6,0x1A,0xFF,0xB8,0xB6,0x1A,
0xFF,0xB9,0xB6,0x1A,0xFF,0xA2,0xCB,0x1E,0x00,0x65,0x66,0x17,0x39,0x6A,0x39,0x00,
0x40,0x03,0xBA,0x1A,0x3D,0x6A,0x39,0x00,0xBF,0x39,0x39,0x02,0xFF,0x34,0xBD,0x1E,
0x40,0x39,0x39,0x00,0xFF,0x39,0x93,0x02,0x10,0x0C,0xA4,0x1A,0x01,0x0B,0xBE,0x1E,
0x80,0x04,0xC3,0x1E,0x01,0x0C,0xC1,0x1E,0x10,0x0C,0xA6,0x1A,0xFF,0x34,0xC8,0x1E,
0x00,0x65,0x4F,0x17,0x00,0x65,0xBA,0x10,0x01,0x6A,0x03,0x17,0x00,0x65,0xAC,0x10,
0xAF,0x6A,0x66,0x00,0x00,0x65,0x4B,0x17,0xC7,0x93,0x93,0x02,0x00,0x65,0x66,0x17,
0x31,0x6A,0x91,0x00,0x00,0x65,0xCB,0x10,0xFF,0xAF,0xEB,0x1A,0xFF,0xB0,0xEB,0x1A,
0xFF,0xB1,0xEB,0x1A,0xFF,0xB2,0xEB,0x1A,0xFF,0x06,0xAE,0x02,0x00,0x65,0x61,0x17,
0xE0,0x64,0x6B,0x19,0xFF,0x12,0xD7,0x1E,0x41,0x6A,0x91,0x00,0xFF,0xAE,0xE4,0x1A,
0x00,0x65,0x6C,0x17,0x00,0x65,0xA4,0x17,0xFF,0x66,0x65,0x02,0xFF,0x6C,0x64,0x02,
0x7F,0x64,0xDE,0x1C,0x80,0x64,0x6D,0x00,0x57,0x6A,0xA1,0x17,0x01,0x57,0x57,0x06,
0x02,0x6A,0x91,0x00,0xFF,0x6A,0x3C,0x00,0x5B,0x6A,0xA0,0x17,0x00,0x65,0x07,0x10,
0x51,0x6A,0x91,0x00,0xF7,0x01,0x01,0x02,0x06,0x6A,0x06,0x00,0x20,0xA0,0xE9,0x1E,
0x0D,0x6A,0x06,0x00,0x08,0x01,0x01,0x00,0x00,0x65,0xD8,0x10,0x34,0x6A,0x66,0x00,
0xA2,0x6A,0x46,0x17,0xFF,0x34,0x64,0x02,0x01,0x64,0x64,0x06,0x00,0xB2,0xF3,0x18,
0x00,0x6A,0xB2,0x02,0x00,0x65,0xF8,0x10,0x00,0x65,0x4F,0x17,0xFF,0x34,0x64,0x02,
0x00,0xB2,0xF2,0x18,0x00,0x6A,0xB2,0x02,0xFF,0x34,0x01,0x1F,0x00,0x65,0x4F,0x17,
0xFF,0x08,0x64,0x02,0x00,0xAF,0xAF,0x06,0xFF,0x09,0x64,0x02,0x00,0xB0,0xB0,0x08,
0xFF,0x0A,0x64,0x02,0x00,0xB1,0xB1,0x08,0xFF,0xB2,0x64,0x02,0x00,0x6A,0xB2,0x08,
0x00,0x65,0xF6,0x10,0x31,0x6A,0x91,0x00,0x00,0x65,0xD2,0x10,0xFF,0x65,0xBA,0x02,
0x02,0x12,0x0A,0x1D,0x04,0x12,0x11,0x1D,0x01,0x12,0x1A,0x1D,0x03,0x12,0x36,0x1D,
0x08,0x12,0x35,0x1D,0x41,0x6A,0x91,0x01,0xA2,0x6A,0x66,0x00,0x34,0x6A,0x46,0x17,
0xB3,0x6A,0x66,0x00,0x14,0x6A,0x47,0x17,0x08,0x6A,0x48,0x17,0xFF,0x06,0x6A,0x02,
0x00,0x65,0xB6,0x10,0x40,0xA0,0x09,0x1F,0x32,0x6A,0x39,0x17,0x00,0x6E,0x09,0x1B,
0x00,0x3C,0x77,0x17,0x1B,0x6A,0x39,0x00,0xA0,0x6A,0x64,0x00,0x88,0x6A,0x8C,0x17,
0x00,0x65,0x6C,0x17,0x00,0x65,0x07,0x10,0x00,0x65,0x60,0x17,0xE0,0x64,0x71,0x19,
0xFF,0x12,0x3A,0x02,0x00,0x65,0x60,0x17,0xE0,0x64,0x71,0x19,0x05,0x3A,0x33,0x19,
0xFF,0x12,0x33,0x1B,0x80,0xA0,0x24,0x1F,0x41,0x6A,0x91,0x00,0x00,0x65,0x0F,0x11,
0x35,0x6A,0x66,0x00,0x00,0x65,0x60,0x17,0xE0,0x64,0x71,0x19,0xFF,0x12,0x6D,0x02,
0x39,0x66,0x25,0x19,0xFF,0x38,0x64,0x02,0x00,0x14,0x88,0x06,0xFF,0x37,0x64,0x02,
0x00,0x15,0x89,0x08,0xFF,0x36,0x64,0x02,0x00,0x16,0x8A,0x08,0xFF,0x35,0x64,0x02,
0x00,0x17,0x8B,0x08,0x02,0x01,0x01,0x00,0x00,0x65,0x0F,0x11,0x71,0x6A,0x91,0x00,
0xFF,0x6A,0x6A,0x08,0xFF,0x06,0x6A,0x03,0x00,0x65,0x60,0x17,0x80,0x64,0x8E,0x1C,
0xFF,0x6A,0x6A,0x03,0xFF,0xA1,0x64,0x02,0x88,0x64,0x3C,0x1F,0x01,0x65,0x65,0x06,
0xFF,0x64,0x6E,0x02,0xFF,0x6C,0x64,0x03,0x1A,0x6A,0x01,0x00,0x4C,0x65,0x65,0x0A,
0x08,0xA1,0x42,0x1F,0x08,0x65,0x65,0x00,0x20,0x65,0x65,0x06,0xFF,0x6C,0x04,0x03,
0xFF,0x6C,0x6D,0x02,0xFF,0x6C,0x6D,0x02,0xFF,0x6C,0x6D,0x02,0xFF,0x6C,0x6D,0x02,
0xFF,0x6C,0x6D,0x02,0xFF,0x6C,0x6D,0x02,0xFF,0x6C,0x6D,0x03,0x00,0x6A,0x6D,0x02,
0x00,0x6A,0x6D,0x02,0x00,0x6A,0x6D,0x02,0x00,0x6A,0x6D,0x03,0xC7,0x93,0x93,0x02,
0x38,0x93,0x50,0x1B,0x88,0x6A,0x66,0x00,0x35,0x6A,0x47,0x17,0x08,0x6A,0x6D,0x00,
0x00,0x65,0x4D,0x17,0x0D,0x93,0x93,0x00,0x08,0x94,0x56,0x1F,0xF7,0x93,0x93,0x02,
0x35,0x6A,0x66,0x00,0x88,0x6A,0x47,0x17,0xFF,0x34,0x34,0x06,0x88,0x6A,0x66,0x00,
0xFF,0x99,0x6D,0x02,0x8F,0x66,0x5C,0x19,0x08,0x6A,0x66,0x00,0x8C,0x6A,0x48,0x11,
0xFF,0x06,0x6A,0x02,0x01,0x0C,0x61,0x1F,0x04,0x0C,0x61,0x1B,0xE0,0x03,0x03,0x02,
0xE0,0x03,0x64,0x03,0x21,0x6A,0x91,0x00,0x00,0x65,0x61,0x17,0xA0,0x64,0x71,0x1F,
0xE0,0x64,0xC6,0x1C,0xC0,0x64,0xCE,0x1C,0xA0,0x64,0x65,0x1D,0x61,0x6A,0x91,0x00,
0x00,0x6A,0x04,0x02,0xF7,0x11,0x11,0x02,0xFF,0x06,0x6A,0x02,0x09,0x0C,0x6F,0x1F,
0x08,0x0C,0x6B,0x1F,0xFF,0x6A,0x6A,0x03,0xFF,0xAF,0xAF,0x06,0xFF,0xAF,0x72,0x1B,
0xFF,0xB0,0xB0,0x06,0xFF,0xB0,0x72,0x19,0x01,0x6A,0x91,0x00,0x02,0x65,0x65,0x0A,
0xFC,0x65,0x64,0x02,0x03,0x65,0x65,0x02,0x00,0x3E,0x88,0x06,0xFF,0x65,0x64,0x02,
0x00,0x3F,0x89,0x08,0x00,0x6A,0x64,0x02,0x00,0x40,0x8A,0x08,0x00,0x41,0x8B,0x08,
0x88,0x6A,0x65,0x00,0x04,0x6A,0x39,0x00,0x88,0x6A,0x64,0x00,0x00,0x65,0xA8,0x17,
0x0D,0x93,0x93,0x00,0x08,0x94,0x85,0x1F,0xF7,0x93,0x93,0x02,0xFF,0x64,0x66,0x02,
0x00,0x39,0x64,0x06,0xFF,0x99,0x6D,0x02,0x00,0x66,0x89,0x19,0xFF,0x6A,0x6A,0x03,
0x00,0x65,0xA8,0x17,0x01,0x6A,0x93,0x00,0xFF,0x64,0x65,0x02,0x00,0x39,0x64,0x06,
0xFF,0x6C,0x99,0x02,0x00,0x65,0x90,0x19,0x0A,0x93,0x93,0x00,0x08,0x94,0x93,0x1F,
0xF7,0x93,0x93,0x03,0x00,0x65,0x77,0x17,0x20,0x6A,0x39,0x00,0x00,0x65,0x9A,0x11,
0x00,0x65,0x77,0x17,0x02,0x6A,0x39,0x00,0xA0,0x6A,0x64,0x00,0x88,0x6A,0x83,0x11,
0xFF,0x65,0x5B,0x02,0x01,0x6A,0x39,0x00,0x34,0x6A,0x64,0x00,0x5B,0x6A,0x83,0x11,
0xFF,0xA1,0x5B,0x02,0x01,0x6A,0x39,0x00,0x3C,0x6A,0x64,0x00,0x00,0x65,0x8C,0x11,
0x4C,0xA1,0x66,0x0A,0x00,0x65,0xA7,0x15,0x08,0x66,0x66,0x00,0x47,0x66,0x66,0x07,
0x88,0x6A,0x66,0x00,0x00,0x65,0x47,0x17,0xFF,0x39,0x6D,0x02,0x00,0x65,0x4D,0x11,
0xF7,0x1F,0x1F,0x02,0x20,0x11,0x11,0x00,0x08,0x1F,0x1F,0x04,0x20,0x11,0x11,0x00,
0x00,0x65,0x07,0x10,
};


#ident	"@(#)xpr:devices/terminfo/R_12.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "Xlib.h"

#include "xpr.h"

#include "xpr_term.h"
#include "text.h"

static Word		bits[] = {
	0x000000fe,0x003f800f,0xf0000000,0x00003e00,0x00000000,
	0x01fe007f,0x80000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000100,0x00000000,
	0x00000000,0x00000015,0x0000003f,0xf8000000,0x0000ff80,
	0x00000000,0x02aa00aa,0x80000000,0x00000000,0x00000000,
	0x00000000,0x00000400,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x01800100,
	0x00c00000,0x00000000,0x000007ff,0x81ffe070,0x0c000000,
	0x0003ffc0,0x00000000,0x07ff81ff,0xe0000000,0x00000001,
	0x68000000,0x00112000,0x06400400,0x00800030,0x00600000,
	0x00000000,0x00000000,0x00000000,0x0e000000,0x00e00030,
	0x00010003,0xe0001800,0x7e000c00,0x03800000,0x0000000e,
	0x01fe0100,0x0fe00039,0x00fe003f,0xe00ff000,0x3800f1e0,
	0x3c0007c0,0x03c700f0,0x0038070c,0x0e003800,0xfe000383,
	0xf80ebff0,0x0f0e03c3,0x80f78e3c,0x380f0e01,0xff007039,
	0x80020000,0x18000000,0x00180000,0x00060000,0x00018000,
	0x60001800,0x06000180,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x03000100,0x00600000,0x00000000,0x00000015,0x400000ff,
	0xfe000000,0x000399e0,0x00000000,0x00b5402a,0xa0000000,
	0x00000003,0xfc000000,0x0019b000,0x06c01f00,0x03fc0078,
	0x0060000c,0x000c0000,0x60000400,0x00000000,0x9b000300,
	0x01f0007c,0x00030003,0xe0007000,0xfe001f00,0x07c00000,
	0x0000001f,0x03030100,0x063000ff,0x00638018,0x20060800,
	0xff0060c0,0x18000180,0x01840060,0x001c0e0e,0x0400c600,
	0x63000fe1,0x8c1ba310,0x06040181,0x00638418,0x10060402,
	0x02007439,0xc0060000,0x38000000,0x00380000,0x001f0000,
	0x00038000,0x60001800,0x0e000380,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x02000100,0x0020003c,0x00000000,0x00001fff,
	0xe7fff8e0,0x06000000,0x00077f70,0x00000000,0x1fffe7ff,
	0xf8000000,0x00000006,0x0c000000,0x00192000,0x04803500,
	0x033c0048,0x00600018,0x00060001,0x98000400,0x00000000,
	0xb1800700,0x03f000dc,0x00050006,0x0000c000,0x86003100,
	0x0c600007,0x00038011,0x86018380,0x061801c3,0x0060c018,
	0x20060001,0xc30060c0,0x18000180,0x01880060,0x001c0e07,
	0x04018300,0x61801c71,0x86318300,0x06040181,0x0063840c,
	0x20030800,0x0600440b,0x40040000,0x18000000,0x00180000,
	0x00190000,0x00018000,0x00000000,0x06000180,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x02000100,0x0020003c,0x00000000,
	0x0000000a,0xa00000ff,0xfe000000,0x00067f30,0x00000000,
	0x20154805,0x50000000,0x00000004,0x04000000,0x00192000,
	0x0c803400,0x06080048,0x00200010,0x00020001,0xf8000400,
	0x00000000,0xb1800300,0x0210008c,0x00050006,0x00018000,
	0x84003100,0x0c60001e,0x0001e011,0x8c348380,0x06180181,
	0x0060c018,0x00060003,0x810060c0,0x18000180,0x01900060,
	0x001e1e07,0x84030180,0x61803831,0x86308300,0x060400c3,
	0x00318406,0x20018800,0x0c00440a,0x00060000,0x18000000,
	0x00180000,0x00180000,0x00018000,0x00000000,0x06000180,
	0x00000000,0x00000000,0x00000000,0x00000000,0x01800000,
	0x00000000,0x00000000,0x00000000,0x02000100,0x00200000,
	0x00000000,0x5cc03fff,0xfffffcc0,0x02000000,0x000c6730,
	0xf8007c00,0x3fffffff,0xfce800e7,0x0000000c,0x04000000,
	0x00192000,0x7fe03c00,0x0658007b,0x80400030,0x00030001,
	0xf8000400,0x00000001,0x31800300,0x0010000c,0x00090007,
	0x80036000,0x04003900,0x0c6660f8,0x3ff07c11,0x98f406c0,
	0x06380301,0x00606018,0x40061003,0x000060c0,0x18000180,
	0x01b00060,0x00161605,0xc4030180,0x61803019,0x86380300,
	0x060400c2,0x00318806,0xc0019000,0x18004408,0x00067800,
	0x1b000780,0x01f8003c,0x003e0003,0x8001f000,0x60003800,
	0x06600180,0x005ce017,0x00038001,0xf0003e00,0x1e000700,
	0x03c000ef,0x0039c00e,0xe603b800,0xe7003f00,0x02000100,
	0x00200000,0x00000000,0xfde00005,0x500000ff,0xfe000000,
	0x000cc319,0xb800fe00,0x002ab002,0xa9180042,0x0000000c,
	0x00000000,0x00100000,0x7fe01e00,0x04530073,0x80000030,
	0x00030001,0x98000400,0x00000001,0x31800300,0x00100038,
	0x00110007,0xc003f000,0x0c001e00,0x0c6663c0,0x3ff00f03,
	0x118c04c0,0x07f00300,0x0060601f,0xc007f003,0x000060c0,
	0x18000180,0x01e00060,0x00171604,0xe4030180,0x63003019,
	0x8c1e0300,0x06040062,0x0031c803,0x8000d000,0x38004208,
	0x0000ec00,0x1d800ec0,0x01180066,0x00180006,0xe0019800,
	0xe0001800,0x06400180,0x00fff839,0x8006c003,0x98006600,
	0x3a000880,0x01800066,0x00188004,0x42019000,0xe2002300,
	0x02000100,0x00200000,0x00000000,0x63303fff,0xfffffce0,
	0x06000000,0x000cc31b,0x18006300,0x3fffffff,0xfe080022,
	0x0000000c,0x00000000,0x00100000,0x1b000f00,0x07b781f3,
	0x00000020,0x00010001,0x68007fe0,0x00000001,0x31800300,
	0x0030003c,0x00110000,0xc0023800,0x0c001e00,0x0e644600,
	0x00000382,0x118c0440,0x06300300,0x00606018,0x40061003,
	0x07807fc0,0x18000180,0x01e00060,0x00132604,0x74030180,
	0x7e003019,0xfc0f0300,0x06040064,0x001ac801,0x8000e000,
	0x70004208,0x00008c00,0x18c00c40,0x031800c2,0x00180004,
	0x40019800,0x60001800,0x06800180,0x00631819,0x800c6001,
	0x8c00c600,0x18000c00,0x01800066,0x00188006,0x6400e000,
	0x62000600,0x0e000100,0x00380000,0x00000000,0x63300000,
	0x0000007f,0xfe000000,0x000dc19a,0x18006300,0x30aaa80a,
	0xaa080024,0x0000000c,0x00000000,0x00000000,0xffc00780,
	0x032c8332,0x00000020,0x00010000,0x60000400,0x00000f02,
	0x31800300,0x0060000c,0x00210000,0x60020800,0x08003700,
	0x07e00780,0x3ff00782,0x11088c60,0x06180300,0x00606018,
	0x00060003,0x030060c0,0x18000180,0x01b80060,0x00132604,
	0x74030180,0x60003019,0x98038300,0x06040074,0x001af003,
	0xc0006000,0x60004308,0x00003c00,0x18c00c00,0x031800fe,
	0x00180004,0x40019800,0x60001800,0x07000180,0x00631819,
	0x800c6001,0x8c00c600,0x18000700,0x01800066,0x00090003,
	0x64006000,0x34000c00,0x02000100,0x00200000,0x00000000,
	0x63303fff,0xfffffc78,0x0e000000,0x000dff9a,0x18006300,
	0x3fffffff,0xfe080014,0x00000004,0x00000000,0x00000000,
	0xffc00580,0x0058833e,0x00000020,0x00010000,0x00000400,
	0x00000f02,0x31800300,0x00400004,0x003fc000,0x60020800,
	0x18002300,0x006001f0,0x3ff03c00,0x11188fe0,0x06180100,
	0x00604018,0x00060003,0x030060c0,0x18000180,0x019c0060,
	0x0011c604,0x3c030180,0x60003019,0x8c018300,0x0604003c,
	0x001e7004,0xe0006000,0xe0004108,0x00006c00,0x18c00c00,
	0x031800c0,0x00180006,0xc0019800,0x60001800,0x07800180,
	0x00631819,0x800c6001,0x8c00c600,0x18000300,0x01800066,
	0x000d0003,0xb8006000,0x3c000c00,0x02000100,0x00200000,
	0x00000000,0x63300000,0x0000003f,0xfc000000,0x000dffb2,
	0x18006300,0x15555415,0x56080018,0x00000006,0x04000000,
	0x00000000,0x36002580,0x00d8831c,0x00000020,0x00010000,
	0x00000400,0x00000002,0x31800300,0x00800004,0x003fc000,
	0x40030800,0x18002100,0x00c0003c,0x0000f000,0x11191830,
	0x06180181,0x0060c018,0x20060003,0x830060c0,0x18000180,
	0x018e0060,0x0011c604,0x1c018300,0x60001831,0x8c208300,
	0x06040038,0x000c7008,0x60006001,0xc0004108,0x0000cc00,
	0x18c00c00,0x031800c0,0x00180007,0x80019800,0x60001800,
	0x06c00180,0x00631819,0x800c6001,0x8c00c600,0x18000180,
	0x01800066,0x000e0003,0x3800b000,0x18001800,0x02000100,
	0x00200000,0x00000000,0x63301fff,0xe7fff81f,0xf8000000,
	0x0007ffb3,0x38006600,0x3fffefff,0xf9180008,0x00000003,
	0x0c000000,0x00180000,0x26002580,0x0099039f,0xc0000030,
	0x00030000,0x00000400,0x18000034,0x1b000300,0x01f800cc,
	0x0001000c,0xc0039800,0x10003100,0x0186600f,0x00038006,
	0x19ef1030,0x063800c3,0x00618018,0x20060001,0xc30060c0,
	0x18000d80,0x01870060,0x80108604,0x0c00c600,0x60001c71,
	0x86318300,0x030c0018,0x000c2010,0x30006003,0x81004088,
	0x0000cc00,0x19c00ec0,0x01180062,0x00180006,0x00019800,
	0x60001800,0x06600180,0x00631819,0x8006c001,0x98006600,
	0x18000880,0x01800066,0x00060001,0x10011800,0x18003100,
	0x02000100,0x00200000,0x00000000,0xe7780000,0x00000007,
	0xf8000000,0x000799f1,0xf8007e00,0x15554555,0x50e80018,
	0x00000001,0xf8000000,0x00180000,0x6c003f00,0x018e03f7,
	0x80000018,0x00030000,0x00000400,0x18000034,0x0e0007c0,
	0x03f800f8,0x0001000f,0x8001f000,0x10001e00,0x0f066001,
	0x00020006,0x0ec53878,0x0ff0007e,0x00ff003f,0xe00f0000,
	0xff00f1e0,0x3c000f00,0x03ef80ff,0x80388f0e,0x04007c00,
	0xf00007c3,0xe7bf0780,0x03f80010,0x0008203c,0x7800f003,
	0xfe004088,0x1fe0fe00,0x3f000780,0x00ec003c,0x003c000f,
	0xe003bc00,0xf0001800,0x0e7003c0,0x00e73839,0xc0038001,
	0xf0003e00,0x3c000f00,0x01c0003f,0x00040001,0x10031c00,
	0x18003f00,0x02000100,0x00200000,0x00000000,0x00000fff,
	0xe3fff801,0xf0000000,0x0001ffc0,0x18006000,0x0fffe3ff,
	0xf8080010,0x00000000,0x00000000,0x00000000,0x00000400,
	0x00000000,0x00000008,0x00040000,0x00000000,0x18000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00006000,0x00000000,0x03de0000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x000001c0,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00004008,0x1fe00000,0x00000000,0x00000000,
	0x00000008,0x20000000,0x00001800,0x00000000,0x00000000,
	0x00000001,0x80000600,0x00000000,0x00000000,0x00000000,
	0x00000000,0x10000000,0x02000100,0x00200000,0x00000000,
	0x00000000,0x00000001,0xe0000000,0x0000ff80,0x18006000,
	0x02aa80aa,0xa00800a0,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000004,0x00080000,0x00000000,
	0x10000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00004000,0x00000000,0x00f80000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x000000f8,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00007038,0x00000000,0x00000000,
	0x00000000,0x0000000c,0x60000000,0x00003000,0x00000000,
	0x00000000,0x00000001,0x80000600,0x00000000,0x00000000,
	0x00000000,0x00000000,0xe0000000,0x03000100,0x00600000,
	0x00000000,0x000007ff,0x81ffe003,0xe0000000,0x00003c00,
	0x3c00f000,0x07ff81ff,0xe01c0040,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000007,0xc0000000,0x00000000,
	0x00000000,0x00000000,0x00000003,0x80000f00,0x00000000,
	0x00000000,0x00000000,0x00000000,0xc0000000,0x01800100,
	0x00c00000,0x00000000,0x00000000,0x00000007,0xc0000000,
	0x00000000,0x00000000,0x035500d5,0x40000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000100,0x00000000,0x00000001,0x000000fe,0x003f8007,
	0xc0000000,0x00000000,0x00000000,0x00fe003f,0x80000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000001,
};

static XImage		image = {
	1790,				/* width */
	19,				/* height */
	0,				/* xoffset */
	XYBitmap,			/* format */
	(char *)bits,			/* data */
	LSBFirst,			/* byte_order */
	WORDSIZE,			/* bitmap_unit */
	MSBFirst,			/* bitmap_bit_order */
	WORDSIZE,			/* bitmap_pad */
	1,				/* depth */
	ROUNDUP(1790, WORDSIZE) / 8,	/* bytes_per_line */
	1,				/* bits_per_pixel */
};

struct {
	short			n;
	char			height,
				ascent;
	XImage			*image;
	_Fontchar		info[129];
}			R_12 = {
	127,
	 19,
	 13,
	&image,
	{
		{   0,  13,  13,   0,   0 },
		{   0,  13,  13,   0,   0 },
		{   0,  13,  13,   0,   0 },
		{   0,   6,  14,   0,   0 },
		{  18,  13,  13,   0,   0 },
		{  18,  13,  13,   0,   0 },
		{  18,   0,  19,   0,   0 },
		{  36,   0,  19,   0,  18 },
		{  54,   0,   0,   0,   0 },
		{  54,   0,  19,   0,   0 },
		{  72,   0,   0,   0,   0 },
		{  90,   0,   0,   0,   0 },
		{  90,   0,   0,   0,   0 },
		{ 108,   0,  17,   0,   0 },
		{ 126,   0,   0,   0,   0 },
		{ 126,   6,  17,   0,   0 },
		{ 144,   6,  17,   0,   0 },
		{ 162,   0,   0,   0,   0 },
		{ 162,   0,   0,   0,   0 },
		{ 162,   0,  19,   0,   0 },
		{ 180,   0,  19,   0,   0 },
		{ 198,   6,  17,   0,   0 },
		{ 216,   6,  17,   0,   0 },
		{ 234,   0,   0,   0,   0 },
		{ 234,   0,   0,   0,   0 },
		{ 234,   0,   0,   0,   0 },
		{ 234,   0,   0,   0,   0 },
		{ 234,   0,   0,   0,   0 },
		{ 252,   0,   0,   0,   0 },
		{ 252,   0,   0,   0,   0 },
		{ 252,   2,  14,   0,   0 },
		{ 270,   0,   0,   0,   0 },
		{ 288,   0,  19,   0,   9 },
		{ 297,   2,  14,   0,   5 },
		{ 302,   2,   7,   0,   7 },
		{ 320,   2,  14,   0,  12 },
		{ 338,   1,  15,   0,   8 },
		{ 356,   2,  14,   0,  14 },
		{ 374,   2,  14,   0,  13 },
		{ 392,   2,   7,   0,   4 },
		{ 410,   3,  16,   0,   5 },
		{ 428,   3,  16,   0,   5 },
		{ 446,   3,  10,   0,   8 },
		{ 464,   3,  14,   0,  12 },
		{ 482,  12,  16,   0,   4 },
		{ 500,   9,  13,   0,   5 },
		{ 505,  12,  14,   0,   4 },
		{ 509,   3,  14,   0,   5 },
		{ 514,   2,  14,   0,   8 },
		{ 532,   3,  14,   0,   7 },
		{ 550,   2,  14,   0,   8 },
		{ 568,   2,  14,   0,   7 },
		{ 586,   2,  14,   0,   9 },
		{ 604,   2,  14,   0,   8 },
		{ 622,   2,  14,   0,   8 },
		{ 640,   2,  14,   0,   8 },
		{ 658,   2,  14,   0,   7 },
		{ 676,   2,  14,   0,   8 },
		{ 684,   6,  14,   0,   4 },
		{ 688,   6,  16,   0,   4 },
		{ 692,   4,  14,   0,  13 },
		{ 705,   6,  13,   0,  12 },
		{ 717,   4,  14,   0,  13 },
		{ 730,   2,  14,   0,   8 },
		{ 738,   2,  16,   0,  16 },
		{ 754,   2,  14,   0,  12 },
		{ 772,   2,  14,   0,  10 },
		{ 790,   2,  14,   0,  11 },
		{ 808,   2,  14,   0,  12 },
		{ 826,   2,  14,   0,  11 },
		{ 844,   2,  14,   0,  10 },
		{ 862,   2,  14,   0,  12 },
		{ 880,   2,  14,   0,  12 },
		{ 898,   2,  14,   0,   6 },
		{ 916,   2,  14,   0,   7 },
		{ 934,   2,  14,   0,  12 },
		{ 952,   2,  14,   0,  11 },
		{ 970,   2,  14,   0,  15 },
		{ 988,   2,  14,   0,  12 },
		{ 1006,   2,  14,   0,  12 },
		{ 1024,   2,  14,   0,  10 },
		{ 1042,   2,  16,   0,  12 },
		{ 1054,   2,  14,   0,  12 },
		{ 1066,   2,  14,   0,   8 },
		{ 1074,   2,  14,   0,  11 },
		{ 1092,   2,  14,   0,  12 },
		{ 1110,   2,  14,   0,  12 },
		{ 1128,   2,  14,   0,  16 },
		{ 1146,   2,  14,   0,  12 },
		{ 1164,   2,  14,   0,  12 },
		{ 1182,   2,  14,   0,  11 },
		{ 1200,   2,  16,   0,   5 },
		{ 1205,   3,  14,   0,   5 },
		{ 1210,   2,  16,   0,   4 },
		{ 1214,   2,  13,   0,   5 },
		{ 1219,  13,  15,   0,   9 },
		{ 1228,   2,  13,   0,   4 },
		{ 1232,   6,  14,   0,   8 },
		{ 1250,   2,  14,   0,   9 },
		{ 1268,   6,  14,   0,   7 },
		{ 1286,   2,  14,   0,   9 },
		{ 1304,   6,  14,   0,   8 },
		{ 1322,   2,  14,   0,   7 },
		{ 1340,   6,  17,   0,   8 },
		{ 1358,   2,  14,   0,   9 },
		{ 1376,   2,  14,   0,   5 },
		{ 1394,   2,  16,   0,   4 },
		{ 1412,   2,  14,   0,   9 },
		{ 1430,   2,  14,   0,   5 },
		{ 1448,   6,  14,   0,  14 },
		{ 1466,   6,  14,   0,   9 },
		{ 1484,   6,  14,   0,   8 },
		{ 1502,   6,  17,   0,   9 },
		{ 1520,   6,  17,   0,   9 },
		{ 1538,   6,  14,   0,   6 },
		{ 1556,   6,  14,   0,   6 },
		{ 1574,   5,  14,   0,   5 },
		{ 1592,   6,  14,   0,   9 },
		{ 1610,   6,  14,   0,   9 },
		{ 1628,   6,  14,   0,  12 },
		{ 1646,   6,  14,   0,   9 },
		{ 1664,   6,  17,   0,   9 },
		{ 1682,   6,  14,   0,   7 },
		{ 1700,   1,  17,   0,   6 },
		{ 1718,   0,  18,   0,   3 },
		{ 1736,   1,  17,   0,   6 },
		{ 1754,   3,   5,   0,   5 },
		{ 1772,   0,   0,   0,   0 },
		{ 1790,   0,   0,   0,   0 },
	}
};

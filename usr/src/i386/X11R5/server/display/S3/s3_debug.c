/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/s3_debug.c	1.2"
#if (defined(__DEBUG__))

#include <sys/types.h>
#include <sys/inline.h>

#if (!defined(lint))
/*
 * function equivalents for writing out to registers to check the drawing.
 */

unsigned short
in_word(unsigned short regaddr)
{
	return ((unsigned short)inw(regaddr));
}

unsigned char
in_byte(unsigned short regaddr)
{
	return ((unsigned char)inb(regaddr));
}

unsigned long
in_long(unsigned short regaddr)
{
	return ((unsigned long)inl(regaddr));
}

void 
out_long(unsigned short regaddr, unsigned long regval)
{
	outl(regaddr,regval);
}

void 
out_byte(unsigned short regaddr, unsigned char regval)
{
	outb(regaddr,regval);
}

void 
out_word(unsigned short regaddr, unsigned short regval)
{
	outw(regaddr,regval);
}

void
screen_on()
{
		unsigned char 	seq_1;
		outb(0x3c4,1);
		seq_1 = inb(0x3c5);
		seq_1 &= ~0x20;
		outb(0x3c4,1);
		outb(0x3c5,seq_1);
}

screen_off()
{
		unsigned char 	seq_1;
		outb(0x3c4,1);
		seq_1 = inb(0x3c5);
		seq_1 |= 0x20;
		outb(0x3c4,1);
		outb(0x3c5,seq_1);
}

#endif /* !defined lint */

#endif

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:vprobe/AsmMacros.h	1.1"

/*
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of David Wexelblat not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  David Wexelblat makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * DAVID WEXELBLAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL DAVID WEXELBLAT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86: mit/server/ddx/x386/SuperProbe/AsmMacros.h,v 2.0 1993/09/22 15:42:14 dawes Exp $ */

#ifdef __GNUC__

#ifdef GCCUSESGAS
static __inline__ void
outb(port, val)
short port;
char val;
{
   __asm__ __volatile__("outb %0,%1" : :"a" (val), "d" (port));
}


static __inline__ void
outw(port, val)
short port;
short val;
{
   __asm__ __volatile__("outw %0,%1" : :"a" (val), "d" (port));
}

static __inline__ unsigned int
inb(port)
short port;
{
   unsigned char ret;
   __asm__ __volatile__("inb %1,%0" :
       "=a" (ret) :
       "d" (port));
   return ret;
}

static __inline__ unsigned int
inw(port)
short port;
{
   unsigned short ret;
   __asm__ __volatile__("inw %1,%0" :
       "=a" (ret) :
       "d" (port));
   return ret;
}

#else /* GCCUSESGAS */

static __inline__ void
outb(port, val)
     short port;
     char val;
{
  __asm__ __volatile__("out%B0 (%1)" : :"a" (val), "d" (port));
}

static __inline__ void
outw(port, val)
     short port;
     short val;
{
  __asm__ __volatile__("out%W0 (%1)" : :"a" (val), "d" (port));
}

static __inline__ unsigned int
inb(port)
     short port;
{
  unsigned int ret;
  __asm__ __volatile__("in%B0 (%1)" :
                   "=a" (ret) :
                   "d" (port));
  return ret;
}

static __inline__ unsigned int
inw(port)
     short port;
{
  unsigned int ret;
  __asm__ __volatile__("in%W0 (%1)" :
                   "=a" (ret) :
                   "d" (port));
  return ret;
}

#endif /* GCCUSESGAS */

#ifdef linux

#define intr_disable()
#define intr_enable()

#else /* !linux */

static __inline__ void
intr_disable()
{
  __asm__ __volatile__("cli");
}

static __inline__ void
intr_enable()
{
  __asm__ __volatile__("sti");
}

#endif /* else !linux */

#else /* __GNUC__ */

#if defined(_MINIX) && defined(_ACK)

/* inb, outb, inw and outw are defined in the library */

u8_t inb(U16_t);
void outb(U16_t, U8_t);
u16_t inw(U16_t);
void outw(U16_t, U16_t);

#else /* not _MINIX and _ACK */

# if defined(__STDC__) && (__STDC__ == 1)
#  define asm __asm
# endif
# ifdef SVR4
#  include <sys/types.h>
#  ifndef __USLC__
#   define __USLC__
#  endif
# endif
#include <sys/inline.h>
#define intr_disable() asm("cli")
#define intr_enable()  asm("sti")

#endif /* _MINIX and _ACK */
#endif /* __GNUC__ */

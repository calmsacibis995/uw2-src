/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)vtools:vprobe/OS_SYSV.c	1.6"

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

/* $XFree86: mit/server/ddx/x386/SuperProbe/OS_SYSV.c,v 2.3 1993/09/21 15:20:38 dawes Exp $ */

#include "Probe.h"

#include <fcntl.h>
#if defined(SCO) || defined(ESIX)
# include <sys/types.h>
#endif
#ifdef SYSV
# if !defined(O_NDELAY) && defined(O_NONBLOCK)
#  define O_NDELAY O_NONBLOCK
# endif
# ifndef SCO
#  include <sys/at_ansi.h>
# endif
# include <sys/param.h>
#endif
#ifdef SCO
# include <sys/vtkd.h>
#else
# include <sys/kd.h>
# include <sys/vt.h>
#endif
#include <sys/immu.h>
#include <sys/proc.h>
#include <sys/tss.h>
#include <sys/sysi86.h>
#ifdef SVR4
# include <sys/seg.h>
#endif
/*
 * v86.h doesn't exist in ESMP
 */
#ifndef SI86IOPL					/* ESMP */
#include <sys/v86.h>
#endif

/* 
 * Detect ESMP vs standard SVR4.2.  SI86IOPL is a new sysi86 function 
 * in ESMP.  Prior to ESMP, the IOPL was set using a VPIX 
 * sub-function. 
 * For more info, see MR ul93-25930
 */
#ifdef SI86IOPL
#define SET_IOPL(iopl) sysi86(SI86IOPL, (iopl)>>12)
#else
#define SET_IOPL(iopl) sysi86(SI86V86, V86SC_IOPL, (iopl))
#endif


#ifdef __STDC__
int sysi86(int, ...);
int syscall(int, ...);
int munmap(caddr_t, size_t);
#else
int sysi86();
int syscall();
int munmap();
#endif

#ifdef SVR4
# include <sys/mman.h>
# define DEV_MEM 	"/dev/pmem"
#else
# define DEV_MEM	"/dev/mem"
#endif

static int VT_fd = -1;
static int VT_num = -1;
static int BIOS_fd = -1;

/*
 * OpenVideo --
 *
 * Enable access to the installed video hardware.  For SYSV, open a new
 * VT if not running on a VT.  Also, disable IO protection, since we may
 * need to get at extended registers.
 */
#ifdef __STDC__
int OpenVideo(void)
#else
int OpenVideo()
#endif
{
	int fd;
	char fn[20];

#if 0
	if (geteuid() != 0)
	{
		fprintf(stderr,
			"%s: Must be run as root or installed suid-root\n",
			MyName);
		return(-1);
	}
#endif
	if (ioctl(0, KIOCINFO, 0) == (('k'<<8)|'d'))
	{
		VT_fd = 0;
	}
	else 
	{
		if ((fd = open("/dev/console", O_WRONLY, 0)) < 0)
		{
			fprintf(stderr, "%s: Cannot open /dev/console\n", 
				MyName);
			return(-1);
		}
		if ((ioctl(fd, VT_OPENQRY, &VT_num) < 0) || (VT_num == -1))
		{
			fprintf(stderr, "%s: No free VTs\n", MyName);
			return(-1);
		}
		close(fd);
		sprintf(fn, "/dev/vt%02d", VT_num);
		if ((VT_fd = open(fn, O_RDWR|O_NDELAY, 0)) < 0)
		{
			fprintf(stderr, "%s: Could not open VT %s\n", 
				MyName, fn);
			return(-1);
		}
#ifndef SCO
		if (ioctl(VT_fd, VT_WAITACTIVE, VT_num) != 0)
		{
			fprintf(stderr, "%s: VT_WAITACTIVE failed!\n", MyName);
			return(-1);
		}
#endif /* SCO */
		/*
		 * Give the other VT time to release.
		 */
		sleep(1);
	}
	/* (void)sysi86(SI86V86, V86SC_IOPL, PS_IOPL); */
	if (SET_IOPL(PS_IOPL) < 0)
	{
		fprintf(stderr,
			"%s: Must be run as root or installed suid-root\n",
			MyName);
		return(-1);
	}

	return(VT_fd);
}

/*
 * CloseVideo --
 *
 * Disable access to the video hardware.  For SYSV, close the VT if it's open.
 * Also, re-enable IO protection.
 */
#ifdef __STDC__
void CloseVideo(void)
#else
void CloseVideo()
#endif
{
	if (VT_fd != -1)
	{
		/* (void)sysi86(SI86V86, V86SC_IOPL, 0); */
		(void)SET_IOPL(PS_IOPL);
	}
	if (VT_fd > 0)
	{
		close(VT_fd);
	}
}

/*
 * ReadBIOS --
 *
 * Read 'Len' bytes from the video BIOS at address 'Bios_Base'+'Offset' into 
 * buffer 'Buffer'.
 */
#ifdef __STDC__
int ReadBIOS(const unsigned Offset, Byte *Buffer, const int Len)
#else
int ReadBIOS(Offset, Buffer, Len)
unsigned Offset;
Byte *Buffer;
int Len;
#endif
{
#ifndef SVR4
	Word tmp;
	Byte *Base = Bios_Base + Offset;

	if (BIOS_fd == -1)
	{
		if ((BIOS_fd = open(DEV_MEM, O_RDONLY, 0)) < 0)
		{
			fprintf(stderr, "%s: cannot open %s\n", 
				MyName, DEV_MEM);
			return(-1);
		}
	}
	if ((off_t)((off_t)Base & 0x7FFF) != (off_t)0)
	{
		/*
	 	 * Sanity check...
	 	 */
		(void)lseek(BIOS_fd, (off_t)((off_t)Base & 0xF8000), SEEK_SET);
		(void)read(BIOS_fd, &tmp, 2);
		if (tmp != (Word)0xAA55)
		{
			fprintf(stderr, 
				"%s: BIOS sanity check failed, addr=%x\n",
				MyName, (int)Base);
			return(-1);
		}
	}
	if (lseek(BIOS_fd, (off_t)Base, SEEK_SET) < 0)
	{
		fprintf(stderr, "%s: BIOS seek failed\n", MyName);
		return(-1);
	}
	if (read(BIOS_fd, Buffer, Len) != Len)
	{
		fprintf(stderr, "%s: BIOS read failed\n", MyName);
		return(-1);
	}
	return(Len);
#else /* SVR4 */
	Byte *Base = Bios_Base + Offset;
	Word *tmp;
	Byte *ptr;
	unsigned long page_offset = (unsigned long)Base & 0xFFF;
	unsigned long mmap_base = (unsigned long)Base & ~0xFFF;
	unsigned long mmap_len = Len + page_offset;

	if (BIOS_fd == -1)
	{
		if ((BIOS_fd = open(DEV_MEM, O_RDONLY, 0)) < 0)
		{
			fprintf(stderr, "%s: cannot open %s\n", 
				MyName, DEV_MEM);
			return(-1);
		}
	}
	if ((off_t)((off_t)Base & 0x7FFF) != (off_t)0)
	{
		tmp = (Word *)mmap((caddr_t)0, 0x1000, PROT_READ, MAP_SHARED, 
			           BIOS_fd, (off_t)((off_t)Base & 0xF8000));
		if ((int)tmp == -1)
		{
			fprintf(stderr, "%s: Failed to mmap() %s, addr=%x\n",
				MyName, DEV_MEM, (int)Base);
			return(-1);
		}
		if (*tmp != (Word)0xAA55)
		{
			fprintf(stderr, 
				"%s: BIOS sanity check failed, addr=%x\n",
				MyName, (int)Base);
			return(-1);
		}
		(void)munmap((caddr_t)tmp, 0x1000);
	}
	ptr = (Byte *)mmap((caddr_t)0, mmap_len, PROT_READ, MAP_SHARED, 
		   	   BIOS_fd, (off_t)mmap_base);
	if ((int)ptr == -1)
	{
		fprintf(stderr, "%s: Failed to mmap() %s, addr=%x\n",
			MyName, DEV_MEM, (int)Base);
		return(-1);
	}
	(void)memcpy((void *)Buffer, (void *)(ptr + page_offset), Len);
	(void)munmap((caddr_t)ptr, mmap_len);
	return(Len);
#endif /* SVR4 */
}

/*
 * EnableIOPort --
 *
 * Enable access to 'NumPorts' IO ports listed in array 'Ports'.  For SYSV, 
 * we've disabled IO protections so this is a no-op.
 */
#ifdef __STDC__
/*ARGSUSED*/
int EnableIOPorts(const int NumPorts, const Word *Ports)
#else
/*ARGSUSED*/
int EnableIOPorts(NumPorts, Ports)
int NumPorts;
Word *Ports;
#endif
{
	return(0);
}

/*
 * DisableIOPort --
 *
 * Disable access to 'NumPorts' IO ports listed in array  'Ports'.  For SYSV, 
 * we've disabled IO protections so this is a no-op.
 */
#ifdef __STDC__
/*ARGSUSED*/
int DisableIOPorts(const int NumPorts, const Word *Port)
#else
/*ARGSUSED*/
int DisableIOPorts(NumPorts, Port)
int NumPorts;
Word *Port;
#endif
{
	return(0);
}

/*
 * ShortSleep --
 *
 * Sleep for the number of milliseconds specified in 'Delay'.
 */
#ifdef __STDC__
void ShortSleep(const int Delay)
#else
void ShortSleep(Delay)
int Delay;
#endif
{
	syscall(3112, Delay);
}

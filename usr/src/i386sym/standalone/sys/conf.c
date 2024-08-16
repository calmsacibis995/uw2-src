/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* 
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ident	"@(#)stand:i386sym/standalone/sys/conf.c	1.1"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/saio.h>

/*
 * int
 * devread(struct iob *)
 * 	Invoke a device's driver specific read() function via the 
 *	device switch table.
 *
 * Calling/Exit State:
 *	Assumes the driver interface to invoke is initialized in
 *	the device switch table record indexed by the device number
 *	passed in the iob.
 *
 *	The driver specific read() may update fields of the iob, such
 *	as i_error.
 *
 *	Returns the value returned by driver specific read().
 */
int
devread(struct iob *io)
{
	int cc;

	io->i_flgs |= F_RDDATA;
	io->i_error = 0;
	cc = (*devsw[io->i_dev].dv_strategy)(io, READ);
	io->i_flgs &= ~F_TYPEMASK;
	return (cc);
}

/*
 * int
 * devwrite(struct iob *)
 * 	Invoke a device's driver specific write() function via the 
 *	device switch table.
 *
 * Calling/Exit State:
 *	Assumes the driver interface to invoke is initialized in
 *	the device switch table record indexed by the device number
 *	passed in the iob.
 *
 *	The driver specific write() may update fields of the iob, such
 *	as i_error.
 *
 *	Returns the value returned by driver specific write().
 */
int
devwrite(struct iob *io)
{
	int cc;

	io->i_flgs |= F_WRDATA;
	io->i_error = 0;
	cc = (*devsw[io->i_dev].dv_strategy)(io, WRITE);
	io->i_flgs &= ~F_TYPEMASK;
	return (cc);
}

/*
 * void
 * devopen(struct iob *)
 * 	Invoke a device's driver specific open() function via the 
 *	device switch table.
 *
 * Calling/Exit State:
 *	Assumes the driver interface to invoke is initialized in
 *	the device switch table record indexed by the device number
 *	passed in the iob.
 *
 *	The driver specific open() may update fields of the iob, such
 *	as i_error.
 *
 *	No return value.
 */
void
devopen(struct iob *io)
{
	(*devsw[io->i_dev].dv_open)(io);
}

/*
 * int
 * devclose(struct iob *)
 * 	Invoke a device's driver specific close() function via the 
 *	device switch table.
 *
 * Calling/Exit State:
 *	Assumes the driver interface to invoke is initialized in
 *	the device switch table record indexed by the device number
 *	passed in the iob.
 *
 *	The driver specific close() may update fields of the iob, such
 *	as i_error.
 *
 *	No return value.
 */
void
devclose(struct iob *io)
{
	(*devsw[io->i_dev].dv_close)(io);
}

/*
 * int
 * devioctl(struct iob *, int, caddr_t)
 * 	Invoke a device's driver specific ioctl() function via the 
 *	device switch table.
 *
 * Calling/Exit State:
 *	Assumes the driver interface to invoke is initialized in
 *	the device switch table record indexed by the device number
 *	passed in the iob.
 *
 *	The driver specific ioctl() may update fields of the iob, such
 *	as i_error.
 *
 *	Returns the value returned by driver specific ioctl().
 */
int
devioctl(struct iob *io, int cmd, caddr_t arg)
{
	return ((*devsw[io->i_dev].dv_ioctl)(io, cmd, arg));
}

/*
 * off_t
 * devlseek(struct iob *, off_t, int)
 * 	Invoke a device's driver specific lseek() function via the 
 *	device switch table.
 *
 * Calling/Exit State:
 *	Assumes the driver interface to invoke is initialized in
 *	the device switch table record indexed by the device number
 *	passed in the iob.
 *
 *	The driver specific lseek() may update fields of the iob, such
 *	as i_error.
 *
 *	Returns the value returned by driver specific lseek().
 */
off_t
devlseek(struct iob *io, off_t addr, int ptr)
{
	return ((*devsw[io->i_dev].dv_lseek)(io, addr, ptr));
}

/*
 * off_t
 * nulllseek(struct iob *, off_t, int)
 * 	Return an error when an attempt is made to lseek on a driver
 *	which does not support it.
 *
 * Calling/Exit State:
 *	Drivers that do not support lseek initialize their device switch
 *	table's lseek entry with the address of this function.
 *
 *	Returns -1 in all cases, indicating the lseek failed.
 */
/*ARGSUSED*/
off_t
nulllseek(struct iob *io, off_t addr, int ptr)
{
	return (-1);
}

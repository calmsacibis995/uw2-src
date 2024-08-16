/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dcu:dcusilent.c	1.6"

#include	<fcntl.h>
#include	<sys/resmgr.h>
#include	<sys/cm_i386at.h>

main()
{
	struct rm_args	rma;
	cm_num_t	dcu_mode;
	int		rm_fd;

	if((rm_fd = open(DEV_RESMGR, O_RDWR)) < 0) {
		printf("open(DEV_RESMGR) failed.\n");
		exit(0);
	}

	strcpy(rma.rm_param, CM_DCU_MODE);
	rma.rm_key = RM_KEY;
	rma.rm_val = &dcu_mode;
	rma.rm_vallen = sizeof(dcu_mode);
	rma.rm_n = 0;
	if(ioctl(rm_fd, RMIOC_GETVAL, &rma) < 0) {
		printf("ioctl() failed for CM_DCU_MODE.\n");
		exit(0);
	}

	exit(dcu_mode);
}

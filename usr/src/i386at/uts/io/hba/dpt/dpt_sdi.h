/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _IO_HBA_DPT_SDI_H	/* wrapper symbol for kernel use */
#define _IO_HBA_DPT_SDI_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/hba/dpt/dpt_sdi.h	1.6"
#ident  "$Header: dpt_sdi.h 1.1 $"

#if defined(__cplusplus)
extern "C" {
#endif

/*      Copyright (c) 1991  Intel Corporation     */
/*	All Rights Reserved	*/

/*      INTEL CORPORATION PROPRIETARY INFORMATION                          */
/*                                                                         */
/*	This software is supplied under the terms of a license agreement   */
/*	or nondisclosure agreement with Intel Corporation and may not be   */
/*	copied or disclosed except in accordance with the terms of that    */
/*	agreement.							   */
/*                                                                         */

/*
**	sdi function definitions
*/

#ifdef __STDC__
extern void dptintr(unsigned int);
extern struct hbadata *dptgetblk(int);
extern long dptfreeblk(struct hbadata *);
extern long dpticmd(struct hbadata *);
extern int dptinit(void);
extern void dptgetinfo(struct scsi_ad *, struct hbagetinfo *);
extern long dptsend(struct hbadata *);
extern void dptxlat(struct hbadata *, int, struct proc *, int);
extern int dptopen(dev_t *, int, int, cred_t *);
extern int dptclose(dev_t, int, int, cred_t *);
extern int dptioctl(dev_t, int, caddr_t, int, cred_t *, int *);
#else /* __STDC__ */
extern void dptintr();
extern struct hbadata *dptgetblk();
extern long dptfreeblk();
extern long dpticmd();
extern int dptinit();
extern void dptgetinfo();
extern long dptsend();
extern void dptxlat();
extern int dptopen();
extern int dptclose();
extern int dptioctl();
#endif /* __STDC__ */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_HBA_DPT_SDI_H */

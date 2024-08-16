/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/odi/msm/msm.cf/Space.c	1.4"
#ident	"$Header: $"

#include <config.h>

/*
 * msm tunables.
 */
int	msm_physcontig_1kchunks = 	MSM_1KCHUNKS;
int	msm_physcontig_2kchunks = 	MSM_2KCHUNKS;
int	msm_physcontig_6kchunks = 	MSM_6KCHUNKS;
int	msm_physcontig_10kchunks = 	MSM_10KCHUNKS;
int	msm_physcontig_36kchunks = 	MSM_36KCHUNKS;

void	*allocd_2k_chunks[MSM_2KCHUNKS];
void	*allocd_6k_chunks[MSM_6KCHUNKS];
void	*allocd_10k_chunks[MSM_10KCHUNKS];
void	*allocd_36k_chunks[MSM_36KCHUNKS];

void	*tmp_array[MSM_2KCHUNKS];

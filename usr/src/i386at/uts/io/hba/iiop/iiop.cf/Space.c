/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/hba/iiop/iiop.cf/Space.c	1.5"
#ident	"$Header: $"
/*
***************************************************************************
**
**      MODULE NAME:  space.c.iiop 
**
**      PURPOSE:  IIOP device driver table.
**
**      DEPENDENCIES:
**
**
**      REVISION HISTORY:
**      FPR/CRN     Date    Author      Description
**                          S. Cady     Initial release.
**
**
**      COPYRIGHT:
**
**          (c) Tricord systems, Inc. 1990 - 1992
**               All rights reserved.
**
**
***************************************************************************
*/

/*
***************************************************************************
**                      System Include Files
***************************************************************************
*/

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/conf.h>
#include "config.h"
#include "sys/iobuf.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"

/*
***************************************************************************
**                  Module Related Include Files
***************************************************************************
*/

#include "sys/iiop.h"
#include "sys/hba.h"

/*
***************************************************************************
**                                Defines
***************************************************************************
*/
#define IIOP_TIMEOUT     120000 /* Timeout of 2 minutes in milliseconds. */
#define IIOP_MIN         60000  /* Timeout of 1 minute in Hertz. */
/*
** The following values are tuneable:
** 
**   IIOP_10_DEVICES            Number of IORPs and LIORPs
**   NDMA                       Number of Scatter-gather buffers
**   iiop_hba_max               Maximum jobs concurrently on HBA
**   iiop_lu_max	        Maximum jobs per LUN concurrently
**
** By default they are set to the following:
**   IIOP_10_DEVICE = 164
**   NDMA           = 163
**   iiop_hba_max   = 163
**   iiop_lu_max    = 163
**  
** The polled version of the "iiop" driver uses the following tuneables:
**  
**   IIOP_10_DEVICE =  21 
**   NDMA           =  20 
**   iiop_hba_max   =  3 
**   iiop_lu_max    =  3 
**   
**   
** Under certain condition you may increase these tuneable accordingly
** There is a counter in the "iiop" driver "iiop_hit_max_jobs" that is
** bumped if we have more jobs active on the controller than we have
** I/O request packets (IORP's) available
** 
*/ 
#define IIOP_10_DEVICES  164    /* Ensure sufficient buffer space for 
                                   up to 10 SCSI devices. */
#define NDMA             163    /* Number of scatter-gather lists */

/*
** There are no selectable parameters for the user to configure
** on IIOP/EIIOP or ISS controllers.  Interrupts 17-23 are defined
** on a slot basis.  There are no I/O addresses for these
** controllers.  The memory address is dual ported memory and 
** defined by the board id.
** Therefore, there is no need to configure additional controllers
** into the system using any of the PDI configure utilities (i.e. pdiadd).
** We will define the maximum number of controllers supported.
*/
 
/*
***************************************************************************
**                      Global Static Storage
***************************************************************************
*/

/*
***************************************************************************
**    IIOP specific environment, first controller contains boot device.
***************************************************************************
*/
int     iiop_gtol[SDI_MAX_HBAS];     /* global-to-local hba# mapping */
int     iiop_ltog[SDI_MAX_HBAS];     /* local-to-global hba# mapping */

int iiop_hba_max = 163;	        /** Maximum jobs concurrently on HBA **/
int iiop_lu_max = 163;	        /** Maximum jobs per LUN concurrently **/
int	iiop_cntls	= 1; 

struct	ver_no    iiop_sdi_ver;

int	iiop_ctlr_id = 0;       /* HBA's SCSI id number*/


#ifdef	IIOP_CPUBIND

#define	IIOP_VER		HBA_SVR4_2MP | HBA_IDATA_EXT

struct	hba_ext_info	iiop_extinfo = {
	0, IIOP_CPUBIND
};

#else

#define	IIOP_VER		HBA_SVR4_2MP

#endif	/* IIOP_CPUBIND */


struct	hba_idata_v4	_iiopidata[]	= {

	{ IIOP_VER, "(iiop,1) IIOP SCSI",
	  0, 0, -1, 0, -1, 0 }
#ifdef	IIOP_CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&iiop_extinfo }
#endif
};


/*
***************************************************************************
**                          Command timeout values.
***************************************************************************
*/
IIOP_CMD iiop_cmds[] =
{
    /* 0x00 TEST_CMD */ {IIOP_TIMEOUT, 0, IIOP_SCSI_CMD},
    /* 0x01 REWIND_CMD */ {IIOP_MIN*30, 0, IIOP_SCSI_CMD},
    /* unused */ {IIOP_TIMEOUT, 0, -1},
    /* 0x03 SENSE_CMD */ {IIOP_TIMEOUT, 0, IIOP_SCSI_READ},
    /* 0x04 FORMAT_CMD   */ {IIOP_MIN*60*2, 0, -1},
    /* unused */ {IIOP_TIMEOUT, 0, IIOP_SCSI_READ},
    /* unused */ {IIOP_TIMEOUT, 0, -1},
    /* 0x07 RD_BACK_CMD  */ {IIOP_MIN, 0, IIOP_SCSI_WRITE},
    /* 0x08 READ_CMD */ {0xfffffff, 0, IIOP_SCSI_READ},
    /* unused */ {IIOP_TIMEOUT, 0, -1},
    /* 0x0A WRITE_CMD */ {0xfffffff, 0, IIOP_SCSI_WRITE},
    /* 0x0B SEEK_CMD */ {IIOP_TIMEOUT, 0, IIOP_SCSI_CMD},
    /* unused */ {IIOP_TIMEOUT, 0, -1},
    /* unused */ {IIOP_TIMEOUT, 0, -1},
    /* unused */ {IIOP_TIMEOUT, 0, -1},
    /* 0x0F TP_RDB */ {IIOP_TIMEOUT, 0, -1},
    /* 0x10 TP_WFM */ {IIOP_TIMEOUT, 0, IIOP_SCSI_CMD},
    /* 0x11 TP_SP */ {IIOP_MIN*30, 0, IIOP_SCSI_CMD},
    /* 0x12 INQUIRY_CMD  */ {IIOP_TIMEOUT, 0, IIOP_SCSI_READ},
    /* unused */ {IIOP_TIMEOUT, 0, -1},
    /* unused */ {IIOP_TIMEOUT, 0, -1},
    /* 0x15 MODESELECT_CMD */ {IIOP_TIMEOUT, 0, IIOP_SCSI_WRITE},
    /* unused */ {IIOP_TIMEOUT, 0, -1},
    /* unused */ {IIOP_TIMEOUT, 0, -1},
    /* unused */ {IIOP_TIMEOUT, 0, -1},
    /* 0x19 TP_ERASE */ {IIOP_MIN*60*3, 0, IIOP_SCSI_CMD},
    /* 0x1A MODESENSE_CMD */ {IIOP_TIMEOUT, 0, IIOP_SCSI_READ},
    /* 0x1B TP_LOAD */ {IIOP_MIN*30, 0, IIOP_SCSI_CMD},
    /* unused */ {IIOP_TIMEOUT, 0, -1},
    /* unused */ {IIOP_TIMEOUT, 0, -1},
    /* 0x1E unused */ {IIOP_TIMEOUT, 0, IIOP_SCSI_CMD},
    /* unused */ {IIOP_TIMEOUT, 0, -1}, 
    /* unused */ {IIOP_TIMEOUT, 0, -1},
    /* unused */ {IIOP_TIMEOUT, 0, -1},
    /* unused */ {IIOP_TIMEOUT, 0, -1},
    /* unused */ {IIOP_TIMEOUT, 0, -1},
    /* unused */ {IIOP_TIMEOUT, 0, -1},
    /* 0x25 READ_CAP_CMD */ {IIOP_TIMEOUT, 8, IIOP_SCSI_READ},
    /* DEFAULT ENTRY */ {IIOP_TIMEOUT, 0, -1},
};

/*
***************************************************************************
**                          Configuration information
***************************************************************************
*/
int is_K2;                             /* (K2_ADDED) contains value returned */
                                       /* by call to ims_is_K2() */
int iiop_cmds_len = (sizeof(iiop_cmds) / sizeof(IIOP_CMD)) -1;
int iiop_tune_dpm = 0xff;
/*int iiop_tune_dpm = IIOP_DP_PHYSICAL;*/
                                    /* Place some iorp d.s. in dual-port RAM */
int iiop_tune_iorps = IIOP_10_DEVICES;
                                    /* The number of physical IORP d.s.
                                       to allocate.
                                    */
int iiop_tune_liorps = IIOP_10_DEVICES;
                                    /* The number of LOGICAL LIORP d.s.
                                       to allocate.
                                    */
int iiop_tune_dma = NDMA;
int iiop_Log_IIOP_Errors = 1;            /* Enable IIOP SCSI error reports. */
int iiop_max_iiop_elogs = 10000;         /* Number of error reports before being
                                       disabled.
                                    */
int iiop_1st_eisa_slot = IIOP_EISA_SLOT_9;
int iiop_last_eisa_slot = IIOP_EISA_SLOT_13;
int iiop_1st_eisa_irq = IIOP_EISA_VECT_9;
int iiop_dp_ram_incr = IIOP_DP_RAM_INCR;

int iiop_intr_ipl = IIOP_INTR_PRI;  /* Interrupt Priority Level. */
int iiop_raid1_cache = 0;           /* Cache memory in 4kbytes increments; 
                                       default is no memory. 
                                    */

/*
***************************************************************************
** Individual SCSI disk manufacturer's mode select command to
** enable disk drive readahead and bad blocking algorithm.
***************************************************************************
*/
unsigned char iiop_cdc_ms_cmd[] =
{
    /* Mode Select Parameter List */
    0, 0, 0, 0,
    /* Error Recovery Page*/
    0x1, 6,
    /* Error Recovery Parameters */
    0xc0, 0x1b, 0xb, 0, 0, 0xff,
    /* Cache Control Page */
    0x38, 14,
    /* Cache Control Parameters */
    0x11, 0xff, 0x34, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

unsigned char iiop_fuji2_ms_cmd[] =
{
    /* Mode Select Parameter List */
    0, 0, 0, 0,
    /* Error Recovery Page*/
    0x1, 10,
    /* Error Recovery Parameters */
    0xec, 0x12, 0xb, 0, 0, 0, 0x12, 0, 0, 0,
    /* Cache Control Page */
    0x8, 10,
    /* Cache Control Parameters */
    0x0, 0, 0xff, 0xff, 0, 0, 0x00, 0xff, 0x00, 0xff
};

unsigned char CDC[] = {"CDC"};
unsigned char SEAGATE[] = {"SEAGATE"};
unsigned char IMPRIMIS[] = {"IMPRIMIS"};
unsigned char FUJITSU[] = {"FUJITSU"};
/*
** Table of SCSI disk manufacturer.
*/
IIOP_MS_TABLE iiop_ms_table[] =
{
    {CDC, (ulong) 4, iiop_cdc_ms_cmd, (ulong) sizeof(iiop_cdc_ms_cmd)},
    {SEAGATE, (ulong) 7, iiop_cdc_ms_cmd, (ulong) sizeof(iiop_cdc_ms_cmd)},
    {IMPRIMIS, (ulong) 8, iiop_cdc_ms_cmd, (ulong) sizeof(iiop_cdc_ms_cmd)},
    {FUJITSU, (ulong) 7, iiop_fuji2_ms_cmd, (ulong) sizeof(iiop_fuji2_ms_cmd)},
};
int iiop_ms_tbl_entries = sizeof(iiop_ms_table) / sizeof(IIOP_MS_TABLE);

/*
***************************************************************************
**                          STUBS for NON-RAID 1
***************************************************************************
*/
int iiop_release_revision = IIOP_EIOP_REV;

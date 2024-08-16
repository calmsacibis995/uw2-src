/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/ast/ast.h	1.1"
#ident	"$Header: $"

/*
 * ast.h - OEM specific definitions for AST Manhattan.  This file 
 *         supplements the definitions contained in the file ebi.h
 */

#ifndef _SYS_MP_AST_H
#define _SYS_MP_AST_H

#define AST_SIGNATURE            0x01407406    /* EISA ID at 0xC80-0xC83   */

/*
 * Event codes for front panel support
 */
#define AST_EVENT_PWR_FAIL       0      /* Power fail interrupt occured    */
#define AST_EVENT_SHUTDOWN       1      /* Front panel shutdown switch     */
#define AST_EVENT_ATTENTION      0      /* Attention switch pressed        */

/*
 * OEM[0] (AST) special function codes
 */
#define SET_EISA_REFRESH_MODE    0x0    /* EISA mem refresh on or off      */
#define GET_EISA_REFRESH_MODE    0x1    /* get state of EISA mem refresh   */
#define SET_COM2_OVERRIDE_MODE   0x2    /* control COM2 disable/enable     */
#define GET_COM2_OVERRIDE_MODE   0x3    /* get COM2 disable/enable state   */
#define GEN_MULTIBIT_ECC_ERR     0x4    /* generate a multi bit ECC error  */
#define GEN_SINGLEBIT_ECC_ERR    0x5    /* generate a single bit ECC error */
#define SET_IRQ13_LATCH          0x6    /* control IRQ13 latch (FP intr)   */
#define GET_IRQ13_LATCH          0x7    /* sense state of IRQ13 latch      */
#define SET_CACHE_MODE           0x8    /* set cache operation mode        */
#define GET_CACHE_MODE           0x9    /* get cache operation mode        */
#define SET_EISA_INT_MODE        0xA    /* switch to EISA style interrupts */
#define ACTIVATE_BEDBUG          0xB    /* activate BedBug ROM monitor     */
#define GET_NUM_MEM_BOARDS       0xC    /* get number of phys. mem boards  */
#define GET_MEM_BOARD_INFO       0xD    /* get info about a single board   */
#define GET_MEM_BANK_INFO        0xE    /* get info about a single bank    */

/*
 * AST specific error codes
 */
#define ERR_BAD_EISA_REF         OEMErrCode(0,0)
#define ERR_BAD_COM2_OVERRIDE    OEMErrCode(0,1)
#define ERR_BAD_GEN_MULTIBIT     OEMErrCode(0,2)
#define ERR_BAD_GEN_SINGLEBIT    OEMErrCode(0,3)
#define ERR_BAD_IRQ13_LATCH      OEMErrCode(0,4)
#define ERR_BAD_CACHE_MODE       OEMErrCode(0,5)

/*
 * constants for special function code parameters
 */
#define ESIA_REFRESH_MODE_ON     0x1    /* turn on EISA memory refresh     */
#define ESIA_REFRESH_MODE_OFF    0x0    /* turn off EISA memory refresh    */

#define COM2_OVERRIDE_MODE_ON    0x1    /* allow key control of COM2       */
#define COM2_OVERRIDE_MODE_OFF   0x0    /* disallow key control of COM2    */

#define IRQ13_LATCH_ON           0x1    /* set IRQ13 latch                 */
#define IRQ13_LATCH_OFF          0x0    /* clear IRQ13 latch               */

/*
 * bit definitions for SET/GET cache mode
 */
#define DISABLE_CACHE            0x4    
#define FORCE_WRITE_THRU         0x2
#define FORCE_READ_ONLY          0x1
#define DEFAULT_CACHE_MODE       0x0

/*
 * bit definitions for GET_MEM_BOARD_INFO
 */
#define DISTC_1_BYTE             0x1    /* one byte wide DISTC             */
#define DISTC_2_BYTE             0x2    /* two byte wide DISTC             */
#define DISTC_4_BYTE             0x4    /* four byte wide DISTC            */
#define DISTC_8_BYTE             0x8    /* eight byte wide DISTC           */

#define DISTC_RAM_PRES           0x10   /* DISTC RAM is present            */
#define RAM_ECC                  0x100  /* RAM is ECC                      */
#define RAM_INTERLEAVE           0x200  /* RAM arch is interleaved         */
#define RAM_128_BIT              0x400  /* 128 bit wide data path          */

typedef struct {
    unsigned int refreshMode;
} SetEISARefreshModePacket;

typedef struct {
    unsigned int *refreshMode;
} GetEISARefreshModePacket;

typedef struct {
    unsigned int override;
} SetCOM2OverridePacket;

typedef struct {
    unsigned int *override;
} GetCOM2OverridePacket;

typedef struct {
    unsigned int boardNum;
    void         *address;
} SBitMemoryErrPacket;

typedef struct {
    unsigned int boardNum;
    void         *address;
    unsigned int pattern;
} MBitMemoryErrPacket;

typedef struct {
    unsigned int latch;
    unsigned int processorID;
} SetIRQ13LatchPacket;

typedef struct {
    unsigned int *latch;
    unsigned int processorID;
} GetIRQ13LatchPacket;

typedef struct {
    unsigned int cacheMode;
} SetCacheModePacket;

typedef struct {
    unsigned int *cacheMode;
} GetCacheModePacket;

typedef struct {
    unsigned int processorID;
} SetEISAIntModePacket;

typedef struct {
    unsigned int *memory;
} BedBugPacket;

typedef struct {
    unsigned int *numMemBoards;
} GetNumMemBoardsPacket;

typedef struct {
    physAddr startAddr;
    unsigned int size;
    unsigned int ECCSoftErrors;
    unsigned int ECCHardErrors;
    unsigned int RESERVED;
} memBankInfo;

typedef struct {
    unsigned int boardNum;
    unsigned int bankNum;
    memBankInfo *info;
} GetMemBankInfoPacket;

typedef struct {
    unsigned int attributes;
    unsigned int numBanks;
    unsigned int slotNum;
    unsigned int RESERVED;
} MemBoardInfo;

typedef struct {
    unsigned int boardNum;
    MemBoardInfo *info;
} GetMemBoardInfoPacket;

typedef union {
    SetEISARefreshModePacket SetEISARefreshModeParms;
    GetEISARefreshModePacket GetEISARefreshModeParms;
    SetCOM2OverridePacket    SetCOM2OverrideParms; 
    GetCOM2OverridePacket    GetCOM2OverrideParms; 
    SBitMemoryErrPacket      SBitMemoryErrParms;    
    MBitMemoryErrPacket      MBitMemoryErrParms;    
    SetIRQ13LatchPacket      SetIRQ13LatchParms;
    GetIRQ13LatchPacket      GetIRQ13LatchParms;
    SetCacheModePacket       SetCacheModeParms;
    GetCacheModePacket       GetCacheModeParms;
    SetEISAIntModePacket     SetEISAIntModeParms;
    BedBugPacket             BedBugParms;
    GetNumMemBoardsPacket    GetNumMemBoardsParms;
    GetMemBoardInfoPacket    GetMemBoardInfoParms;
    GetMemBankInfoPacket     GetMemBankInfoParms;
} OEM0Parms;    

typedef struct {
    unsigned int subfunction;
    OEM0Parms    parms;
} OEM0ParmPacket;

#define SOFTINT 63

extern struct dispinfo ast_display;
extern EBI_II       ast_calltab;
extern int          ast_event_code;
extern unsigned int ast_cpuid[];
extern unsigned int ast_int_handle[];
extern void         **MMIOTable;
#endif

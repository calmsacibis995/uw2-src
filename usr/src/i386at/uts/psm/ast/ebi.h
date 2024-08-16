/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/ast/ebi.h	1.3"
#ident	"$Header: $"

/*
 * ebi.h -- header file with definitions for the AST EBI II interface.
 * most page numbers referece pages in EBI II Specification, version 2.8
 * where noted page numbers reference version 2.10 of the EBI II spec.
 *
 * M001: added ioctl EBI_CLEAR_EVENTS to clear event flags.
 * M002: added ioctl to turn RAM cache (internal and external) on and off
 */

#ifndef _SYS_MP_EBI_H
#define _SYS_MP_EBI_H

#include "sys/types.h"

/*
 * Event codes for front panel support (not a bit-map--may be in future)
 */
#define EBI_EVENT_PWR_FAIL       1      /* Power fail interrupt occured    */
#define EBI_EVENT_SHUTDOWN       2      /* Front panel shutdown switch     */
#define EBI_EVENT_ATTENTION      4      /* Attention switch pressed        */

/*
 * ioctl's for front panel, etc., support.
 */
#define EBIIOC                   (('e' << 24)|('b' << 16)|('i' << 8))
#define EBI_GET_ALPHA_INFO       (EBIIOC|0) /* get alpha disp. size/type   */
#define EBI_SET_ALPHA_DISP       (EBIIOC|1) /* set contents alpha display  */
#define EBI_GET_ALPHA_DISP       (EBIIOC|2) /* get contents alpha display  */
#define EBI_SET_UPS_LED          (EBIIOC|3) /* set color of UPS LED        */
#define EBI_GET_UPS_LED          (EBIIOC|4) /* get color of UPS LED        */
#define EBI_SET_BAR_GRAPH_MODE   (EBIIOC|5) /* set bar graph mode          */
#define EBI_GET_BAR_GRAPH_MODE   (EBIIOC|6) /* set bar graph mode          */
#define EBI_SET_BAR_CONTENTS     (EBIIOC|7) /* set graph in override mode  */
#define EBI_GET_BAR_CONTENTS     (EBIIOC|8) /* get current graph value     */
#define EBI_GET_THERMAL_STATE    (EBIIOC|9) /* get current thermal state   */
#define EBI_GET_NUM_PWR_SUPPLIES (EBIIOC|10)/* get num of power supplies   */
#define EBI_GET_PWR_INFO         (EBIIOC|11)/* get info on power supplies  */
#define EBI_SET_PWR_OFF          (EBIIOC|12)/* set pwr shutdown flag       */
#define EBI_GET_EVENT            (EBIIOC|13)/* get an asynchronus event    */
/* see addional ioctls, below */

/*
 * structure for EBI_GET_ALPHA_INFO
 */
struct dispinfo {
    unsigned int size;
    unsigned int type;
};

/*
 * signature indicating presence of EBI II BIOS.
 */
typedef struct {
    char sig[4];
    unsigned short seg;
    unsigned short off;
} ebi_iiSig;

#define EBI_SIG_LOC    ((unsigned int)0xf000ffe2) /* real mode (segment:offset) */
                                                  /* address of EBI II          */
						  /* signature location         */

/*
 * AST's notion of a physical address is *big* and does not mesh 
 * with System V's idea of a physical address.
 *
 * definition from Appendix B, page 63
 */
typedef struct {
    unsigned int low;
    unsigned int high;
} physAddr;

/*
 * Function return codes, page 8
 */
#define OK                        0    /* function complete                   */
#define PROC_RUNNING              1    /* processor already running           */
#define PROC_STOPPED              2    /* processor not running               */
#define NO_CACHE                  3    /* processor is broke (no cache :))    */
#define NO_MEMORY_ERRORS          4    /* no RAM errors found                 */
#define MEMORY_ERROR_FOUND        5    /* hardware detected a RAM error       */
#define WRONG_PROC_GRAPH_MODE     6    /* wrong bar graph mode for this op.   */

/*
 * Error return codes, page 8
 */
#define BAD                       -1   /* operation failed                    */
#define ERR_NOT_SUPPORTED         -2   /* function not supported              */
#define ERR_NONESUCH              -3   /* feature not present                 */
#define ERR_BAD_PROC_ID           -4   /* invalid processor id                */
#define ERR_PROC_ABSENT           -5   /* processor is absent                 */
#define ERR_PROC_BAD              -6   /* processor is faulty                 */
#define ERR_UNKNOWN_INT           -7   /* interrupt source not found          */
#define ERR_DISPLAY_OVERFLOW      -8   /* front panel display overflow        */
#define ERR_BAD_CHARS             -9   /* illegal char for front panel display*/
#define ERR_BAD_SELECTOR          -10  /* illegal selector                    */
#define ERR_BAD_VECTOR            -12  /* illegal vector value                */
#define ERR_BAD_COLOR             -13  /* illegal front panel UPS led color   */
#define ERR_BAD_GRAPH_MODE        -14  /* illegal processor bar graph mode    */
#define ERR_BAD_BOARD_NUM         -15  /* illegal memory board number         */
#define ERR_BAD_OFFSWITCH         -18  /* invalid off switch mode             */
#define ERR_BAD_PHYS_ADDR         -19  /* invalid physical address for op     */
#define ERR_BAD_LENGTH            -20  /* this length illegal for operation   */
#define ERR_BAD_BLOCK_NUM         -21  /* info requested for non-existent blk */
#define ERR_BAD_GLOB_MASK_IRQ     -22  /* can't globally mask this irq        */
#define ERR_BAD_LOC_MASK_IRQ      -23  /* can't locally mask this irq         */
#define ERR_CANT_CANCEL_INT       -24  /* can't cancel this/these irq(s)      */
#define ERR_PROC_STILL_RUNNING    -25  /* can't deinit with >1 procs running  */
#define ERR_POWER_SUPPLY_NUM      -26  /* illegal power supply number         */
#define ERR_BAD_VIS_MODE          -27  /* bad visibility mode                 */

/*
 * the following two error return codes were added in
 * verion 2.10
 */
#define ERR_MULTI_TIME_OUT        -28  /* internal semaphore timed out: fatal */
#define ERR_FRONT_PANEL_DISABLED  -29  /* can't get front panel status        */

/*
 * definitions to derive OEM error codes.
 */
#define OEM_COMP_CODE_BASE        65536L
#define OEM_COMP_CODE_GAP         65536L
#define OEM_ERR_CODE_BASE         (-65536L)

#define OEMCompCode(n,c) (OEM_COMP_CODE_BASE + ((n) * OEM_COMP_CODE_GAP) + (c))
#define OEMErrCode(n,c)  (OEM_ERR_CODE_BASE - ((n) * OEM_COMP_CODE_GAP) - (c))

/*
 * the processor config data structure, page 10
 */
typedef struct procConfigData {
    uchar_t  processorStatus;
    uchar_t  processorType;
    uchar_t  coprocessorType;
    uchar_t  serialNum[4];
    uchar_t  boardRev;
    uchar_t  boardType;
    uchar_t  manufacturing[8];
    uchar_t  boardInfo[20];
    int      slotNumber;
} procConfigData;

/*
 * definitions for processor status, table 3, page 11
 */
#define PS_ABSENT    0x0
#define PS_RUNNING   0x1
#define PS_RESET     0x2
#define PS_FAULTY    0xF

/*
 * definitions for processor type, table 4, page 11
 */
#define PT_NONE     0x0

/*
 * Intel processors
 */
#define PT_i386     0x10
#define PT_i486     0x11
#define PT_i586     0x12
#define PT_i686     0x13
#define PT_i860     0x80
#define PT_i960     0x81

/*
 * SPARC, but what version?
 */
#define PT_SPARC    0x20

/*
 * MIPS, but where's the R4400?
 */
#define PT_R4000    0x30
#define PT_R5000    0x31
#define PT_R6000    0x70

/*
 * Motorola 68K family
 */
#define PT_68030    0x40
#define PT_68040    0x41
#define PT_68050    0x42

/*
 * Motorola 88K family
 */
#define PT_88000    0x50
#define PT_88110    0x51

/*
 * TI video coprocessors
 */
#define PT_34010    0x60
#define PT_34020    0x61
#define PT_34030    0x62

/*
 * IBM RS6000/PowerPC
 */
#define PT_R6000    0x70

/*
 * definitions for coprocessorType, table 6 page 11
 */
#define CT_NONE     0x0

/*
 * Intel math coprocessors
 */
#define CT_387      0x10
#define CT_487      0x11
#define CT_587      0x12

/*
 * Weitek math coprocessors
 */
#define CT_3167     0x20
#define CT_4167     0x21
#define CT_5167     0x22

/*
 * Motorola math coprocessors
 */
#define CT_68881    0x30
#define CT_68882    0x31

/*
 * TI(?) math coprocessors
 */
#define CT_34081    0x40
#define CT_34082    0x41

/*
 * definitions for board type, table 5, page 11
 */
#define BT_RESERVED    0x0
#define BT_PROCESSOR   0x1
#define BT_MEMORY      0x2
#define BT_DISK        0x3
#define BT_LAN         0x4
#define BT_GRAPHICS    0x5
#define BT_DSP         0x6

/*
 * cache flush control codes, table 7 page 15
 */
#define CACHE_INVALID    0x0
#define CACHE_NO_INVALID 0x1

/*
 * cache control word, table 8, page 15
 */
#define CACHE_REGION_DISABLE 0x0
#define CACHE_REGION_ENABLE  0x1

/*
 * cacheControlInfo structure, page 16
 */
typedef struct cacheControlInfo {
    unsigned int flags;
    unsigned int controlGranularity;
    unsigned int RESERVED;
} cacheControlInfo;
 
/*
 * definitions for cacheControlInfo flags, table 9, page 16
 */
#define REGIONAL_CACHE_CONTROL_ENABLED  0x1
#define REGIONAL_CACHE_CONTROL_DISABLED 0x0

/*
 * definitions for UPS LED color, table 10, page 17
 */
#define UPS_LED_DARK   0x0
#define UPS_LED_RED    0x1
#define UPS_LED_GREEN  0x2
#define UPS_LED_AMBER  0x3

/*
 * definitions for front panel display mode, table 11, page 18
 */
#define PANEL_MODE_HISTOGRAM 0x0
#define PANEL_MODE_STATUS    0x1
#define PANEL_MODE_OVERRIDE  0x2

/*
 * definitions for front panel key switch state, table 12, page 22
 */
#define KEY_SWITCH_LOCKED    0x0
#define KEY_SWITCH_DIAG      0x1
#define KEY_SWITCH_UNLOCKED  0x2
#define KEY_SWITCH_OFF       0x3

/*
 * definitions for alpha numeric display type, table 13, page 23
 */
#define ALPHA_TYPE_1         0x0
#define ALPHA_ASCII          0x1
#define ALPHA_ALPHA_NUMERIC  0x2
#define ALPHA_7_SEGMENT      0x3

/*
 * definitions for front panel off switch mode, table 14, page 24
 */
#define OFF_SWITCH_SOFT      0x0
#define OFF_SWITCH_HARD      0x1

/*
 * definitions for SetPanelSwitchVisibility, table 15, page 26
 */
#define PANEL_SWITCH_INVISIBLE 0x0
#define PANEL_SWITCH_VISIBLE   0x1

/*
 * definitions for interrupt subsystem support.
 */
#define ATSLAVE          0x2      /* IRQ line for slave pic in 8259 systems */
#define CHKSPUR          0x08080  /* spurious interrupts are reflected here */

/*
 * subsystem type definitions, table 16, page 27
 */
#define INT_TYPE_EISA    0x0    /* EISA interrupt subsystem, no distribution */
#define INT_TYPE_ISA     0x1    /* ISA interrupt subsystem, no distribution  */
#define INT_TYPE_ADI     0x2    /* AST ADI subsystem, distribution optional  */
#define INT_TYPE_MPIC    0x3    /* MPIC subsystem, distributed interrupts    */

/*
 * interrupt mask definitons, table 17, page 28
 */
#define IRQBIT(x) (1 << (x))    /* Generate bitmask from IRQ number          */
#define SPI    24               /* bit for system priority interrupt         */
#define LSI    25               /* bit for local software interrupt          */
#define IPI    26               /* bit for inter processor interrupt         */
#define NINT   32               /* we can have a max of 32 interrupts        */

/*
 * definitions for interprocessor interrupt (IPI)
 */
#define IPI_ID(x) (1 << (x))            /* Generate IPI ID for cpu  */
#define IPI_VECT  (0x1A + PIC_VECTBASE) /* Vector for IPI           */
#define IPI_IPL   PLHI                  /* Comes in at IPL HI       */

/*
 * definitions for system priority interrupt (SPI)
 */
#define SPI_VECT  (0x18 + PIC_VECTBASE) /* Vector for SPI           */
#define SPI_IPL   0x8                   /* Comes in at IPL HI       */

/*
 * definitions for local software interrupt (LSI)
 */
#define LSI_VECT  (0x19 + PIC_VECTBASE) /* Vector for LSI           */
#define LSI_IPL   0x8                   /* Comes in at IPL HI       */

/*
 * NMI and SPI source definitions, table 19, page 38
 */
#define INT_NO_SOURCE       0x0           /* unknown interrupt source        */
#define INT_IO_ERROR        0x1           /* system i/o error                */
#define INT_SOFT_NMI        0x2           /* software generated NMI          */
#define INT_MEMORY_ERROR    0x3           /* memory error                    */
#define INT_CPU_ERROR       0x4           /* processor error                 */
#define INT_POWER_FAIL      0x5           /* power failure                   */
#define INT_BUS_ERR         0x6           /* bus address/data parity error   */
#define INT_BUS_TIMEOUT     0x7           /* system bus timeout              */
#define INT_SHUTDOWN        0x8           /* shutdown button                 */
#define INT_ATTENTION       0x9           /* attention button                */

/*
 * definition of memoryBlockInfo, figure 9, page 45
 */
typedef struct memoryBlockInfo {
    physAddr     blockStartAddr;
    unsigned int blockSize;
    unsigned int RESERVED;
} memoryBlockInfo;

/*
 * definition of memoryErrorInfo, figure 12, page 47
 */
typedef struct memoryErrorInfo {
    physAddr     location;
    unsigned int length;
    unsigned int count;
    unsigned int memErrFlags;
    unsigned int slotNumber;
    unsigned int moduleNumber;
    struct memoryErrorInfo *next;
} memoryErrorInfo;

/*
 * definitions for memErrFlags, table 20, page 47
 */
#define MEMERR_NOERR        0x0    /* no errors detected   */
#define MEMERR_PARITY       0x1    /* parity error         */
#define MEMERR_ECC_SINGLE   0x2    /* single bit ECC error */
#define MEMERR_ECC_MULTI    0x3    /* multi bit ECC error  */

/*
 * definition of revisionCode, figure 13, page 48
 */
typedef struct revisionCode {
    uchar_t major;
    uchar_t minor;
    uchar_t RESERVED;
} revisionCode;

/*
 * we support revision 2.10 and up of EBI II 
 */
#define MIN_MAJOR    2             /* major revision must be at least 2  */
#define MIN_MINOR    10            /* minor revision must be at least 10 */

/*
 * definition of IOInfoTable, figure 14, page 50
 */
typedef struct {
    physAddr     address;
    unsigned int length;
    unsigned int flags;
} IOInfoTable;

/*
 * definitions for flags, table 21, page 50
 */
#define IO_MAPIN       0x0    /* map in virtual address space for this slot */
#define IO_ALLOCATE    0x1    /* allocate RAM for this slot                 */

/*
 * definition for powerSupplyInfo, figure 15, page 53
 */
typedef struct {
    unsigned int present;       /* 1 - installed, 0 - absent                 */
    unsigned int onLine;        /* 1 - supply is online, 0 - supply is kaput */
    unsigned int RESERVED[8];
} powerSupplyInfo;

/*
 * structure for EBI_GET_PWR_INFO
 */
struct pwrinfo {
	int	ps_num;		          /* which supply to get info for */
	powerSupplyInfo *ps_info;         /* place to store info          */
};

/*
 * definitions for maskInfo structure, page 41, figure 7 (version 2.10)
 */
typedef struct {
    unsigned int numPorts;       /* number of i/o ports used */
    unsigned int flags;          /* info on ports            */
    unsigned int portAddress[4]; /* port addresses           */
    unsigned int reserved[2];
} maskInfo;

/*
 * definitions for maskInfo.flags version 2.10, page 40, table 20
 */
#define MI_PORT_TYPE    0x3      /* mask for port type bits  */
#define MI_PORT_MEM     0x0      /* ports are memory mapped  */
#define MI_PORT_IO      0x1      /* ports are in i/o space   */

#define MI_PORT_SIZE    0x1c     /* mask for port width bits */
#define MI_PORT_8       0x0      /* ports are 8 bits wide    */
#define MI_PORT_16      0x4      /* ports are 16 bits wide   */
#define MI_PORT_32      0x8      /* ports are 32 bits wide   */

typedef struct {
    /*
     * processor manipulation group
     */
    int (*GetNumProcs)(void *, unsigned int *);                             /* 1  */
    int (*GetProcConf)(void *, unsigned int, procConfigData *);             /* 2  */
    int (*StartProc)(void *, unsigned int);                                 /* 3  */
    int (*StopProc)(void *, unsigned int);                                  /* 4  */
    int (*GetProcID)(void *, unsigned int *);                               /* 5  */

    /*
     * cache control group
     */
    int (*EnableRAMCache)(void *);                                          /* 6  */
    int (*DisableRAMCache)(void *);                                         /* 7  */
    int (*FlushRAMCache)(void *, unsigned int);                             /* 8  */
    int (*ControlCacheRegion)(void *, unsigned int, physAddr, unsigned int);/* 9  */
    int (*GetCacheControlInfo)(void *, cacheControlInfo *);                 /* 10 */

    /*
     * front panel control group
     */
    int (*SetPanelUPS)(void *, unsigned int);                               /* 11 */
    int (*GetPanelUPS)(void *, unsigned int *);                             /* 12 */
    int (*SetPanelProcGraphMode)(void *, unsigned int);                     /* 13 */
    int (*GetPanelProcGraphMode)(void *, unsigned int *);                   /* 14 */
    int (*SetPanelProcGraphValue)(void *, unsigned int);                    /* 15 */
    int (*GetPanelProcGraphValue)(void *, unsigned int *);                  /* 16 */
    int (*LogProcIdle)(void *);                                             /* 17 */
    int (*LogProcBusy)(void *);                                             /* 18 */
    int (*GetPanelAttnSwitchLatch)(void *, unsigned int *);                 /* 19 */
    int (*GetPanelOffSwitchLatch)(void *, unsigned int *);                  /* 20 */
    int (*GetPanelKeyPos)(void *, unsigned int *);                          /* 21 */
    int (*GetPanelAlphaNumInfo)(void *, unsigned int *, unsigned int *);    /* 22 */
    int (*GetPanelAlphaNum)(void *, unsigned char *);                       /* 23 */
    int (*SetPanelAlphaNum)(void *, unsigned char *);                       /* 24 */
    int (*SetPanelOffSwitchMode)(void *, unsigned int);                     /* 25 */
    int (*GetPanelOffSwitchMode)(void *, unsigned int *);                   /* 26 */

    /*
     * interrupt subsystem control group
     */
    int (*GetIntSubsysType)(void *, unsigned int *);                        /* 27 */
    int (*SetGlobalIntMask)(void *, unsigned int);                          /* 28 */
    int (*GetGlobalIntMask)(void *, unsigned int *);                        /* 29 */
    int (*SetLocalIntMask)(void *, unsigned int, unsigned int);             /* 30 */
    int (*GetLocalIntMask)(void *, unsigned int *, unsigned int);           /* 31 */
    int (*SetAdvIntMode)(void *);                                           /* 32 */
    int (*SetIRQVectorAssign)(void *, unsigned int, unsigned int);          /* 33 */
    int (*GetIRQVectorAssign)(void *, unsigned int, unsigned int *);        /* 34 */

    /*
     * why they put the power supply stuff in here is beyond my ken...
     */
    int (*GetNumPowerSupplies)(void *, unsigned int *);                     /* 35 */
    int (*GetPowerSupplyInfo)(void *, unsigned int, powerSupplyInfo *);     /* 36 */
    
    int (*DeInitEBI)(void *);                                               /* 37 */
    
    /*
     * interrupt control for NMI, SPI, LSI and IPI...
     */
    int (*SetLSIVector)(void *, unsigned int, unsigned int);                /* 38 */
    int (*GetLSIVector)(void *, unsigned int, unsigned int *);              /* 39 */
    int (*SetSPIVector)(void *, unsigned int, unsigned int);                /* 40 */
    int (*GetSPIVector)(void *, unsigned int, unsigned int *);              /* 41 */
    int (*SetIPIVector)(void *, unsigned int, unsigned int);                /* 42 */
    int (*GetIPIVector)(void *, unsigned int, unsigned int *);              /* 43 */
    int (*SetIPIID)(void *, unsigned int, unsigned int);                    /* 44 */
    int (*GetIPIID)(void *, unsigned int, unsigned int *);                  /* 45 */
    int (*GenIPI)(void *, unsigned int);                                    /* 46 */
    int (*GenLSI)(void *);                                                  /* 47 */
    int (*GetNMISource)(void *, unsigned int *);                            /* 48 */
    int (*GetSPISource)(void *, unsigned int *);                            /* 49 */

    /*
     * more interrupt control stuff...
     */
    int (*GetLocalIRQStatus)(void *, unsigned int, unsigned int *, unsigned int *);
                                                                            /* 50 */
    int (*MaskableIntEOI)(void *, unsigned int);                            /* 51 */
    int (*NonMaskableIntEOI)(void *);                                       /* 52 */
    int (*CancelInterrupt)(void *, unsigned int, unsigned int);             /* 53 */
    
    /*
     * Timer control stuff
     */
    int (*GetSysTimer)(void *, unsigned int *);                             /* 54 */
    int (*GetSysTimerFreq)(void *, unsigned int *);                         /* 55 */
    
    /*
     * memory subsystem stuff...
     */
    int (*GetNumMemBlocks)(void *, unsigned int *);                         /* 56 */
    int GetNumMemBlocks16;                                                  /* 57 */
    int (*GetMemBlockInfo)(void *, memoryBlockInfo *, unsigned int);        /* 58 */
    int GetMemBlocksInfo16;                                                 /* 59 */
    int (*GetMemErrorInfo)(void *, memoryErrorInfo *);                      /* 60 */
    
    /*
     * miscellaneous functions
     */
    int (*GetRevision)(void *, revisionCode *);                             /* 61 */
    int (*GetNumSlots)(unsigned int *);                                     /* 62 */
    int (*GetMMIOTable)(IOInfoTable *);                                     /* 63 */
    int (*InitEBI)(void *);                                                 /* 64 */
    int (*GetThermalState)(void *, unsigned int *);                         /* 65 */
    int (*ShutdownPowerSupply)(void *);                                     /* 66 */
    int (*SimulatePowerFail)(void *, unsigned int);                         /* 67 */
    int (*SetPanelSwitchVisibility)(void *, unsigned int);                  /* 68 */
    int (*GetPanelSwitchVisibility)(void *, unsigned int *);                /* 69 */
    int (*GetGlobalIRQStatus)(void *, unsigned int *, unsigned int *);      /* 70 */
    void (*FastSetLocalIntMask)(unsigned int, unsigned int);                /* 71 */
    int (*GetProcIntHandle)(void *, unsigned int, unsigned int *);          /* 72 */
    void (*RegSetLocalIntMask)();                                           /* 73 */
    int (*GetLocalIntMaskInfo)(void *, maskInfo *, unsigned int);           /* 74 */
    int (*SpuriousIntEOI)(void *);                                          /* 75 */
    int (*ASTx[21])(void *, ...);
    int (*OEM[32])(void *, ...);
} EBI_II;

/*
 * BIOS offset table
 */
typedef unsigned int offsetTable[128];

/*
 * location of BIOS segment
 */
#define EBI_BIOS_SEG    ((unsigned int)0xF0000000)
#define EBI_BIOS_SIZ    ((unsigned int)0xFFFF)
#define EBI_BIOS_LIM    ((unsigned int)(EBI_BIOS_SEG + EBI_BIOS_SIZ))

/*
 * Definitions for 4.0MP HAL interface stuff...
 */
#define EBI_SS          (0x25 * 8)    /* offset of gdt selector 25               */
#define EBI_STACK_SZ    0x1000        /* 1 page of stack area, 512 bytes per cpu */
#define EBI_MAX_CPUS    8             /* maximum number of cpu's we will support */

#define NPIC_CPU0       2             /* the usual number..., this will go away  */

#define IS_BOOT_ENG(x)  ((x) == BOOTENG)
#endif

/*
 *  ADDITIONAL FRONT PANEL DISPLAY SUPPORT
 */

/* Additional ioctls */
#define EBI_SET_ALPHA_MODE       (EBIIOC|14) /* set mode of alpha display  */
#define EBI_GET_ALPHA_MODE       (EBIIOC|15) /* get mode of alpha display  */
#define EBI_CLEAR_EVENTS         (EBIIOC|16) /* clear all event flags  M001 */
#define EBI_SET_RAM_CACHE	 (EBIIOC|17) /* turn on external cache M002 */

 /*
  * Front panel display modes added by Microport per USG requirement
  */
#define PANEL_MODE_ONLINE	 0x10000	/* added by Microport */

 /*
  * Alpha display modes added by Microport--not in EBI
  */
#define ALPHA_MODE_TEXT 	0x0			/* can display strings */
#define ALPHA_MODE_UTIL		0x1			/* shows CPU utilization */
#define UTIL_REFRESH_SLOW	0x2			/* select slow utilization refresh */
#define UTIL_REFRESH_MEDIUM 0x3			/* select medium utilization refresh */
#define UTIL_REFRESH_FAST   0x4			/* select slow utilization refresh */

#define UTIL_SLOW_RATE		25			/* value for slow refresh */
#define UTIL_MEDIUM_RATE	50			/* value for slow refresh */
#define UTIL_FAST_RATE		100			/* value for slow refresh */

/*
 * RAM cache enable/disable (implemented in astm driver)
 * controls both primary (internal) and secondary (external) 
 */
#define RAM_CACHE_DISABLE	0x0
#define RAM_CACHE_ENABLE  	0x1

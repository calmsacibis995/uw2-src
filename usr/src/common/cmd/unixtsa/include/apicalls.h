/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/apicalls.h	1.1"

#ifndef APICALLS
#define APICALLS 1

#define WS_NAME_SIZE 48
#define WS_VERSION_SIZE 10
#define WS_OS_TYPE_SIZE 30

#define MAX_WORKSTATION_TYPES 1
#define WORKSTATION_TSR       1

#define DOS_VERSION_SIZE      10
#define DOS_DIRECTORY_SIZE    64
#define DOS_LOCAL_DRIVES      26
#define DOS_BUFFER_SIZE       500
#define MAX_ENCRYPTED_PASSWORD 16

typedef struct
{
   char LocalDrives[ DOS_LOCAL_DRIVES ] ;
   short NumberOfBuffers ;
} DOS_TSR ;

#ifdef ENGINE_DEFINED
int WS_Sizes[ MAX_WORKSTATION_TYPES ] =
{
   sizeof( DOS_TSR )
};
#else
extern int WS_Sizes[];
#endif

typedef union
{
   DOS_TSR DosTsr ;
} MAX_WORKSTATION_SIZE ;

#define MAX_FUNCTIONS          9

#define LOGIN_TO_SERVER        0
#define LOGOUT_FROM_SERVER     1
#define RET_DOS_INT            2
#define GET_WORKSTATION_NAME   3
#define CONNECT_TO_WORKSTATION 4
#define DOS_INT                5
#define DOS_READ_BUFFER        6
#define VERIFY_DOS_PASSWORD    7
#define GET_NETWORK_ADDRESS    8


typedef struct
{
   short Function ;                               /* Function Number         */
   unsigned short Size ;                          /* Size of this structure  */
   long NetworkAddress ;                          /* Network Number          */
   char NodeAddress[6] ;                          /* Node Number             */
   char WorkstationName[WS_NAME_SIZE+1];          /* Local Workstation Name  */
   short Task;                                    /* Task Number             */
   short WorkstationType ;                        /* Workstation Type        */
        short Trust;                                   /* Trust                   */
   char WorkstationVersion[ WS_VERSION_SIZE ] ;
   char WorkstationOS[ WS_OS_TYPE_SIZE ] ;
   char WorkstationData[ sizeof(MAX_WORKSTATION_SIZE) ] ;
} LOGIN_CALL;

typedef struct
{
   short ErrorCode;
   unsigned short Size;                           /* Size of this structure  */
   short UserNo;                                  /* User Number             */
}LOGIN_RET;


typedef struct
{
   short Function;                                /* Function Number         */
   unsigned short Size;                           /* Size of this structure  */
   char  Password[10];
}LOGOUT_CALL;

typedef struct
{
   short ErrorCode;
   unsigned short Size;                           /* Size of this structure  */
}LOGOUT_RET;

/* Input buffer information goes  in BufferType[0] */
/* Output buffer information goes in BufferType[1] */
#define NO_BUFFER    0
#define BUFFER_TO_DX 1
#define BUFFER_TO_SI 2
#define BUFFER_TO_DI 3

struct SMS_WORDREGS
{
        unsigned short  ax, bx, cx, dx, cflag, flags;
};


struct SMS_BYTEREGS
{
        unsigned char   al, ah, bl, bh, cl, ch, dl, dh;
};
typedef union
{
        struct  SMS_WORDREGS x;
        struct  SMS_BYTEREGS h;
} REGS;


typedef struct
{
   short Function;                                /* Function Number         */
   unsigned short Size;                           /* Size of this structure  */
   REGS Registers;
   char BufferType[2];                            /* see if data coming back */
   char Buffer[DOS_BUFFER_SIZE];                  /* 512 byte write buffer   */
} INT_DOS_CALL;

typedef struct
{
   short Function;                                /* Function Number         */
   unsigned short Size;                           /* Size of this structure  */
   short ErrorCode;
   REGS Registers;
   char  Buffer[DOS_BUFFER_SIZE];                 /* 512 byte read buffer    */
} INT_DOS_RET;


typedef struct
{
   short Function;                                /* Function Number         */
   unsigned short Size;                           /* Size of this structure  */
   REGS Registers;
   char BufferType[2];                            /* see if data coming back */
   char Buffer[DOS_BUFFER_SIZE];                 /* 512 byte write buffer    */
   char *WorkBuffer ;
} INT_DOS_READ_CALL;


typedef struct
{
   short Function;                                /* Function Number         */
   unsigned short Size;                           /* Size of this structure  */
   long WorkstationNumber ;                       /* see if data coming back */
} GET_WORKSTATION_NAME_CALL ;

typedef struct
{
   short ErrorCode ;
   unsigned short Size ;                          /* Size of this structure  */
   long  Sequence ;
   short WorkstationType ;
   char  WorkstationVersion[ WS_VERSION_SIZE ] ;
   char  WorkstationOS[ WS_OS_TYPE_SIZE ] ;
   char  WorkstationName[ WS_NAME_SIZE + 1 ] ;    /* Area to return Name     */
} GET_WORKSTATION_NAME_RET ;


typedef struct
{
   short Function;                                /* Function Number         */
   unsigned short Size;                           /* Size of this structure  */
   char  WorkstationName[ WS_NAME_SIZE + 1 ] ;    /* Area to return Name     */
	char  LoginName[ 49 ] ;
	char  Password[ 49 ] ;
} TSA_CONNECT_CALL ;

typedef struct
{
   short ErrorCode ;
   unsigned short Size ;                          /* Size of this structure  */
   long ConnectHandle ;
   void  *WorkstationData ;
} TSA_CONNECT_RET ;

typedef struct
{
   short Function;                                /* Function Number         */
   unsigned short Size;                           /* Size of this structure  */
   char  WorkstationName[ WS_NAME_SIZE + 1 ] ;    /* Wrkstation name         */
} GET_NETWORK_ADDRESS_CALL ;

typedef struct
{
   short ErrorCode ;
   unsigned short Size ;                          /* Size of this structure  */
   char networkAddress [12];                      /* betwork Address         */
} GET_NETWORK_ADDRESS_RET ;

typedef struct
{
   short Function;                                /* Function Number         */
   unsigned short Size;                           /* Size of this structure  */
   char  Password[ MAX_ENCRYPTED_PASSWORD] ;      /* Area to return Name     */
} VERIFY_DOS_PASSWORD_CALL ;

typedef struct
{
   short Function;                                /* Function Number         */
   unsigned short Size ;                          /* Size of this structure  */
   short ErrorCode ;
} VERIFY_DOS_PASSWORD_RET ;

typedef union
{
   LOGIN_CALL Login ;
   LOGOUT_CALL Logout ;
   INT_DOS_CALL IntDos ;
   GET_WORKSTATION_NAME_CALL GetName ;
   TSA_CONNECT_CALL GetConnect ;
   VERIFY_DOS_PASSWORD_CALL GetPassword;
} API_SENDING_CALLS ;

typedef union
{
   LOGIN_RET Login ;
   LOGOUT_RET Logout ;
   INT_DOS_RET IntDos ;
   GET_WORKSTATION_NAME_RET GetName ;
   TSA_CONNECT_RET GetConnect ;
   VERIFY_DOS_PASSWORD_RET GetPassword;
} API_RETURNING_CALLS ;


#define WSE_OK                              0
#define WSE_ERROR_INVALID_FUNCTION         -1
#define WSE_ERROR_LOGIN_AGAIN              -2
#define WSE_ERROR_USER_TABLE_OVERFLOW      -3
#define WSE_ERROR_ALLOCATING_MEMORY        -4
#define WSE_ERROR_INVALID_WORKSTATION_TYPE -5
#define WSE_ERROR_SERVER_NOT_RESPONDING    -6
#define WSE_ERROR_READ_WRITE_RETRY         -7
#define WSE_ERROR_VERIFYING_PASSWORD       -8
#define WSE_ERROR_INVALID_WORKSTATION_NAME -9
long CallWSEngine( long CommunicationHandle, void *Send ,void *Ret ) ;
#endif

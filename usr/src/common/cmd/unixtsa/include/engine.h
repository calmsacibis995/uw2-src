/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/engine.h	1.1"

#ifdef SERVER
#define CLIENT 0
#else
#define SERVER 0
#endif

#ifdef NLM
#include <process.h>
#define VAP 0
#define OS2 0
#define TSR 0
#else
#ifdef VAP
#define NLM 0
#define OS2 0
#define TSR 0
#else
#ifdef OS2
#define NLM 0
#define VAP 0
#define TSR 0
#else
#ifdef TSR
#define NLM 0
#define VAP 0
#define OS2 0
#endif
#endif
#endif
#endif

#if OS2 == 1
#define INCL_BASE 1
#include <os2.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <process.h>
#include <conio.h>
#include <smsspx.h>
#include <packet.h>
#include <apicalls.h>

#define NUMBER_OF_USERS 100
#define SOCKET 0x902e                             /* Rconsole Socket Value   */

#ifdef SYSVNO
typedef void   *BUFFERPTR;
#else
typedef unsigned char   *BUFFERPTR;
#fi

typedef struct
{
   long CommHandle ;                              /* Communication Handle    */
   unsigned long NetworkAddress ;                 /* Physical Network        */
   unsigned char NodeAddress[6] ;                 /* Physical Node addr      */
   long WorkstationSemaphore ;                    /* Workstation Semaphore   */
   char WorkstationName[ WS_NAME_SIZE+1 ] ;
   int  WorkstationType ;
   char WorkstationVersion[ WS_VERSION_SIZE ] ;
   char WorkstationOS[ WS_OS_TYPE_SIZE ] ;
   char WorkstationData[ sizeof( MAX_WORKSTATION_SIZE) ] ;
   short Unique ;
   short Task ;
   short Trust ;
   short UserNo ;                                 /* User number of data     */
   char DataIn[ sizeof( API_SENDING_CALLS ) ] ;
   char DataOut[ sizeof( API_RETURNING_CALLS ) ] ;
   char *SbackupRdWrBuffer ;
} USER_DATA ;

void MCCSLogin( long CommHandle, LOGIN_CALL *Login, LOGIN_RET *LoginRet ) ;
void MCCSLogOut( USER_DATA *U, LOGOUT_CALL *Send, LOGOUT_RET *Ret ) ;
void IntDOS( USER_DATA *U, INT_DOS_CALL *Send, INT_DOS_RET *Ret ) ;
void IntDOSRead( USER_DATA *U, INT_DOS_READ_CALL *Send, INT_DOS_RET *Ret ) ;
void GetWorkstationName( USER_DATA *U, GET_WORKSTATION_NAME_CALL *Get,
                                             GET_WORKSTATION_NAME_RET  *ret ) ;
void GetNetworkAddress( USER_DATA *U,
                        GET_NETWORK_ADDRESS_CALL *Get,
                        GET_NETWORK_ADDRESS_RET *Ret );

void ConnectToWorkstation( USER_DATA *U, TSA_CONNECT_CALL *Get,
                                                       TSA_CONNECT_RET *Ret ) ;
void CallIntDOS( long ConnectHandle, INT_DOS_CALL *DosIn,
                                                       INT_DOS_RET **DosOut ) ;

void CallIntDOSRead( USER_DATA *U, INT_DOS_RET *DosIn, INT_DOS_RET *DosOut );

void SMSNotUsed( void ) ;

int RegisterWorkstation(
		BUFFERPTR workstationName,
		BUFFERPTR workstationAddress);


#ifdef ENGINE_DEFINED
 void ( *call[ MAX_FUNCTIONS ] )() =
 {
    MCCSLogin,
    MCCSLogOut,
    IntDOS,
    GetWorkstationName,
    ConnectToWorkstation,
    CallIntDOS,
    CallIntDOSRead,
	 IntDOS,
	 GetNetworkAddress
 };
 int  Unloaded = 0 ;
 int  NumberLoggedUsers = 0 ;
 long CommunicationHandle = 0 ;
 long SessionHandle = 0 ;
 short Unique = 0;
 char dbuf[80] ;
 USER_DATA **User ;
#else
 extern void ( *call[] )() ;
 extern int Unloaded ;
 extern int NumberLoggedUsers ;
 extern long CommunicationHandle ;
 extern long SessionHandle ;
 extern short Unique ;
 extern char dbuf[] ;
 extern USER_DATA **User ;
#endif

void InitEngine( void ) ;
void DeInitMCCS( void ) ;
void WaitForUsers( void ) ;
void MCCSEngine( USER_DATA *U ) ;
int  MCCSUnload( void ) ;

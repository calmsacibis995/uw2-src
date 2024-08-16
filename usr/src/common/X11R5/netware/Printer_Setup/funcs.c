/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:funcs.c	1.12"
#ident "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Printer_Setup/funcs.c,v 1.12.2.1 1994/10/18 14:53:35 wrees Exp $"


/*------------------------------------------------------------
** File : funcs.c
**
** Description : This file contains functions to load and 
**               execute functions from the libnwapi library.
**               This enables Printer_Setup to run without 
**               linking in libnwapi.so ( ie NUC package not
**               loaded ).
**-----------------------------------------------------------*/

/*-------------------------------------------------------------
**                      I N C L U D E S 
**-----------------------------------------------------------*/


#include <Xm/Xm.h>
#include <stdio.h>
#include <nct.h>
#include <nw/nwcalls.h>
#include <dlfcn.h>
#include <sys/utsname.h>
#include <mail/link.h>
#include <sys/types.h>
#include <netdb.h>
#include <mail/hosts.h>
#include <mail/tree.h>
#include <mail/table.h>



/*******************************************************************/
/*
 			D E F I N E S
*/
#define NWMAX_JOB_FILE_NAME_LENGTH      14
#define NWMAX_JOB_DESCRIPTION_LENGTH    50
#define NWMAX_QUEUE_JOB_TIME_SIZE       6
#define NWMAX_FORM_NAME_LENGTH          16
#define NWMAX_OBJECT_NAME_LENGTH        48
#define NWMAX_PROPERTY_NAME_LENGTH      16
#define NWMAX_SERVER_NAME_LENGTH        48
#define NWMAX_FILE_NAME_LENGTH          14
#define NWMAX_PROPERTY_VALUE_LENGTH     128
#define NWMAX_QUEUE_NAME_LENGTH         48
#define NWMAX_BANNER_NAME_FIELD_LENGTH  13
#define NWMAX_BANNER_FILE_FIELD_LENGTH  13
#define NWMAX_HEADER_FILE_NAME_LENGTH   14
#define NWMAX_JOB_DIR_PATH_LENGTH       80

/********************************************************************/
/*
	Global structures
*/
typedef struct sapi {
	unsigned int serverType;
	unsigned char serverName[48];
} SAPI;


typedef struct NptRequest {
	uint8	FunctionNumber;			/* used by all packets */
	uint8	ByPage;
	char	Character;
	uint8	Immediately;
	uint8	JobOutcome;
	uint8	MaxNumberOfEntries;
	uint8	PrinterNumber;
	uint8	Priority;
	uint8	Relative;
	uint8	Sequence8;
	uint8	ServiceMode;
	uint8	Shared;
	uint16	ConnectionNumber;
	uint16	CopyNumber;
	uint16	FirstNotice;
	uint16	FormNumber;
	uint16	Interval;
	uint16	ObjectType;
	uint16	Sequence16;
	uint32	Offset;
	char	FileServerName[NWMAX_SERVER_NAME_LENGTH];
	char	ObjectName[NWMAX_OBJECT_NAME_LENGTH];
	char	QueueName[NWMAX_QUEUE_NAME_LENGTH];
	char	Password[NWMAX_PROPERTY_VALUE_LENGTH];
} NptRequest_t;

typedef struct NptReply {
	uint16	CompletionCode;			/* used by all packets */
	uint8	AccessLevel;
	uint8	ActiveJob;
	uint8	AttachedPrinters;
	uint8	ErrorCode;
	uint8	MajorVersionNumber;
	uint8	MinorVersionNumber;
	uint8	NumPrinters;
	uint8	PSType;
	uint8	PrinterNumber;
	uint8	Priority;
	uint8	RevisionNumber;
	uint8	Sequence8;
	uint8	ServiceMode;
	uint8	status;
	uint8	TextByteStream;
	uint16	BaudRate;
	uint16	Blocks;
	uint16	CopiesInJob;
	uint16	CopiesPrinted;
	uint16	DataBits;
	uint16	FirstNotice;
	uint16	FormNumber;
	uint16	IRQNumber;
	uint16	Interval;
	uint16	JobQueueNumber;
	uint16	ObjectType;
	uint16	ParityType;
	uint16	PrinterType;
	uint16	Protocol;
	uint16	Sequence16;
	uint16	Socket;
	uint16	StopBits;
	uint16	UseInterrupts;
	uint32	BytesIntoCurrentCopy;
	uint32	CopySize;
	uint8	SerialNumber[4];
	char	FormName[NWMAX_FORM_NAME_LENGTH];
	char	FileServerName[NWMAX_SERVER_NAME_LENGTH];
	char	QueueName[NWMAX_QUEUE_NAME_LENGTH];
	char	ObjectName[NWMAX_OBJECT_NAME_LENGTH];
	char	PrinterName[NWMAX_OBJECT_NAME_LENGTH];
	char	JobDescription[NWMAX_JOB_DESCRIPTION_LENGTH];
	uint8	*ServicingQueueArray;
} NptReply_t;

int SAPGetAllServers( int, unsigned int *, SAPI *, int );



/*-------------------------------------------------------------
**                      D E F I N E S 
**-----------------------------------------------------------*/
#define LIBNWAPI	"libnwapi.so"
#define LIBSERVER	"libserver.so"
#define LIBHOSTS	"libhosts.so"
#define LIBTREE		"libtree.so"
#define LIBNWCAL	"libNwCal.so"
#define LIBNWUTIL	"libnwutil.so"

/*-------------------------------------------------------------
**                      T Y P E D E F S 
**-----------------------------------------------------------*/
/*-------------------------------------------------------------
**                    DANGER, DANGER, DANGER
** This structure is also defined in /lib/libnwapi/netWareTypes.c.
** The structure definition should really be moved to a header
** file, but since this change is happening sooooooo late.... I
** was told to just duplicate the definition here. Sorryyyyyy!!!
**------------------------------------------------------------*/ 
/*typedef struct netWareTypeFields_s
{
   int stf_size;
   unsigned char stf_protocol, *stf_vector;
} netWareTypeField_t;*/

/*-------------------------------------------------------------
**                      V A R I A B L E S 
**-----------------------------------------------------------*/
static void *NetWareHandle 	= 0;
static void *HostsHandle 	= 0;
static void *TreeHandle 	= 0;
static void *NWCalHandle	= 0;
static void *NWUtilHandle	= 0;
static void *XServerHandle	= 0;
static void *ServerHandle	= 0;

/*--------------------------------------------------------------
** Function : linkNew 
**-------------------------------------------------------------*/
/*link_t *linkNew( void *owner )
{
   static link_t * ( *func )() = NULL;
   if ( func == NULL )
   {
       if ( NetWareHandle != 0 )
           func = ( link_t * (*)() )dlsym( NetWareHandle, "linkNew");
       else if ( ( NetWareHandle = dlopen( LIBNWAPI, RTLD_LAZY ) )  != 0 )
           func = ( link_t * (*)() )dlsym( NetWareHandle, "linkNew");
   }
   if ( func != NULL )
       return( (*func)( owner ) );
   else
       return( NULL );
}*/

/*--------------------------------------------------------------
** Function : linkNext 
**-------------------------------------------------------------*/
/*link_t *linkNext( link_t *link )
{
   static link_t * ( *func )() = NULL;

   if ( func == NULL )
   {
       if ( NetWareHandle != 0 )
           func = ( link_t * (*)() )dlsym( NetWareHandle, "linkNext");
       else if ( ( NetWareHandle = dlopen( LIBNWAPI, RTLD_LAZY ) )  != 0 )
           func = ( link_t * (*)() )dlsym( NetWareHandle, "linkNext");
   }
   if ( func != NULL )
       return( (*func)( link ) );
   else 
       return( NULL );
}*/

/*--------------------------------------------------------------
** Function : linkFree 
**-------------------------------------------------------------*/
/*void  linkFree( link_t *link )
{
   static void ( *func )() = NULL;

   if ( func == NULL )
   {
       if ( NetWareHandle != 0 )
           func = ( void (*)() )dlsym( NetWareHandle, "linkFree");
       else if ( ( NetWareHandle = dlopen( LIBNWAPI, RTLD_LAZY ) )  != 0 )
           func = ( void (*)() )dlsym( NetWareHandle, "linkFree");
   }
   if ( func != NULL )
       (*func)( link );
}*/

/*--------------------------------------------------------------
** Function : linkAddSorted
**-------------------------------------------------------------*/
/*link_t *linkAddSorted( link_t *list, link_t *link, int (*compFunc)() )
{
   static link_t * ( *func )() = NULL;

   if ( func == NULL )
   {
       if ( NetWareHandle != 0 )
           func = ( link_t * (*)() )dlsym( NetWareHandle, "linkAddSorted");
       else if ( ( NetWareHandle = dlopen( LIBNWAPI, RTLD_LAZY ) )  != 0 )
           func = ( link_t * (*)() )dlsym( NetWareHandle, "linkAddSorted");
   }
   if ( func != NULL )
       return( (*func)( list, link, compFunc ));
   else 
       return( NULL );
}*/

/*--------------------------------------------------------------
** Function : linkOwner
**-------------------------------------------------------------*/
/*void *linkOwner( link_t *link )
{
   static void * ( *func )() = NULL;

   if ( func == NULL )
   {
       if ( NetWareHandle != 0 )
           func = ( void * (*)() )dlsym( NetWareHandle, "linkOwner");
       else if ( ( NetWareHandle = dlopen( LIBNWAPI, RTLD_LAZY ) )  != 0 )
           func = ( void * (*)() )dlsym( NetWareHandle, "linkOwner");
   }
   if ( func != NULL )
       return( (*func)( link ));
   else 
       return( NULL );
}*/

/*--------------------------------------------------------------
** Function : NWIsConnectionAuthenticated 
**-------------------------------------------------------------*/
/*NWBoolean_ts NWIsConnectionAuthenticated( uint16 connId )
{
   static NWBoolean_ts ( *func )() = NULL;

   if ( func == NULL )
   {
       if ( NetWareHandle != 0 )
           func = ( NWBoolean_ts (*)() )dlsym( NetWareHandle, "NWIsConnectionAuthenticated");
       else if ( ( NetWareHandle = dlopen( LIBNWAPI, RTLD_LAZY ) )  != 0 )
           func = ( NWBoolean_ts (*)() )dlsym( NetWareHandle, "NWIsConnectionAuthenticated");
   }
   if ( func != NULL )
       return( (*func)( connId ));
   else 
       return( FALSE );
}*/
/*--------------------------------------------------------------
** Function : NWGetServerConnID 
**-------------------------------------------------------------*/
/*int NWGetServerConnID( uint8 *fileServerName, uint16 *connID )
{
   static int ( *func )() = NULL;

   if ( func == NULL )
   {
       if ( NetWareHandle != 0 )
           func = ( int (*)() )dlsym( NetWareHandle, "NWGetServerConnID");
       else if ( ( NetWareHandle = dlopen( LIBNWAPI, RTLD_LAZY ) )  != 0 )
           func = ( int (*)() )dlsym( NetWareHandle, "NWGetServerConnID");
   }
   if ( func != NULL )
       return( (*func)( fileServerName, connID ));
   else 
       return( -1 );
}*/


/*--------------------------------------------------------------
** Function : NWScanObject 
**-------------------------------------------------------------*/
/*NWBoolean_ts NWScanObject( uint16 serverConnID, char *searchObjectName,
                           uint16 searchObjectType, int32 *sequence,
                           NWObjectInfo_t *objectInformation )
{
   static NWBoolean_ts ( *func )() = NULL;

   if ( func == NULL )
   {
       if ( NetWareHandle != 0 )
           func = ( NWBoolean_ts (*)() )dlsym( NetWareHandle, "NWScanObject");
       else if ( ( NetWareHandle = dlopen( LIBNWAPI, RTLD_LAZY ) )  != 0 )
           func = ( NWBoolean_ts (*)() )dlsym( NetWareHandle, "NWScanObject");
   }
   if ( func != NULL )
       return( (*func)( serverConnID, searchObjectName, searchObjectType,
                        sequence, objectInformation ));
   else 
       return( FALSE );
}*/


/*--------------------------------------------------------------
** Function : NWScanPropertyValue 
**-------------------------------------------------------------*/
/*NWBoolean_ts NWScanPropertyValue( uint16 serverConnID, char *objectName,
                           uint16 objectType, char *propertyName,
                           uint8 *segmentNumber, char *segmentData,
                           uint8 *moreSegments, uint8 *propertyType )
{
   static NWBoolean_ts ( *func )() = NULL;

   if ( func == NULL )
   {
       if ( NetWareHandle != 0 )
           func = ( NWBoolean_ts (*)() )dlsym( NetWareHandle, "NWScanPropertyValue");
       else if ( ( NetWareHandle = dlopen( LIBNWAPI, RTLD_LAZY ) )  != 0 )
           func = ( NWBoolean_ts (*)() )dlsym( NetWareHandle, "NWScanPropertyValue");
   }
   if ( func != NULL )
       return( (*func)( serverConnID, objectName, objectType, propertyName,
                      segmentNumber, segmentData, moreSegments, propertyType ));
   else 
       return( FALSE );
}*/

/*--------------------------------------------------------------
** Function : NWGetObjectName 
**-------------------------------------------------------------*/
/*int NWGetObjectName( uint16 serverConnID, uint32 objectID,
                     char *objectName, uint16 *objectType )
{
   static int ( *func )() = NULL;

   if ( func == NULL )
   {
       if ( NetWareHandle != 0 )
           func = ( int (*)() )dlsym( NetWareHandle, "NWGetObjectName");
       else if ( ( NetWareHandle = dlopen( LIBNWAPI, RTLD_LAZY ) )  != 0 )
           func = ( int (*)() )dlsym( NetWareHandle, "NWGetObjectName");
   }
   if ( func != NULL )
       return( (*func)( serverConnID, objectID, objectName, objectType ) ); 
   else 
       return( FALSE );
}*/

/*--------------------------------------------------------------
** Function : netWareServerListGet 
**-------------------------------------------------------------*/
/*char **netWareServerListGet( void *sapTypeField )
{
   static char ** ( *func )() = NULL;

   if ( func == NULL )
   {
       if ( NetWareHandle != 0 )
          func = ( char ** (*)() )dlsym(NetWareHandle, "netWareServerListGet");
       else if ( ( NetWareHandle = dlopen( LIBNWAPI, RTLD_LAZY ) )  != 0 )
          func = ( char ** (*)() )dlsym(NetWareHandle, "netWareServerListGet");
   }
   if ( func != NULL )
       return( (*func)( sapTypeField ) ); 
   else 
       return( NULL );
}*/

/*--------------------------------------------------------------
** Function : netWareServerListFree 
**-------------------------------------------------------------*/
/*void netWareServerListFree( char **list )
{
   static void ( *func )() = NULL;

   if ( func == NULL )
   {
       if ( NetWareHandle != 0 )
          func = ( void (*)() )dlsym(NetWareHandle, "netWareServerListFree");
       else if ( ( NetWareHandle = dlopen( LIBNWAPI, RTLD_LAZY ) )  != 0 )
          func = ( void (*)() )dlsym(NetWareHandle, "netWareServerListFree");
   }
   if ( func != NULL )
       (*func)( list ); 
}*/

/*--------------------------------------------------------------
** Function : netWareTypeNewWithData 
**-------------------------------------------------------------*/
/*void *netWareTypeNewWithData( unsigned char *data, int datLen )
{
   static void  *( *func )() = NULL;

   if ( func == NULL )
   {
       if ( NetWareHandle != 0 )
          func = ( void *(*)() )dlsym(NetWareHandle, "netWareTypeNewWithData");
       else if ( ( NetWareHandle = dlopen( LIBNWAPI, RTLD_LAZY ) )  != 0 )
          func = ( void *(*)() )dlsym(NetWareHandle, "netWareTypeNewWithData");
   }
   if ( func != NULL )
       return ( (*func)( data, datLen ) ); 
   else
       return( NULL );
}*/


/*--------------------------------------------------------------
** Function : netWareTypeSet 
**-------------------------------------------------------------*/
/*void netWareTypeSet( netWareTypeField_t *field, netWareType_t type )
{
   static void ( *func )() = NULL;

   if ( func == NULL )
   {
       if ( NetWareHandle != 0 )
          func = ( void (*)() )dlsym(NetWareHandle, "netWareTypeSet");
       else if ( ( NetWareHandle = dlopen( LIBNWAPI, RTLD_LAZY ) )  != 0 )
          func = ( void (*)() )dlsym(NetWareHandle, "netWareTypeSet");
   }
   if ( func != NULL )
       (*func)( field, type ); 
}*/


/*--------------------------------------------------------------
** Function : netWareTypeFree 
**-------------------------------------------------------------*/
/*void netWareTypeFree( netWareTypeField_t *field )
{
   static void ( *func )() = NULL;

   if ( func == NULL )
   {
       if ( NetWareHandle != 0 )
          func = ( void (*)() )dlsym(NetWareHandle, "netWareTypeFree");
       else if ( ( NetWareHandle = dlopen( LIBNWAPI, RTLD_LAZY ) )  != 0 )
          func = ( void (*)() )dlsym(NetWareHandle, "netWareTypeFree");
   }
   if ( func != NULL )
       (*func)( field ); 
}*/



/*--------------------------------------------------------------
** Function : NWCloseTask 
**-------------------------------------------------------------*/
/*int NWCloseTask( uint16 connID )
{
   static int ( *func )() = NULL;

   if ( func == NULL )
   {
       if ( NetWareHandle != 0 )
          func = ( int (*)() )dlsym(NetWareHandle, "NWCloseTask");
       else if ( ( NetWareHandle = dlopen( LIBNWAPI, RTLD_LAZY ) )  != 0 )
          func = ( int (*)() )dlsym(NetWareHandle, "NWCloseTask");
   }
   if ( func != NULL )
       return ( (*func)( connID ) ); 
   else
       return( -1 );
}*/


/*--------------------------------------------------------------
** Function : hostName 
**-------------------------------------------------------------*/
char *hostName( void *handle_p )
{
   static void *( *func )( void * ) = NULL;

   if ( func == NULL )
   {
       if ( HostsHandle != 0 )
          func = ( void  *(*)( void * ) )
			dlsym(HostsHandle, "hostName");
       else if ( ( HostsHandle = dlopen( LIBHOSTS, RTLD_LAZY ) )  != 0 )
          func = ( void *(*)( void * ) )
			dlsym(HostsHandle, "hostName");
   }
   if ( func != NULL )
       return ( (*func)( handle_p ) ); 
   else
       return( NULL );
}


/*--------------------------------------------------------------
** Function : hostInit 
**-------------------------------------------------------------*/
void *hostInit( int debugLevel )
{
   static void *( *func )( int ) = NULL;

   if ( func == NULL )
   {
       if ( HostsHandle != 0 )
          func = ( void  *(*)( int ) )
			dlsym(HostsHandle, "hostInit");
       else if ( ( HostsHandle = dlopen( LIBHOSTS, RTLD_LAZY ) )  != 0 )
          func = ( void *(*)( int ) )
			dlsym(HostsHandle, "hostInit");
   }
   if ( func != NULL )
       return ( (*func)( debugLevel ) ); 
   else
       return( NULL );
}


/*--------------------------------------------------------------
** Function : hostNew 
**-------------------------------------------------------------*/
void *hostNew( char *name, char *address, void *data, void ( *dataFree )() )
{
   static void *( *func )( char *, char *, void *, void(*)() ) = NULL;

   if ( func == NULL )
   {
       if ( HostsHandle != 0 )
          func = ( void  *(*)( char *, char *, void *, void(*)() ) )
			dlsym(HostsHandle, "hostNew");
       else if ( ( HostsHandle = dlopen( LIBHOSTS, RTLD_LAZY ) )  != 0 )
          func = ( void *(*)( char *, char *, void *, void(*)() ) )
			dlsym(HostsHandle, "hostNew");
   }
   if ( func != NULL )
       return ( (*func)( name, address, data, dataFree ) ); 
   else
       return( NULL );
}
/*--------------------------------------------------------------
** Function : tableDeleteEntryByValue 
**-------------------------------------------------------------*/
int tableDeleteEntryByValue( table_t *table, void *data_p ) 
{
   static int ( *func )( table_t *, void * ) = NULL;

   if ( func == NULL )
   {
       if ( ServerHandle != 0 )
          func = ( int (*)( table_t *, void * ) )
			dlsym(ServerHandle, "tableDeleteEntryByValue");
       else if ( ( ServerHandle = dlopen( LIBSERVER, RTLD_LAZY ) )  != 0 )
          func = ( int (*)( table_t *, void * ) )
			dlsym( ServerHandle, "tableDeleteEntryByValue");
   }
   if ( func != NULL )
       return ( (*func)( table, data_p ) ); 
   else
       return( NULL );
}

/*--------------------------------------------------------------
** Function : nodeNext 
**-------------------------------------------------------------*/
node_t *nodeNext( node_t *node_p )
{
   static node_t *( *func )( node_t * ) = NULL;

   if ( func == NULL )
   {
       if ( TreeHandle != 0 )
          func = ( node_t  *(*)( node_t * ) )
			dlsym(TreeHandle, "nodeNext");
       else if ( ( TreeHandle = dlopen( LIBTREE, RTLD_LAZY ) )  != 0 )
          func = ( node_t *(*)( node_t * ) )
			dlsym(TreeHandle, "nodeNext");
   }
   if ( func != NULL )
       return ( (*func)( node_p ) ); 
   else
       return( NULL );
}
/*--------------------------------------------------------------
** Function : treeListGetFirst 
**-------------------------------------------------------------*/
node_t *treeListGetFirst( treeList_t *treeList_p )
{
   static node_t *( *func )( treeList_t * ) = NULL;

   if ( func == NULL )
   {
       if ( TreeHandle != 0 )
          func = ( node_t  *(*)( treeList_t * ) )
			dlsym(TreeHandle, "treeListGetFirst");
       else if ( ( TreeHandle = dlopen( LIBTREE, RTLD_LAZY ) )  != 0 )
          func = ( node_t *(*)( treeList_t * ) )
			dlsym(TreeHandle, "treeListGetFirst");
   }
   if ( func != NULL )
       return ( (*func)( treeList_p ) ); 
   else
       return( NULL );
}
/*--------------------------------------------------------------
** Function : treeListOpen 
**-------------------------------------------------------------*/
int treeListOpen( treeList_t *treeList_p, void (*callback)(), void *data ) 
{
   static int ( *func )( treeList_t *, void (*)(), void * ) = NULL;

   if ( func == NULL )
   {
       if ( TreeHandle != 0 )
          func = ( int  (*)( treeList_t *, void (*)(), void * ) )
			dlsym(TreeHandle, "treeListOpen");
       else if ( ( TreeHandle = dlopen( LIBTREE, RTLD_LAZY ) )  != 0 )
          func = ( int (*)( treeList_t *, void (*)(), void * ) )
			dlsym(TreeHandle, "treeListOpen");
   }
   if ( func != NULL )
       return ( (*func)( treeList_p, callback, data ) ); 
   else
       return( -1 );
}
/*--------------------------------------------------------------
** Function : nodeData
**-------------------------------------------------------------*/
void *nodeData( node_t *node_p )
{
   static void *( *func )( node_t * ) = NULL;

   if ( func == NULL )
   {
       if ( TreeHandle != 0 )
          func = ( void  *(*)( node_t * ) )
			dlsym(TreeHandle, "nodeData");
       else if ( ( TreeHandle = dlopen( LIBTREE, RTLD_LAZY ) )  != 0 )
          func = ( void *(*)( node_t * ) )
			dlsym(TreeHandle, "nodeData");
   }
   if ( func != NULL )
       return ( (*func)( node_p ) ); 
   else
       return( NULL );
}
/*--------------------------------------------------------------
** Function : nodeIsInternal 
**-------------------------------------------------------------*/
int nodeIsInternal( node_t *node_p )
{
   static int ( *func )( node_t * ) = NULL;

   if ( func == NULL )
   {
       if ( TreeHandle != 0 )
          func = ( int (*)( node_t * ) )
			dlsym(TreeHandle, "nodeIsInternal");
       else if ( ( TreeHandle = dlopen( LIBTREE, RTLD_LAZY ) )  != 0 )
          func = ( int (*)( node_t * ) )
			dlsym(TreeHandle, "nodeIsInternal");
   }
   if ( func != NULL )
       return ( (*func)( node_p ) ); 
   else
       return( -1 );
}
/*--------------------------------------------------------------
** Function : nodeTreeList 
**-------------------------------------------------------------*/
void *nodeTreeList( node_t *node_p )
{
   static void *( *func )( node_t * ) = NULL;

   if ( func == NULL )
   {
       if ( TreeHandle != 0 )
          func = ( void  *(*)( node_t * ) )
			dlsym(TreeHandle, "nodeTreeList");
       else if ( ( TreeHandle = dlopen( LIBTREE, RTLD_LAZY ) )  != 0 )
          func = ( void *(*)( node_t * ) )
			dlsym(TreeHandle, "nodeTreeList");
   }
   if ( func != NULL )
       return ( (*func)( node_p ) ); 
   else
       return( NULL );
}
/*--------------------------------------------------------------
** Function : nodeName 
**-------------------------------------------------------------*/
char *nodeName( node_t *node_p )
{
   static void *( *func )( node_t * ) = NULL;

   if ( func == NULL )
   {
       if ( TreeHandle != 0 )
          func = ( void  *(*)( node_t * ) )
			dlsym(TreeHandle, "nodeName");
       else if ( ( TreeHandle = dlopen( LIBTREE, RTLD_LAZY ) )  != 0 )
          func = ( void *(*)( node_t * ) )
			dlsym(TreeHandle, "nodeName");
   }
   if ( func != NULL )
       return ( (*func)( node_p ) ); 
   else
       return( NULL );
}
/*--------------------------------------------------------------
** Function : NWScanObject 
**-------------------------------------------------------------*/
NWCCODE N_API NWScanObject( NWCONN_HANDLE connID, pnstr8 strSearchName,
		nuint16 SearchType, pnuint32 ObjID, pnstr8 strObjName,
		pnuint16 ObjType, pnuint8 HasPropertiesFlag,
		pnuint8 ObjFlags, pnuint8 ObjSecurity )
{
   static NWCCODE N_API ( *func )( NWCONN_HANDLE, pnstr8, nuint16,
			pnuint32, pnstr8, pnuint16, pnuint8,
			pnuint8, pnuint8 ) = NULL;

   if ( func == NULL )
   {
       if ( NWCalHandle != 0 )
          func = ( NWCCODE N_API (*)( NWCONN_HANDLE, pnstr8, nuint16,
				pnuint32, pnstr8, pnuint16, pnuint8,
				pnuint8, pnuint8 ) )
			dlsym(NWCalHandle, "NWScanObject");
       else if ( ( NWCalHandle = dlopen( LIBNWCAL, RTLD_LAZY ) )  != 0 )
          func = ( NWCCODE N_API (*)( NWCONN_HANDLE, pnstr8, nuint16,
				pnuint32, pnstr8, pnuint16, pnuint8,
				pnuint8, pnuint8 ) )
			dlsym(NWCalHandle, "NWScanObject");
   }
   if ( func != NULL )
       return ( (*func)( connID, strSearchName, SearchType,
			ObjID, strObjName, ObjType, 
			HasPropertiesFlag, ObjFlags, ObjSecurity ) ); 
   else
       return( -1 );
}


/*--------------------------------------------------------------
** Function : NWGetConnectionNumber 
**-------------------------------------------------------------*/
NWCCODE N_API NWGetConnectionNumber( NWCONN_HANDLE conn,
			NWCONN_NUM NWFAR *connNumber ) 
{
   static NWCCODE N_API ( *func )( NWCONN_HANDLE, NWCONN_NUM NWFAR * ) = NULL;

   if ( func == NULL )
   {
       if ( NWCalHandle != 0 )
          func = ( NWCCODE N_API (*)( NWCONN_HANDLE, NWCONN_NUM NWFAR * ) )
			dlsym(NWCalHandle, "NWGetConnectionNumber");
       else if ( ( NWCalHandle = dlopen( LIBNWCAL, RTLD_LAZY ) )  != 0 )
          func = ( NWCCODE N_API (*)( NWCONN_HANDLE, NWCONN_NUM NWFAR * ) )
			dlsym(NWCalHandle, "NWGetConnectionNumber");
   }
   if ( func != NULL )
       return ( (*func)( conn, connNumber ) ); 
   else
       return( -1 );
}
/*--------------------------------------------------------------
** Function : SAPGetAllServers 
**-------------------------------------------------------------*/
int SAPGetAllServers( int sapType, unsigned int *serverEntry, 
		SAPI *serverBuf, int maxEntries )
{
   static int ( *func )( int, unsigned int *, SAPI *, int ) = NULL;

   if ( func == NULL )
   {
       if ( NWUtilHandle != 0 )
          func = ( int (*)( int, unsigned int *, SAPI *, int  ) )
			dlsym(NWUtilHandle, "SAPGetAllServers");
       else if ( ( NWUtilHandle = dlopen( LIBNWUTIL, RTLD_LAZY ) )  != 0 )
          func = ( int (*)( int, unsigned int *, SAPI *, int ) )
			dlsym(NWUtilHandle, "SAPGetAllServers");
   }
   if ( func != NULL )
       return ( (*func)( sapType, serverEntry, serverBuf, maxEntries ) ); 
   else
       return( -1 );
}
/*--------------------------------------------------------------
** Function : connSetApplicationContext 
**-------------------------------------------------------------*/
void connSetApplicationContext( XtAppContext context )
{
   static void ( *func )( XtAppContext ) = NULL;

   if ( func == NULL )
   {
       if ( ServerHandle != 0 )
          func = ( void (*)( XtAppContext ) )
			dlsym(ServerHandle, "connSetApplicationContext");
       else if ( ( ServerHandle = dlopen( LIBSERVER, RTLD_LAZY ) )  != 0 )
          func = ( void (*)( XtAppContext ) )
			dlsym(ServerHandle, "connSetApplicationContext");
   }
   if ( func != NULL )
       (*func)( context ); 
}


/*--------------------------------------------------------------
** Function : NWOpenConnByName 
**-------------------------------------------------------------*/
NWCCODE N_API NWOpenConnByName( NWCONN_HANDLE startConnHandle, 
				pNWCConnString pName, pnstr pstrServiceType,
				nuint uConnFlags, nuint uTranType,
				NWCONN_HANDLE NWPTR pConnHandle )
{
   static NWCCODE N_API ( *func )( NWCONN_HANDLE, pNWCConnString,
				pnstr, nuint, nuint, NWCONN_HANDLE NWPTR) = NULL;

   if ( func == NULL )
   {
       if ( NWCalHandle != 0 )
          func = ( NWCCODE N_API (*)( NWCONN_HANDLE, pNWCConnString, 
					pnstr, nuint, nuint, NWCONN_HANDLE NWPTR ) ) 
					dlsym(NWCalHandle, "NWOpenConnByName");
       else if ( ( NWCalHandle = dlopen( LIBNWCAL, RTLD_LAZY ) )  != 0 )
          func = ( NWCCODE N_API (*)( NWCONN_HANDLE, pNWCConnString,
				pnstr, nuint, nuint, NWCONN_HANDLE NWPTR ) )
			dlsym(NWCalHandle, "NWOpenConnByName");
#ifdef DEBUG
		printf("IN first if statement\n");
#endif
   }
   if ( func != NULL )
	{
#ifdef DEBUG
		printf("Before calling NWOpenConnByName in funcs.c\n" );
#endif
       return ( (*func)( startConnHandle, pName, pstrServiceType,
					uConnFlags, uTranType, pConnHandle ) ); 
	}
   else
       return( -1 );
}


/*--------------------------------------------------------------
** Function : NWGetConnInformation 
**-------------------------------------------------------------*/
NWCCODE N_API NWGetConnInformation( NWCONN_HANDLE connHandle, 
				nuint uInfoLevel, nuint uinfoLen, nptr pConnInfo )
{
   static NWCCODE N_API ( *func )( NWCONN_HANDLE, nuint,
									nuint, nptr ) = NULL;
   if ( func == NULL )
   {
       if ( NWCalHandle != 0 )
          func = ( NWCCODE N_API (*)( NWCONN_HANDLE, nuint,
									nuint, nptr ) ) 
					dlsym(NWCalHandle, "NWGetConnInformation");
       else if ( ( NWCalHandle = dlopen( LIBNWCAL, RTLD_LAZY ) )  != 0 )
          func = ( NWCCODE N_API (*)( NWCONN_HANDLE, nuint,
									nuint, nptr ) )
			dlsym(NWCalHandle, "NWGetConnInformation");
   }
   if ( func != NULL )
       return ( (*func)( connHandle, uInfoLevel, uinfoLen, pConnInfo ) ); 
   else
       return( -1 );
}

/*--------------------------------------------------------------
** Function : NWCloseConn 
**-------------------------------------------------------------*/
NWCCODE N_API NWCloseConn( NWCONN_HANDLE connHandle)
{
	static NWCCODE N_API			(*func)(NWCONN_HANDLE) = NULL;

	if (func == NULL) {
		if (NWCalHandle != 0) {
		  func = (NWCCODE N_API(*)(NWCONN_HANDLE)) 
								   dlsym(NWCalHandle, "NWCloseConn");
		}
		else {
			if ((NWCalHandle = dlopen (LIBNWCAL, RTLD_LAZY)) != 0) {
		  		func = (NWCCODE N_API(*)(NWCONN_HANDLE))
										 dlsym(NWCalHandle, "NWCloseConn");
			}
		}
	}
	if (func != NULL)
		return ((*func)(connHandle)); 
	return (-1);
}


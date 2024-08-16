/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/packet.h	1.1"

#define NULL_PTR (char *)0
#define STACK_SIZE 12286
#define DATA_SIZE 522
#define PACKET_DATA 4090
#define MAX_BLOCKS_PER_USER 8
#define ENGINE_DATA 4096

/* Note the following structure is also defined in spxengin.h. Any changes   */
/* Must also be made in there.                                               */
typedef struct {
   short User;                                    /* User number 0 if LOGIN  */
   short Task;
   short Size;                                    /* Size of packet          */
   short PacketNumber;                            /* Number of packet        */
   short TotalPackets;                            /* Total Packets sent      */
   short ErrorCode ;                              /* Error Code to Stop      */
   char  Data[DATA_SIZE];                         /* Data sending            */
} PACKET_HEADER;

/* define the size of the PACKET without PACKET_DATA */
#define PACKET_OTHER 6
typedef struct {
   short Block;                                   /* Block number sent       */
   short Function;                                /* Function number of data */
   short Size;                                    /* Size of packet          */
   char  Data[PACKET_DATA];                       /* Size of data packets    */
} PACKET;


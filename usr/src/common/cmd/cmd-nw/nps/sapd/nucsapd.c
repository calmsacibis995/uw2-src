/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/sapd/nucsapd.c	1.8"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: nucsapd.c,v 1.7 1994/08/18 16:07:33 vtag Exp $"

/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <sys/sap_app.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <nwconfig.h>
#include <memmgr_types.h>
#include <unistd.h>
#include <ctype.h>


#ifdef OS_AIX
typedef struct dirent dirent_t;
#endif

#define SAPD_PID_FILE "/var/spool/sap/nucsapd.pid"
#define SAPD_IN_DIR "/var/spool/sap/in"
#define SAPD_OUT_DIR "/var/spool/sap/out"

static int		bcast_interval;

typedef struct adOutList {
	char	serverName[48];
	uint16	serverType;
	uint16	socket;
	uint8	checkedOff;
	struct adOutList *next;	
} adOutList_t;

static adOutList_t		*newListHead = NULL;
static adOutList_t		*newListTail = NULL;
static adOutList_t		*oldListHead = NULL;


static void
CompareAndAdvertiseLists( void )
{
	adOutList_t		*tmp, *currPtr;

	if(oldListHead)
	{

	/*
	**	Compare the new list to old.  If the server exists in both lists
	**	or if the server is new, check it off.
	*/
		tmp = newListHead;
		do
		{
			currPtr = oldListHead;
			do
			{
				if(!strcmp(tmp->serverName, currPtr->serverName))
				{
					if(tmp->serverType == currPtr->serverType)
					{
						/* match in both lists */
						currPtr->checkedOff = 1;
						break;
					}
				}
				currPtr = currPtr->next;
			} while(currPtr != NULL);

			tmp = tmp->next;
		} while(tmp != NULL);

	/*
	**	Go through the old list, if not "checkedOff", unadvertise it.
	**	Free the oldList entries regardless.
	*/
		currPtr = tmp = oldListHead;
		do
		{
			if(!tmp->checkedOff)
			{
				(void) SAPAdvertiseMyServer(tmp->serverType,
								(uint8 *)tmp->serverName, tmp->socket, 
								SAP_STOP_ADVERTISING );
			}
			tmp = currPtr->next;
			(void)NWFREE( currPtr );
			currPtr = tmp;
		} while(currPtr != NULL);
	}

	/*
	**	Set the oldListHead == newListHead, advertise everything in it.
	*/
	oldListHead = newListHead;
	newListHead = NULL;

	tmp = oldListHead;
	do
	{
		(void) SAPAdvertiseMyServer(tmp->serverType,
						(uint8 *)tmp->serverName, tmp->socket, 
						SAP_ADVERTISE );

		tmp = tmp->next;
	} while(tmp != NULL);
}

static int 
AdvertOutServers( void )
{
	DIR			*dfd;
	dirent_t	*dp;
	char		name[26];
	char		readBuf[2];
	char		fileBuf[60];
	char		*ptr;
	char		socketBuf[6];
	int			fd, i;
	uint16		socket, type;
	struct utsname		utsname;
	adOutList_t	*element;


	/*
	**	Open the /var/spool/sap/out directory
	*/
	if((dfd = opendir(SAPD_OUT_DIR)) == NULL)
		return(-4);

	/*
	**	Read the uname of the machine we are running on, we are going to
	**	need it.  Upper case the name before passing it on.
	*/
	if((i = uname(&utsname)) == -1)
		return(-5);

	for(i=0; i<(int)strlen(utsname.nodename); i++)
		utsname.nodename[i] = (char)toupper(utsname.nodename[i]);

	/*
	**	Read entries from directory, read the contents of each file
	**	Skip over the "." and ".." files naturally.
	*/
	while((dp = readdir(dfd)) != NULL)
	{
		if(strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
			continue;

		sprintf(name, "%s/%s", SAPD_OUT_DIR, dp->d_name);
		if((fd = open(name, O_RDONLY)) == -1)
			continue;
		/*
		**	Convert the file name to a hex value
		*/
		type = (uint16)strtol( dp->d_name, NULL, 0);

		memset(fileBuf, 0, sizeof(fileBuf));
		i = 0;
		while(read(fd, readBuf, 1))
		{
			if(readBuf[0] == '\n')
			{
				fileBuf[i] = 0;
				if(i == 0)
					continue;
				else
				{
		/*
		**	If a TAB character is in the file entry, we have a socket and
		**	name to parse out.  Otherwise, we have a NULL socket and we will
		**	use the generated `uname`.
		*/
					if((ptr = strchr(fileBuf, 9)) != NULL)
					{
						strncpy(socketBuf, fileBuf, (ptr-fileBuf));
						socketBuf[ptr-fileBuf] = 0;
						strcpy(name, &fileBuf[(int)(ptr-fileBuf)+1]);
					} else {
						strcpy(socketBuf, fileBuf);
						strcpy(name, utsname.nodename);
					}
					socket = (uint16)strtol( socketBuf, NULL, 0);
				}

				memset(fileBuf, 0, sizeof(fileBuf));

		/*
		**	add the entry to the new list.
		*/
				element = (adOutList_t *)NWALLOC( sizeof(adOutList_t) );
				if(element == NULL)
					return( -6 );
					
				element->next = NULL;
				element->checkedOff = 0;
				element->serverType = type;
				element->socket = socket;
				strcpy(element->serverName, name);

				if(!newListHead)
				{
					newListHead = element;
				}
				else
				{
					newListTail->next = element;
				}
				newListTail = element;
				i = 0;

			}
			else
				fileBuf[i++] = readBuf[0];
		}
	}
	/*
	**	Do list compare and advertising.
	*/
	CompareAndAdvertiseLists();

	closedir(dfd);
	return( 0 );
}

static void
PrintServerEntry( FILE *fp, SAPIP Server)
{
	int i;
	ipxAddr_t *ipxAddr;

	fprintf(fp, "%s\t", Server->serverName);

	ipxAddr = (ipxAddr_t *)&Server->serverAddress;
	for (i=0; i<4; i++)
		fprintf(fp,"%02x",ipxAddr->net[i]);
	fprintf(fp, ".");
	for (i=0; i<6; i++)
		fprintf(fp,"%02X",ipxAddr->node[i]);
	fprintf(fp, ".");
	for (i=0; i<2; i++)
		fprintf(fp,"%02X",ipxAddr->sock[i]);

	fprintf(fp, "\t%d", Server->serverHops);
	fprintf(fp, "\t%d\n", Server->serverType);
}

int
GetAllServers( uint16 type)
{
	SAPI	Server;
	SAPI	NServer;
	int		ServerEntry = 0;
	uint32	numServers = 0;
	int		status;
	char	path[120];
	FILE	*fp;


	sprintf(path, "%s/0x%lx", SAPD_IN_DIR, (long)type);

	if( SAPMapMemory() != 0) {
		fprintf(stderr, "nucsapd: Cannot attach to sap daemon shared memory\n");
		return(-1);
	}

	if (( fp = fopen ( path, "w" )) == NULL ) {
		fprintf(stderr, "nucsapd: open %s failed\n", path);
		return (-11);
	}

	/*
	**	nearest server is always first
	*/
	if( SAPGetNearestServer( type, &NServer) == 1) {
		PrintServerEntry( fp, &NServer);
	}

	while( (status = SAPGetAllServers( type, &ServerEntry, &Server, 1)) > 0) {
		
		numServers++;
		/*
		**	Don't duplicate nearest server
		*/
		if( strcmp( (char *)Server.serverName, (char *)NServer.serverName) != 0)
		{
			PrintServerEntry( fp, &Server);
		}
	}
	if( status < 0) {
		fprintf(stderr, "nucsapd: SAPGetAllServers failed");
		SAPPerror( status, "");
		return(-1);
	}
	fclose( fp);
	SAPUnmapMemory();
	return(numServers);
}

void
usage( void)
{
	fprintf( stderr, "usage: nucsapd server_type_# ...\n");
	return;
}

void
Daemonize()
{
	pid_t pid;

	sigignore(SIGCLD);

	if ((pid = fork()) == -1)
	{
		fprintf(stderr, "nucsapd: fork failed\n");
		exit(-1);
	}
	else if (pid != 0) {
		exit(0);
	}

	(void) setsid();

	if ((pid = fork()) == -1)
	{
		fprintf(stderr, "fork failed\n");
		exit(-1);
	}
	else if (pid != 0) {
		exit(0);
	}

	(void) chdir("/");
	(void) umask(0);

	sigignore(SIGCLD);
	return;
}

/*
**	Dummy signal function
*/
/*ARGSUSED*/
void
alarmsig( int sig) {
	alarm(bcast_interval * 30);
	return;
}

/*ARGSUSED*/
void
hangupsig( int sig )
{
	int retval;

	if((retval = AdvertOutServers()) < 0)
	{
		fprintf(stderr, "nucsapd: AdvertOutServers failed\n");
		exit( retval );
	}
}

/*ARGSUSED*/
void
term( int sig) {
	unlink(SAPD_PID_FILE);
	exit(0);
}

int
main( int argc, char *argv[])
{ 

	long	type, i;
	pid_t	pid = 0;
	FILE	*fp;
	int		newnum, oldnum, err;

	if( argc < 2) {
		usage();
		return(-2);
	}

	/*
	**	Get the broadcast interval
	*/
	if((err = NWCMGetParam( "lan_1_sap_bcast_intervals", NWCP_INTEGER,
						&bcast_interval )) != SUCCESS)
	{
		fprintf(stderr, 
			"Could not obtain NWCM lan_1_sap_bcast_intervals parameter\n");
		return( -1 );
	}

	/*
	**	Wait until sapd has finished initializing
	*/

	oldnum = 0;
	type = strtol( argv[1], NULL, 0);
	if( type == 0) {
		usage();
		return(-2);
	}

	while( (newnum = GetAllServers( type)) >= 0) {
		if( (newnum > 0) || (newnum == oldnum))
			break;
		oldnum = newnum;
	}
	if( newnum < 0)
		return(newnum);

	if((err = AdvertOutServers()) < 0)
	{
		fprintf(stderr, 
			"Could not advertise /var/spool/sap/out services.\n");
		return(err);
	}

	Daemonize();

	sigset( SIGALRM, alarmsig);
	sigset( SIGHUP, hangupsig);
	sigset( SIGTERM, term);
	alarm(bcast_interval * 30);

	/*
	**	Update the /in files that were specified
	*/
	for( ;; ) {
		for( i = 1; i < argc; i++) {
			type = strtol( argv[i], NULL, 0);
			if( type == 0) {
				usage();
				return(-2);
			}
			if( GetAllServers( (uint16)type) < 0) {
				return( -1);
			}
		}
		if( pid == 0) {
			if (( fp = fopen ( SAPD_PID_FILE, "w" )) == NULL ) {
				fprintf(stderr, "Could not create %s file\n", SAPD_PID_FILE);
				return(-3);
			}
			pid = getpid();
			fprintf(fp, "%ld\n", (long)pid);
			fclose( fp);
		}
		pause();
	}
}

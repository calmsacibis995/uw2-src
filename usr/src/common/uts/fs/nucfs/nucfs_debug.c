/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nucfs_debug.c	1.12"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nucfs_debug.c,v 2.5.2.13 1995/01/30 16:22:24 stevbam Exp $"

/*
 * Functions to assist in debugging nucfs.
 */

#include <sys/types.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/slstruct.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/spilcommon.h>
#include <net/tiuser.h>
#include <fs/vnode.h>
#include <fs/nucfs/nwfschandle.h>
#include <fs/nucfs/nucfscommon.h>
#include <fs/nucfs/nwficommon.h>
#include <fs/nucfs/nwfsvolume.h>
#include <fs/nucfs/nwfsnode.h>
#include <fs/nucfs/nwfsname.h>
#include <fs/nucfs/nwfidata.h>
#include <fs/nucfs/nwfslock.h>
#include <fs/nucfs/flock_cache.h>
#include <util/cmn_err.h>
#include <util/nuc_tools/trace/nwctrace.h>

#ifdef DEBUG_TRACE

/*
 * Define the tracing mask;
 */
#define NVLT_ModMask    NVLTM_fs

#endif /* DEBUG_TRACE */

#if defined(DEBUG) || defined(DEBUG_TOOLS) || defined(DEBUG_TRACE)

/* forward declarations */
void  print_cacheinfo(NWFS_CACHE_INFO_T *);
void  print_chandle(NWFS_CLIENT_HANDLE_T *);
void  print_chname(NWFS_CHILD_NAME_T *);
void  print_list(NWFI_LIST_T *);
void  print_netbuf(struct netbuf *);
void  print_nwfscred(NWFS_CRED_T *);
void  print_nwlock(NWFI_LOCK_T *, char *);
void  print_nwmnta(NWFI_MOUNT_ARGS_T *mP);
void  print_nwname(NWFS_NAME_T *);
void  print_nwsnode(NWFS_SERVER_NODE_T *);
void  print_timestamp(NWFI_TIME_STAMP_T *, char *);
void  print_volume(NWFS_SERVER_VOLUME_T *);
void  get_perms(uint32 , char *);

void
print_nwfscred(NWFS_CRED_T *ncP)
{
	debug_printf("    uid=%d gid=%d\n", ncP->userId, ncP->groupId);
}

void
print_nwsnode(NWFS_SERVER_NODE_T *nsP)
{
	debug_printf("    %14s at 0x%8x      %14s at 0x%8x\n",
		"identity Chain", &(nsP->header.chain[0]),
		"volume chain", &(nsP->header.chain[1]));
	debug_printf("    %14s =  0x%8x      %14s =  0x%8x\n",
		"nodeState", nsP->nodeState,
		"nodeVolume", nsP->nodeVolume);
	debug_printf("    %14s =  0x%8x      %14s =  %8d\n",
		"nodeType", nsP->nodeType,
		"nodeNumber", nsP->nodeNumber);
	print_nwlock(&nsP->snodeLock, "snodeLock: ");
	debug_printf("    %14s at 0x%8x      %14s =  0x%8x\n",
		"snodeSync", &nsP->snodeSync,
		"nodeFlags", nsP->nodeFlags);
	debug_printf("    %14s =    %8d      %14s =  %8d\n",
		"hardHoldCount", nsP->hardHoldCount,
		"softHoldCount", nsP->softHoldCount);

	debug_printf("    %14s =    %8d      %14s =  %8d\n",
		"nameCacheSoftHolds", nsP->nameCacheSoftHolds,
		"asyncError", nsP->asyncError);
	debug_printf("    %14s =    %8d      %14s =  %8d\n",
		"openResourceCount", nsP->openResourceCount,
		"r_mapcnt", nsP->r_mapcnt);
	debug_printf("    %14s =  0x%8x      %14s =  0x%8x\n",
		"clientHandleStamp", nsP->clientHandleStamp,
		"gfsNode", nsP->gfsNode);
	debug_printf("    %14s at 0x%8x      %14s =  %8d\n",
		"clientHandle", &(nsP->clientHandle),
		"nodeNumLinks", nsP->nodeNumberOfLinks);
	debug_printf("    %14s =    %8d      %14s =  %8d\n",
		"accessTime", "nsP->accessTime",
		"modifyTime", nsP->modifyTime);
	debug_printf("    %14s =    %8d      %14s =  %8d\n",
		"changeTime", nsP->changeTime,
		"nodeSize", nsP->nodeSize);
	debug_printf("    %14s =    %8d      %14s at 0x%8x\n",
		"nFlocksCached", nsP->nFlocksCached,
		"snodeRwLock", &(nsP->snodeRwLock));
	debug_printf("    %14s =  0x%8x      %14s =  0x%8x\n",
		"seqFaultAddr", nsP->seqFaultAddr,
		"vmSize", nsP->vmSize);
	debug_printf("    %14s =    %8d      %14s =  %8d\n",
		"postExtSize", nsP->postExtSize,
		"closeLockCount", nsP->closeLockCount);
	print_cacheinfo(&nsP->cacheInfo);
	debug_printf("    %14s =    %8d      %14s = 0x%x\n", 
		"snodeTimeStamp",
		nsP->snodeTimeStamp,
		"delayDeleteCHandle",
		nsP->delayDeleteClientHandle);
}



void
print_volflushdata(NWFI_VOLFLUSH_DATA_T *vfdP)
{
	debug_printf("    %14s =    0x%8x    %14s =  0x%8x\n",
			"volume address",
			vfdP->serverVolume,
			"flags:volFlushData",
			vfdP->flags);
	debug_printf("    %14s at 0x%8x      %14s at 0x%8x\n",
			"attFlushEvent",
			&(vfdP->attFlushEvent),
			"pagePushEvent",
			&(vfdP->pagePushEvent));
	debug_printf("    %14s =    %8d      %14s =    %8d    %14s =    %8d\n",
			"attFlushLwpId",
			vfdP->attFlushLwpId,
			"pagePushLwpId",
			vfdP->pagePushLwpId,
			"lastStaleCheckTime",
			vfdP->lastStaleCheckTime);
}




void
print_volume(NWFS_SERVER_VOLUME_T *svP)
{
	char buf[80];

	debug_printf("    %14s at 0x%8x      %14s at 0x%8x    %14s at 0x%8x\n",
			"serverVolumeList",
			&svP->serverVolumeList,
			"timedNodeList",
			&svP->timedNodeList,
			"activeNodeList",
			&svP->activeNodeList);
	debug_printf("    %14s at 0x%8x      %14s =    %8d    %14s =    %8d\n",
			"cacheNodeList",
			&svP->cacheNodeList,
			"activeOrTimedCount",
			svP->activeOrTimedCount,
			"cacheCount",
			svP->cacheCount);
	debug_printf("    %14s =    %8d      %14s at 0x%8x    %14s at 0x%8x\n",
			"staleCount",
			svP->staleCount,
			"asyncIoList",
			&svP->asyncIoList,
			"idHashTable",
			&(svP->idHashTable[0]));
	debug_printf("    %14s =    %8d      %14s =    %8d    %14s =  0x%8x\n",
			"idHashStamp",
			svP->idHashStamp,
			"logicalBlockSize",
			svP->logicalBlockSize,
			"serverAddress",
			svP->address);
	debug_printf("    %14s is   %32s\n", "volumeName", svP->volumeName);
	debug_printf("    %14s =  0x%8x      %14s =  0x%8x    %14s =  0x%8x\n",
			"volumeFlags",
			svP->volumeFlags,
			"mountPoint",
			svP->mountPoint,
			"rootNode",
			svP->rootNode);
	debug_printf("    %14s =  0x%8x      %14s =  0x%8x    %14s =  0x%8x\n",
			"spilVolumeHandle", 
			svP->spilVolumeHandle,
			"volFlushData",
			svP->volFlushData,
			"activeStamp",
			svP->activeStamp);
	buf[0] = '\0';	
	if (svP->nucNlmMode & NUC_NLM_NETWARE_MODE) 
		strcat(buf, "NETWARE MODE ");
	if (svP->nucNlmMode & NUC_NLM_UNIX_MODE) 
		strcat(buf, "UNIX MODE ");
	debug_printf("    NLM Mount Flags: %s\n", buf);
}

void
print_chname(NWFS_CHILD_NAME_T *cnP)
{
	debug_printf("    child name at 0x%x: childNameslist at 0x%x \n", 
		cnP, &(cnP->childNames));
	debug_printf("    child snode: 0x%x, cacheTime=%d hashed name struct: 0x%x\n",
		cnP->childNode, 
		cnP->cacheTime, 
		cnP->hashedName);
}

void
print_nwmnta(NWFI_MOUNT_ARGS_T *mP)
{
	char buf[80];

	debug_printf("\n&NWFI_MOUNT_ARGS_T  0x%x:\n", mP);
	debug_printf("    struct netbuf address, at 0x%x\n", &(mP->address));
	buf[0] = '\0';	
	strncat(buf, mP->volumeName, MAX_VOLUME_NAME_LENGTH);
	debug_printf("    volumeName =              %s\n", buf);
	buf[0] = '\0';	
	if (mP->mountFlags == 0) {
		 strcat(buf, "NO FLAGS ");	
		 goto strPrint;
	}
	if (mP->mountFlags & NWFI_VOLUME_READ_ONLY ) 
					strcat(buf, "RO ");
	if (mP->mountFlags & NWFI_USE_UID ) 
					strcat(buf, "useUID ");
	if (mP->mountFlags & NWFI_USE_GID ) 
					strcat(buf, "useGID ");
	if (mP->mountFlags & NWFI_INHERIT_PARENT_GID ) 
					strcat(buf, "getParentGID ");
	if (mP->mountFlags & NWFI_DISALLOW_SETUID ) 
					strcat(buf, "noSUID ");
	if (mP->mountFlags & NWFI_NO_NAME_TRUNCATION ) 
					strcat(buf, "NOTRUNC ");
strPrint:
	debug_printf("    mountFlags:               %s\n", buf);
	debug_printf("    cred structure at:        0x%x\n", 
		&(mP->credStruct));
}

void
print_netbuf(struct netbuf *nbP)
{
	debug_printf("\n&netbuf at 0x%x:\n", nbP);
	debug_printf("    maxlen=  %8d   len=  %8d\n", 
					nbP->maxlen, nbP->len);
	debug_printf("    netbuf->buf at            0x%x\n", &(nbP->buf));
}

void
print_nwlock(NWFI_LOCK_T *locP, char *name)
{
	/*
	 * assumed: k_pl_t is not a compound datatype.
	 */
	if (name != NULL) {
		debug_printf("    %16s at 0x%8x savedPl = %8d\n", 
				name, locP, locP->savedPl);
	} else {
		debug_printf("    Lockp = 0x%8x savedPl = %8d\n", 
				locP, locP->savedPl);
	}
}

void
print_timestamp(NWFI_TIME_STAMP_T *tsP, char *Str)
{
	/*
	 * assumed: clock_t is not a compound datatype.
	 */
	debug_printf("    %s    secnds=  %8d  seqNo= %8u\n", 
				Str, (tsP)->ticks, (tsP)->seqNo);
}

void
print_list(NWFI_LIST_T *L)
{
	int i = 0;
	NWFI_LIST_T *c = L;

	debug_printf("    addr         flink           rlink      \n");
	do {
		debug_printf("    0x%8x   0x%8x      0x%8x   [%5d] \n",
			c, c->flink, c->rlink, i);
		/*
		 * kludge: don't follow a bad or uninitialized list.
		 */
		if ((c->flink->rlink != c) || (c->rlink->flink != c)){
			break;
		}
		c = c->flink;
		i++;
	} while (c != L);	
}

void
print_list_max(NWFI_LIST_T *L, uint32 n, void (*funcP)(), int byteOffset)
{
	int i;
	NWFI_LIST_T	*curr;
	NWFI_LIST_T	*prev;
	
	if (n == 0) { n = 512; } /* arbitrary */
	curr = L->flink;
	if (curr == L) {
		debug_printf("    Empty List at             0x%x\n", L);
		return;
	}
	prev = L;
	for (i = 0; i < n; i++) {
		if (curr == L) return;
		if (curr->rlink != prev) {
		debug_printf("    **** mangled links at     0x%x\n", curr);
		return;
		}
		prev = curr;
		(*funcP)(((char *)curr - byteOffset));
		debug_printf("\n");
		curr = curr->flink;
	}
	debug_printf("\nprint_list_max: %6d items printed.\n", i);
	return;
}

void
print_nwname(NWFS_NAME_T *nP)
{
	char buf[80];
	int i;

	debug_printf("NWFS_NAME: hashChain(list) at 0x%x: \n", 
		&(nP->hashChain));

	/* print_list(&(np->hashChain)); */

	if (nP->nameLen > 79) {
		buf[0] = '\0';
		strcat(buf, "name too long to print. ");
	} else {
		for(i=0; i < (int) nP->nameLen; i++) {
			buf[i] = nP->nameData[i];
		}
		buf[nP->nameLen] = '\0';
	}
	debug_printf("    holdCount= %d   nameLen=%d name= %s\n",
		nP->holdCount, nP->nameLen, buf);
}

void
print_chandle(NWFS_CLIENT_HANDLE_T *chP)
{
	char buf[80];

	buf[0] = '\0';

	switch (chP->nodeState) {
		case SNODE_TIMED: strcat(buf, "SNODE_TIMED "); break;
		case SNODE_ACTIVE: strcat(buf, "SNODE_ACTIVE "); break;
		case SNODE_INACTIVE: strcat(buf, "SNODE_INACTIVE "); break;
		case SNODE_STALE: strcat(buf, "SNODE_STALE "); break;
		case SNODE_MARKER: strcat(buf, "SNODE_MARKER "); break;
		case SNODE_CHANDLE: strcat(buf, "SNODE_CHANDLE "); break;
		case SNODE_EHANDLE: strcat(buf, "SNODE_EHANDLE "); break;
		case SNODE_DEAD: strcat(buf, "SNODE_DEAD "); break;
		default: strcat(buf, "INVALID STATE ");
	}

	debug_printf("    state=%14s  &cliHndleList=0x%8x  &namesList=0x%8x\n",
			buf, 
			&(chP->clientHandleList),
			&(chP->namesList));


	print_nwlock(&(chP->nameLock), "nameLock");
	debug_printf("    nameStamp= %8d  holdCount= %8d  resHoldCount= %8d\n",
			chP->nameStamp, 
			chP->holdCount, 
			chP->resourceHoldCount);
			
	/*
	 * identify and print client handle state(s).
	 */
	buf[0] = '\0';
	strcat(buf, "cHandleFlags:");
	if (chP->clientHandleFlags == 0)
		strcat(buf, "NONE ");
	if (chP->clientHandleFlags & NWCH_AT_VALID)
		strcat(buf, "AT_VALID ");
	if (chP->clientHandleFlags & NWCH_RH_CREATING)
		strcat(buf, "RH_CRTING ");
	if (chP->clientHandleFlags & NWCH_RH_DRAINING)
		strcat(buf, "RH_DRAIN ");
	if (chP->clientHandleFlags & NWCH_RH_CMOC)
		strcat(buf, "RH_CMOC ");
	if (chP->clientHandleFlags & NWCH_WRITE_FAULT)
		strcat(buf, "WrFAULT ");
	if (chP->clientHandleFlags & NWCH_DATA_DIRTY)
		strcat(buf, "DIRTY ");
	if (chP->clientHandleFlags & NWCH_RH_WRITE)
		strcat(buf, "RH_WRITE ");
	if (chP->clientHandleFlags & NWCH_RH_NEWLY_CREATED)
		strcat(buf, "RH_NEW_CREAT ");
	if (chP->clientHandleFlags & NWCH_DESTROY)
		strcat(buf, "DESTROY ");
	debug_printf("    %s \n", buf);
	debug_printf("    cloneVnodeP= 0x%x, serverNodeP= 0x%x \n", 
		chP->cloneVnode, 
		chP->snode);
	debug_printf("    &nwfsCred= 0x%x  resourceHandle= 0x%x \n",
		&(chP->credentials), 
		chP->resourceHandle);

	get_perms(chP->nodePermissions, &buf[0]);
	debug_printf("    perms= %s   userId=%8d  groupId=%8d \n",
			buf,  
			chP->userId, 
			chP->groupId);
	print_cacheinfo(&chP->cacheInfo);
	debug_printf("    %14s at 0x%8x   flockCacheLen=%8d\n",
		     "flockCacheChain", &chP->flockCacheChain,
		     chP->flockCacheLen);
#if defined(DEBUG) || defined(DEBUG_TRACE)
	debug_printf("    inflated=%8d\n", chP->inflated);
#endif
}

void
print_cacheinfo(NWFS_CACHE_INFO_T *ciP)
{
	print_timestamp(&(ciP->beforeTime), "beforeTime: ");
	print_timestamp(&(ciP->afterTime), "afterTime: ");
	if (ciP->stale) {
		debug_printf("    cache info is stale\n");
	} else {
		debug_printf("    cache info is not stale\n");
	}
} 

void
get_perms(uint32 p, char *pvec)
{
	pvec[0] = '\0';

#define	permMaskSet(P, B, Mask, Str, altStr) \
	{ if ((P) & (B)) { strcat(Mask, Str); } else {strcat(Mask, altStr); }}

	permMaskSet( p, NS_HIDDEN_FILE_BIT, pvec, "HF ", "   ");
	permMaskSet( p, NS_CD_ALLOWED_BIT, pvec, "CD ", "   ");
	permMaskSet( p, NS_FILE_EXECUTABLE_BIT, pvec, "FE ", "   ");
	permMaskSet( p, NS_OWNER_READ_BIT, pvec, "r", "-");
	permMaskSet( p, NS_OWNER_WRITE_BIT, pvec, "w", "-");
	if (p & NS_OWNER_EXECUTE_BIT) {
		permMaskSet( p, NS_SET_UID_BIT, pvec, "s", "x");
	} else {
		strcat(pvec, "-");
	}
	permMaskSet( p, NS_GROUP_READ_BIT, pvec, "r", "-");
	permMaskSet( p, NS_GROUP_WRITE_BIT, pvec, "w", "-");
	if (p & NS_GROUP_EXECUTE_BIT) {
		permMaskSet( p, NS_SET_GID_BIT, pvec, "s", "x");
	} else {
		permMaskSet( p, NS_SET_GID_BIT, pvec, "l", "-");
	}
	permMaskSet( p, NS_OTHER_READ_BIT, pvec, "r", "-");
	permMaskSet( p, NS_OTHER_WRITE_BIT, pvec, "w", "-");
	if (p & NS_STICKY_BIT) {
		permMaskSet( p, NS_OWNER_EXECUTE_BIT, pvec, "t", "T");
	} else {
		permMaskSet( p, NS_OWNER_EXECUTE_BIT, pvec, "x", "-");
	}
}

void
print_nucfs_lock(NUCFS_LOCK_T *lock)
{
	char	*command;
	char	*type;

	if (!lock || ((int)lock & (sizeof(long) - 1))) {
		debug_printf("bad lock pointer 0x%x\n", (int)lock);
		return;
	}
	switch (lock->lockCommand) {
	case NWFS_SET_LOCK:
		command = "NWFS_SET_LOCK";
		break;
	case NWFS_SET_WAIT_LOCK:
		command = "NWFS_SET_WAIT_LOCK";
		break;
	case NWFS_REMOVE_LOCK:
		command = "NWFS_REMOVE_LOCK";
		break;
	default:
		command = "UNKNOWN";
		break;
	}
	switch (lock->lockType) {
	case NWFS_SHARED_LOCK:
		type = "NWFS_SHARED_LOCK";
		break;
	case NWFS_EXCLUSIVE_LOCK:
		type = "NWFS_EXCLUSIVE_LOCK";
		break;
	default:
		type = "UNKNOWN";
		break;
	}
	debug_printf("lock at 0x%x command %s type %s offset %d end %d\n"
		     "\tPid %d userId %d groupId %d\n",
		     (int)lock, command, type, lock->lockOffset,
		     lock->lockEnd, lock->lockPid, lock->lockCred.userId,
		     lock->lockCred.groupId);
}

void
print_flock_cache(NUCFS_FLOCK_CACHE_T *cache)
{
	if (!cache || ((int)cache & (sizeof(long) - 1))) {
		debug_printf("bad cache pointer 0x%x\n", (int)cache);
		return;
	}
	debug_printf("cache at 0x%x: cacheChain at 0x%x cachePidCount %d\n"
		     "\tcachePidChain at 0x%x cacheState at 0x%x\n",
		     (int)cache, (int)&cache->cacheChain, cache->cachePidCount,
		     (int)&cache->cachePidChain, (int)&cache->cacheState);
}

#endif /* DEBUG || DEBUG_TOOLS || DEBUG_TRACE */

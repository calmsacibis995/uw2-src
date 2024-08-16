#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/hardlink.C	1.2"

#include "tsaglobl.h"

int IsInHardLinksList(BACKED_UP_DEVS *disks, 
	dev_t devNo, 
	ino_t inodeNo, UINT32 modifyTime, char *path,
	BACKED_UP_HARDLINKS **linkNodePtr)
{
	BACKED_UP_HARDLINKS *links ;
	int rcode = 0 ;

	if ( disks == NULL ) {
		return(rcode);
	}
	while(disks) {
		if ( disks->devNo == devNo ) {
			break ;
		}
		disks = disks->next ;
	}
	if ( disks == NULL ) {
		return(rcode);
	}
	links = disks->links ;

	if ( links == NULL ) {
		return(rcode);
	}
	while(links) {
		if ( links->inodeNo == inodeNo ) {
#ifdef DEBUG
			logerror(PRIORITYWARN,
			"IsInHardLinksList: path %s lpath %s", path, links->path);
			logerror(PRIORITYWARN, "mtime %s ", ctime((time_t *)&modifyTime));
			logerror(PRIORITYWARN,"lmtime %s\n", 
				ctime((time_t *)&(links->modifyTime)));
			FLUSHLOG ;
#endif

			if ( strcmp(links->path, path) == 0 ) {
				/*This looks like the re-restore. So just set the 
				  modifyTime in the node and return appropriate value*/
				links->modifyTime = modifyTime ;
				rcode = 1 ;
				break ;
			}
			if ( links->modifyTime == modifyTime ) {
				/* This is a link */
				rcode = 2 ;
				break ;
			}
		}
		links = links->next ;
	}
	*linkNodePtr = links ;
	return(rcode);
}

int AddToHardLinksList(BACKED_UP_DEVS **disks, dev_t devNo, 
	ino_t inodeNo, UINT32 modifyTime, char *path)
{
	BACKED_UP_HARDLINKS *links, *newlink ;
	BACKED_UP_DEVS *newdisk, *tmpdisk ;
	int pathLength ;

	pathLength = strlen(path) + 1 ;

#ifdef DEBUG
	logerror(PRIORITYWARN, "AddToHardLinksList: path %s mtime %s\n", 
		path, ctime((time_t *)&modifyTime));
	FLUSHLOG ;
#endif
	if ( *disks == NULL ) {
		if ((tmpdisk = (BACKED_UP_DEVS *)calloc(1,
					sizeof(BACKED_UP_DEVS))) == NULL) {
			return(-1);
		}
		if ((links = (BACKED_UP_HARDLINKS *)calloc(1,
			sizeof(BACKED_UP_HARDLINKS)+pathLength)) == NULL) {
			free((char *)tmpdisk);
			return(-1);
		}
		tmpdisk->devNo = devNo ;
		links->inodeNo = inodeNo ;
		links->path = (char *)links + sizeof(BACKED_UP_HARDLINKS);
		strcpy(links->path,path);
		links->modifyTime = modifyTime ;
		tmpdisk->links = links ;
		tmpdisk->next = NULL ;
		links->next = NULL ;
		*disks = tmpdisk ;
		return(0);	
	}
	newdisk = *disks ;
	while (newdisk) {
		if (newdisk->devNo == devNo) {
			break ;
		}
		tmpdisk = newdisk ;
		newdisk = newdisk->next ;
	}
	if ( newdisk == NULL ) {
		if ((newdisk = (BACKED_UP_DEVS *)calloc(1,
					sizeof(BACKED_UP_DEVS))) == NULL) {
			return(-1);
		}
		if ((links = (BACKED_UP_HARDLINKS *)calloc(1,
			sizeof(BACKED_UP_HARDLINKS)+pathLength)) == NULL) {
			free((char *)newdisk);
			return(-1);
		}
		newdisk->devNo = devNo ;
		links->inodeNo = inodeNo ;
		links->path = (char *)links + sizeof(BACKED_UP_HARDLINKS);
		strcpy(links->path,path);
		links->modifyTime = modifyTime ;
		newdisk->links = links ;
		newdisk->next = NULL ;
		links->next = NULL ;
		tmpdisk->next = newdisk ;
		return(0);	
	}
	newlink = newdisk->links ;
	while (newlink) {
//		if (newlink->inodeNo == inodeNo) {
//			return(0);
//		}
		links = newlink ;
		newlink = newlink->next ;
	}
	if ((newlink = (BACKED_UP_HARDLINKS *)calloc(1,
			sizeof(BACKED_UP_HARDLINKS)+pathLength)) == NULL) {
		return(-1);
	}
	newlink->inodeNo = inodeNo ;
	newlink->path = (char *)newlink + sizeof(BACKED_UP_HARDLINKS);
	strcpy(newlink->path,path);
	newlink->modifyTime = modifyTime ;
	newlink->next = NULL ;
	links->next = newlink ;
	return(0);	
}

void ReleaseHardLinksList(BACKED_UP_DEVS **disks)
{
	BACKED_UP_HARDLINKS *links, *tmplink ;
	BACKED_UP_DEVS *devs, *tmpdisk ;

	if ( *disks == NULL ) {
		return ;
	}
	
	devs = *disks ;
	while ( devs != NULL ) {
		links = devs->links ;
		while ( links != NULL ) {
			tmplink = links->next ;
			free((char *)links);
			links = tmplink ;
		}
		tmpdisk = devs->next ;
		free((char *)devs);
		devs = tmpdisk ;
	}
	*disks = NULL ;
}

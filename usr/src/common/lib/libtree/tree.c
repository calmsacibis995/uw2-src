/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libtree:tree.c	1.1"
#include	<dirent.h>
#include	<unistd.h>
#include	<ctype.h>
#include	<dlfcn.h>
#include	<malloc.h>
#include	<stdio.h>
#include	<string.h>
#include	<time.h>
#include	<sys/file.h>
#include	<sys/types.h>
#include	<mail/link.h>

#define	TREE_ACCESS	1
#define	TREE_NOMEM	2
#define	TREE_OPEN	3
#define	TREE_NOSYM	4

#define	MINUITES(x)	(60 * (x))

#define	EXPIRE_DELTA MINUITES(5)

typedef struct node_s
    {
    void
	*nd_link,
	*nd_data,
	*nd_treeList,
	(*nd_select)(),
	(*nd_dataFree)();
    
    char
	*nd_name;
    
    int
	(*nd_canDelete)();
    }	node_t;

typedef struct treeListVops_s
    {
    void
	(*tlvop_add)(),
	(*tlvop_close)(),
	(*tlvop_free)();

    int
	(*tlvop_open)();
    }	treeListVops_t;

typedef struct treeList_s
    {
    void
	*tnl_link,
	*tnl_list,
	*tnl_callbackList,
	*tnl_data;
    
    int
	tnl_openState,
	tnl_openCount;
    
    time_t
	tnl_expire;

    treeListVops_t
	tnl_vops;

    struct treeList_s
	*tnl_parent;
    }	treeList_t;

typedef struct callback_s
    {
    void
	*clb_link,
	(*clb_func)(),
	*clb_data;
    }	callback_t;

typedef struct treeDir_s
    {
    char
	*td_path;
    
    void
	*td_soList;
    }	treeDir_t;

#define	TREE_OBJ
#include	<mail/tree.h>

static int
    DebugLevel = 0;

static void
    *NodeListList = NULL,
    treeListFree();

static int
    treeDirectoryOpen(void *parentList_p, treeDir_t *treeDir_p);

static int
    strcasecmp(char *str1, char *str2)
	{
	char
	    c1,
	    c2;
	
	for
	    (
	    c1 = toupper(*str1),
		c2 = toupper(*str2);
	    *str1 != '\0' && c1 == c2;
	    c1 = toupper(*++str1),
		c2 = toupper(*++str2)
	    );
	
	if(c1 == c2)
	    {
	    return(0);
	    }
	else
	    {
	    return((c1 > c2)? 1: -1);
	    }
	}

static void
    callbackFree(callback_t *callback_p)
	{
	if(callback_p != NULL)
	    {
	    if(callback_p->clb_link) linkFree(callback_p->clb_link);
	    free(callback_p);
	    }
	}

static callback_t
    *callbackNew(treeList_t *treeList_p, void (*func)(), void *data)
	{
	callback_t
	    *result;

	if
	    (
		(
		result = (callback_t *)calloc
		    (
		    sizeof(*result),
		    1
		    )
		) == NULL
	    )
	    {
	    /* ERROR No Memory */
	    }
	else if((result->clb_link = linkNew(result)) == NULL)
	    {
	    /* ERROR No Memory */
	    callbackFree(result);
	    result = NULL;
	    }
	else
	    {
	    result->clb_data = data;
	    result->clb_func = func;
	    (void) linkAppend(treeList_p->tnl_callbackList, result->clb_link);
	    }

	return(result);
	}

void
    treeListAdd(treeList_t *treeList_p)
	{
	if(treeList_p != NULL && treeList_p->tnl_vops.tlvop_add != NULL)
	    {
	    treeList_p->tnl_vops.tlvop_add(treeList_p);
	    }
	}

void
    treeListCallbackDo(treeList_t *treeList_p, int state)
	{
	callback_t
	    *curCallback_p;
	
	treeList_p->tnl_openState = (state)? 2: 0;
	while
	    (
		(
		curCallback_p = (callback_t *)linkOwner
		    (
		    linkNext(treeList_p->tnl_callbackList)
		    )
		) != NULL
	    )
	    {
	    curCallback_p->clb_func(treeList_p, curCallback_p->clb_data, state);
	    callbackFree(curCallback_p);
	    }
	}

static void
    nodeFree(node_t *node_p)
	{
	if(node_p != NULL)
	    {
	    if(node_p->nd_link != NULL) linkFree(node_p->nd_link);
	    if(node_p->nd_name != NULL) free(node_p->nd_name);
	    if(node_p->nd_treeList != NULL) treeListFree(node_p->nd_treeList);
	    if(node_p->nd_data != NULL && node_p->nd_dataFree != NULL)
		{
		node_p->nd_dataFree(node_p->nd_data);
		}

	    free(node_p);
	    }
	}

static int
    nodeCmpFunc(node_t *node1_p, node_t *node2_p)
	{
	int
	    result;
	
	if(nodeIsInternal(node1_p) == nodeIsInternal(node2_p))
	    {
	    result = strcmp(node1_p->nd_name, node2_p->nd_name);
	    }
	else
	    {
	    result = nodeIsInternal(node1_p)? -1: 1;
	    }

	return(result);
	}

static node_t
    *nodeNewInternal
	(
	char *name,
	void *data,
	void (*dataFreeFunc)(),
	void (*selectFunc)(),
	int (*canDeleteFunc)(),
	treeList_t *treeList_p,
	treeList_t *parentTreeList_p,
	int sorted
	)
	{
	node_t
	    *result;
	
	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"nodeNewInternal(%s, 0x%x, 0x%x, 0xEx, 0x%x, 0x%x, 0x%x, %d) Entered.\n",
		(name == NULL)? "NIL": name,
		(int) data,
		(int) dataFreeFunc,
		(int) selectFunc,
		(int) canDeleteFunc,
		(int) treeList_p,
		(int) parentTreeList_p,
		sorted
		);
	    }

	if((result = (node_t *)calloc(sizeof(*result), 1)) == NULL)
	    {
	    /* ERROR No Memory */
	    }
	else if((result->nd_link = linkNew(result)) == NULL)
	    {
	    /* ERROR No Memory */
	    nodeFree(result);
	    result = NULL;
	    }
	else if((result->nd_name = strdup(name)) == NULL)
	    {
	    /* ERROR No Memory */
	    nodeFree(result);
	    result = NULL;
	    }
	else
	    {
	    result->nd_data = data;
	    result->nd_dataFree = dataFreeFunc;
	    result->nd_treeList = treeList_p;
	    if(parentTreeList_p == NULL)
		{
		}
	    else if(sorted)
		{
		(void) linkAddSorted
		    (
		    parentTreeList_p->tnl_list,
		    result->nd_link,
		    nodeCmpFunc
		    );
		}
	    else
		{
		(void) linkAppend(parentTreeList_p->tnl_list, result->nd_link);
		}
	    }

	if(DebugLevel > 2) (void) fprintf(stderr, "nodeNewInternal() = 0x%x Exited.\n", (int) result);

	return(result);
	}

node_t
    *nodeNew(char *name, treeList_t *treeList_p, void *data, void (*dataFree)(), void (*selectFunc)(), int (*canDeleteFunc)())
	{
	return
	    (
	    nodeNewInternal
		(
		name,
		data,
		dataFree,
		selectFunc,
		canDeleteFunc,
		NULL,
		treeList_p,
		1
		)
	    );
	}

void
    nodeSelect(node_t *node_p)
	{
	if(node_p != NULL && node_p->nd_select != NULL) node_p->nd_select(node_p);
	}

int
    nodeIsDeleteable(node_t *node_p)
	{
	int
	    result;

	if(node_p == NULL)
	    {
	    result = -1;
	    }
	else if(node_p->nd_canDelete == NULL)
	    {
	    result = -1;
	    }
	else
	    {
	    result = node_p->nd_canDelete(node_p);
	    }
	
	return(result);
	}

int
    nodeDelete(node_t *node_p)
	{
	int
	    result;

	if(node_p == NULL)
	    {
	    result = -1;
	    }
	else if(node_p->nd_canDelete == NULL)
	    {
	    result = -1;
	    }
	else if(node_p->nd_canDelete(node_p))
	    {
	    result = 0;
	    nodeFree(node_p);
	    }
	else
	    {
	    result = -1;
	    }
	
	return(result);
	}

char
    *nodeName(node_t *node_p)
	{
	return(node_p->nd_name);
	}

int
    nodeIsInternal(node_t *node_p)
	{
	return((node_p->nd_treeList != NULL)? 1 :0);
	}

void
    *nodeTreeList(node_t *node_p)
	{
	return(node_p->nd_treeList);
	}

void
    *nodeData(node_t *node_p)
	{
	return(node_p->nd_data);
	}

node_t
    *nodeNext(node_t *node_p)
       {
       return((node_t *)linkOwner(linkNext(node_p->nd_link)));
       }

static void
    treeListFree(treeList_t *treeList_p)
	{
	node_t
	    *curNode_p;

	if(DebugLevel > 5)
	    {
	    (void) fprintf
		(
		stderr,
		"treeListFree(0x%x) Entered.\n",
		(int) treeList_p
		);
	    }

	if(treeList_p != NULL)
	    {
	    if(treeList_p->tnl_link != NULL) linkFree(treeList_p->tnl_link);
	    if(treeList_p->tnl_list)
		{
		while
		    (
		    curNode_p = (node_t *)linkOwner
			(
			linkNext(treeList_p->tnl_list)
			)
		    )
		    {
		    nodeFree(curNode_p);
		    }

		linkFree(treeList_p->tnl_list);
		}

	    if(treeList_p->tnl_data != NULL && treeList_p->tnl_vops.tlvop_free != NULL)
		{
		treeList_p->tnl_vops.tlvop_free(treeList_p->tnl_data);
		}

	    free(treeList_p);
	    }

	if(DebugLevel > 5) (void) fprintf(stderr, "treeListFree() Exited.\n");
	}

static treeList_t
    *treeListDataFuncNew
	(
	char *name,
	treeList_t *parent,
	void *data,
	void *(*dataFunc)(),
	void (*freeFunc)(),
	int (*openFunc)(),
	void (*closeFunc)(),
	void (*addFunc)()
	)
	{
	treeList_t
	    *result;
	
	if(DebugLevel > 5)
	    {
	    (void) fprintf
		(
		stderr,
		"treeListDataFuncNew(%s, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x) Entered.\n",
		name,
		(int) parent,
		(int) data,
		(int) dataFunc,
		(int) freeFunc,
		(int) openFunc,
		(int) closeFunc,
		(int) addFunc
		);
	    }

	if(NodeListList == NULL && (NodeListList = linkNew(NULL)) == NULL)
	    {
	    /* ERROR No Memory */
	    result = NULL;
	    }
	else if((result = (treeList_t *)calloc(sizeof(*result), 1)) == NULL)
	    {
	    /* ERROR No Memory */
	    }
	else if((result->tnl_link = linkNew(result)) == NULL)
	    {
	    /* ERROR No Memory */
	    treeListFree(result);
	    result = NULL;
	    }
	else if((result->tnl_list = linkNew(NULL)) == NULL)
	    {
	    /* ERROR No Memory */
	    treeListFree(result);
	    result = NULL;
	    }
	else if((result->tnl_callbackList = linkNew(NULL)) == NULL)
	    {
	    /* ERROR No Memory */
	    treeListFree(result);
	    result = NULL;
	    }
	else if
	    (
		(parent != NULL) &&
		    (
		    nodeNewInternal
			(
			name,
			NULL,
			NULL,
			NULL,
			NULL,
			result,
			parent,
			1
			) == NULL
		    )
	    )
	    {
	    treeListFree(result);
	    }
	else
	    {
	    result->tnl_openState = 0;
	    result->tnl_openCount = 0;
	    result->tnl_vops.tlvop_open = openFunc;
	    result->tnl_vops.tlvop_free = freeFunc;
	    result->tnl_vops.tlvop_close = closeFunc;
	    result->tnl_vops.tlvop_add = addFunc;
	    result->tnl_data = (dataFunc == NULL)? data: dataFunc(result);
	    result->tnl_parent = parent;

	    (void) linkAppend(NodeListList, result->tnl_link);
	    }

	if(DebugLevel > 5) (void) fprintf(stderr, "0x%x = treeListDataFuncNew() Exited.\n", (int) result);

	return(result);
	}
 
treeList_t
    *treeListNew
	(
	char *name,
	treeList_t *parent,
	void *data,
	void (*freeFunc)(),
	int (*openFunc)(),
	void (*closeFunc)(),
	void (*addFunc)()
	)
	{
	return
	    (
	    treeListDataFuncNew
		(
		name,
		parent,
		data,
		NULL,
		freeFunc,
		openFunc,
		closeFunc,
		addFunc
		)
	    );
	}
 
int
    treeListOpen(treeList_t *treeList_p, void (*callback)(), void *data)
	{
	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"treeListOpen(0x%x, 0x%x, 0x%x) Entered.\n",
		(int) treeList_p,
		(int) callback,
		(int) data
		);
	    }

	if(treeList_p == NULL)
	    {
	    }
	else
	    {
	    switch(treeList_p->tnl_openCount)
		{
		default:
		    {
		    treeList_p->tnl_openCount++;
		    break;
		    }

		case	0:
		    {
		    treeList_p->tnl_openCount = 2;
		    (void) treeListOpen(treeList_p->tnl_parent, NULL, NULL);
		    break;
		    }
		}
	    
	    if(DebugLevel > 5)
		{
		(void) fprintf
		    (
		    stderr,
		    "\ttnl_openState = %d.\n",
		    treeList_p->tnl_openState
		    );
		}

	    switch(treeList_p->tnl_openState)
		{
		case	0:
		    {
		    if(treeList_p->tnl_vops.tlvop_open == NULL)
			{
			if(callback != NULL) callback(treeList_p, data, 0);
			}
		    else if
			(
			callback != NULL
			    && callbackNew(treeList_p, callback, data) == NULL
			)
			{
			callback(treeList_p, data, 0);
			}
		    else
			{
			treeList_p->tnl_openState = 1;
			if(DebugLevel > 5)
			    {
			    (void) fprintf
				(
				stderr,
				"Openning node list 0x%x.\n",
				(int) treeList_p
				);
			    }

			treeList_p->tnl_vops.tlvop_open
			    (
			    treeList_p,
			    treeList_p->tnl_data
			    );
			}

		    break;
		    }
		
		case	1:
		    {
		    if
			(
			callback != NULL
			    && callbackNew(treeList_p, callback, data) == NULL
			)
			{
			callback(treeList_p, data, 0);
			}

		    break;
		    }
		
		case	2:
		    {
		    if(callback != NULL) callback(treeList_p, data, 1);
		    break;
		    }
		}
	    }

	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"treeListOpen() = %d Exited.\n",
		(treeList_p == NULL)? -1: treeList_p->tnl_openCount
		);
	    }

	return((treeList_p == NULL)? -1: treeList_p->tnl_openCount);
	}

int
    treeListClose(treeList_t *treeList_p)
	{
	if(treeList_p == NULL)
	    {
	    }
	else switch(treeList_p->tnl_openCount)
	    {
	    default:
		{
		treeList_p->tnl_openCount--;
		treeList_p->tnl_expire = time(NULL) + EXPIRE_DELTA;
		break;
		}
	    
	    case	0:
		{
		break;
		}

	    case	1:
		{
		node_t
		    *curNode_p;

		if(time(NULL) > treeList_p->tnl_expire)
		    {
		    if(treeList_p->tnl_vops.tlvop_close)
			{
			treeList_p->tnl_vops.tlvop_close
			    (
			    treeList_p,
			    treeList_p->tnl_data
			    );
			}

		    while
			(
			curNode_p = (node_t *)linkOwner
			    (
			    linkNext(treeList_p->tnl_list)
			    )
			)
			{
			nodeFree(curNode_p);
			}

		    (void) treeListClose(treeList_p->tnl_parent);
		    treeList_p->tnl_openCount--;
		    }
		    
		break;
		}
	    }

	return((treeList_p == NULL)? -1: treeList_p->tnl_openCount);
	}

static void
    treeListTimeout()
	{
	treeList_t
	    *treeList_p;

	if(NodeListList == NULL) NodeListList = linkNew(NULL);
	if(NodeListList == NULL)
	    {
	    /* ERROR No Memory */
	    }
	else for
	    (
	    treeList_p = (treeList_t *)linkOwner(linkNext(NodeListList));
	    treeList_p != NULL;
	    treeList_p = (treeList_t *)linkOwner(linkNext(treeList_p->tnl_link))
	    )
	    {
	    if(treeList_p->tnl_openCount == 1)
		{
		(void) treeListClose(treeList_p);
		}
	    }
	}

node_t
    *treeListGetFirst(treeList_t *treeList_p)
	{
	return((node_t *)linkOwner(linkNext(treeList_p->tnl_list)));
	}

node_t
    *treeNodeGetByName(node_t *node_p, char *name)
	{
	treeList_t
	    *treeList_p,
	    *newNodeList_p;

	node_t
	    *result;

	char
	    *curElement_p;

	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"treeNodeGetByName(0x%x, \"%s\") entered.\n",
		(int) node_p,
		name
		);
	    }

	if(node_p == NULL)
	    {
	    name = NULL;
	    result = NULL;
	    }
	else if((treeList_p = nodeTreeList(node_p)) == NULL)
	    {
	    name = NULL;
	    result = NULL;
	    }
	else if(name == NULL || *name == '\0')
	    {
	    name = NULL;
	    result = node_p;
	    }
	else if(treeListOpen(treeList_p, NULL, NULL) < 0)
	    {
	    name = NULL;
	    result = NULL;
	    }
	else if((name = strdup(name)) == NULL)
	    {
	    /* ERROR No Memory */
	    result = NULL;
	    }
	else for
	    (
	    curElement_p = strtok(name, ".");
	    curElement_p != NULL && treeList_p != NULL;
	    curElement_p = strtok(NULL, ".")
	    )
	    {
	    if(DebugLevel > 5)
		{
		(void) fprintf(stderr, "\tcurElement_p = \"%s\"\n", curElement_p);
		}

	    for
		(
		result = treeListGetFirst(treeList_p);
		result != NULL;
		result = nodeNext(result)
		)
		{
		if(DebugLevel > 5)
		    {
		    (void) fprintf(stderr, "\t\tnodeName = \"%s\"\n", nodeName(result));
		    }

		if(strcasecmp(curElement_p, nodeName(result)))
		    {
		    }
		else
		    {
		    if(nodeIsInternal(result))
			{
			newNodeList_p = (treeList_t *)nodeTreeList(result);
			(void) treeListOpen(newNodeList_p, NULL, NULL);
			(void) treeListClose(treeList_p);
			treeList_p = newNodeList_p;
			}

		    break;
		    }
		}
	    
	    if(result == NULL) break;
	    }

	if(name != NULL) free(name);

	if(DebugLevel > 5) (void) fprintf(stderr, "treeNodeGetByName() exited.\n");
	return(result);
	}

static node_t
    *localTreeInit
	(
	int (*openFunc)(),
	void (*closeFunc)(),
	void *data,
	void (*dataFreeFunc)(),
	int debugLevel
	)
	{
	treeList_t
	    *rootNodeList;

	node_t
	    *result;

	DebugLevel = debugLevel;

	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"localTreeInit(0x%x, 0x%x, 0x%x, 0x%x, %d) Entered.\n",
		(int) openFunc,
		(int) closeFunc,
		(int) data,
		(int) dataFreeFunc,
		debugLevel
		);
	    }

	if
	    (
		(
		rootNodeList = treeListNew
		    (
		    "",
		    NULL,
		    data,
		    dataFreeFunc,
		    openFunc,
		    closeFunc,
		    NULL
		    )
		) == NULL
	    )
	    {
	    result = NULL;
	    }
	else if
	    (
		(
		result = nodeNewInternal
		    (
		    "",
		    NULL,
		    NULL,
		    NULL,
		    NULL,
		    rootNodeList,
		    NULL,
		    1
		    )
		) == NULL
	    )
	    {
	    treeListFree(rootNodeList);
	    }

	if(DebugLevel > 2)
	    {
	    (void) fprintf(stderr, "localTreeInit() = 0x%x Exited.\n", (int) result);
	    }

	return(result);
	}

static struct translator
    {
    node_t
	*(*tr_nodeNew)();

    int
	(*tr_openList)(),
	tr_fd;		/* library descriptor	*/
    char
	*tr_listName,
	*tr_name;	/* Full path		*/
    void
	*(*tr_newListData)(),
	(*tr_freeListData)(),
	(*tr_closeList)(),
	(*tr_addListElement)(),
	*tr_link;
    };

extern int
    errno;

int
    _tree_error;

int
    stricmp(char *, char *);

void
    freeElement(void *);

static void
    treeDirectoryFree(treeDir_t *treeDir_p)
	{
	if(treeDir_p != NULL)
	    {
	    if(treeDir_p->td_path != NULL) free(treeDir_p->td_path);
	    if(treeDir_p->td_soList != NULL)
		{
		linkFree(treeDir_p->td_soList);
		}
	    
	    free(treeDir_p);
	    }
	}

static treeDir_t
    *treeDirectoryNew(char *path)
	{
	treeDir_t
	    *result;
	
	if((result = (treeDir_t *)calloc(sizeof(*result), 1)) == NULL)
	    {
	    }
	else if((result->td_path = strdup(path)) == NULL)
	    {
	    treeDirectoryFree(result);
	    result = NULL;
	    }
	
	return(result);
	}

/*
 * load_xlate is a routine that will attempt to dynamically link in the
 * file specified by the network configuration structure.
 */
static struct translator
    *load_xlate(char *name)
	{
	struct translator
	    *result = NULL;

	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"load_xlate(%s) Entered.\n",
		(name == NULL)? "NIL": name
		);
	    }

	/* do a sanity check on the file ... */
	if (access(name, 00) != 0)
	    {
	    _tree_error = TREE_ACCESS;
	    }
	else if
	    (
		(
		result = (struct translator *) malloc(sizeof (struct translator))
		) == NULL
	    )
	    {
	    _tree_error = TREE_NOMEM;
	    }
	else if((result->tr_link = linkNew(result)) == NULL)
	    {
	    _tree_error = TREE_NOMEM;
	    free((char *)result);
	    result = NULL;
	    }
	else if((result->tr_name = strdup(name)) == NULL)
	    {
	    _tree_error = TREE_NOMEM;
	    linkFree(result->tr_link);
	    free((char *)result);
	    result = NULL;
	    }
	else if((result->tr_fd = _dlopen(name, RTLD_NOW)) == 0)
	    {
	    _tree_error = TREE_OPEN;
	    (void) printf("%s\n", (char *) _dlerror());
	    linkFree(result->tr_link);
	    free((char *)result->tr_name);
	    free((char *)result);
	    result = NULL;
	    }
	else
	    {
	    if
		(
		    (
		    result->tr_nodeNew = (node_t *(*)())_dlsym
			(
			result->tr_fd,
			"_nodeNew"
			)
		    ) == NULL
		)
		{
		result->tr_nodeNew = NULL;
		}

	    if
		(
		    (
		    result->tr_openList = (int (*)())_dlsym
			(
			result->tr_fd,
			"_tree_openList"
			)
		    ) == NULL
		)
		{
		result->tr_openList = NULL;
		}

	    if
		(
		    (
		    result->tr_closeList = (void (*)())_dlsym
			(
			result->tr_fd,
			"_tree_closeList"
			)
		    ) == NULL
		)
		{
		result->tr_closeList = NULL;
		}

	    if
		(
		    (
		    result->tr_addListElement = (void (*)())_dlsym
			(
			result->tr_fd,
			"_tree_addListElement"
			)
		    ) == NULL
		)
		{
		result->tr_addListElement = NULL;
		}

	    if
		(
		    (
		    result->tr_newListData = (void *(*)())_dlsym
			(
			result->tr_fd,
			"_tree_newListData"
			)
		    ) == NULL
		)
		{
		result->tr_newListData = NULL;
		}

	    if
		(
		    (
		    result->tr_freeListData = (void (*)())_dlsym
			(
			result->tr_fd,
			"_tree_freeListData"
			)
		    ) == NULL
		)
		{
		result->tr_freeListData = NULL;
		}

	    result->tr_listName = (char *)_dlsym
		(
		result->tr_fd,
		"_tree_listName"
		);
	    }

	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"load_xlate() = 0x%x Exited.\n",
		(int) result
		);
	    }

	return (result);
	}

static void
    configTreeDir(treeDir_t *treeDir_p, treeList_t *parentList_p)
	{
	struct dirent
	    *dirent_p;

	DIR
	    *dp;

	char
	    *dotPos,
	    buffer[256];

	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"configTreeDir(0x%x) Entered.\n",
		(int) treeDir_p
		);
	    }

	if((dp = opendir(treeDir_p->td_path)) == NULL)
	    {
	    }
	else while((dirent_p = readdir(dp)) != NULL)
	    {
	    if(*dirent_p->d_name == '.')
		{
		/* Invisible file */
		}
	    else if((dotPos = strrchr(dirent_p->d_name, '.')) == NULL || strcmp(dotPos, ".so"))
		{
		(void) strcpy(buffer, treeDir_p->td_path);
		(void) strcat(buffer, "/");
		(void) strcat(buffer, dirent_p->d_name);

		(void) treeListNew
		    (
		    dirent_p->d_name,
		    parentList_p,
		    (void *) treeDirectoryNew(buffer),
		    treeDirectoryFree,
		    treeDirectoryOpen,
		    NULL,
		    NULL
		    );
		}
	    }

	if(dp != NULL) (void) closedir(dp);

	if(DebugLevel > 2)
	    {
	    (void) fprintf(stderr, "configTreeDir() Exited.\n");
	    }
	}

static void
    configTreeSO(treeDir_t *treeDir_p, treeList_t *parentList_p)
	{
	struct translator
	    *cur_p;

	struct dirent
	    *dirent_p;

	DIR
	    *dp;

	char
	    *dotPos,
	    buffer[256];

	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"configTreeSO(0x%x) Entered.\n",
		(int) treeDir_p
		);
	    }

	if(treeDir_p->td_soList != NULL)
	    {
	    }
	else if((treeDir_p->td_soList = linkNew(NULL)) == NULL)
	    {
	    }
	else if((dp = opendir(treeDir_p->td_path)) == NULL)
	    {
	    }
	else while((dirent_p = readdir(dp)) != NULL)
	    {
	    if(*dirent_p->d_name == '.')
		{
		/* Invisible file */
		}
	    else if((dotPos = strrchr(dirent_p->d_name, '.')) == NULL)
		{
		/* Not a .so */
		}
	    else if(strcmp(dotPos, ".so"))
		{
		/* Not a .so */
		}
	    else
		{
		(void) strcpy(buffer, treeDir_p->td_path);
		(void) strcat(buffer, "/");
		(void) strcat(buffer, dirent_p->d_name);

		if((cur_p = load_xlate(buffer)) == NULL)
		    {
		    }
		else if(cur_p->tr_nodeNew != NULL)
		    {
		    cur_p->tr_nodeNew(parentList_p);
		    }
		else if(cur_p->tr_listName == NULL)
		    {
		    }
		else
		    {
		    (void) treeListDataFuncNew
			(
			cur_p->tr_listName,
			parentList_p,
			NULL,
			cur_p->tr_newListData,
			cur_p->tr_freeListData,
			cur_p->tr_openList,
			cur_p->tr_closeList,
			cur_p->tr_addListElement
			);
		    }
		}
	    }

	if(dp != NULL) (void) closedir(dp);

	if(DebugLevel > 2)
	    {
	    (void) fprintf(stderr, "configTreeSO() Exited.\n");
	    }
	}

static int
    treeDirectoryOpen(void *parentList_p, treeDir_t *treeDir_p)
	{
	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"treeDirectoryOpen(0x%x, 0x%x) Entered.\n",
		(int) parentList_p,
		(int) treeDir_p
		);
	    }

	configTreeDir(treeDir_p, parentList_p);
	configTreeSO(treeDir_p, parentList_p);

	treeListCallbackDo(parentList_p, 1);

	if(DebugLevel > 2) (void) fprintf(stderr, "treeDirectoryOpen() Exited.\n");

	return(1);
	}

void
    *treeInit(char *treeDirectoryPath, int debugLevel)
	{
	static void
	    *hostTreeRoot = NULL;
	
	treeDir_t
	    *treeDir_p;

	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"treeInit(%s, %d) Entered.\n",
		(treeDirectoryPath == NULL)? "NIL": treeDirectoryPath,
		debugLevel
		);
	    }

	if(hostTreeRoot != NULL)
	    {
	    }
	else if(treeDirectoryPath == NULL)
	    {
	    }
	else if((treeDir_p = treeDirectoryNew(treeDirectoryPath)) == NULL)
	    {
	    }
	else
	    {
	    hostTreeRoot = localTreeInit
		(
		treeDirectoryOpen,
		NULL,
		treeDir_p,
		free,
		debugLevel
		);
	    }

	if(DebugLevel > 2)
	    {
	    (void) fprintf(stderr, "treeInit() = 0x%x Exited.\n", (int) hostTreeRoot);
	    }

	return(hostTreeRoot);
	}

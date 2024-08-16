/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:mail/tree.h	1.2"
#if	!defined(TREE_H)
#define	TREE_H

#define	TREE_ACCESS	1
#define	TREE_NOMEM	2
#define	TREE_OPEN	3
#define	TREE_NOSYM	4

#if	!defined(TREE_OBJ)
typedef void node_t;
typedef void treeList_t;
#endif
 
#if	defined(__cplusplus)
extern "C" char *nodeName(node_t *node_p);

extern "C" int nodeIsInternal(node_t *node_p);
extern "C" void nodeDeleteFuncSet(node_t *node_p, void (*func_p)(node_t *));
extern "C" void nodeSelectFuncSet(node_t *node_p, void (*func_p)(node_t *));
extern "C" void nodeSelect(node_t *node_p);
extern "C" int nodeIsDeleteable(node_t *node_p);
extern "C" int nodeDelete(node_t *node_p);
extern "C" int treeListClose(treeList_t *treeList_p);
extern "C" int treeListOpen(treeList_t *treeList_p, void (*callback)(), void *data);

extern "C" node_t *nodeNew(char *name, treeList_t *treeList_p, void *data, void (*dataFree)(), void (*selectFunc)(), int (*canDeleteFunc)());
extern "C" node_t *nodeNext(node_t *node_p);
extern "C" node_t *treeListGetFirst(treeList_t *treeList_p);
extern "C" node_t *treeNodeGetByName(node_t *node_p, char *name);

extern "C" treeList_t *treeListNew ( char *name, treeList_t *parent, void *data, void (*freeFunc)(), int (*openFunc)(), void (*closeFunc)(), void (*addFunc)());

extern "C" void *nodeData(node_t *node_p);
extern "C" void *nodeTreeList(node_t *node_p);
extern "C" void *treeInit(char *treeDirectoryPath, int debugLevel);
extern "C" void treeListAdd(treeList_t *treeList_p);
extern "C" void treeListCallbackDo(treeList_t *treeList_p, int state);
#else
char
    *nodeName(node_t *node_p);

int
    nodeIsInternal(node_t *node_p),
    nodeIsDeleteable(node_t *node_p),
    nodeDelete(node_t *node_p),
    treeListClose(treeList_t *treeList_p),
    treeListOpen(treeList_t *treeList_p, void (*callback)(), void *data);

node_t
    *nodeNew
	(
	char *name,
	treeList_t *treeList_p,
	void *data,
	void (*dataFree)(),
	void (*selectFunc)(),
	int (*canDeleteFunc)()
	),
    *nodeNext(node_t *node_p),
    *treeListGetFirst(treeList_t *treeList_p),
    *treeNodeGetByName(node_t *node_p, char *name);

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
	);

void
    *nodeData(node_t *node_p),
    *nodeTreeList(node_t *node_p),
    nodeDeleteFuncSet(node_t *node_p, void (*func_p)(node_t *)),
    nodeSelectFuncSet(node_t *node_p, void (*func_p)(node_t *)),
    nodeSelect(node_t *node_p),
    *treeInit(char *treeDirectoryPath, int debugLevel),
    treeListAdd(treeList_t *treeList_p),
    treeListCallbackDo(treeList_t *treeList_p, int state);

#endif

#endif

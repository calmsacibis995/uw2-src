/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident "@(#)sc:Path/ksh/t.c	3.1" */
#include <Pool.h>
#include <String.h>
#include <assert.h>
#include <iostream.h>
#include <sys/param.h>

static VPool *pool;

static void poolcreate(size_t n, size_t exp)
{
	pool = new VPool(n, exp);
}

static void found(const char *p)
{
	cout << "found: " << p << endl;
}

static char * alloc()
{
	return (char*)pool->alloc();
}

static void shrink(char *p, size_t n)
{
	int i = pool->shrink(p, n);
	assert(i==1);
}

extern "C" path_expand(const char *, 
		void (*found)(const char *), 
		void (*poolcreate)(size_t, size_t), 
		char * (*alloc)(), 
		void (*shrink)(char *, size_t));

main()
{
	String s;
	while (1)
	{
		cout << "Pattern to expand? ";
		cin >> s;
		int i = path_expand(s, found, poolcreate, alloc, shrink);
//		cout << i << " files (mem utilization was " << pool->memory_utilization() << ")\n";
		delete pool;
	}

}

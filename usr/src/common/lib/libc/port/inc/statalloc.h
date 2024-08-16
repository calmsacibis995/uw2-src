/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:inc/statalloc.h	1.2"

#include <stddef.h>	/* for offsetof, size_t */
#include <stdlib.h>	/* for malloc, realloc */
#include <string.h>	/* for memset */

	/*
	* STATALLOC(ptr, type, len, error):
	*	assigns to "ptr" the address of the initial element of a
	*	static storage duration array of "len" objects with type
	*	"type", executing the "error" statement if it cannot do so.
	*
	* "ptr" must be a modifiable lvalue with type pointer to "type".
	* "type" must be such that "type *" is a pointer to "type" and
	*	that "type[]" is an array of "type".
	* "len" must be a positive constant integer expression.
	* "error" can be a [compound] statement, not just an expression,
	*	but it somehow must cause an exit from the macro's code.
	*
	* Variants (controlled by whether these macros are now defined):
	*	UNCLEARED_ALLOC	-- initial zero value not necessary.
	*	VARYING_ALLOC	-- len is not a constant expression
	*	SMALL_ALLOC	-- sizeof(type[len]) small enough
	*	_REENTRANT	-- multiple objects maintained.
	*	DSHLIB		-- allocate single object at runtime.
	*/

#undef STATALLOC
#undef _S_A_DCL
#undef _S_A_CHK
#undef _S_A_SET
#undef _S_A_CLR

#ifdef _REENTRANT

#include <stdlock.h>	/* for _libc_self */
#include <sys/types.h>	/* for id_t */

	/*
	* This version produces a unique object for each separate execution
	* flow [as identified by (*_libc_self)()].  _s_a_get() locks all
	* its lists to prevent concurrent modifications.  But since each
	* list is walked without locking, this code assumes that pointers
	* are assigned atomically and that the order of the pointer
	* assignments in the listed list updates are similarly reflected
	* in all separate execution flows.
	*/

#ifdef VARYING_ALLOC
#   define _S_A_DCL	size_t _len;
#   define _S_A_CHK(n)	if (_lp->_len < (n)) break;
#   define _S_A_SET(n)	_lp->_len = (n);
#else
#   define _S_A_DCL
#   define _S_A_CHK(n)
#   define _S_A_SET(n)
#endif

#ifdef __STDC__
extern void	*_s_a_get(void *, void *, size_t, id_t);
#else
extern void	*_s_a_get();
#endif

#define STATALLOC(ptr, type, len, error) \
do { \
	struct _list \
	{ \
		struct _list	*_next; \
		id_t		_owner; \
		_S_A_DCL \
		type		_obj[1]; \
	}; \
	static struct _s_a \
	{ \
		struct _s_a	*_link; \
		struct _list	*_head; \
	} _static_s_a; \
	register struct _list *_lp; \
	register id_t _self = (*_libc_self)(); \
\
	for (_lp = _static_s_a._head; _lp != 0; _lp = _lp->_next) \
	{ \
		if (_lp->_owner != _self) \
			continue; \
		_S_A_CHK(len) \
		goto _set_ptr; \
	} \
	_lp = (struct _list *)_s_a_get((void *)&_static_s_a, (void *)_lp, \
		(len) * sizeof(type) + offsetof(struct _list, _obj), _self); \
	if (_lp == 0) \
		error; \
	_S_A_SET(len) \
_set_ptr:; \
	(ptr) = &_lp->_obj[0]; \
} while (0)

#else /*!_REENTRANT*/

#if defined(VARYING_ALLOC) || (defined(DSHLIB) && !defined(SMALL_ALLOC))

	/*
	* These versions wait to produce the space until the function
	* is called, but only manage a single object for the process.
	*/

#ifdef UNCLEARED_ALLOC
#   define _S_A_CLR(p, s)
#else
#   define _S_A_CLR(p, s)	(void)memset((void *)(p), 0, (size_t)(s))
#endif

#ifdef VARYING_ALLOC

#ifdef UNCLEARED_ALLOC

	/*
	* There is enough simplification provided by realloc() for
	* UNCLEARED allocations to warrent its own version.
	*/

#define STATALLOC(ptr, type, len, error) \
do { \
	static type *_static_ptr; \
	static size_t _static_len; \
\
	if (_static_len < (len)) \
	{ \
		_static_ptr = (type *)realloc((void *)_static_ptr, \
			(len) * sizeof(type)); \
		if (_static_ptr == 0) \
			error; \
		_static_len = (len); \
	} \
	(ptr) = _static_ptr; \
} while (0)

#else /*!UNCLEARED_ALLOC*/

	/*
	* When the first time allocation must clear the object,
	* the code cannot collapse the first and subsequent
	* allocations into one realloc().
	*/

#define STATALLOC(ptr, type, len, error) \
do { \
	static type *_static_ptr; \
	static size_t _static_len; \
\
	if (((ptr) = _static_ptr) == 0) \
	{ \
		if ((_static_ptr = (type *)malloc((len) * sizeof(type))) != 0) \
			_S_A_CLR(_static_ptr, (len) * sizeof(type)); \
		goto _first; \
	} \
	else if (_static_len < (len)) \
	{ \
		_static_ptr = (type *)realloc((void *)_static_ptr, \
			(len) * sizeof(type)); \
	_first:; \
		if (((ptr) = _static_ptr) == 0) \
			error; \
		_static_len = (len); \
	} \
} while (0)

#endif /*UNCLEARED_ALLOC*/

#else /*!VARYING_ALLOC*/

	/*
	* The object has constant size, so only reach allocation once.
	*/

#define STATALLOC(ptr, type, len, error) \
do { \
	static type *_static_ptr; \
\
	if (((ptr) = _static_ptr) == 0) \
	{ \
		if ((_static_ptr = (type *)malloc(sizeof(type[len]))) == 0) \
			error; \
		(ptr) = _static_ptr; \
		_S_A_CLR(ptr); \
	} \
} while (0)

#endif /*VARYING_ALLOC*/

#else /*!VARYING_ALLOC && (!DSHLIB || SMALL_ALLOC)*/

	/*
	* This version trades space for time, and thus has no "error".
	* Again, only one actual constant-sized object is used.
	*/

#define STATALLOC(ptr, type, len, error) \
do { \
	static type _static_obj[len]; \
\
	(ptr) = &_static_obj[0]; \
} while (0)

#endif /*VARYING_ALLOC || (DSHLIB && !SMALL_ALLOC)*/

#endif /*_REENTRANT*/

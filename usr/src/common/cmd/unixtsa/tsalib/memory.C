#ident	"@(#)unixtsa:common/cmd/unixtsa/tsalib/memory.C	1.1"

#include <smsutapi.h>

#include <stdio.h>

FILE *MEM = NULL;

void MEMOpen(
		char *path)
{
	MEM = fopen(path, "w");
}


void MEMClose()
{
	if (MEM)
		fclose(MEM);
}


void MEMCall(
		char	*routine,
		char	*rname,
		char	*file,
		UINT32	 line)
{
	if (MEM)
		fprintf(MEM, "            %-32s called %-32s  (%s:%u)\n",
				rname, routine, file, line);
}


void *MEMcalloc(
		char	*rname,
		char	*file,
		UINT32	 line,
		size_t	 n,
		size_t	 size)
{
	void *p;

	p = calloc(n, size);
//	p = ~calloc(n, size);

	if (MEM)
		fprintf(MEM, "%08p A  %-32s <calloc(%u, %u)>  (%s:%u)\n",
				p, rname, n, size, file, line);

	return (p);
}


void _MEMfree(
		char	*rname,
		char	*file,
		UINT32	 line,
		void	*ptr)
{
	if (MEM)
		fprintf(MEM, "%p f  %-32s <free>            (%s:%u)\n",
				ptr, rname, file, line);
}

void MEMfree(
		char	*rname,
		char	*file,
		UINT32	 line,
		void	*ptr)
{
	free(ptr);
//	~free(ptr);

	if (MEM)
		fprintf(MEM, "%p F  %-32s <free>            (%s:%u)\n",
				ptr, rname, file, line);
}


void *MEMmalloc(
		char	*rname,
		char	*file,
		UINT32	 line,
		size_t	 size)
{
	void *p;

	p = malloc(size);
//	p = ~malloc(size);

	if (MEM)
		fprintf(MEM, "%p A  %-32s <malloc(%u)>      (%s:%u)\n",
				p, rname, size, file, line);

	return (p);
}


void *MEMrealloc(
		char	*rname,
		char	*file,
		UINT32	 line,
		void	*oldBlk,
		size_t	 size)
{
	void *p;

	p = realloc(oldBlk, size);
//	p = ~realloc(oldBlk, size);

	if (MEM and p isnt oldBlk)
	{
		fprintf(MEM, "%p Fr %-32s <realloc freed>   (%s:%u)\n",
				oldBlk, rname, file, line);
		fprintf(MEM, "%p Ar %-32s <realloc(%p, %u)> (%s:%u)\n",
				p, rname, oldBlk, size, file, line);
	}

	return (p);
}

void MEMStart(
		char	*routine,
		char	*file,
		UINT32	 line)
{
	if (MEM)
		fprintf(MEM, "            %-32s          (%s:%u)\n",
				routine, file, line);
}


char *MEMstrdup(
		char		*rname,
		char		*file,
		UINT32		 line,
		const char	*src)
{
	char *s;

	s = strdup(src);
//	s = ~strdup(src);

	if (MEM)
		fprintf(MEM, "%p A  %-32s <strdup(\"%s\")>      (%s:%u)\n",
				s, rname, src, file, line);

	return (s);
}


#include <nut.h>
void Free(void *address);
#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "DestroyList"
#endif
InitList(NUTInfo *handle, char *what);

void DestroyList(
		NUTInfo	*handle)
{
	LIST *el1, *el2;

	el1 = handle->head;
	while (el1)
	{
		el2 = el1;
		el1 = el1->next;
		if (el2->otherInfo)
			if (handle->freeProcedure)
			{
			   _MEMfree("DestroyList", __FILE__, __LINE__, el2->otherInfo);
			   (*(handle->freeProcedure))(el2->otherInfo);
			}

		Free(el2);
	}

	InitList(handle, NULL);
}

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "DestroyListPtr"
#endif
void DestroyListPtr(
		LISTPTR	*list)
{
	LIST *el1, *el2;

	el1 = list->head;
	while (el1)
	{
		el2 = el1;
		el1 = el1->next;
		if (el2->otherInfo)
			if (list->freeProcedure)
			{
			   _MEMfree("DestroyListPtr", __FILE__, __LINE__, el2->otherInfo);
				(*(list->freeProcedure))(el2->otherInfo);
			}

		Free(el2);
	}

	list->head = list->tail = NULL;
}


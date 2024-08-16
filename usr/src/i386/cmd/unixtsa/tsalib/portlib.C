#ident	"@(#)unixtsa:i386/cmd/unixtsa/tsalib/portlib.C	1.3"
/***
 *
 *  name	portlib.C - porting library for unix tsa
 *
 *		functions to do byte swapping necessary for data
 *		packets going to/from NetWare server (which is
 *		intel).
 ***/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <smstypes.h>
#include <string.h>

#include <tsalib.h>

/*
 * Byte swap a 4 byte value.
 * input - 4 byte unsigned integer 
 * output - 4 byte unsigned integer 
 */
UINT32 SwapUINT32(const UINT32 in)
{
	return(in);
}

/*
 * Byte swap a 2 byte value.
 * input - 2 byte unsigned integer 
 * output - 2 byte unsigned integer 
 */
UINT16 SwapUINT16(const UINT16 in)
{
	return(in);
}

 
/*
 * Byte swap a 4 byte value.
 * input - A pointer to a character buffer, the first 4 bytes are of interest
 * output - 4 byte unsigned integer 
 */
UINT32 SwapUINT32p(const char *in)
{
        UINT32 out ;

	out = *( (UINT32 *) in) ;
        return(out);
}
 
/*
 * Byte swap a 2 byte value.
 * input - A pointer to a character buffer, the first 2 bytes are of interest
 * output - 2 byte unsigned integer 
 */
UINT16 SwapUINT16p(const char *in)
{
	return( *((UINT16 *)in) ) ;
}

/*
 * Byte swap a 4 byte value. The swaping is done in place i.e, input is modified
 * input - A pointer to a character buffer, the first 4 bytes are of interest
 * output - same as input
 */
char *SwapUINT32buf(char *in)
{
        //char ch ;

        //ch = in[3];
        //in[3] = in[0] ;
        //in[0] = ch ;
        //ch = in[2];
        //in[2] = in[1] ;
        //in[1] = ch ;
 
        return(in);
}
 
/*
 * Byte swap a 2 byte value. The swaping is done in place i.e, input is modified
 * input - A pointer to a character buffer, the first 2 bytes are of interest
 * output - same as input
 */
char *SwapUINT16buf(char *in)
{
        //char ch ;

        //ch = in[1];
        //in[1] = in[0] ;
        //in[0] = ch ;
 
        return(in);
}

/*
 * Byte swap the members of a UINT64 variable. The swaping is done in place 
 * i.e, input is modified
 * input - A pointer to a UINT64 structure.
 * output - same as input
 */
UINT64 *SwapUINT64buf(UINT64 *in)
{
	//in->v[0] = SwapUINT32(in->v[0]);
	//in->v[1] = SwapUINT32(in->v[1]);
	return(in);
}

/*
 * Byte swap the members of a UINT64 variable. input is not modified
 * input - A pointer to a UINT64 structure.
 * output - A pointer to a UINT64 structure. It is a static structure. The 
 *          caller should make a copy if necessary.
 */
UINT64 *SwapUINT64(const UINT64 *in)
{
	static UINT64 out ;

	out.v[0] = SwapUINT32(in->v[0]);
	out.v[1] = SwapUINT32(in->v[1]);
	return(&out);
}

/*
 * Byte swap the members of a UINT64 variable. input is not modified
 * input - A pointer to a character buffer in which first 8 bytes are of 
 *         interest.
 * output - A pointer to a UINT64 structure. It is a static structure. The 
 *          caller should make a copy if necessary.
 */
UINT64 *SwapUINT64p(const char *in)
{
	static UINT64 out ;

	out.v[0] = SwapUINT32p(in);
	out.v[1] = SwapUINT32p(in+4);
	return(&out);
}

void *AllocateMemory(const unsigned int size)
{
	char	*ptr ;
	ptr = (char *)malloc(size);

#ifdef DEBUG
	logerror(PRIORITYWARN, "MEMORY: Allocating %u: %x\n", size, ptr) ;
	fflush(PRIORITYWARN) ;
#endif

	return( (void *)ptr ) ;
}

void FreeMemory(void *buffer)
{
#ifdef DEBUG
	logerror(PRIORITYWARN, "MEMORY: freeing %x\n", buffer) ;
	fflush(PRIORITYWARN) ;
#endif
	if ( buffer != NULL ){
		free((char *)buffer) ;
	}
}

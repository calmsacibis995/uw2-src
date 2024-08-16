#ident	"@(#)unixtsa:common/cmd/unixtsa/tsalib/gencrc.C	1.1"

#include <smsutapi.h>

#include <process.h>


UINT32 _NWSMGenerateCRC(
		UINT32		size,
		UINT32		crc,
		BUFFERPTR	ptr);

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMGenerateCRC"
#endif
UINT32 NWSMGenerateCRC(
		UINT32		size,
		UINT32		crc,
		BUFFERPTR	ptr)
{
#if defined(__TURBOC__) or defined(__MSC__)
	crc = _NWSMGenerateCRC(size, crc, ptr);
#else
	UINT32 _size;

	while (size > 0)
	{
		_size = _min(64 * 1024, size);
		crc = _NWSMGenerateCRC(_size, crc, ptr);
		ptr += _size;
		size -= _size;

		ThreadSwitch();
	}
#endif
	return (crc);
}


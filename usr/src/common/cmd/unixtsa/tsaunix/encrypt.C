#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/encrypt.C	1.4"

#include <string.h>
#include <smdr.h>
#include <time.h>
#include <smsutapi.h>

STATIC BUFFER decodeSubstitutionTable[][16] =
{
	 8,  9,  5, 12, 13,  2, 10,  3,  1,  7, 15, 14,  4, 11,  6,  0,
	 5,  6,  0,  9, 11, 14,  3, 15,  7, 12, 10, 13,  1,  8,  2,  4,
	 7, 13,  1, 14,  5,  0, 10, 15, 11,  2,  9, 12,  4,  6,  8,  3,
	 8, 12,  2, 11,  9,  6,  3,  4,  5,  7, 13, 14, 10,  1, 15,  0,
	 7, 11,  2, 14, 10,  0,  9,  6,  8, 15,  5,  3, 13,  4,  1, 12,
	 8, 10,  1, 13, 14,  4,  6, 12,  0,  5,  3,  9,  7, 11, 15,  2,
	 2, 13,  6,  9,  4, 11, 14,  7,  1,  3, 10,  5,  8, 12,  0, 15,
	12,  0, 13, 10,  1,  8, 14,  6,  2, 11,  3,  5, 15,  4,  7,  9,
	11,  9,  5,  1,  8,  0, 12, 13,  3, 15,  7,  4,  2, 10,  6, 14,
	 1, 12,  9, 14,  5, 13,  0,  8, 10, 15, 11,  2,  6,  4,  3,  7,
	 7,  5, 10, 12,  9,  1,  8, 14, 15, 11,  2,  0,  6, 13,  3,  4,
	 3, 11,  1, 14,  7, 12, 15,  0,  5, 10,  2,  9,  8, 13,  4,  6,
	 8,  5, 12,  9,  1,  4, 14,  0, 10,  3, 13,  7,  6, 15, 11,  2,
	 3,  6, 11,  5,  1,  8, 14, 10,  2,  0,  4, 12,  7, 15, 13,  9,
	12,  7, 11,  6,  2,  1, 14,  3,  5,  0, 15,  9, 10,  8,  4, 13,
	 7,  8, 15,  5, 13,  4, 12, 11, 10,  0,  1,  2,  9,  3, 14,  6,
};


STATIC BUFFER decodePositionTable[] =
{
	10, 11,  3,  0,  6,  7,  9, 13, 15,  8, 14, 12,  5,  4,  1,  2,
};

/****************************************************************************/

STATIC void Decode(
		UINT8 *key,
		UINT8 *input,
		UINT8 *output)
{
	UINT8 temp, buf[8];
	int i, j;

/*	Perform the decryption operation 16 times */
	memcpy(buf, input, 8);
	for (j = 0; j < 16; j++)
	{
	/*	Permute the nibbles in the input */
		memset(output, '\0', 8);
		for (i = 0; i < 16; i++)
		{
		/*	Fetch the nibble using the permutation table */
			temp = decodePositionTable[i];
			if (temp & 1)
				temp = buf[temp / 2] >> 4;
			else
				temp = buf[temp / 2] & 0x0F;

		/*	Now copy the selected nibble into the output */
			if (i & 1)
				output[i / 2] |= temp << 4;
			else
				output[i / 2] |= temp;
		}

		memcpy(buf, output, 8);

	/*	Rotate the nibbles in the encryption key */
		temp = key[0];
		for (i = 0; i < 7; i++)
			key[i] = (key[i + 1] << 4) | (key[i] >> 4);
		key[7] = (temp << 4) | (key[7] >> 4);

	/*	Do a single decryption */
		for (i = 0; i < 8; i++)
		{
		/* Do a position-sensitive substitution of nibbles in the byte. */
			temp = buf[i];
			temp = (decodeSubstitutionTable[i * 2][temp & 0x0F]) |
					(decodeSubstitutionTable[i * 2 + 1][temp >> 4] << 4);

		/*	Xor a byte of the key and a byte of the input */
			buf[i] = temp ^ key[i];
		}
	}

/*	Copy the final results to the output */
	memcpy(output, buf, 8);
}

void DecryptPassword(
UINT8  *key, 		// 8 byte encryption key
UINT8  *authentication,	// encrypted pw, len is first two bytes
UINT8  *password)	// returns clear password, len is first two bytes
{
	int index, len;

	*(UINT16 *)password = 0;

	len = SwapUINT16p((char *)authentication);
	if (!len || len > 128)
		return;

	if (authentication[2])
		return;

/*	Decode the authentication into password */
	for (index = 0; index < len; index += 8)
		Decode((UINT8*)key, (UINT8*)(authentication + 3 + index), (UINT8*)(password + 2 + index));

/*	Terminate the clear password and return */
	*(UINT8 *)(password + len - 2) = 0;
	*(UINT16 *)password = strlen((char*)(password + 2));
}

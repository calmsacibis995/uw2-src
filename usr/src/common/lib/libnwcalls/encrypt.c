/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:encrypt.c	1.5"
#include "ntypes.h"
#include "nwclient.h"
#include "nwcaldef.h"
#include "nwaccess.h"
#include "ncpbind.h"
#include "encrypt.h"


/* Tables used by Encode() */

static nuint8 ES[16][16] =
{ /* Nibble substitution table */
  {15, 8, 5, 7,12, 2,14, 9, 0, 1, 6,13, 3, 4,11,10},
  { 2,12,14, 6,15, 0, 1, 8,13, 3,10, 4, 9,11, 5, 7},
  { 5, 2, 9,15,12, 4,13, 0,14,10, 6, 8,11, 1, 3, 7},
  {15,13, 2, 6, 7, 8, 5, 9, 0, 4,12, 3, 1,10,11,14},
  { 5,14, 2,11,13,10, 7, 0, 8, 6, 4, 1,15,12, 3, 9},
  { 8, 2,15,10, 5, 9, 6,12, 0,11, 1,13, 7, 3, 4,14},
  {14, 8, 0, 9, 4,11, 2, 7,12, 3,10, 5,13, 1, 6,15},
  { 1, 4, 8,10,13,11, 7,14, 5,15, 3, 9, 0, 2, 6,12},
  { 5, 3,12, 8,11, 2,14,10, 4, 1,13, 0, 6, 7,15, 9},
  { 6, 0,11,14,13, 4,12,15, 7, 2, 8,10, 1, 5, 3, 9},
  {11, 5,10,14,15, 1,12, 0, 6, 4, 2, 9, 3,13, 7, 8},
  { 7, 2,10, 0,14, 8,15, 4,12,11, 9, 1, 5,13, 3, 6},
  { 7, 4,15, 9, 5, 1,12,11, 0, 3, 8,14, 2,10, 6,13},
  { 9, 4, 8, 0,10, 3, 1,12, 5,15, 7, 2,11,14, 6,13},
  { 9, 5, 4, 7,14, 8, 3, 1,13,11,12, 2, 0,15, 6,10},
  { 9,10,11,13, 5, 3,15, 0, 1,12, 8, 7, 6, 4,14, 2}
};

static nuint8 EP[16] =
{ /* Nibble permutation table */
   3,14,15, 2,13,12, 4, 5, 9, 6, 0, 1,11, 7,10, 8
};

/* Tables used by Encrypt() */

static nuint8 S[] =
{ /* Substitution table */
   7, 8, 0, 8, 6, 4,14, 4, 5,12, 1, 7,11,15,10, 8,
  15, 8,12,12, 9, 4, 1,14, 4, 6, 2, 4, 0,10,11, 9,
   2,15,11, 1,13, 2, 1, 9, 5,14, 7, 0, 0, 2, 6, 6,
   0, 7, 3, 8, 2, 9, 3,15, 7,15,12,15, 6, 4,10, 0,
   2, 3,10,11,13, 8, 3,10, 1, 7,12,15, 1, 8, 9,13,
   9, 1, 9, 4,14, 4,12, 5, 5,12, 8,11, 2, 3, 9,14,
   7, 7, 6, 9,14,15,12, 8,13, 1,10, 6,14,13, 0, 7,
   7,10, 0, 1,15, 5, 4,11, 7,11,14,12, 9, 5,13, 1,
  11,13, 1, 3, 5,13,14, 6, 3, 0,11,11,15, 3, 6, 4,
   9,13,10, 3, 1, 4, 9, 4, 8, 3,11,14, 5, 0, 5, 2,
  12,11,13, 5,13, 5,13, 2,13, 9,10,12,10, 0,11, 3,
   5, 3, 6, 9, 5, 1,14,14, 0,14, 8, 2,13, 2, 2, 0,
   4,15, 8, 5, 9, 6, 8, 6,11,10,11,15, 0, 7, 2, 8,
  12, 7, 3,10, 1, 4, 2, 5,15, 7,10,12,14, 5, 9, 3,
  14, 7, 1, 2,14, 1,15, 4,10, 6,12, 6,15, 4, 3, 0,
  12, 0, 3, 6,15, 8, 7,11, 2,13,12, 6,10,10, 8,13,
};

static nuint8 P[] =
{ /* Value subtracted from nuint8 to make it position sensitive */
  0x48, 0x93, 0x46, 0x67, 0x98, 0x3D, 0xE6, 0x8D,
  0xB7, 0x10, 0x7A, 0x26, 0x5A, 0xB9, 0xB1, 0x35,
  0x6B, 0x0F, 0xD5, 0x70, 0xAE, 0xFB, 0xAD, 0x11,
  0xF4, 0x47, 0xDC, 0xA7, 0xEC, 0xCF, 0x50, 0xC0,
};

/***************************************************************************
      This function retrieves the encryption key from an encrypted
      password.

      Apparently there are two keys used in the encryption of the
      password.  One is the login key, and one is the password key.  The
      password is first encrypted with the login key, then in order to
      get the password key, this function is called and the login key is
      passed in.


 PARAMETERS   :   -> input          <-output

      -> inputKey
         - Pointer to the login key

      -> cryptPass
         - Pointer to the encrypted password

      <- outputKey
         - Returns a pointer to the password key
****************************************************************************/
N_GLOBAL_LIBRARY( void )
GetPasswordKey
(
  pnuint8 inputKey,
  pnuint8 cryptPass,
  pnuint8 outputKey
)
{
   nuint8 temp[32];
   int  i, j;

#ifdef N_PLAT_UNIX
   nuint32	lptr;

   NWCMemCpy((char *)&lptr, (char *)inputKey, 4);
   EncryptPassword(lptr, (pnstr8)cryptPass, 16, temp);

   NWCMemCpy((char *)&lptr, (char *)inputKey+4, 4);
   EncryptPassword(lptr, (pnstr8)cryptPass, 16, temp + 16);

#else

   /* Encode crypt password with 1st half of key */
   EncryptPassword(*(pnuint32)inputKey, cryptPass, 16, temp);

   /* Encode crypt password with 2nd half of key */
   EncryptPassword(*(pnuint32)(inputKey + 4), cryptPass, 16, temp + 16);

#endif

   /* Fold temp results into 8 nuint8s */
   for (i = 0, j = 31; i < 16; i++, j--)
      temp[i] ^= temp[j];
   for (i = 0, j = 15; i < 8; i++, j--)
      outputKey[i] = temp[i] ^ temp[j];
}

/****************************************************************************
      This function encrypts a password using the object's bindery ID.

      First, the password is taken and divided into 32 nuint8 chunks. If
      the password is not evenly divisible by 32 the last chunk is
      processed separately.  Then the 32 nuint8 chunks are exclusive ORed
      together into a 32 nuint8 buffer, and if there is an odd chunk, it
      is also added.  Then the 4 nuint8s from the object's bindery ID are
      exclusive ORed on top of the resulting 32 nuint8 buffer.  Finally,
      the buffer is passed into Encrypt which rolls out the 16 nuint8
      encrytped password.

      This function is mis-labeled, since the output is not an encrypted
      password.  The output is really what would be called a footprint.
      The reason for the footprint is to provide as close to a "unique"
      method for the real encryption function to use.

      The footprint generated for each object will be stored in the
      bindery, therefore allowing the server to extract this footprint
      from an encrypted password and have a high degree of confidence
      that the object submitting a password is indeed the object that
      generated the footprint.  In other nuint16s, the footprint is used
      for authentication.
****************************************************************************/
N_GLOBAL_LIBRARY( void )
EncryptPassword
(
  nuint32   ID,
  pnstr8    password,
  nuint16   length,
  pnuint8   output
)
{
   pnstr8 p;
   nuint8 buf[32];
   nuint16 i;

   /* Strip off trailing zero's.  This means that a password less than
      128 nuint8s long will be the same as one padded with zeros to be
      128 nuint8s in length.
   */

   if(length)
   {
      p = password + length - 1;
      while (*p == 0 && length != 0)
      {
         p--;
         length--;
      }
   }

   /* Fold the password into 32 nuint8s.  The password is divided into 32
      nuint8 chunks.  if the last chunk is less than 32 nuint8s long, it is
      repeated to fill 32 nuint8s. The individual hunks are xor-ed together
      and the 32 nuint8 result is stored in buf.  Then the user-id is
      xor-ed through the 32 nuint8s to make the encryption partially
      dependent on the user-id.
   */

   NWCMemSet( buf, 0, 32 );
   while (length >= 32)
   {
      for (i = 0; i < 32; i++)
         buf[i] ^= *password++;
      length -= 32;
   }
   p = password;
   if(length)
   {
      for (i = 0; i < 32; i++)
      {
         /* Repeat the remaining portion of the password until all 32
            nuint8s are filled.  A null is used to separate adjacent
            copies of the password
         */
         if(p == (password + length))
         {
            p = password;
            buf[i] ^= P[i];
         }
         else
            buf[i] ^= *p++;
      }
   }

   for (i = 0; i < 32; i++)
   {
      /* xor in nuint8s from the ID (a 4 nuint8 quantity) */
      buf[i] ^= ((pnuint8 )&ID)[i & 0x03];
   }

   /* encrypt the folded password */
   Encrypt(buf, output);
}

/****************************************************************************
      This function will produce encrypt the input, returning the result
      in output.

      The encryption is done by using some internal substitution tables
      and an algorithm which may have to be documented later.

      -> input
         - Pointer to the 32 nuint8 input buffer

      <- output
         - Pointer to the 16 nuint8 output buffer
****************************************************************************/
N_GLOBAL_LIBRARY( void )
Encrypt
(
   pnuint8 input,
   pnuint8 output
)
{
   int i, j;
   nuint8 k, C;

   C = 0;
   for (j = 0; j < 2; j++)
   {
      for (i = 0; i < 32; i++)
      {
         /* Exclusive-or a pair of nuint8s offset from one another by 7,
            considering the 16 input nuint8s as a ring.  Modify this value
            to be position sensitive by subtracting 1 of 32 values in the
            range 0..255.
         */
         k = (input[i] + C) ^ (input[(i + C) & 0x1F] - P[i]);
         C += k;

         input[i] = k;
      }
   }
   /* Now convert nuint8s to nibbles using the substitution table */
   NWCMemSet(output, 0, 16);
   for (i = 0; i < 32; i++)
   {
      /* Look up the substitution nibble from the table */
      if(i & 1)
         output[i/2] |= S[input[i]] << 4;
      else
         output[i/2] |= S[input[i]];
   }
}

/****************************************************************************
      Encode() takes an 8 nuint8 key, an 8 nuint8 input, and produces an 8
      nuint8 encrypted output.  Decode() takes the same key and 8 nuint8s of
      code encrypted by Encode() and produces an 8 nuint8 decrypted output

 PARAMETERS   :   -> input          <-output

      -> key
         - Pointer to the 8 nuint8 key

      -> input
         - Pointer to the 8 nuint8 input buffer

      <- output
         - Pointer to the 8 nuint8 output buffer
****************************************************************************/
N_GLOBAL_LIBRARY( void )
Encode
(
   pnuint8 key,
   pnuint8 input,
   pnuint8 output
)
{
   nuint8 temp, buf[8];
   int i, j;

  /* Copy the input to the intermediate buffer */

   NWCMemMove( buf, input, 8 );

  /* Perform the encryption operation 16 times */
   for (j = 0; j < 16; j++)
   {
      /* Do a single encryption */
      for (i = 0; i < 8; i++)
      {
         /* Xor a nuint8 of the key and a nuint8 of the input */
         temp = buf[i] ^ key[i];

         /* Do a position-sensitive substitution of the nibbles in the
            nuint8.
         */
         buf[i] = (nuint8)(ES[i*2][temp & 0x0F])|(nuint8)(ES[i*2+1][temp >> 4] << 4);
      }

      /* Rotate the nibbles in the encryption key */
      temp = key[7];
      for (i = 7; i > 0; i--)
         key[i] = (nuint8)((key[i] << 4) | (key[i-1] >> 4));

      key[0] = (nuint8)(key[0] << 4) | (nuint8)(temp >> 4);

      /* Now permute the nibbles */
      NWCMemSet(output, 0, 8);
      for (i = 0; i < 16; i++)
      {
         /* Fetch the nibble using the permutation table */
         temp = EP[i];
         if(temp & 1)
            temp = (nuint8)(buf[temp/2] >> 4);
         else
            temp = (nuint8)(buf[temp/2] & 0x0F);

         /* Now copy the selected nibble into the output */
         if(i & 1)
            output[i/2] |= temp << 4;
         else
            output[i/2] |= temp;
      }
      NWCMemMove( buf, output, 8 );
   }
}

/****************************************************************************
 DESCRIPTION  :

      This function returns a key (a random number) provided
      by the server speicied.

      This key can then be used for password encryption.

      <- key
        - Returns a pointer to the key

 NCP:   23 23  Get Login Key
****************************************************************************/
N_GLOBAL_LIBRARY( NWCCODE )
GetLoginKey
(
   NWCONN_HANDLE  conn,
   pnuint8        pbuKeyB8
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP23s23GetLoginKey(&access, pbuKeyB8));
}

/* *********************************************************************** */

N_GLOBAL_LIBRARY( NWCCODE )
NWGetLoginPasswordKey
(
   NWCONN_HANDLE conn, 
   pnuint8 pbuCryptPwd,
   pnuint8 pbuPwdKey
)
{
   NWCCODE ccode;

   ccode = GetLoginKey(conn, pbuPwdKey);
   if ( ccode == 0 )
   {
      GetPasswordKey(pbuPwdKey, pbuCryptPwd, pbuPwdKey);
   }
   
   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/encrypt.c,v 1.7 1994/09/26 17:45:23 rebekah Exp $
*/


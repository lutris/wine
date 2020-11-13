/*
 * Hash definitions
 *
 * Copyright 2005 Huw Davies
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#ifndef __WIDL_HASH_H
#define __WIDL_HASH_H

#include "windef.h"

extern unsigned int lhash_val_of_name_sys( syskind_t skind, LCID lcid, LPCSTR lpStr);

typedef struct
{
   ULONG Unknown[6];
   ULONG State[5];
   ULONG Count[2];
   UCHAR Buffer[64];
} SHA_CTX;

VOID A_SHAInit(SHA_CTX *ctx);
VOID A_SHAUpdate(SHA_CTX *ctx, const UCHAR *buffer, UINT size);
VOID A_SHAFinal(SHA_CTX *ctx, PULONG result);

#endif

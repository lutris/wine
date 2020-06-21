/*
 * Copyright (C) 2017 Michael Müller
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
 */
#ifndef _API_SET_H_
#define _API_SET_H_

#include <windef.h>

typedef struct _API_SET_NAMESPACE_ENTRY
{
    ULONG NameOffset;
    ULONG NameLength;
    ULONG DataOffset;
} API_SET_NAMESPACE_ENTRY, *PAPI_SET_NAMESPACE_ENTRY;

typedef struct _API_SET_NAMESPACE_ARRAY
{
    ULONG Version;
    ULONG Count;
    API_SET_NAMESPACE_ENTRY Array[1];
} API_SET_NAMESPACE_ARRAY, *PAPI_SET_NAMESPACE_ARRAY;

#endif

/*
 * Copyright 2019 Alistair Leslie-Hughes
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
#ifndef WINE_DSDMO_PRIVATE
#define WINE_DSDMO_PRIVATE

#include "windows.h"
#include "mmsystem.h"
#include "mediaobj.h"

#include "wine/heap.h"
#include "wine/debug.h"

#include "dsound.h"

extern HRESULT WINAPI EchoFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)  DECLSPEC_HIDDEN;
extern HRESULT WINAPI ChrousFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv) DECLSPEC_HIDDEN;
extern HRESULT WINAPI CompressorFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv) DECLSPEC_HIDDEN;
extern HRESULT WINAPI DistortionFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv) DECLSPEC_HIDDEN;
extern HRESULT WINAPI FlangerFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv) DECLSPEC_HIDDEN;
extern HRESULT WINAPI GargleFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv) DECLSPEC_HIDDEN;
extern HRESULT WINAPI ParamEqFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv) DECLSPEC_HIDDEN;
extern HRESULT WINAPI ReverbFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv) DECLSPEC_HIDDEN;
extern HRESULT WINAPI I3DL2Reverb_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv) DECLSPEC_HIDDEN;

#endif

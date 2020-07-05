/*
 * Copyright 2014 Sebastian Lackner for Pipelight
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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"

#include "wine/debug.h"

#define COBJMACROS
#include "d3d9.h"
#include "dxva2api.h"
#include "dxva2_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxva2);

typedef struct
{
    IDirect3DDeviceManager9 IDirect3DDeviceManager9_iface;
    LONG refCount;
    UINT token;
    IDirect3DDevice9 *device;
} Direct3DDeviceManager9Impl;

static inline Direct3DDeviceManager9Impl *impl_from_Direct3DDeviceManager9( IDirect3DDeviceManager9 *iface )
{
    return CONTAINING_RECORD(iface, Direct3DDeviceManager9Impl, IDirect3DDeviceManager9_iface);
}

/*****************************************************************************
 * IDirect3DDeviceManager9  interface
 */

static HRESULT WINAPI Direct3DDeviceManager9_QueryInterface( IDirect3DDeviceManager9 *iface, REFIID riid, LPVOID *ppv )
{
    Direct3DDeviceManager9Impl *This = impl_from_Direct3DDeviceManager9(iface);
    TRACE("(%p/%p)->(%s, %p)\n", iface, This, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirect3DDeviceManager9))
        *ppv = &This->IDirect3DDeviceManager9_iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI Direct3DDeviceManager9_AddRef( IDirect3DDeviceManager9 *iface )
{
    Direct3DDeviceManager9Impl *This = impl_from_Direct3DDeviceManager9(iface);
    ULONG refCount = InterlockedIncrement(&This->refCount);

    TRACE("(%p)->() AddRef from %d\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI Direct3DDeviceManager9_Release( IDirect3DDeviceManager9 *iface )
{
    Direct3DDeviceManager9Impl *This = impl_from_Direct3DDeviceManager9(iface);
    ULONG refCount = InterlockedDecrement(&This->refCount);

    TRACE("(%p)->() Release from %d\n", This, refCount + 1);

    if (!refCount)
    {
        TRACE("Destroying\n");

        if (This->device)
            IDirect3DDevice9_Release(This->device);

        CoTaskMemFree(This);
    }

    return refCount;
}

static HRESULT WINAPI Direct3DDeviceManager9_ResetDevice( IDirect3DDeviceManager9 *iface, IDirect3DDevice9 *pDevice, UINT resetToken )
{
    Direct3DDeviceManager9Impl *This = impl_from_Direct3DDeviceManager9(iface);

    FIXME("(%p)->(%p, %u): semi-stub\n", This, pDevice, resetToken);

    if (This->device)
        return E_FAIL;

    if (resetToken != This->token)
        return E_INVALIDARG;

    This->device = pDevice;
    IDirect3DDevice9_AddRef(This->device);

    /* TODO: Reset the device, verify token ... */

    return S_OK;
}

static HRESULT WINAPI Direct3DDeviceManager9_OpenDeviceHandle( IDirect3DDeviceManager9 *iface, HANDLE *phDevice )
{
    Direct3DDeviceManager9Impl *This = impl_from_Direct3DDeviceManager9(iface);

    FIXME("(%p)->(%p): semi-stub\n", This, phDevice);

    *phDevice = (HANDLE)This->device;
    return S_OK;
}

static HRESULT WINAPI Direct3DDeviceManager9_CloseDeviceHandle( IDirect3DDeviceManager9 *iface, HANDLE hDevice )
{
    Direct3DDeviceManager9Impl *This = impl_from_Direct3DDeviceManager9(iface);

    FIXME("(%p)->(%p): stub\n", This, hDevice);

    return S_OK;
}

static HRESULT WINAPI Direct3DDeviceManager9_TestDevice( IDirect3DDeviceManager9 *iface, HANDLE hDevice )
{
    Direct3DDeviceManager9Impl *This = impl_from_Direct3DDeviceManager9(iface);
    static int once = 0;

    if (!once++)
        FIXME("(%p)->(%p): stub\n", This, hDevice);

    return S_OK;
}

static HRESULT WINAPI Direct3DDeviceManager9_LockDevice( IDirect3DDeviceManager9 *iface, HANDLE hDevice, IDirect3DDevice9 **ppDevice, BOOL fBlock )
{
    Direct3DDeviceManager9Impl *This = impl_from_Direct3DDeviceManager9(iface);

    FIXME("(%p)->(%p, %p, %d): semi-stub\n", This, hDevice, ppDevice, fBlock);

    *ppDevice = (IDirect3DDevice9 *)hDevice;
    IDirect3DDevice9_AddRef(*ppDevice);
    return S_OK;
}

static HRESULT WINAPI Direct3DDeviceManager9_UnlockDevice( IDirect3DDeviceManager9 *iface, HANDLE hDevice, BOOL fSaveState )
{
    Direct3DDeviceManager9Impl *This = impl_from_Direct3DDeviceManager9(iface);

    FIXME("(%p)->(%p, %d): stub\n", This, hDevice, fSaveState);

    return S_OK;
}

static HRESULT WINAPI Direct3DDeviceManager9_GetVideoService( IDirect3DDeviceManager9 *iface, HANDLE hDevice, REFIID riid, void **ppService )
{
    Direct3DDeviceManager9Impl *This = impl_from_Direct3DDeviceManager9(iface);

    FIXME("(%p)->(%p, %p, %p): semi-stub\n", This, hDevice, riid, ppService);

    return videoservice_create( (IDirect3DDevice9 *)hDevice, riid, ppService );
}

static const IDirect3DDeviceManager9Vtbl Direct3DDeviceManager9_VTable =
{
    Direct3DDeviceManager9_QueryInterface,
    Direct3DDeviceManager9_AddRef,
    Direct3DDeviceManager9_Release,
    Direct3DDeviceManager9_ResetDevice,
    Direct3DDeviceManager9_OpenDeviceHandle,
    Direct3DDeviceManager9_CloseDeviceHandle,
    Direct3DDeviceManager9_TestDevice,
    Direct3DDeviceManager9_LockDevice,
    Direct3DDeviceManager9_UnlockDevice,
    Direct3DDeviceManager9_GetVideoService
};

HRESULT devicemanager_create( UINT *resetToken, void **ppv )
{
    Direct3DDeviceManager9Impl *devicemanager;

    if (!resetToken || !ppv)
        return E_INVALIDARG;

    *ppv = NULL;

    devicemanager = CoTaskMemAlloc(sizeof(Direct3DDeviceManager9Impl));
    if (!devicemanager)
        return E_OUTOFMEMORY;

    devicemanager->IDirect3DDeviceManager9_iface.lpVtbl = &Direct3DDeviceManager9_VTable;
    devicemanager->refCount = 1;
    devicemanager->token    = 0xdeadbeef; /* FIXME: provide some better value? */
    devicemanager->device   = NULL;

    *resetToken = devicemanager->token;
    *ppv        = devicemanager;
    return S_OK;
}

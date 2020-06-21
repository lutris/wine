/*
 * Copyright 2014-2015 Michael Müller for Pipelight
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
    IDirectXVideoAccelerationService IDirectXVideoAccelerationService_iface;
    IDirectXVideoDecoderService      IDirectXVideoDecoderService_iface;
    IDirectXVideoProcessorService    IDirectXVideoProcessorService_iface;

    LONG refCount;
    IDirect3DDevice9 *device;
    IWineVideoService *backend;
} DirectXVideoAccelerationServiceImpl;

static inline DirectXVideoAccelerationServiceImpl *impl_from_IDirectXVideoAccelerationService( IDirectXVideoAccelerationService *iface )
{
    return CONTAINING_RECORD(iface, DirectXVideoAccelerationServiceImpl, IDirectXVideoAccelerationService_iface);
}

static inline DirectXVideoAccelerationServiceImpl *impl_from_IDirectXVideoDecoderService( IDirectXVideoDecoderService *iface )
{
    return CONTAINING_RECORD(iface, DirectXVideoAccelerationServiceImpl, IDirectXVideoDecoderService_iface);
}

static inline DirectXVideoAccelerationServiceImpl *impl_from_IDirectXVideoProcessorService( IDirectXVideoProcessorService *iface )
{
    return CONTAINING_RECORD(iface, DirectXVideoAccelerationServiceImpl, IDirectXVideoProcessorService_iface);
}

/*****************************************************************************
 * IDirectXVideoAccelerationService interface
 */

static HRESULT WINAPI DirectXVideoAccelerationService_QueryInterface( IDirectXVideoAccelerationService *iface, REFIID riid, LPVOID *ppv )
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoAccelerationService(iface);
    TRACE("(%p/%p)->(%s, %p)\n", iface, This, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = &This->IDirectXVideoAccelerationService_iface;
    else if (IsEqualIID(riid, &IID_IDirectXVideoAccelerationService))
        *ppv = &This->IDirectXVideoAccelerationService_iface;
    else if (IsEqualIID(riid, &IID_IDirectXVideoDecoderService))
        *ppv = &This->IDirectXVideoDecoderService_iface;
    else if (IsEqualIID(riid, &IID_IDirectXVideoProcessorService))
        *ppv = &This->IDirectXVideoProcessorService_iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI DirectXVideoAccelerationService_AddRef( IDirectXVideoAccelerationService *iface )
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoAccelerationService(iface);
    ULONG refCount = InterlockedIncrement(&This->refCount);

    TRACE("(%p)->() AddRef from %d\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI DirectXVideoAccelerationService_Release( IDirectXVideoAccelerationService *iface )
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoAccelerationService(iface);
    ULONG refCount = InterlockedDecrement(&This->refCount);

    TRACE("(%p)->() Release from %d\n", This, refCount + 1);

    if (!refCount)
    {
        TRACE("Destroying\n");
        IDirect3DDevice9_Release(This->device);
        IWineVideoService_Release(This->backend);
        CoTaskMemFree(This);
    }

    return refCount;
}

static HRESULT WINAPI DirectXVideoAccelerationService_CreateSurface( IDirectXVideoAccelerationService *iface, UINT width, UINT height,
        UINT backBuffers, D3DFORMAT format, D3DPOOL pool, DWORD usage, DWORD dxvaType, IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle )
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoAccelerationService(iface);
    HRESULT hr = S_OK;
    int i;

    FIXME("(%p)->(%u, %u, %u, %#x, 0x%x, 0x%x, 0x%x, %p, %p): semi-stub\n",
        This, width, height, backBuffers, format, pool, usage, dxvaType, ppSurface, pSharedHandle);

    /* We create only backbuffers as the front buffer usually does not support this format */
    for (i = 0; i < backBuffers + 1; i++)
    {
        hr = IDirect3DDevice9_CreateOffscreenPlainSurface(This->device, width, height, format,
                                                          pool, &ppSurface[i], pSharedHandle);
        if (FAILED(hr))
        {
            while (i-- > 0)
                IDirect3DSurface9_Release(ppSurface[i]);
            return hr;
        }
    }

    return S_OK;
}

static const IDirectXVideoAccelerationServiceVtbl DirectXVideoAccelerationService_VTable =
{
    DirectXVideoAccelerationService_QueryInterface,
    DirectXVideoAccelerationService_AddRef,
    DirectXVideoAccelerationService_Release,
    DirectXVideoAccelerationService_CreateSurface
};


/*****************************************************************************
 * IDirectXVideoDecoderService interface
 */

static HRESULT WINAPI DirectXVideoDecoderService_QueryInterface( IDirectXVideoDecoderService *iface, REFIID riid, LPVOID *ppv )
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoDecoderService(iface);

    TRACE("(%p/%p)->(%s, %p)\n", iface, This, debugstr_guid(riid), ppv);

    return DirectXVideoAccelerationService_QueryInterface(&This->IDirectXVideoAccelerationService_iface, riid, ppv);
}

static ULONG WINAPI DirectXVideoDecoderService_AddRef( IDirectXVideoDecoderService *iface )
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoDecoderService(iface);

    return DirectXVideoAccelerationService_AddRef(&This->IDirectXVideoAccelerationService_iface);
}

static ULONG WINAPI DirectXVideoDecoderService_Release( IDirectXVideoDecoderService *iface )
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoDecoderService(iface);

    return DirectXVideoAccelerationService_Release(&This->IDirectXVideoAccelerationService_iface);
}

static HRESULT WINAPI DirectXVideoDecoderService_CreateSurface( IDirectXVideoDecoderService *iface, UINT width, UINT height, UINT backBuffers,
        D3DFORMAT format, D3DPOOL pool, DWORD usage, DWORD dxvaType, IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle )
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoDecoderService(iface);

    FIXME("(%p/%p)->(%u, %u, %u, %#x, 0x%x, 0x%x, 0x%x, %p, %p): stub\n",
        iface, This, width, height, backBuffers, format, pool, usage, dxvaType, ppSurface, pSharedHandle);

    return DirectXVideoAccelerationService_CreateSurface(&This->IDirectXVideoAccelerationService_iface,
        width, height, backBuffers, format, pool, usage, dxvaType, ppSurface, pSharedHandle);
}

static HRESULT WINAPI DirectXVideoDecoderService_CreateVideoDecoder( IDirectXVideoDecoderService *iface, REFGUID guid,
        const DXVA2_VideoDesc *pVideoDesc, DXVA2_ConfigPictureDecode *pConfig, IDirect3DSurface9 **ppDecoderRenderTargets, UINT NumSurfaces,
        IDirectXVideoDecoder **ppDecode )
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoDecoderService(iface);
    IWineVideoDecoder *decoder_backend;
    HRESULT hr;

    FIXME("(%p/%p)->(%s, %p, %p, %p, %u, %p): stub\n",
        iface, This, debugstr_guid(guid), pVideoDesc, pConfig, ppDecoderRenderTargets, NumSurfaces, ppDecode);

    if (!guid || !pVideoDesc || !pConfig || !ppDecoderRenderTargets || !NumSurfaces || !ppDecode)
        return E_INVALIDARG;

    hr = IWineVideoService_CreateVideoDecoder(This->backend, guid, pVideoDesc, pConfig, NumSurfaces, &decoder_backend);
    if (SUCCEEDED(hr))
    {
        hr = genericdecoder_create(iface, pVideoDesc, pConfig, ppDecoderRenderTargets, NumSurfaces, decoder_backend, ppDecode);
        IWineVideoDecoder_Release(decoder_backend);
    }

    return hr;
}

static HRESULT WINAPI DirectXVideoDecoderService_GetDecoderConfigurations( IDirectXVideoDecoderService *iface, REFGUID guid,
        const DXVA2_VideoDesc *pVideoDesc, IUnknown *pReserved, UINT *pCount, DXVA2_ConfigPictureDecode **ppConfigs )
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoDecoderService(iface);

    FIXME("(%p/%p)->(%s, %p, %p, %p, %p): semi-stub\n",
        iface, This, debugstr_guid(guid), pVideoDesc, pReserved, pCount, ppConfigs);

    if (!guid || !pVideoDesc || !pCount || !ppConfigs)
        return E_INVALIDARG;

    return IWineVideoService_GetDecoderConfigurations(This->backend, guid, pVideoDesc, pReserved, pCount, ppConfigs);
}

static HRESULT WINAPI DirectXVideoDecoderService_GetDecoderDeviceGuids( IDirectXVideoDecoderService *iface, UINT *count, GUID **pGuids )
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoDecoderService(iface);

    FIXME("(%p/%p)->(%p, %p): semi-stub\n", iface, This, count, pGuids);

    if (!count || !pGuids)
        return E_INVALIDARG;

    return IWineVideoService_GetDecoderDeviceGuids(This->backend, count, pGuids);
}

static HRESULT WINAPI DirectXVideoDecoderService_GetDecoderRenderTargets( IDirectXVideoDecoderService *iface, REFGUID guid,
                                                                          UINT *pCount, D3DFORMAT **pFormats )
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoDecoderService(iface);

    FIXME("(%p/%p)->(%s, %p, %p): semi-stub\n", iface, This, debugstr_guid(guid), pCount, pFormats);

    if (!guid || !pCount || !pFormats)
        return E_INVALIDARG;

    return IWineVideoService_GetDecoderRenderTargets(This->backend, guid, pCount, pFormats);
}

static const IDirectXVideoDecoderServiceVtbl DirectXVideoDecoderService_VTable =
{
    DirectXVideoDecoderService_QueryInterface,
    DirectXVideoDecoderService_AddRef,
    DirectXVideoDecoderService_Release,
    DirectXVideoDecoderService_CreateSurface,
    DirectXVideoDecoderService_GetDecoderDeviceGuids,
    DirectXVideoDecoderService_GetDecoderRenderTargets,
    DirectXVideoDecoderService_GetDecoderConfigurations,
    DirectXVideoDecoderService_CreateVideoDecoder
};

/*****************************************************************************
 * IDirectXVideoProcessorService interface
 */

static HRESULT WINAPI DirectXVideoProcessorService_QueryInterface( IDirectXVideoProcessorService *iface, REFIID riid, LPVOID *ppv )
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoProcessorService(iface);

    TRACE("(%p/%p)->(%s, %p)\n", iface, This, debugstr_guid(riid), ppv);

    return DirectXVideoAccelerationService_QueryInterface(&This->IDirectXVideoAccelerationService_iface, riid, ppv);
}

static ULONG WINAPI DirectXVideoProcessorService_AddRef( IDirectXVideoProcessorService *iface )
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoProcessorService(iface);

    return DirectXVideoAccelerationService_AddRef(&This->IDirectXVideoAccelerationService_iface);
}

static ULONG WINAPI DirectXVideoProcessorService_Release( IDirectXVideoProcessorService *iface )
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoProcessorService(iface);

    return DirectXVideoAccelerationService_Release(&This->IDirectXVideoAccelerationService_iface);
}

static HRESULT WINAPI DirectXVideoProcessorService_CreateSurface( IDirectXVideoProcessorService *iface, UINT width, UINT height,
        UINT backBuffers, D3DFORMAT format, D3DPOOL pool, DWORD usage, DWORD dxvaType, IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle )
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoProcessorService(iface);

    FIXME("(%p/%p)->(%u, %u, %u, %#x, 0x%x, 0x%x, 0x%x, %p, %p): stub\n",
        iface, This, width, height, backBuffers, format, pool, usage, dxvaType, ppSurface, pSharedHandle);

    return DirectXVideoAccelerationService_CreateSurface(&This->IDirectXVideoAccelerationService_iface,
        width, height, backBuffers, format, pool, usage, dxvaType, ppSurface, pSharedHandle);
}

static HRESULT WINAPI DirectXVideoProcessorService_RegisterVideoProcessorSoftwareDevice( IDirectXVideoProcessorService *iface, void *pCallbacks)
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoProcessorService(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, pCallbacks);

    return E_NOTIMPL;
}

static HRESULT WINAPI DirectXVideoProcessorService_GetVideoProcessorDeviceGuids( IDirectXVideoProcessorService *iface,
                                                                                 const DXVA2_VideoDesc *pVideoDesc, UINT *pCount, GUID **pGuids )
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoProcessorService(iface);
    GUID *guids;

    FIXME("(%p/%p)->(%p, %p, %p): stub\n", iface, This, pVideoDesc, pCount, pGuids);

    guids = CoTaskMemAlloc(sizeof(GUID));
    if(!guids)
        return E_OUTOFMEMORY;

    memcpy(guids, &DXVA2_VideoProcSoftwareDevice, sizeof(GUID));

    *pGuids = guids;
    *pCount = 1;

    return S_OK;
}

static HRESULT WINAPI DirectXVideoProcessorService_GetVideoProcessorRenderTargets( IDirectXVideoProcessorService *iface,
    REFGUID VideoProcDeviceGuid, const DXVA2_VideoDesc *pVideoDesc, UINT *pCount, D3DFORMAT **pFormats )
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoProcessorService(iface);

    FIXME("(%p/%p)->(%s, %p, %p, %p): stub\n",
        iface, This, debugstr_guid(VideoProcDeviceGuid), pVideoDesc, pCount, pFormats);

    return E_NOTIMPL;
}

static HRESULT WINAPI DirectXVideoProcessorService_GetVideoProcessorSubStreamFormats( IDirectXVideoProcessorService *iface,
    REFGUID VideoProcDeviceGuid, const DXVA2_VideoDesc *pVideoDesc, D3DFORMAT RenderTargetFormat, UINT *pCount, D3DFORMAT **pFormats )
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoProcessorService(iface);

    FIXME("(%p/%p)->(%s, %p, %#x, %p, %p): stub\n",
        iface, This, debugstr_guid(VideoProcDeviceGuid), pVideoDesc, RenderTargetFormat, pCount, pFormats);

    return E_NOTIMPL;
}

static HRESULT WINAPI DirectXVideoProcessorService_GetVideoProcessorCaps( IDirectXVideoProcessorService *iface, REFGUID VideoProcDeviceGuid,
        const DXVA2_VideoDesc *pVideoDesc, D3DFORMAT RenderTargetFormat, DXVA2_VideoProcessorCaps *pCaps)
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoProcessorService(iface);

    FIXME("(%p/%p)->(%s, %p, %#x, %p): stub\n",
        iface, This, debugstr_guid(VideoProcDeviceGuid), pVideoDesc, RenderTargetFormat, pCaps);

    return E_NOTIMPL;
}

static HRESULT WINAPI DirectXVideoProcessorService_GetProcAmpRange( IDirectXVideoProcessorService *iface, REFGUID VideoProcDeviceGuid,
        const DXVA2_VideoDesc *pVideoDesc, D3DFORMAT RenderTargetFormat, UINT ProcAmpCap, DXVA2_ValueRange *pRange )
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoProcessorService(iface);

    FIXME("(%p/%p)->(%s, %p, %#x, %u, %p): stub\n",
        iface, This, debugstr_guid(VideoProcDeviceGuid), pVideoDesc, RenderTargetFormat, ProcAmpCap, pRange);

    return E_NOTIMPL;
}

static HRESULT WINAPI DirectXVideoProcessorService_GetFilterPropertyRange( IDirectXVideoProcessorService *iface, REFGUID VideoProcDeviceGuid,
        const DXVA2_VideoDesc *pVideoDesc, D3DFORMAT RenderTargetFormat, UINT FilterSetting, DXVA2_ValueRange *pRange )
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoProcessorService(iface);

    FIXME("(%p/%p)->(%s, %p, %#x, %u, %p): stub\n",
        iface, This, debugstr_guid(VideoProcDeviceGuid), pVideoDesc, RenderTargetFormat, FilterSetting, pRange);

    return E_NOTIMPL;
}

static HRESULT WINAPI DirectXVideoProcessorService_CreateVideoProcessor( IDirectXVideoProcessorService *iface, REFGUID VideoProcDeviceGuid,
        const DXVA2_VideoDesc *pVideoDesc, D3DFORMAT RenderTargetFormat, UINT MaxNumSubStreams, IDirectXVideoProcessor **ppVidProcess )
{
    DirectXVideoAccelerationServiceImpl *This = impl_from_IDirectXVideoProcessorService(iface);

    FIXME("(%p/%p)->(%s, %#x, %u, %p): semi-stub\n",
        iface, This, debugstr_guid(VideoProcDeviceGuid), RenderTargetFormat, MaxNumSubStreams, ppVidProcess);

    if (IsEqualIID(VideoProcDeviceGuid, &DXVA2_VideoProcSoftwareDevice))
        return processor_software_create(iface, This->device, pVideoDesc, RenderTargetFormat, MaxNumSubStreams, ppVidProcess);

    return E_NOTIMPL;
}

static const IDirectXVideoProcessorServiceVtbl DirectXVideoProcessorService_VTable =
{
    DirectXVideoProcessorService_QueryInterface,
    DirectXVideoProcessorService_AddRef,
    DirectXVideoProcessorService_Release,
    DirectXVideoProcessorService_CreateSurface,
    DirectXVideoProcessorService_RegisterVideoProcessorSoftwareDevice,
    DirectXVideoProcessorService_GetVideoProcessorDeviceGuids,
    DirectXVideoProcessorService_GetVideoProcessorRenderTargets,
    DirectXVideoProcessorService_GetVideoProcessorSubStreamFormats,
    DirectXVideoProcessorService_GetVideoProcessorCaps,
    DirectXVideoProcessorService_GetProcAmpRange,
    DirectXVideoProcessorService_GetFilterPropertyRange,
    DirectXVideoProcessorService_CreateVideoProcessor
};

HRESULT videoservice_create( IDirect3DDevice9 *device, REFIID riid, void **ppv )
{
    IWineVideoService *backend;
    DirectXVideoAccelerationServiceImpl *videoservice;

    if (!device || !riid || !ppv)
        return E_INVALIDARG;

    *ppv = NULL;

    backend = vaapi_videoservice_create();
    if (!backend)
        return E_NOINTERFACE;

    videoservice = CoTaskMemAlloc(sizeof(DirectXVideoAccelerationServiceImpl));
    if (!videoservice)
    {
        IWineVideoService_Release(videoservice->backend);
        return E_OUTOFMEMORY;
    }

    videoservice->IDirectXVideoAccelerationService_iface.lpVtbl = &DirectXVideoAccelerationService_VTable;
    videoservice->IDirectXVideoDecoderService_iface.lpVtbl      = &DirectXVideoDecoderService_VTable;
    videoservice->IDirectXVideoProcessorService_iface.lpVtbl    = &DirectXVideoProcessorService_VTable;
    videoservice->refCount = 1;
    videoservice->device = device;
    videoservice->backend = backend;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = &videoservice->IDirectXVideoAccelerationService_iface;
    else if (IsEqualIID(riid, &IID_IDirectXVideoAccelerationService))
        *ppv = &videoservice->IDirectXVideoAccelerationService_iface;
    else if (IsEqualIID(riid, &IID_IDirectXVideoDecoderService))
        *ppv = &videoservice->IDirectXVideoDecoderService_iface;
    else if (IsEqualIID(riid, &IID_IDirectXVideoProcessorService))
        *ppv = &videoservice->IDirectXVideoProcessorService_iface;
    else
    {
        FIXME("Service %s not implemented\n", debugstr_guid(riid));
        IWineVideoService_Release(videoservice->backend);
        CoTaskMemFree(videoservice);
        return E_NOINTERFACE;
    }

    IDirect3DDevice9_AddRef(device);
    return S_OK;
}

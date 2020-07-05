/*
 * Copyright 2015 Michael Müller for Pipelight
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
    IDirectXVideoProcessor IDirectXVideoProcessor_iface;
    LONG refCount;

    IDirectXVideoProcessorService *service;
    IDirect3DDevice9 *device;
} DirectXVideoProcessorImpl;

static inline DirectXVideoProcessorImpl *impl_from_IDirectXVideoProcessor( IDirectXVideoProcessor *iface )
{
    return CONTAINING_RECORD(iface, DirectXVideoProcessorImpl, IDirectXVideoProcessor_iface);
}

static HRESULT WINAPI DirectXVideoProcessor_QueryInterface( IDirectXVideoProcessor *iface, REFIID riid, LPVOID *ppv )
{
    DirectXVideoProcessorImpl *This = impl_from_IDirectXVideoProcessor(iface);
    TRACE("(%p/%p)->(%s, %p)\n", iface, This, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirectXVideoProcessor))
        *ppv = (LPVOID)iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI DirectXVideoProcessor_AddRef( IDirectXVideoProcessor *iface )
{
    DirectXVideoProcessorImpl *This = impl_from_IDirectXVideoProcessor(iface);
    ULONG refCount = InterlockedIncrement(&This->refCount);

    TRACE("(%p)->() AddRef from %d\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI DirectXVideoProcessor_Release( IDirectXVideoProcessor *iface )
{
    DirectXVideoProcessorImpl *This = impl_from_IDirectXVideoProcessor(iface);
    ULONG refCount = InterlockedDecrement(&This->refCount);

    TRACE("(%p)->() Release from %d\n", This, refCount + 1);

    if (!refCount)
    {
        TRACE("Destroying\n");

        IDirectXVideoProcessorService_Release(This->service);
        IDirect3DDevice9_Release(This->device);

        CoTaskMemFree(This);
    }

    return refCount;
}

static HRESULT WINAPI DirectXVideoProcessor_GetVideoProcessorService(IDirectXVideoProcessor *iface,
                                                                     IDirectXVideoProcessorService** ppService)
{
    DirectXVideoProcessorImpl *This = impl_from_IDirectXVideoProcessor(iface);

    FIXME("(%p)->(%p): stub\n", This, ppService);

    if (!ppService)
        return E_INVALIDARG;

    IDirectXVideoProcessorService_AddRef(This->service);
    *ppService = This->service;

    return S_OK;
}

static HRESULT WINAPI DirectXVideoProcessor_GetCreationParameters(IDirectXVideoProcessor *iface,
                                                                  GUID* pDeviceGuid, DXVA2_VideoDesc* pVideoDesc,
                                                                  D3DFORMAT* pRenderTargetFormat,
                                                                  UINT* pMaxNumSubStreams)
{
    DirectXVideoProcessorImpl *This = impl_from_IDirectXVideoProcessor(iface);

    FIXME("(%p)->(%p, %p, %p, %p): stub\n", This, pDeviceGuid, pVideoDesc, pRenderTargetFormat, pMaxNumSubStreams);

    return E_NOTIMPL;
}

static HRESULT WINAPI DirectXVideoProcessor_GetVideoProcessorCaps(IDirectXVideoProcessor *iface,
                                                                  DXVA2_VideoProcessorCaps* pCaps)
{
    DirectXVideoProcessorImpl *This = impl_from_IDirectXVideoProcessor(iface);

    FIXME("(%p)->(%p): stub\n", This, pCaps);

    return E_NOTIMPL;
}

static HRESULT WINAPI DirectXVideoProcessor_GetProcAmpRange(IDirectXVideoProcessor *iface, UINT ProcAmpCap,
                                                            DXVA2_ValueRange* pRange)
{
    DirectXVideoProcessorImpl *This = impl_from_IDirectXVideoProcessor(iface);

    FIXME("(%p)->(%u, %p): stub\n", This, ProcAmpCap, pRange);

    return E_NOTIMPL;
}

static HRESULT WINAPI DirectXVideoProcessor_GetFilterPropertyRange(IDirectXVideoProcessor *iface,
                                                                   UINT FilterSetting, DXVA2_ValueRange* pRange)
{
    DirectXVideoProcessorImpl *This = impl_from_IDirectXVideoProcessor(iface);

    FIXME("(%p)->(%u, %p): stub\n", This, FilterSetting, pRange);

    return E_NOTIMPL;
}

static HRESULT WINAPI DirectXVideoProcessor_VideoProcessBlt(IDirectXVideoProcessor *iface, IDirect3DSurface9* pRenderTarget,
                                                            const DXVA2_VideoProcessBltParams* pBltParams,
                                                            const DXVA2_VideoSample* pSamples, UINT NumSamples,
                                                            HANDLE* complete)
{
    DirectXVideoProcessorImpl *This = impl_from_IDirectXVideoProcessor(iface);

    TRACE("(%p)->(%p, %p, %p, %u, %p)\n", This, pRenderTarget, pBltParams, pSamples, NumSamples, complete);

    if (!pRenderTarget || !pBltParams || !pSamples)
        return E_INVALIDARG;

    if (NumSamples > 1)
        FIXME("Deinterlacing not implemented, expect horrible video output!\n");

    return IDirect3DDevice9_StretchRect(This->device, pSamples[0].SrcSurface, &pSamples[0].SrcRect,
                                        pRenderTarget, &pSamples[0].DstRect, D3DTEXF_LINEAR);
}

static const IDirectXVideoProcessorVtbl DirectXVideoProcessor_VTable =
{
    DirectXVideoProcessor_QueryInterface,
    DirectXVideoProcessor_AddRef,
    DirectXVideoProcessor_Release,
    DirectXVideoProcessor_GetVideoProcessorService,
    DirectXVideoProcessor_GetCreationParameters,
    DirectXVideoProcessor_GetVideoProcessorCaps,
    DirectXVideoProcessor_GetProcAmpRange,
    DirectXVideoProcessor_GetFilterPropertyRange,
    DirectXVideoProcessor_VideoProcessBlt
};

HRESULT processor_software_create( IDirectXVideoProcessorService *processor_service, IDirect3DDevice9 *device,
                                   const DXVA2_VideoDesc *pVideoDesc, D3DFORMAT RenderTargetFormat,
                                   UINT MaxNumSubStreams, IDirectXVideoProcessor **processor )
{
    DirectXVideoProcessorImpl *software_processor;

    if (!processor_service || !pVideoDesc)
        return E_INVALIDARG;

    software_processor = CoTaskMemAlloc(sizeof(*software_processor));
    if (!software_processor)
        return E_OUTOFMEMORY;

    software_processor->IDirectXVideoProcessor_iface.lpVtbl = &DirectXVideoProcessor_VTable;
    software_processor->refCount = 1;
    software_processor->service = processor_service;
    software_processor->device = device;

    IDirectXVideoProcessorService_AddRef(processor_service);
    IDirect3DDevice9_AddRef(device);

    *processor = &software_processor->IDirectXVideoProcessor_iface;
    return S_OK;
}

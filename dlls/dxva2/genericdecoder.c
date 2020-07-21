/*
 * Copyright 2014-2015 Michael Müller for Pipelight
 * Copyright 2014-2015 Sebastian Lackner for Pipelight
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
#include "dxva.h"
#include "dxva2_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxva2);

static void convert_nv12_nv12(WineVideoImage *image, D3DSURFACE_DESC *d3ddesc, D3DLOCKED_RECT *d3drect)
{
    unsigned int y, height, width;
    char *src, *dst;

    height = min(d3ddesc->Height, image->height);
    width  = min(d3ddesc->Width,image->width);

    src = (char *)image->buffer + image->offsets[0];
    dst = d3drect->pBits;

    for (y = 0; y < height; y++)
    {
        memcpy(dst, src, width);
        src += image->pitches[0];
        dst += d3drect->Pitch;
    }

    src = (char *)image->buffer + image->offsets[1];
    dst = (char *)d3drect->pBits + d3drect->Pitch * d3ddesc->Height;

    for (y = 0; y < height / 2; y++)
    {
        memcpy(dst, src, width);
        src += image->pitches[1];
        dst += d3drect->Pitch;
    }
}

struct image_converter
{
    D3DFORMAT src;
    D3DFORMAT dst;
    void (*func)(WineVideoImage *image, D3DSURFACE_DESC *d3ddesc, D3DLOCKED_RECT *d3drect);
};

static struct image_converter image_converters[] =
{
    {MAKEFOURCC('N','V','1','2'), MAKEFOURCC('N','V','1','2'), convert_nv12_nv12}
};

static struct image_converter *get_image_converter(WineVideoImage *imageInfo, D3DSURFACE_DESC *d3ddesc)
{
    unsigned int i;
    for (i = 0; i < sizeof(image_converters) / sizeof(image_converters[0]); i++)
    {
        if (image_converters[i].src == imageInfo->format && image_converters[i].dst == d3ddesc->Format)
            return &image_converters[i];
    }
    return NULL;
}

typedef struct
{
    IDirectXVideoDecoder IDirectXVideoDecoder_iface;

    LONG refCount;
    IDirectXVideoDecoderService *videodecoder;
    IWineVideoDecoder *backend;

    /* video attributes */
    UINT width;
    UINT height;

    /* surfaces used by this decoder */
    UINT surfaceCount;
    IDirect3DSurface9 **surfaces;
    int currentSurface;
} DirectXVideoDecoderGenericImpl;

static inline DirectXVideoDecoderGenericImpl *impl_from_IDirectXVideoDecoderGeneric( IDirectXVideoDecoder *iface )
{
    return CONTAINING_RECORD(iface, DirectXVideoDecoderGenericImpl, IDirectXVideoDecoder_iface);
}

/*****************************************************************************
 * IDirectXVideoDecoderGeneric interface
 */

static HRESULT WINAPI DirectXVideoDecoderGeneric_QueryInterface( IDirectXVideoDecoder *iface, REFIID riid, LPVOID *ppv )
{
    DirectXVideoDecoderGenericImpl *This = impl_from_IDirectXVideoDecoderGeneric(iface);
    TRACE("(%p/%p)->(%s, %p)\n", iface, This, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirectXVideoDecoder))
        *ppv = (LPVOID)iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI DirectXVideoDecoderGeneric_AddRef( IDirectXVideoDecoder *iface )
{
    DirectXVideoDecoderGenericImpl *This = impl_from_IDirectXVideoDecoderGeneric(iface);
    ULONG refCount = InterlockedIncrement(&This->refCount);

    TRACE("(%p)->() AddRef from %d\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI DirectXVideoDecoderGeneric_Release( IDirectXVideoDecoder *iface )
{
    DirectXVideoDecoderGenericImpl *This = impl_from_IDirectXVideoDecoderGeneric(iface);
    ULONG refCount = InterlockedDecrement(&This->refCount);
    unsigned int i;

    TRACE("(%p)->() Release from %d\n", This, refCount + 1);

    if (!refCount)
    {
        TRACE("Destroying\n");

        if (This->currentSurface != -1)
            ERR("decoder destroyed while decoding frame\n");

        for (i = 0; i < This->surfaceCount; i++)
            IDirect3DSurface9_Release(This->surfaces[i]);
        HeapFree(GetProcessHeap(), 0, This->surfaces);

        IDirectXVideoDecoderService_Release(This->videodecoder);
        IWineVideoDecoder_Release(This->backend);
        CoTaskMemFree(This);
    }

    return refCount;
}

static HRESULT WINAPI DirectXVideoDecoderGeneric_GetVideoDecoderService( IDirectXVideoDecoder *iface, IDirectXVideoDecoderService **ppService )
{
    DirectXVideoDecoderGenericImpl *This = impl_from_IDirectXVideoDecoderGeneric(iface);

    TRACE("(%p)->(%p)\n", This, ppService);

    if (!ppService)
        return E_INVALIDARG;

    IDirectXVideoDecoderService_AddRef(This->videodecoder);
    *ppService = This->videodecoder;

    return S_OK;
}

static HRESULT WINAPI DirectXVideoDecoderGeneric_GetCreationParameters( IDirectXVideoDecoder *iface, GUID *pDeviceGuid, DXVA2_VideoDesc *pVideoDesc,
                                                                        DXVA2_ConfigPictureDecode *pConfig, IDirect3DSurface9 ***pDecoderRenderTargets,
                                                                        UINT *pNumSurfaces )
{
    DirectXVideoDecoderGenericImpl *This = impl_from_IDirectXVideoDecoderGeneric(iface);

    FIXME("(%p)->(%p, %p, %p, %p, %p)\n", This, pDeviceGuid, pVideoDesc, pConfig, pDecoderRenderTargets, pNumSurfaces);

    return E_NOTIMPL;
}

static HRESULT WINAPI DirectXVideoDecoderGeneric_GetBuffer( IDirectXVideoDecoder *iface, UINT BufferType, void **ppBuffer, UINT *pBufferSize )
{
    DirectXVideoDecoderGenericImpl *This = impl_from_IDirectXVideoDecoderGeneric(iface);

    TRACE("(%p)->(%u, %p, %p)\n", This, BufferType, ppBuffer, pBufferSize);

    if (!ppBuffer || !pBufferSize)
        return E_INVALIDARG;

    return IWineVideoDecoder_LockBuffer(This->backend, BufferType, ppBuffer, pBufferSize);
}

static HRESULT WINAPI DirectXVideoDecoderGeneric_ReleaseBuffer( IDirectXVideoDecoder *iface, UINT BufferType )
{
    DirectXVideoDecoderGenericImpl *This = impl_from_IDirectXVideoDecoderGeneric(iface);

    TRACE("(%p)->(%u)\n", This, BufferType);

    return IWineVideoDecoder_UnlockBuffer(This->backend, BufferType);
}

static HRESULT WINAPI DirectXVideoDecoderGeneric_BeginFrame( IDirectXVideoDecoder *iface, IDirect3DSurface9 *pRenderTarget, void *pvPVPData )
{
    DirectXVideoDecoderGenericImpl *This = impl_from_IDirectXVideoDecoderGeneric(iface);
    unsigned int i;
    HRESULT hr;

    TRACE("(%p)->(%p, %p)\n", This, pRenderTarget, pvPVPData);

    if (!pRenderTarget)
        return E_INVALIDARG;

    if (This->currentSurface >= 0)
    {
        ERR("previous frame was not finished properly, resetting decoder\n");
        IWineVideoDecoder_EndFrame(This->backend);
        This->currentSurface = -1;
    }

    for (i = 0; i < This->surfaceCount; i++)
    {
        if (This->surfaces[i] == pRenderTarget)
            break;
    }
    if (i >= This->surfaceCount)
    {
        ERR("render target %p is not in the list of surfaces\n", pRenderTarget);
        return E_INVALIDARG;
    }

    hr = IWineVideoDecoder_BeginFrame(This->backend, i);
    if (FAILED(hr))
    {
        FIXME("Failed to begin frame!\n");
        return hr;
    }

    /* set the new current frame */
    This->currentSurface = i;
    return S_OK;
}

static HRESULT WINAPI DirectXVideoDecoderGeneric_EndFrame( IDirectXVideoDecoder *iface, HANDLE *pHandleComplete )
{
    DirectXVideoDecoderGenericImpl *This = impl_from_IDirectXVideoDecoderGeneric(iface);
    struct image_converter *converter;
    D3DSURFACE_DESC d3ddesc;
    D3DLOCKED_RECT d3drect;
    WineVideoImage imageInfo;
    int last_surface;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, pHandleComplete);

    last_surface = This->currentSurface;
    This->currentSurface = -1;

    if (last_surface < 0)
        WARN("called without frame context\n");

    hr = IWineVideoDecoder_EndFrame(This->backend);
    if (FAILED(hr))
    {
        FIXME("Failed to end frame!\n");
        return hr;
    }

    hr = IWineVideoDecoder_LockImage(This->backend, &imageInfo);
    if (SUCCEEDED(hr))
    {
        hr = IDirect3DSurface9_GetDesc(This->surfaces[last_surface], &d3ddesc);
        if (SUCCEEDED(hr))
        {
            hr = IDirect3DSurface9_LockRect(This->surfaces[last_surface], &d3drect, NULL, D3DLOCK_DISCARD);
            if (SUCCEEDED(hr))
            {
                converter = get_image_converter(&imageInfo, &d3ddesc);
                if (converter)
                {
                    converter->func(&imageInfo, &d3ddesc, &d3drect);
                }
                else
                {
                    FIXME("could not find any suiteable converter\n");
                    hr = E_FAIL;
                }
                IDirect3DSurface9_UnlockRect(This->surfaces[last_surface]);
            }
        }
        IWineVideoDecoder_UnlockImage(This->backend);
    }

    return hr;
}

static HRESULT WINAPI DirectXVideoDecoderGeneric_Execute( IDirectXVideoDecoder *iface, const DXVA2_DecodeExecuteParams *pExecuteParams )
{
    DirectXVideoDecoderGenericImpl *This = impl_from_IDirectXVideoDecoderGeneric(iface);
    DXVA2_DecodeBufferDesc *pictureParam = NULL, *qMatrix = NULL, *sliceInfo = NULL, *bitStream = NULL;
    unsigned int i;

    TRACE("(%p)->(%p)\n", This, pExecuteParams);

    if (!pExecuteParams || !pExecuteParams->pCompressedBuffers)
        return E_INVALIDARG;

    for (i = 0; i < pExecuteParams->NumCompBuffers; i++)
    {
        switch (pExecuteParams->pCompressedBuffers[i].CompressedBufferType)
        {
            case DXVA2_PictureParametersBufferType:
                if (!pictureParam)
                {
                    pictureParam = &pExecuteParams->pCompressedBuffers[i];
                    continue;
                }
                break;

            case DXVA2_InverseQuantizationMatrixBufferType:
                if (!qMatrix)
                {
                    qMatrix = &pExecuteParams->pCompressedBuffers[i];
                    continue;
                }
                break;

            case DXVA2_SliceControlBufferType:
                if (!sliceInfo)
                {
                    sliceInfo = &pExecuteParams->pCompressedBuffers[i];
                    continue;
                }
                break;

            case DXVA2_BitStreamDateBufferType:
                if (!bitStream)
                {
                    bitStream = &pExecuteParams->pCompressedBuffers[i];
                    continue;
                }
                break;

            default:
                FIXME("ignoring unsupported buffer type 0x%x\n",
                      pExecuteParams->pCompressedBuffers[i].CompressedBufferType);
                continue;
        }

        ERR("buffer type 0x%x specified multiple times\n",
            pExecuteParams->pCompressedBuffers[i].CompressedBufferType);
        return E_FAIL;
    }

    return IWineVideoDecoder_ExecuteBuffers(This->backend, pictureParam, qMatrix, sliceInfo, bitStream);
}

static const IDirectXVideoDecoderVtbl DirectXVideoDecoderGeneric_VTable =
{
    DirectXVideoDecoderGeneric_QueryInterface,
    DirectXVideoDecoderGeneric_AddRef,
    DirectXVideoDecoderGeneric_Release,
    DirectXVideoDecoderGeneric_GetVideoDecoderService,
    DirectXVideoDecoderGeneric_GetCreationParameters,
    DirectXVideoDecoderGeneric_GetBuffer,
    DirectXVideoDecoderGeneric_ReleaseBuffer,
    DirectXVideoDecoderGeneric_BeginFrame,
    DirectXVideoDecoderGeneric_EndFrame,
    DirectXVideoDecoderGeneric_Execute
};

HRESULT genericdecoder_create( IDirectXVideoDecoderService *videodecoder, const DXVA2_VideoDesc *videoDesc,
                               DXVA2_ConfigPictureDecode *config, IDirect3DSurface9 **decoderRenderTargets,
                               UINT numSurfaces, IWineVideoDecoder *backend, IDirectXVideoDecoder **decoder )
{
    DirectXVideoDecoderGenericImpl *genericdecoder;
    unsigned int i;

    if (!videoDesc || !config || !decoderRenderTargets || !decoder || !backend)
        return E_INVALIDARG;

    *decoder = NULL;

    genericdecoder = CoTaskMemAlloc(sizeof(*genericdecoder));
    if (!genericdecoder)
        return E_OUTOFMEMORY;

    genericdecoder->IDirectXVideoDecoder_iface.lpVtbl = &DirectXVideoDecoderGeneric_VTable;
    genericdecoder->refCount             = 1;
    genericdecoder->videodecoder         = videodecoder;
    genericdecoder->backend              = backend;

    genericdecoder->width                = videoDesc->SampleWidth;
    genericdecoder->height               = videoDesc->SampleHeight;

    genericdecoder->surfaceCount         = numSurfaces;
    genericdecoder->surfaces             = NULL;
    genericdecoder->currentSurface       = -1;

    genericdecoder->surfaces = HeapAlloc(GetProcessHeap(), 0, sizeof(IDirect3DSurface9*) * numSurfaces);
    if (!genericdecoder->surfaces)
    {
        CoTaskMemFree(genericdecoder);
        return E_FAIL;
    }

    for (i = 0; i < numSurfaces; i++)
    {
        genericdecoder->surfaces[i] = decoderRenderTargets[i];
        IDirect3DSurface9_AddRef(genericdecoder->surfaces[i]);
    }

    IDirectXVideoDecoderService_AddRef(videodecoder);
    IWineVideoDecoder_AddRef(backend);

    *decoder = &genericdecoder->IDirectXVideoDecoder_iface;
    return S_OK;
}

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
#define COBJMACROS

#include "dsdmo_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsdmo);

struct dmo_distortionfx
{
    IDirectSoundFXDistortion  IDirectSoundFXDistortion_iface;
    IMediaObject        IMediaObject_iface;
    IMediaObjectInPlace IMediaObjectInPlace_iface;
    LONG ref;

    DSFXDistortion params;
};

static inline struct dmo_distortionfx *impl_from_IDirectSoundFXDistortion(IDirectSoundFXDistortion *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_distortionfx, IDirectSoundFXDistortion_iface);
}

static inline struct dmo_distortionfx *impl_from_IMediaObject(IMediaObject *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_distortionfx, IMediaObject_iface);
}

static inline struct dmo_distortionfx *impl_from_IMediaObjectInPlace(IMediaObjectInPlace *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_distortionfx, IMediaObjectInPlace_iface);
}

static HRESULT WINAPI distortionfx_mediaobj_QueryInterface(IMediaObject *iface, REFIID riid, void **obj)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    return IDirectSoundFXDistortion_QueryInterface(&This->IDirectSoundFXDistortion_iface, riid, obj);
}

static ULONG WINAPI distortionfx_mediaobj_AddRef(IMediaObject *iface)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    return IDirectSoundFXDistortion_AddRef(&This->IDirectSoundFXDistortion_iface);
}

static ULONG WINAPI distortionfx_mediaobj_Release(IMediaObject *iface)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    return IDirectSoundFXDistortion_Release(&This->IDirectSoundFXDistortion_iface);
}

static HRESULT WINAPI distortionfx_mediaobj_GetStreamCount(IMediaObject *iface, DWORD *inputs, DWORD *outputs)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %p, %p\n", This, inputs, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI distortionfx_mediaobj_GetInputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI distortionfx_mediaobj_GetOutputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI distortionfx_mediaobj_GetInputType(IMediaObject *iface, DWORD index, DWORD type, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %d, %p\n", This, index, type, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI distortionfx_mediaobj_GetOutputType(IMediaObject *iface, DWORD index, DWORD type, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %d, %p\n", This, index, type, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI distortionfx_mediaobj_SetInputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *pmt, DWORD flags)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %x\n", This, index, pmt, flags);
    return S_OK;
}

static HRESULT WINAPI distortionfx_mediaobj_SetOutputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *pmt, DWORD flags)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %x\n", This, index, pmt, flags);
    return S_OK;
}

static HRESULT WINAPI distortionfx_mediaobj_GetInputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI distortionfx_mediaobj_GetOutputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI distortionfx_mediaobj_GetInputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size, DWORD *ahead, DWORD *alignment)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %p, %p\n", This, index, size, ahead, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI distortionfx_mediaobj_GetOutputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size, DWORD *alignment)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %p\n", This, index, size, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI distortionfx_mediaobj_GetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME *latency)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, latency);
    return E_NOTIMPL;
}

static HRESULT WINAPI distortionfx_mediaobj_SetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME latency)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %s\n", This, index, wine_dbgstr_longlong(latency));
    return E_NOTIMPL;
}

static HRESULT WINAPI distortionfx_mediaobj_Flush(IMediaObject *iface)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI distortionfx_mediaobj_Discontinuity(IMediaObject *iface, DWORD index)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d\n", This, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI distortionfx_mediaobj_AllocateStreamingResources(IMediaObject *iface)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI distortionfx_mediaobj_FreeStreamingResources(IMediaObject *iface)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI distortionfx_mediaobj_GetInputStatus(IMediaObject *iface, DWORD index, DWORD *flags)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI distortionfx_mediaobj_ProcessInput(IMediaObject *iface, DWORD index, IMediaBuffer *buffer,
                DWORD flags, REFERENCE_TIME timestamp, REFERENCE_TIME length)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %x, %s, %s\n", This, index, buffer, flags, wine_dbgstr_longlong(timestamp), wine_dbgstr_longlong(length));
    return E_NOTIMPL;
}

static HRESULT WINAPI distortionfx_mediaobj_ProcessOutput(IMediaObject *iface, DWORD flags, DWORD count,
                DMO_OUTPUT_DATA_BUFFER *buffers, DWORD *status)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %x, %d, %p, %p\n", This, flags, count, buffers, status);
    return E_NOTIMPL;
}

static HRESULT WINAPI distortionfx_mediaobj_Lock(IMediaObject *iface, LONG lock)
{
    struct dmo_distortionfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d\n", This, lock);
    return E_NOTIMPL;
}

static const IMediaObjectVtbl distortionfx_mediaobjectVtbl =
{
    distortionfx_mediaobj_QueryInterface,
    distortionfx_mediaobj_AddRef,
    distortionfx_mediaobj_Release,
    distortionfx_mediaobj_GetStreamCount,
    distortionfx_mediaobj_GetInputStreamInfo,
    distortionfx_mediaobj_GetOutputStreamInfo,
    distortionfx_mediaobj_GetInputType,
    distortionfx_mediaobj_GetOutputType,
    distortionfx_mediaobj_SetInputType,
    distortionfx_mediaobj_SetOutputType,
    distortionfx_mediaobj_GetInputCurrentType,
    distortionfx_mediaobj_GetOutputCurrentType,
    distortionfx_mediaobj_GetInputSizeInfo,
    distortionfx_mediaobj_GetOutputSizeInfo,
    distortionfx_mediaobj_GetInputMaxLatency,
    distortionfx_mediaobj_SetInputMaxLatency,
    distortionfx_mediaobj_Flush,
    distortionfx_mediaobj_Discontinuity,
    distortionfx_mediaobj_AllocateStreamingResources,
    distortionfx_mediaobj_FreeStreamingResources,
    distortionfx_mediaobj_GetInputStatus,
    distortionfx_mediaobj_ProcessInput,
    distortionfx_mediaobj_ProcessOutput,
    distortionfx_mediaobj_Lock
};

static HRESULT WINAPI distortionfx_mediainplace_QueryInterface(IMediaObjectInPlace *iface, REFIID riid, void **obj)
{
    struct dmo_distortionfx *This = impl_from_IMediaObjectInPlace(iface);
    return IDirectSoundFXDistortion_QueryInterface(&This->IDirectSoundFXDistortion_iface, riid, obj);
}

static ULONG WINAPI distortionfx_mediainplace_AddRef(IMediaObjectInPlace *iface)
{
    struct dmo_distortionfx *This = impl_from_IMediaObjectInPlace(iface);
    return IDirectSoundFXDistortion_AddRef(&This->IDirectSoundFXDistortion_iface);
}

static ULONG WINAPI distortionfx_mediainplace_Release(IMediaObjectInPlace *iface)
{
    struct dmo_distortionfx *This = impl_from_IMediaObjectInPlace(iface);
    return IDirectSoundFXDistortion_Release(&This->IDirectSoundFXDistortion_iface);
}

static HRESULT WINAPI distortionfx_mediainplace_Process(IMediaObjectInPlace *iface, ULONG size, BYTE *data, REFERENCE_TIME start, DWORD flags)
{
    struct dmo_distortionfx *This = impl_from_IMediaObjectInPlace(iface);
    static BOOL once = 0;
    if(!once++)
        FIXME("%p, %d, %p, %s, %x\n", This, size, data, wine_dbgstr_longlong(start), flags);
    else
        TRACE("%p, %d, %p, %s, %x\n", This, size, data, wine_dbgstr_longlong(start), flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI distortionfx_mediainplace_Clone(IMediaObjectInPlace *iface, IMediaObjectInPlace **object)
{
    struct dmo_distortionfx *This = impl_from_IMediaObjectInPlace(iface);
    FIXME("%p, %p\n", This, object);
    return E_NOTIMPL;
}

static HRESULT WINAPI distortionfx_mediainplace_GetLatency(IMediaObjectInPlace *iface, REFERENCE_TIME *latency)
{
    struct dmo_distortionfx *This = impl_from_IMediaObjectInPlace(iface);
    FIXME("%p, %p\n", This, latency);
    return E_NOTIMPL;
}

static const IMediaObjectInPlaceVtbl distortionfx_mediainplaceVtbl =
{
    distortionfx_mediainplace_QueryInterface,
    distortionfx_mediainplace_AddRef,
    distortionfx_mediainplace_Release,
    distortionfx_mediainplace_Process,
    distortionfx_mediainplace_Clone,
    distortionfx_mediainplace_GetLatency
};

static HRESULT WINAPI distortionfx_QueryInterface(IDirectSoundFXDistortion *iface, REFIID riid, void **ppv)
{
    struct dmo_distortionfx *This = impl_from_IDirectSoundFXDistortion(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirectSoundFXDistortion))
    {
        *ppv = &This->IDirectSoundFXDistortion_iface;
    }
    else if(IsEqualGUID(riid, &IID_IMediaObject))
    {
        *ppv = &This->IMediaObject_iface;
    }
    else if(IsEqualGUID(riid, &IID_IMediaObjectInPlace))
    {
        *ppv = &This->IMediaObjectInPlace_iface;
    }

    if(!*ppv)
    {
        FIXME("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);

    return S_OK;
}

static ULONG WINAPI distortionfx_AddRef(IDirectSoundFXDistortion *iface)
{
    struct dmo_distortionfx *This = impl_from_IDirectSoundFXDistortion(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI distortionfx_Release(IDirectSoundFXDistortion *iface)
{
    struct dmo_distortionfx *This = impl_from_IDirectSoundFXDistortion(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        heap_free(This);
    }
    return ref;
}

static HRESULT WINAPI distortionfx_SetAllParameters(IDirectSoundFXDistortion *iface, const DSFXDistortion *distortion)
{
    struct dmo_distortionfx *This = impl_from_IDirectSoundFXDistortion(iface);

    TRACE("(%p) %p\n", This, distortion);

    if(!distortion)
        return E_POINTER;

    if( (distortion->fGain < DSFXDISTORTION_GAIN_MIN || distortion->fGain > DSFXDISTORTION_GAIN_MAX) ||
        (distortion->fEdge < DSFXDISTORTION_EDGE_MIN || distortion->fEdge > DSFXDISTORTION_EDGE_MAX) ||
        (distortion->fPostEQCenterFrequency < DSFXDISTORTION_POSTEQCENTERFREQUENCY_MIN || distortion->fPostEQCenterFrequency > DSFXDISTORTION_POSTEQCENTERFREQUENCY_MAX) ||
        (distortion->fPostEQBandwidth < DSFXDISTORTION_POSTEQBANDWIDTH_MIN || distortion->fPostEQBandwidth > DSFXDISTORTION_POSTEQBANDWIDTH_MAX) ||
        (distortion->fPreLowpassCutoff < DSFXDISTORTION_PRELOWPASSCUTOFF_MIN || distortion->fPreLowpassCutoff > DSFXDISTORTION_PRELOWPASSCUTOFF_MAX) )
    {
        return E_INVALIDARG;
    }

    This->params = *distortion;

    return S_OK;
}

static HRESULT WINAPI distortionfx_GetAllParameters(IDirectSoundFXDistortion *iface, DSFXDistortion *distortion)
{
    struct dmo_distortionfx *This = impl_from_IDirectSoundFXDistortion(iface);

    TRACE("(%p) %p\n", This, distortion);

    if(!distortion)
        return E_INVALIDARG;

    *distortion = This->params;

    return S_OK;
}

static const struct IDirectSoundFXDistortionVtbl distortionfxVtbl =
{
    distortionfx_QueryInterface,
    distortionfx_AddRef,
    distortionfx_Release,
    distortionfx_SetAllParameters,
    distortionfx_GetAllParameters
};

HRESULT WINAPI DistortionFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    struct dmo_distortionfx *object;
    HRESULT ret;

    TRACE("(%p, %s, %p)\n", outer, debugstr_guid(riid), ppv);

    *ppv = NULL;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IDirectSoundFXDistortion_iface.lpVtbl = &distortionfxVtbl;
    object->IMediaObject_iface.lpVtbl = &distortionfx_mediaobjectVtbl;
    object->IMediaObjectInPlace_iface.lpVtbl = &distortionfx_mediainplaceVtbl;
    object->ref = 1;

    object->params.fGain                  =  -18.0f;
    object->params.fEdge                  =   15.0f;
    object->params.fPostEQCenterFrequency = 2400.0f;
    object->params.fPostEQBandwidth       = 2400.0f;
    object->params.fPreLowpassCutoff      = 3675.0f;

    ret = distortionfx_QueryInterface(&object->IDirectSoundFXDistortion_iface, riid, ppv);
    distortionfx_Release(&object->IDirectSoundFXDistortion_iface);

    return ret;
}

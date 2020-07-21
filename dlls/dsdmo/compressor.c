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

struct dmo_compressorfx
{
    IDirectSoundFXCompressor IDirectSoundFXCompressor_iface;
    IMediaObject        IMediaObject_iface;
    IMediaObjectInPlace IMediaObjectInPlace_iface;
    LONG ref;

    DSFXCompressor params;
};

static inline struct dmo_compressorfx *impl_from_IDirectSoundFXCompressor(IDirectSoundFXCompressor *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_compressorfx, IDirectSoundFXCompressor_iface);
}

static inline struct dmo_compressorfx *impl_from_IMediaObject(IMediaObject *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_compressorfx, IMediaObject_iface);
}

static inline struct dmo_compressorfx *impl_from_IMediaObjectInPlace(IMediaObjectInPlace *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_compressorfx, IMediaObjectInPlace_iface);
}

static HRESULT WINAPI compressor_mediaobj_QueryInterface(IMediaObject *iface, REFIID riid, void **obj)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    return IDirectSoundFXCompressor_QueryInterface(&This->IDirectSoundFXCompressor_iface, riid, obj);
}

static ULONG WINAPI compressor_mediaobj_AddRef(IMediaObject *iface)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    return IDirectSoundFXCompressor_AddRef(&This->IDirectSoundFXCompressor_iface);
}

static ULONG WINAPI compressor_mediaobj_Release(IMediaObject *iface)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    return IDirectSoundFXCompressor_Release(&This->IDirectSoundFXCompressor_iface);
}

static HRESULT WINAPI compressor_mediaobj_GetStreamCount(IMediaObject *iface, DWORD *inputs, DWORD *outputs)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %p, %p\n", This, inputs, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI compressor_mediaobj_GetInputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI compressor_mediaobj_GetOutputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI compressor_mediaobj_GetInputType(IMediaObject *iface, DWORD index, DWORD type, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %d, %p\n", This, index, type, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI compressor_mediaobj_GetOutputType(IMediaObject *iface, DWORD index, DWORD type, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %d, %p\n", This, index, type, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI compressor_mediaobj_SetInputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *pmt, DWORD flags)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %x\n", This, index, pmt, flags);
    return S_OK;
}

static HRESULT WINAPI compressor_mediaobj_SetOutputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *pmt, DWORD flags)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %x\n", This, index, pmt, flags);
    return S_OK;
}

static HRESULT WINAPI compressor_mediaobj_GetInputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI compressor_mediaobj_GetOutputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI compressor_mediaobj_GetInputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size, DWORD *ahead, DWORD *alignment)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %p, %p\n", This, index, size, ahead, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI compressor_mediaobj_GetOutputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size, DWORD *alignment)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %p\n", This, index, size, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI compressor_mediaobj_GetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME *latency)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, latency);
    return E_NOTIMPL;
}

static HRESULT WINAPI compressor_mediaobj_SetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME latency)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %s\n", This, index,  wine_dbgstr_longlong(latency));
    return E_NOTIMPL;
}

static HRESULT WINAPI compressor_mediaobj_Flush(IMediaObject *iface)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI compressor_mediaobj_Discontinuity(IMediaObject *iface, DWORD index)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d\n", This, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI compressor_mediaobj_AllocateStreamingResources(IMediaObject *iface)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI compressor_mediaobj_FreeStreamingResources(IMediaObject *iface)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI compressor_mediaobj_GetInputStatus(IMediaObject *iface, DWORD index, DWORD *flags)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI compressor_mediaobj_ProcessInput(IMediaObject *iface, DWORD index, IMediaBuffer *buffer,
                DWORD flags, REFERENCE_TIME timestamp, REFERENCE_TIME length)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %x, %s, %s\n", This, index, buffer, flags, wine_dbgstr_longlong(timestamp), wine_dbgstr_longlong(length));
    return E_NOTIMPL;
}

static HRESULT WINAPI compressor_mediaobj_ProcessOutput(IMediaObject *iface, DWORD flags, DWORD count,
                DMO_OUTPUT_DATA_BUFFER *buffers, DWORD *status)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %x, %d, %p, %p\n", This, flags, count, buffers, status);
    return E_NOTIMPL;
}

static HRESULT WINAPI compressor_mediaobj_Lock(IMediaObject *iface, LONG lock)
{
    struct dmo_compressorfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d\n", This, lock);
    return E_NOTIMPL;
}

static const IMediaObjectVtbl compressor_mediaobjectVtbl =
{
    compressor_mediaobj_QueryInterface,
    compressor_mediaobj_AddRef,
    compressor_mediaobj_Release,
    compressor_mediaobj_GetStreamCount,
    compressor_mediaobj_GetInputStreamInfo,
    compressor_mediaobj_GetOutputStreamInfo,
    compressor_mediaobj_GetInputType,
    compressor_mediaobj_GetOutputType,
    compressor_mediaobj_SetInputType,
    compressor_mediaobj_SetOutputType,
    compressor_mediaobj_GetInputCurrentType,
    compressor_mediaobj_GetOutputCurrentType,
    compressor_mediaobj_GetInputSizeInfo,
    compressor_mediaobj_GetOutputSizeInfo,
    compressor_mediaobj_GetInputMaxLatency,
    compressor_mediaobj_SetInputMaxLatency,
    compressor_mediaobj_Flush,
    compressor_mediaobj_Discontinuity,
    compressor_mediaobj_AllocateStreamingResources,
    compressor_mediaobj_FreeStreamingResources,
    compressor_mediaobj_GetInputStatus,
    compressor_mediaobj_ProcessInput,
    compressor_mediaobj_ProcessOutput,
    compressor_mediaobj_Lock
};

static HRESULT WINAPI echo_mediainplace_QueryInterface(IMediaObjectInPlace *iface, REFIID riid, void **obj)
{
    struct dmo_compressorfx *This = impl_from_IMediaObjectInPlace(iface);
    return IDirectSoundFXCompressor_QueryInterface(&This->IDirectSoundFXCompressor_iface, riid, obj);
}

static ULONG WINAPI echo_mediainplace_AddRef(IMediaObjectInPlace *iface)
{
    struct dmo_compressorfx *This = impl_from_IMediaObjectInPlace(iface);
    return IDirectSoundFXCompressor_AddRef(&This->IDirectSoundFXCompressor_iface);
}

static ULONG WINAPI echo_mediainplace_Release(IMediaObjectInPlace *iface)
{
    struct dmo_compressorfx *This = impl_from_IMediaObjectInPlace(iface);
    return IDirectSoundFXCompressor_Release(&This->IDirectSoundFXCompressor_iface);
}

static HRESULT WINAPI echo_mediainplace_Process(IMediaObjectInPlace *iface, ULONG size, BYTE *data, REFERENCE_TIME start, DWORD flags)
{
    struct dmo_compressorfx *This = impl_from_IMediaObjectInPlace(iface);
    static BOOL once = 0;
    if(!once++)
        FIXME("%p, %d, %p, %s, %x\n", This, size, data, wine_dbgstr_longlong(start), flags);
    else
        TRACE("%p, %d, %p, %s, %x\n", This, size, data, wine_dbgstr_longlong(start), flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI echo_mediainplace_Clone(IMediaObjectInPlace *iface, IMediaObjectInPlace **object)
{
    struct dmo_compressorfx *This = impl_from_IMediaObjectInPlace(iface);
    FIXME("%p, %p\n", This, object);
    return E_NOTIMPL;
}

static HRESULT WINAPI echo_mediainplace_GetLatency(IMediaObjectInPlace *iface, REFERENCE_TIME *latency)
{
    struct dmo_compressorfx *This = impl_from_IMediaObjectInPlace(iface);
    FIXME("%p, %p\n", This, latency);
    return E_NOTIMPL;
}

static const IMediaObjectInPlaceVtbl compressor_mediainplaceVtbl =
{
    echo_mediainplace_QueryInterface,
    echo_mediainplace_AddRef,
    echo_mediainplace_Release,
    echo_mediainplace_Process,
    echo_mediainplace_Clone,
    echo_mediainplace_GetLatency
};

static HRESULT WINAPI compressorfx_QueryInterface(IDirectSoundFXCompressor *iface, REFIID riid, void **ppv)
{
    struct dmo_compressorfx *This = impl_from_IDirectSoundFXCompressor(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirectSoundFXCompressor))
    {
        *ppv = &This->IDirectSoundFXCompressor_iface;
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

static ULONG WINAPI compressorfx_AddRef(IDirectSoundFXCompressor *iface)
{
    struct dmo_compressorfx *This = impl_from_IDirectSoundFXCompressor(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI compressorfx_Release(IDirectSoundFXCompressor *iface)
{
    struct dmo_compressorfx *This = impl_from_IDirectSoundFXCompressor(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        heap_free(This);
    }
    return ref;
}

static HRESULT WINAPI compressorfx_SetAllParameters(IDirectSoundFXCompressor *iface, const DSFXCompressor *compressor)
{
    struct dmo_compressorfx *This = impl_from_IDirectSoundFXCompressor(iface);

    TRACE("(%p) %p\n", This, compressor);

    if(!compressor)
        return E_POINTER;

    if( (compressor->fGain < DSFXCOMPRESSOR_GAIN_MIN || compressor->fGain > DSFXCOMPRESSOR_GAIN_MAX) ||
        (compressor->fAttack < DSFXCOMPRESSOR_ATTACK_MIN || compressor->fAttack > DSFXCOMPRESSOR_ATTACK_MAX) ||
        (compressor->fThreshold < DSFXCOMPRESSOR_THRESHOLD_MIN || compressor->fThreshold > DSFXCOMPRESSOR_THRESHOLD_MAX) ||
        (compressor->fRatio < DSFXCOMPRESSOR_RATIO_MIN || compressor->fRatio > DSFXCOMPRESSOR_RATIO_MAX) ||
        (compressor->fPredelay < DSFXCOMPRESSOR_PREDELAY_MIN || compressor->fPredelay > DSFXCOMPRESSOR_PREDELAY_MAX))
    {
        return E_INVALIDARG;
    }

    This->params = *compressor;

    return S_OK;
}

static HRESULT WINAPI compressorfx_GetAllParameters(IDirectSoundFXCompressor *iface, DSFXCompressor *compressor)
{
    struct dmo_compressorfx *This = impl_from_IDirectSoundFXCompressor(iface);

    TRACE("(%p) %p\n", This, compressor);

    if(!compressor)
        return E_INVALIDARG;

    *compressor = This->params;

    return S_OK;
}

static const struct IDirectSoundFXCompressorVtbl echofxVtbl =
{
    compressorfx_QueryInterface,
    compressorfx_AddRef,
    compressorfx_Release,
    compressorfx_SetAllParameters,
    compressorfx_GetAllParameters
};

HRESULT WINAPI CompressorFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    struct dmo_compressorfx *object;
    HRESULT ret;

    TRACE("(%p, %s, %p)\n", outer, debugstr_guid(riid), ppv);

    *ppv = NULL;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IDirectSoundFXCompressor_iface.lpVtbl = &echofxVtbl;
    object->IMediaObject_iface.lpVtbl = &compressor_mediaobjectVtbl;
    object->IMediaObjectInPlace_iface.lpVtbl = &compressor_mediainplaceVtbl;
    object->ref = 1;

    object->params.fGain      =   0.0f;
    object->params.fAttack    =  10.0f;
    object->params.fThreshold = -20.0f;
    object->params.fRatio     =   3.0f;
    object->params.fPredelay  =   4.0f;

    ret = compressorfx_QueryInterface(&object->IDirectSoundFXCompressor_iface, riid, ppv);
    compressorfx_Release(&object->IDirectSoundFXCompressor_iface);

    return ret;
}

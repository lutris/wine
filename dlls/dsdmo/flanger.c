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

struct dmo_flangerfx
{
    IDirectSoundFXFlanger  IDirectSoundFXFlanger_iface;
    IMediaObject        IMediaObject_iface;
    IMediaObjectInPlace IMediaObjectInPlace_iface;
    LONG ref;

    DSFXFlanger params;
};

static inline struct dmo_flangerfx *impl_from_IDirectSoundFXFlanger(IDirectSoundFXFlanger *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_flangerfx, IDirectSoundFXFlanger_iface);
}

static inline struct dmo_flangerfx *impl_from_IMediaObject(IMediaObject *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_flangerfx, IMediaObject_iface);
}

static inline struct dmo_flangerfx *impl_from_IMediaObjectInPlace(IMediaObjectInPlace *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_flangerfx, IMediaObjectInPlace_iface);
}

static HRESULT WINAPI flanger_mediaobj_QueryInterface(IMediaObject *iface, REFIID riid, void **obj)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    return IDirectSoundFXFlanger_QueryInterface(&This->IDirectSoundFXFlanger_iface, riid, obj);
}

static ULONG WINAPI flanger_mediaobj_AddRef(IMediaObject *iface)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    return IDirectSoundFXFlanger_AddRef(&This->IDirectSoundFXFlanger_iface);
}

static ULONG WINAPI flanger_mediaobj_Release(IMediaObject *iface)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    return IDirectSoundFXFlanger_Release(&This->IDirectSoundFXFlanger_iface);
}

static HRESULT WINAPI flanger_mediaobj_GetStreamCount(IMediaObject *iface, DWORD *inputs, DWORD *outputs)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %p, %p\n", This, inputs, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI flanger_mediaobj_GetInputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI flanger_mediaobj_GetOutputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI flanger_mediaobj_GetInputType(IMediaObject *iface, DWORD index, DWORD type, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %d, %p\n", This, index, type, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI flanger_mediaobj_GetOutputType(IMediaObject *iface, DWORD index, DWORD type, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %d, %p\n", This, index, type, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI flanger_mediaobj_SetInputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *pmt, DWORD flags)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %x\n", This, index, pmt, flags);
    return S_OK;
}

static HRESULT WINAPI flanger_mediaobj_SetOutputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *pmt, DWORD flags)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %x\n", This, index, pmt, flags);
    return S_OK;
}

static HRESULT WINAPI flanger_mediaobj_GetInputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI flanger_mediaobj_GetOutputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI flanger_mediaobj_GetInputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size, DWORD *ahead, DWORD *alignment)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %p, %p\n", This, index, size, ahead, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI flanger_mediaobj_GetOutputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size, DWORD *alignment)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %p\n", This, index, size, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI flanger_mediaobj_GetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME *latency)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, latency);
    return E_NOTIMPL;
}

static HRESULT WINAPI flanger_mediaobj_SetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME latency)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %s\n", This, index, wine_dbgstr_longlong(latency));
    return E_NOTIMPL;
}

static HRESULT WINAPI flanger_mediaobj_Flush(IMediaObject *iface)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI flanger_mediaobj_Discontinuity(IMediaObject *iface, DWORD index)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d\n", This, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI flanger_mediaobj_AllocateStreamingResources(IMediaObject *iface)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI flanger_mediaobj_FreeStreamingResources(IMediaObject *iface)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI flanger_mediaobj_GetInputStatus(IMediaObject *iface, DWORD index, DWORD *flags)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI flanger_mediaobj_ProcessInput(IMediaObject *iface, DWORD index, IMediaBuffer *buffer,
                DWORD flags, REFERENCE_TIME timestamp, REFERENCE_TIME length)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %x, %s, %s\n", This, index, buffer, flags, wine_dbgstr_longlong(timestamp), wine_dbgstr_longlong(length));
    return E_NOTIMPL;
}

static HRESULT WINAPI flanger_mediaobj_ProcessOutput(IMediaObject *iface, DWORD flags, DWORD count,
                DMO_OUTPUT_DATA_BUFFER *buffers, DWORD *status)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %x, %d, %p, %p\n", This, flags, count, buffers, status);
    return E_NOTIMPL;
}

static HRESULT WINAPI flanger_mediaobj_Lock(IMediaObject *iface, LONG lock)
{
    struct dmo_flangerfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d\n", This, lock);
    return E_NOTIMPL;
}

static const IMediaObjectVtbl flanger_mediaobjectVtbl =
{
    flanger_mediaobj_QueryInterface,
    flanger_mediaobj_AddRef,
    flanger_mediaobj_Release,
    flanger_mediaobj_GetStreamCount,
    flanger_mediaobj_GetInputStreamInfo,
    flanger_mediaobj_GetOutputStreamInfo,
    flanger_mediaobj_GetInputType,
    flanger_mediaobj_GetOutputType,
    flanger_mediaobj_SetInputType,
    flanger_mediaobj_SetOutputType,
    flanger_mediaobj_GetInputCurrentType,
    flanger_mediaobj_GetOutputCurrentType,
    flanger_mediaobj_GetInputSizeInfo,
    flanger_mediaobj_GetOutputSizeInfo,
    flanger_mediaobj_GetInputMaxLatency,
    flanger_mediaobj_SetInputMaxLatency,
    flanger_mediaobj_Flush,
    flanger_mediaobj_Discontinuity,
    flanger_mediaobj_AllocateStreamingResources,
    flanger_mediaobj_FreeStreamingResources,
    flanger_mediaobj_GetInputStatus,
    flanger_mediaobj_ProcessInput,
    flanger_mediaobj_ProcessOutput,
    flanger_mediaobj_Lock
};

static HRESULT WINAPI flanger_mediainplace_QueryInterface(IMediaObjectInPlace *iface, REFIID riid, void **obj)
{
    struct dmo_flangerfx *This = impl_from_IMediaObjectInPlace(iface);
    return IDirectSoundFXFlanger_QueryInterface(&This->IDirectSoundFXFlanger_iface, riid, obj);
}

static ULONG WINAPI flanger_mediainplace_AddRef(IMediaObjectInPlace *iface)
{
    struct dmo_flangerfx *This = impl_from_IMediaObjectInPlace(iface);
    return IDirectSoundFXFlanger_AddRef(&This->IDirectSoundFXFlanger_iface);
}

static ULONG WINAPI flanger_mediainplace_Release(IMediaObjectInPlace *iface)
{
    struct dmo_flangerfx *This = impl_from_IMediaObjectInPlace(iface);
    return IDirectSoundFXFlanger_Release(&This->IDirectSoundFXFlanger_iface);
}

static HRESULT WINAPI flanger_mediainplace_Process(IMediaObjectInPlace *iface, ULONG size, BYTE *data, REFERENCE_TIME start, DWORD flags)
{
    struct dmo_flangerfx *This = impl_from_IMediaObjectInPlace(iface);
    static BOOL once = 0;
    if(!once++)
        FIXME("%p, %d, %p, %s, %x\n", This, size, data, wine_dbgstr_longlong(start), flags);
    else
        TRACE("%p, %d, %p, %s, %x\n", This, size, data, wine_dbgstr_longlong(start), flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI flanger_mediainplace_Clone(IMediaObjectInPlace *iface, IMediaObjectInPlace **object)
{
    struct dmo_flangerfx *This = impl_from_IMediaObjectInPlace(iface);
    FIXME("%p, %p\n", This, object);
    return E_NOTIMPL;
}

static HRESULT WINAPI flanger_mediainplace_GetLatency(IMediaObjectInPlace *iface, REFERENCE_TIME *latency)
{
    struct dmo_flangerfx *This = impl_from_IMediaObjectInPlace(iface);
    FIXME("%p, %p\n", This, latency);
    return E_NOTIMPL;
}

static const IMediaObjectInPlaceVtbl flanger_mediainplaceVtbl =
{
    flanger_mediainplace_QueryInterface,
    flanger_mediainplace_AddRef,
    flanger_mediainplace_Release,
    flanger_mediainplace_Process,
    flanger_mediainplace_Clone,
    flanger_mediainplace_GetLatency
};

static HRESULT WINAPI flangerfx_QueryInterface(IDirectSoundFXFlanger *iface, REFIID riid, void **ppv)
{
    struct dmo_flangerfx *This = impl_from_IDirectSoundFXFlanger(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirectSoundFXFlanger))
    {
        *ppv = &This->IDirectSoundFXFlanger_iface;
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

static ULONG WINAPI flangerfx_AddRef(IDirectSoundFXFlanger *iface)
{
    struct dmo_flangerfx *This = impl_from_IDirectSoundFXFlanger(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI flangerfx_Release(IDirectSoundFXFlanger *iface)
{
    struct dmo_flangerfx *This = impl_from_IDirectSoundFXFlanger(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        heap_free(This);
    }
    return ref;
}

static HRESULT WINAPI flangerfx_SetAllParameters(IDirectSoundFXFlanger *iface, const DSFXFlanger *flanger)
{
    struct dmo_flangerfx *This = impl_from_IDirectSoundFXFlanger(iface);

    TRACE("(%p) %p\n", This, flanger);

    if(!flanger)
        return E_POINTER;

    if( (flanger->fWetDryMix < DSFXECHO_WETDRYMIX_MIN || flanger->fWetDryMix > DSFXECHO_WETDRYMIX_MAX) ||
        (flanger->fDepth < DSFXFLANGER_DEPTH_MIN   || flanger->fDepth > DSFXFLANGER_DEPTH_MAX) ||
        (flanger->fFeedback < DSFXFLANGER_FEEDBACK_MIN   || flanger->fFeedback > DSFXFLANGER_FEEDBACK_MAX) ||
        (flanger->fFrequency < DSFXFLANGER_FREQUENCY_MIN || flanger->fFrequency > DSFXFLANGER_FREQUENCY_MAX) ||
        (flanger->lWaveform != DSFXFLANGER_WAVE_SIN && flanger->lWaveform != DSFXFLANGER_WAVE_TRIANGLE) ||
        (flanger->fDelay < DSFXFLANGER_DELAY_MIN || flanger->fDelay > DSFXFLANGER_DELAY_MAX) ||
        (flanger->lPhase < DSFXFLANGER_PHASE_MIN || flanger->lPhase > DSFXFLANGER_PHASE_MAX) )
    {
        return E_INVALIDARG;
    }

    This->params = *flanger;

    return S_OK;
}

static HRESULT WINAPI flangerfx_GetAllParameters(IDirectSoundFXFlanger *iface, DSFXFlanger *flanger)
{
    struct dmo_flangerfx *This = impl_from_IDirectSoundFXFlanger(iface);

    TRACE("(%p) %p\n", This, flanger);

    if(!flanger)
        return E_INVALIDARG;

    *flanger = This->params;

    return S_OK;
}

static const struct IDirectSoundFXFlangerVtbl flangerfxVtbl =
{
    flangerfx_QueryInterface,
    flangerfx_AddRef,
    flangerfx_Release,
    flangerfx_SetAllParameters,
    flangerfx_GetAllParameters
};

HRESULT WINAPI FlangerFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    struct dmo_flangerfx *object;
    HRESULT ret;

    TRACE("(%p, %s, %p)\n", outer, debugstr_guid(riid), ppv);

    *ppv = NULL;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IDirectSoundFXFlanger_iface.lpVtbl = &flangerfxVtbl;
    object->IMediaObject_iface.lpVtbl = &flanger_mediaobjectVtbl;
    object->IMediaObjectInPlace_iface.lpVtbl = &flanger_mediainplaceVtbl;
    object->ref = 1;

    object->params.fWetDryMix =  50.0f;
    object->params.fDepth     = 100.0f;
    object->params.fFeedback  = -50.0f;
    object->params.fFrequency =   0.25f;
    object->params.lWaveform = DSFXFLANGER_WAVE_SIN;
    object->params.fDelay     =   2.0f;
    object->params.lPhase     =   2;

    ret = flangerfx_QueryInterface(&object->IDirectSoundFXFlanger_iface, riid, ppv);
    flangerfx_Release(&object->IDirectSoundFXFlanger_iface);

    return ret;
}

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

struct dmo_chorusfx
{
    IDirectSoundFXChorus IDirectSoundFXChorus_iface;
    IMediaObject         IMediaObject_iface;
    IMediaObjectInPlace  IMediaObjectInPlace_iface;
    LONG ref;

    DSFXChorus params;
};

static inline struct dmo_chorusfx *impl_from_IDirectSoundFXChorus(IDirectSoundFXChorus *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_chorusfx, IDirectSoundFXChorus_iface);
}

static inline struct dmo_chorusfx *impl_from_IMediaObject(IMediaObject *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_chorusfx, IMediaObject_iface);
}

static inline struct dmo_chorusfx *impl_from_IMediaObjectInPlace(IMediaObjectInPlace *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_chorusfx, IMediaObjectInPlace_iface);
}

static HRESULT WINAPI chorus_mediaobj_QueryInterface(IMediaObject *iface, REFIID riid, void **obj)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    return IDirectSoundFXChorus_QueryInterface(&This->IDirectSoundFXChorus_iface, riid, obj);
}

static ULONG WINAPI chorus_mediaobj_AddRef(IMediaObject *iface)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    return IDirectSoundFXChorus_AddRef(&This->IDirectSoundFXChorus_iface);
}

static ULONG WINAPI chorus_mediaobj_Release(IMediaObject *iface)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    return IDirectSoundFXChorus_Release(&This->IDirectSoundFXChorus_iface);
}

static HRESULT WINAPI chorus_mediaobj_GetStreamCount(IMediaObject *iface, DWORD *inputs, DWORD *outputs)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %p, %p\n", This, inputs, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI chorus_mediaobj_GetInputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI chorus_mediaobj_GetOutputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI chorus_mediaobj_GetInputType(IMediaObject *iface, DWORD index, DWORD type, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %d, %p\n", This, index, type, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI chorus_mediaobj_GetOutputType(IMediaObject *iface, DWORD index, DWORD type, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %d, %p\n", This, index, type, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI chorus_mediaobj_SetInputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *pmt, DWORD flags)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %x\n", This, index, pmt, flags);
    return S_OK;
}

static HRESULT WINAPI chorus_mediaobj_SetOutputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *pmt, DWORD flags)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %x\n", This, index, pmt, flags);
    return S_OK;
}

static HRESULT WINAPI chorus_mediaobj_GetInputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI chorus_mediaobj_GetOutputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI chorus_mediaobj_GetInputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size, DWORD *ahead, DWORD *alignment)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %p, %p\n", This, index, size, ahead, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI chorus_mediaobj_GetOutputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size, DWORD *alignment)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %p\n", This, index, size, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI chorus_mediaobj_GetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME *latency)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, latency);
    return E_NOTIMPL;
}

static HRESULT WINAPI chorus_mediaobj_SetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME latency)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %s\n", This, index, wine_dbgstr_longlong(latency));
    return E_NOTIMPL;
}

static HRESULT WINAPI chorus_mediaobj_Flush(IMediaObject *iface)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI chorus_mediaobj_Discontinuity(IMediaObject *iface, DWORD index)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d\n", This, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI chorus_mediaobj_AllocateStreamingResources(IMediaObject *iface)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI chorus_mediaobj_FreeStreamingResources(IMediaObject *iface)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI chorus_mediaobj_GetInputStatus(IMediaObject *iface, DWORD index, DWORD *flags)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI chorus_mediaobj_ProcessInput(IMediaObject *iface, DWORD index, IMediaBuffer *buffer,
                DWORD flags, REFERENCE_TIME timestamp, REFERENCE_TIME length)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %x, %s, %s\n", This, index, buffer, flags, wine_dbgstr_longlong(timestamp), wine_dbgstr_longlong(length));
    return E_NOTIMPL;
}

static HRESULT WINAPI chorus_mediaobj_ProcessOutput(IMediaObject *iface, DWORD flags, DWORD count,
                DMO_OUTPUT_DATA_BUFFER *buffers, DWORD *status)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %x, %d, %p, %p\n", This, flags, count, buffers, status);
    return E_NOTIMPL;
}

static HRESULT WINAPI chorus_mediaobj_Lock(IMediaObject *iface, LONG lock)
{
    struct dmo_chorusfx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d\n", This, lock);
    return E_NOTIMPL;
}

static const IMediaObjectVtbl echo_mediaobjectVtbl =
{
    chorus_mediaobj_QueryInterface,
    chorus_mediaobj_AddRef,
    chorus_mediaobj_Release,
    chorus_mediaobj_GetStreamCount,
    chorus_mediaobj_GetInputStreamInfo,
    chorus_mediaobj_GetOutputStreamInfo,
    chorus_mediaobj_GetInputType,
    chorus_mediaobj_GetOutputType,
    chorus_mediaobj_SetInputType,
    chorus_mediaobj_SetOutputType,
    chorus_mediaobj_GetInputCurrentType,
    chorus_mediaobj_GetOutputCurrentType,
    chorus_mediaobj_GetInputSizeInfo,
    chorus_mediaobj_GetOutputSizeInfo,
    chorus_mediaobj_GetInputMaxLatency,
    chorus_mediaobj_SetInputMaxLatency,
    chorus_mediaobj_Flush,
    chorus_mediaobj_Discontinuity,
    chorus_mediaobj_AllocateStreamingResources,
    chorus_mediaobj_FreeStreamingResources,
    chorus_mediaobj_GetInputStatus,
    chorus_mediaobj_ProcessInput,
    chorus_mediaobj_ProcessOutput,
    chorus_mediaobj_Lock
};

static HRESULT WINAPI chorus_mediainplace_QueryInterface(IMediaObjectInPlace *iface, REFIID riid, void **obj)
{
    struct dmo_chorusfx *This = impl_from_IMediaObjectInPlace(iface);
    return IDirectSoundFXChorus_QueryInterface(&This->IDirectSoundFXChorus_iface, riid, obj);
}

static ULONG WINAPI chorus_mediainplace_AddRef(IMediaObjectInPlace *iface)
{
    struct dmo_chorusfx *This = impl_from_IMediaObjectInPlace(iface);
    return IDirectSoundFXChorus_AddRef(&This->IDirectSoundFXChorus_iface);
}

static ULONG WINAPI chorus_mediainplace_Release(IMediaObjectInPlace *iface)
{
    struct dmo_chorusfx *This = impl_from_IMediaObjectInPlace(iface);
    return IDirectSoundFXChorus_Release(&This->IDirectSoundFXChorus_iface);
}

static HRESULT WINAPI chorus_mediainplace_Process(IMediaObjectInPlace *iface, ULONG size, BYTE *data, REFERENCE_TIME start, DWORD flags)
{
    struct dmo_chorusfx *This = impl_from_IMediaObjectInPlace(iface);
    static BOOL once = 0;
    if(!once++)
        FIXME("%p, %d, %p, %s, %x\n", This, size, data, wine_dbgstr_longlong(start), flags);
    else
        TRACE("%p, %d, %p, %s, %x\n", This, size, data, wine_dbgstr_longlong(start), flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI chorus_mediainplace_Clone(IMediaObjectInPlace *iface, IMediaObjectInPlace **object)
{
    struct dmo_chorusfx *This = impl_from_IMediaObjectInPlace(iface);
    FIXME("%p, %p\n", This, object);
    return E_NOTIMPL;
}

static HRESULT WINAPI chorus_mediainplace_GetLatency(IMediaObjectInPlace *iface, REFERENCE_TIME *latency)
{
    struct dmo_chorusfx *This = impl_from_IMediaObjectInPlace(iface);
    FIXME("%p, %p\n", This, latency);
    return E_NOTIMPL;
}

static const IMediaObjectInPlaceVtbl echo_mediainplaceVtbl =
{
    chorus_mediainplace_QueryInterface,
    chorus_mediainplace_AddRef,
    chorus_mediainplace_Release,
    chorus_mediainplace_Process,
    chorus_mediainplace_Clone,
    chorus_mediainplace_GetLatency
};

static HRESULT WINAPI chrousfx_QueryInterface(IDirectSoundFXChorus *iface, REFIID riid, void **ppv)
{
    struct dmo_chorusfx *This = impl_from_IDirectSoundFXChorus(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirectSoundFXChorus))
    {
        *ppv = &This->IDirectSoundFXChorus_iface;
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

static ULONG WINAPI chrousfx_AddRef(IDirectSoundFXChorus *iface)
{
    struct dmo_chorusfx *This = impl_from_IDirectSoundFXChorus(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI chrousfx_Release(IDirectSoundFXChorus *iface)
{
    struct dmo_chorusfx *This = impl_from_IDirectSoundFXChorus(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        heap_free(This);
    }
    return ref;
}

static HRESULT WINAPI chrousfx_SetAllParameters(IDirectSoundFXChorus *iface, const DSFXChorus *chorus)
{
    struct dmo_chorusfx *This = impl_from_IDirectSoundFXChorus(iface);

    TRACE("(%p) %p\n", This, chorus);

    if(!chorus)
        return E_POINTER;

    /* Out of Range values */
    if( (chorus->fWetDryMix < DSFXCHORUS_WETDRYMIX_MIN || chorus->fWetDryMix > DSFXCHORUS_WETDRYMIX_MAX) ||
        (chorus->fDepth < DSFXCHORUS_DEPTH_MIN || chorus->fDepth > DSFXCHORUS_DEPTH_MAX) ||
        (chorus->fFeedback < DSFXCHORUS_FEEDBACK_MIN || chorus->fFeedback >  DSFXCHORUS_FEEDBACK_MAX) ||
        (chorus->fFrequency < DSFXCHORUS_FREQUENCY_MIN || chorus->fFrequency > DSFXCHORUS_FREQUENCY_MAX) ||
        (chorus->lWaveform != DSFXCHORUS_WAVE_SIN && chorus->lWaveform != DSFXCHORUS_WAVE_TRIANGLE ) ||
        (chorus->fDelay < DSFXCHORUS_DELAY_MIN || chorus->fDelay > DSFXCHORUS_DELAY_MAX) ||
        (chorus->lPhase < DSFXCHORUS_PHASE_MIN || chorus->lPhase > DSFXCHORUS_PHASE_MAX) )
    {
        return E_INVALIDARG;
    }

    This->params = *chorus;

    return S_OK;
}

static HRESULT WINAPI chrousfx_GetAllParameters(IDirectSoundFXChorus *iface, DSFXChorus *chorus)
{
    struct dmo_chorusfx *This = impl_from_IDirectSoundFXChorus(iface);

    TRACE("(%p) %p\n", This, chorus);

    if(!chorus)
        return E_INVALIDARG;

    *chorus = This->params;

    return S_OK;
}

static const struct IDirectSoundFXChorusVtbl chorusfxVtbl =
{
    chrousfx_QueryInterface,
    chrousfx_AddRef,
    chrousfx_Release,
    chrousfx_SetAllParameters,
    chrousfx_GetAllParameters
};

HRESULT WINAPI ChrousFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    struct dmo_chorusfx *object;
    HRESULT ret;

    TRACE("(%p, %s, %p)\n", outer, debugstr_guid(riid), ppv);

    *ppv = NULL;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IDirectSoundFXChorus_iface.lpVtbl = &chorusfxVtbl;
    object->IMediaObject_iface.lpVtbl = &echo_mediaobjectVtbl;
    object->IMediaObjectInPlace_iface.lpVtbl = &echo_mediainplaceVtbl;
    object->ref = 1;

    object->params.fWetDryMix = 50.0f;
    object->params.fDepth     = 10.0f;
    object->params.fFeedback  = 25.0f;
    object->params.fFrequency =  1.1f;
    object->params.lWaveform  = DSFXCHORUS_WAVE_SIN;
    object->params.fDelay     = 16.0f;
    object->params.lPhase     =  3;

    ret = chrousfx_QueryInterface(&object->IDirectSoundFXChorus_iface, riid, ppv);
    chrousfx_Release(&object->IDirectSoundFXChorus_iface);

    return ret;
}

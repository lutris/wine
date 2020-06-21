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

struct dmo_reverb2fx
{
    IDirectSoundFXI3DL2Reverb IDirectSoundFXI3DL2Reverb_iface;
    IMediaObject        IMediaObject_iface;
    IMediaObjectInPlace IMediaObjectInPlace_iface;
    LONG ref;

    DSFXI3DL2Reverb params;
};

static inline struct dmo_reverb2fx *impl_from_IDirectSoundFXI3DL2Reverb(IDirectSoundFXI3DL2Reverb *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_reverb2fx, IDirectSoundFXI3DL2Reverb_iface);
}

static inline struct dmo_reverb2fx *impl_from_IMediaObject(IMediaObject *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_reverb2fx, IMediaObject_iface);
}

static inline struct dmo_reverb2fx *impl_from_IMediaObjectInPlace(IMediaObjectInPlace *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_reverb2fx, IMediaObjectInPlace_iface);
}

static HRESULT WINAPI reverb2_mediaobj_QueryInterface(IMediaObject *iface, REFIID riid, void **obj)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    return IDirectSoundFXI3DL2Reverb_QueryInterface(&This->IDirectSoundFXI3DL2Reverb_iface, riid, obj);
}

static ULONG WINAPI reverb2_mediaobj_AddRef(IMediaObject *iface)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    return IDirectSoundFXI3DL2Reverb_AddRef(&This->IDirectSoundFXI3DL2Reverb_iface);
}

static ULONG WINAPI reverb2_mediaobj_Release(IMediaObject *iface)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    return IDirectSoundFXI3DL2Reverb_Release(&This->IDirectSoundFXI3DL2Reverb_iface);
}

static HRESULT WINAPI reverb2_mediaobj_GetStreamCount(IMediaObject *iface, DWORD *inputs, DWORD *outputs)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %p, %p\n", This, inputs, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI reverb2_mediaobj_GetInputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI reverb2_mediaobj_GetOutputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI reverb2_mediaobj_GetInputType(IMediaObject *iface, DWORD index, DWORD type, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %d, %p\n", This, index, type, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI reverb2_mediaobj_GetOutputType(IMediaObject *iface, DWORD index, DWORD type, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %d, %p\n", This, index, type, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI reverb2_mediaobj_SetInputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *pmt, DWORD flags)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %x\n", This, index, pmt, flags);
    return S_OK;
}

static HRESULT WINAPI reverb2_mediaobj_SetOutputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *pmt, DWORD flags)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %x\n", This, index, pmt, flags);
    return S_OK;
}

static HRESULT WINAPI reverb2_mediaobj_GetInputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI reverb2_mediaobj_GetOutputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI reverb2_mediaobj_GetInputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size, DWORD *ahead, DWORD *alignment)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %p, %p\n", This, index, size, ahead, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI reverb2_mediaobj_GetOutputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size, DWORD *alignment)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %p\n", This, index, size, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI reverb2_mediaobj_GetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME *latency)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, latency);
    return E_NOTIMPL;
}

static HRESULT WINAPI reverb2_mediaobj_SetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME latency)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %s\n", This, index, wine_dbgstr_longlong(latency));
    return E_NOTIMPL;
}

static HRESULT WINAPI reverb2_mediaobj_Flush(IMediaObject *iface)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI reverb2_mediaobj_Discontinuity(IMediaObject *iface, DWORD index)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d\n", This, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI reverb2_mediaobj_AllocateStreamingResources(IMediaObject *iface)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI reverb2_mediaobj_FreeStreamingResources(IMediaObject *iface)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI reverb2_mediaobj_GetInputStatus(IMediaObject *iface, DWORD index, DWORD *flags)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI reverb2_mediaobj_ProcessInput(IMediaObject *iface, DWORD index, IMediaBuffer *buffer,
                DWORD flags, REFERENCE_TIME timestamp, REFERENCE_TIME length)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %x, %s, %s\n", This, index, buffer, flags, wine_dbgstr_longlong(timestamp), wine_dbgstr_longlong(length));
    return E_NOTIMPL;
}

static HRESULT WINAPI reverb2_mediaobj_ProcessOutput(IMediaObject *iface, DWORD flags, DWORD count,
                DMO_OUTPUT_DATA_BUFFER *buffers, DWORD *status)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %x, %d, %p, %p\n", This, flags, count, buffers, status);
    return E_NOTIMPL;
}

static HRESULT WINAPI reverb2_mediaobj_Lock(IMediaObject *iface, LONG lock)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d\n", This, lock);
    return E_NOTIMPL;
}

static const IMediaObjectVtbl reverb2_mediaobjectVtbl =
{
    reverb2_mediaobj_QueryInterface,
    reverb2_mediaobj_AddRef,
    reverb2_mediaobj_Release,
    reverb2_mediaobj_GetStreamCount,
    reverb2_mediaobj_GetInputStreamInfo,
    reverb2_mediaobj_GetOutputStreamInfo,
    reverb2_mediaobj_GetInputType,
    reverb2_mediaobj_GetOutputType,
    reverb2_mediaobj_SetInputType,
    reverb2_mediaobj_SetOutputType,
    reverb2_mediaobj_GetInputCurrentType,
    reverb2_mediaobj_GetOutputCurrentType,
    reverb2_mediaobj_GetInputSizeInfo,
    reverb2_mediaobj_GetOutputSizeInfo,
    reverb2_mediaobj_GetInputMaxLatency,
    reverb2_mediaobj_SetInputMaxLatency,
    reverb2_mediaobj_Flush,
    reverb2_mediaobj_Discontinuity,
    reverb2_mediaobj_AllocateStreamingResources,
    reverb2_mediaobj_FreeStreamingResources,
    reverb2_mediaobj_GetInputStatus,
    reverb2_mediaobj_ProcessInput,
    reverb2_mediaobj_ProcessOutput,
    reverb2_mediaobj_Lock
};

static HRESULT WINAPI reverb2_mediainplace_QueryInterface(IMediaObjectInPlace *iface, REFIID riid, void **obj)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObjectInPlace(iface);
    return IDirectSoundFXI3DL2Reverb_QueryInterface(&This->IDirectSoundFXI3DL2Reverb_iface, riid, obj);
}

static ULONG WINAPI reverb2_mediainplace_AddRef(IMediaObjectInPlace *iface)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObjectInPlace(iface);
    return IDirectSoundFXI3DL2Reverb_AddRef(&This->IDirectSoundFXI3DL2Reverb_iface);
}

static ULONG WINAPI reverb2_mediainplace_Release(IMediaObjectInPlace *iface)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObjectInPlace(iface);
    return IDirectSoundFXI3DL2Reverb_Release(&This->IDirectSoundFXI3DL2Reverb_iface);
}

static HRESULT WINAPI reverb2_mediainplace_Process(IMediaObjectInPlace *iface, ULONG size, BYTE *data, REFERENCE_TIME start, DWORD flags)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObjectInPlace(iface);
    static BOOL once = 0;
    if(!once++)
        FIXME("%p, %d, %p, %s, %x\n", This, size, data, wine_dbgstr_longlong(start), flags);
    else
        TRACE("%p, %d, %p, %s, %x\n", This, size, data, wine_dbgstr_longlong(start), flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI reverb2_mediainplace_Clone(IMediaObjectInPlace *iface, IMediaObjectInPlace **object)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObjectInPlace(iface);
    FIXME("%p, %p\n", This, object);
    return E_NOTIMPL;
}

static HRESULT WINAPI reverb2_mediainplace_GetLatency(IMediaObjectInPlace *iface, REFERENCE_TIME *latency)
{
    struct dmo_reverb2fx *This = impl_from_IMediaObjectInPlace(iface);
    FIXME("%p, %p\n", This, latency);
    return E_NOTIMPL;
}

static const IMediaObjectInPlaceVtbl reverb2_mediainplaceVtbl =
{
    reverb2_mediainplace_QueryInterface,
    reverb2_mediainplace_AddRef,
    reverb2_mediainplace_Release,
    reverb2_mediainplace_Process,
    reverb2_mediainplace_Clone,
    reverb2_mediainplace_GetLatency
};

static HRESULT WINAPI reverb2_QueryInterface(IDirectSoundFXI3DL2Reverb *iface, REFIID riid, void **ppv)
{
    struct dmo_reverb2fx *This = impl_from_IDirectSoundFXI3DL2Reverb(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirectSoundFXI3DL2Reverb))
    {
        *ppv = &This->IDirectSoundFXI3DL2Reverb_iface;
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

static ULONG WINAPI reverb2_AddRef(IDirectSoundFXI3DL2Reverb *iface)
{
    struct dmo_reverb2fx *This = impl_from_IDirectSoundFXI3DL2Reverb(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI reverb2_Release(IDirectSoundFXI3DL2Reverb *iface)
{
    struct dmo_reverb2fx *This = impl_from_IDirectSoundFXI3DL2Reverb(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        heap_free(This);
    }
    return ref;
}

static HRESULT WINAPI reverb2_SetAllParameters(IDirectSoundFXI3DL2Reverb *iface, const DSFXI3DL2Reverb *reverb)
{
    struct dmo_reverb2fx *This = impl_from_IDirectSoundFXI3DL2Reverb(iface);

    TRACE("(%p) %p\n", This, reverb);

    if(!reverb)
        return E_POINTER;

    if( (reverb->lRoom < DSFX_I3DL2REVERB_ROOM_MIN || reverb->lRoom > DSFX_I3DL2REVERB_ROOM_MAX) ||
        (reverb->flRoomRolloffFactor < DSFX_I3DL2REVERB_ROOMROLLOFFFACTOR_MIN || reverb->flRoomRolloffFactor > DSFX_I3DL2REVERB_ROOMROLLOFFFACTOR_MAX) ||
        (reverb->flDecayTime < DSFX_I3DL2REVERB_DECAYTIME_MIN || reverb->flDecayTime > DSFX_I3DL2REVERB_DECAYTIME_MAX) ||
        (reverb->flDecayHFRatio < DSFX_I3DL2REVERB_DECAYHFRATIO_MIN || reverb->flDecayHFRatio > DSFX_I3DL2REVERB_DECAYHFRATIO_MAX) ||
        (reverb->lReflections < DSFX_I3DL2REVERB_REFLECTIONS_MIN || reverb->lReflections > DSFX_I3DL2REVERB_REFLECTIONS_MAX) ||
        (reverb->lReverb < DSFX_I3DL2REVERB_REVERB_MIN || reverb->lReverb > DSFX_I3DL2REVERB_REVERB_MAX) ||
        (reverb->flReverbDelay < DSFX_I3DL2REVERB_REFLECTIONSDELAY_MIN || reverb->flReverbDelay > DSFX_I3DL2REVERB_REFLECTIONSDELAY_MAX) ||
        (reverb->flDiffusion < DSFX_I3DL2REVERB_DIFFUSION_MIN || reverb->flDiffusion > DSFX_I3DL2REVERB_DIFFUSION_MAX) ||
        (reverb->flDensity < DSFX_I3DL2REVERB_DENSITY_MIN || reverb->flDensity > DSFX_I3DL2REVERB_DENSITY_MAX) ||
        (reverb->flHFReference < DSFX_I3DL2REVERB_HFREFERENCE_MIN || reverb->flHFReference > DSFX_I3DL2REVERB_HFREFERENCE_MAX) )
    {
        return E_INVALIDARG;
    }

    This->params = *reverb;

    return S_OK;
}

static HRESULT WINAPI reverb2_GetAllParameters(IDirectSoundFXI3DL2Reverb *iface, DSFXI3DL2Reverb *reverb)
{
     struct dmo_reverb2fx *This = impl_from_IDirectSoundFXI3DL2Reverb(iface);

    TRACE("(%p) %p\n", This, reverb);

    if(!reverb)
        return E_INVALIDARG;

    *reverb = This->params;

    return S_OK;
}

static HRESULT WINAPI reverb2_SetPreset(IDirectSoundFXI3DL2Reverb *iface, DWORD preset)
{
    struct dmo_reverb2fx *This = impl_from_IDirectSoundFXI3DL2Reverb(iface);
    FIXME("(%p) %d\n", This, preset);

    return E_NOTIMPL;
}

static HRESULT WINAPI reverb2_GetPreset(IDirectSoundFXI3DL2Reverb *iface, DWORD *preset)
{
    struct dmo_reverb2fx *This = impl_from_IDirectSoundFXI3DL2Reverb(iface);
    FIXME("(%p) %p\n", This, preset);

    return E_NOTIMPL;
}

static HRESULT WINAPI reverb2_SetQuality(IDirectSoundFXI3DL2Reverb *iface, LONG quality)
{
    struct dmo_reverb2fx *This = impl_from_IDirectSoundFXI3DL2Reverb(iface);
    FIXME("(%p) %d\n", This, quality);

    return E_NOTIMPL;
}

static HRESULT WINAPI reverb2_GetQuality(IDirectSoundFXI3DL2Reverb *iface, LONG *quality)
{
    struct dmo_reverb2fx *This = impl_from_IDirectSoundFXI3DL2Reverb(iface);
    FIXME("(%p) %p\n", This, quality);

    return E_NOTIMPL;
}

static const struct IDirectSoundFXI3DL2ReverbVtbl reverb2fxVtbl =
{
    reverb2_QueryInterface,
    reverb2_AddRef,
    reverb2_Release,
    reverb2_SetAllParameters,
    reverb2_GetAllParameters,
    reverb2_SetPreset,
    reverb2_GetPreset,
    reverb2_SetQuality,
    reverb2_GetQuality
};

HRESULT WINAPI I3DL2Reverb_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    struct dmo_reverb2fx *object;
    HRESULT ret;

    TRACE("(%p, %s, %p)\n", outer, debugstr_guid(riid), ppv);

    *ppv = NULL;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IDirectSoundFXI3DL2Reverb_iface.lpVtbl = &reverb2fxVtbl;
    object->IMediaObject_iface.lpVtbl = &reverb2_mediaobjectVtbl;
    object->IMediaObjectInPlace_iface.lpVtbl = &reverb2_mediainplaceVtbl;
    object->ref = 1;

    ret = reverb2_QueryInterface(&object->IDirectSoundFXI3DL2Reverb_iface, riid, ppv);
    reverb2_Release(&object->IDirectSoundFXI3DL2Reverb_iface);

    object->params.lRoom          =  DSFX_I3DL2REVERB_ROOM_DEFAULT;
    object->params.flRoomRolloffFactor = DSFX_I3DL2REVERB_ROOMROLLOFFFACTOR_DEFAULT;
    object->params.flDecayTime    =  DSFX_I3DL2REVERB_DECAYTIME_DEFAULT;
    object->params.flDecayHFRatio =  DSFX_I3DL2REVERB_DECAYHFRATIO_DEFAULT;
    object->params.lReflections   =  DSFX_I3DL2REVERB_REFLECTIONS_DEFAULT;
    object->params.lReverb        =  DSFX_I3DL2REVERB_REVERB_DEFAULT;
    object->params.flReverbDelay  =  DSFX_I3DL2REVERB_REVERBDELAY_DEFAULT;
    object->params.flDiffusion    =  DSFX_I3DL2REVERB_DIFFUSION_DEFAULT;
    object->params.flDensity      =  DSFX_I3DL2REVERB_DENSITY_DEFAULT;
    object->params.flHFReference  =  DSFX_I3DL2REVERB_HFREFERENCE_DEFAULT;

    return ret;
}

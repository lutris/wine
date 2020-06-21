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

struct dmo_echofx
{
    IDirectSoundFXEcho  IDirectSoundFXEcho_iface;
    IMediaObject        IMediaObject_iface;
    IMediaObjectInPlace IMediaObjectInPlace_iface;
    LONG ref;

    DSFXEcho params;
};

static inline struct dmo_echofx *impl_from_IDirectSoundFXEcho(IDirectSoundFXEcho *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_echofx, IDirectSoundFXEcho_iface);
}

static inline struct dmo_echofx *impl_from_IMediaObject(IMediaObject *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_echofx, IMediaObject_iface);
}

static inline struct dmo_echofx *impl_from_IMediaObjectInPlace(IMediaObjectInPlace *iface)
{
    return CONTAINING_RECORD(iface, struct dmo_echofx, IMediaObjectInPlace_iface);
}

static HRESULT WINAPI echo_mediaobj_QueryInterface(IMediaObject *iface, REFIID riid, void **obj)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    return IDirectSoundFXEcho_QueryInterface(&This->IDirectSoundFXEcho_iface, riid, obj);
}

static ULONG WINAPI echo_mediaobj_AddRef(IMediaObject *iface)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    return IDirectSoundFXEcho_AddRef(&This->IDirectSoundFXEcho_iface);
}

static ULONG WINAPI echo_mediaobj_Release(IMediaObject *iface)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    return IDirectSoundFXEcho_Release(&This->IDirectSoundFXEcho_iface);
}

static HRESULT WINAPI echo_mediaobj_GetStreamCount(IMediaObject *iface, DWORD *inputs, DWORD *outputs)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %p, %p\n", This, inputs, outputs);
    return E_NOTIMPL;
}

static HRESULT WINAPI echo_mediaobj_GetInputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI echo_mediaobj_GetOutputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI echo_mediaobj_GetInputType(IMediaObject *iface, DWORD index, DWORD type, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %d, %p\n", This, index, type, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI echo_mediaobj_GetOutputType(IMediaObject *iface, DWORD index, DWORD type, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %d, %p\n", This, index, type, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI echo_mediaobj_SetInputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *pmt, DWORD flags)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %x\n", This, index, pmt, flags);
    return S_OK;
}

static HRESULT WINAPI echo_mediaobj_SetOutputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *pmt, DWORD flags)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %x\n", This, index, pmt, flags);
    return S_OK;
}

static HRESULT WINAPI echo_mediaobj_GetInputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI echo_mediaobj_GetOutputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *pmt)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, pmt);
    return E_NOTIMPL;
}

static HRESULT WINAPI echo_mediaobj_GetInputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size, DWORD *ahead, DWORD *alignment)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %p, %p\n", This, index, size, ahead, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI echo_mediaobj_GetOutputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size, DWORD *alignment)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %p\n", This, index, size, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI echo_mediaobj_GetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME *latency)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, latency);
    return E_NOTIMPL;
}

static HRESULT WINAPI echo_mediaobj_SetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME latency)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %s\n", This, index, wine_dbgstr_longlong(latency));
    return E_NOTIMPL;
}

static HRESULT WINAPI echo_mediaobj_Flush(IMediaObject *iface)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI echo_mediaobj_Discontinuity(IMediaObject *iface, DWORD index)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d\n", This, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI echo_mediaobj_AllocateStreamingResources(IMediaObject *iface)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI echo_mediaobj_FreeStreamingResources(IMediaObject *iface)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI echo_mediaobj_GetInputStatus(IMediaObject *iface, DWORD index, DWORD *flags)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p\n", This, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI echo_mediaobj_ProcessInput(IMediaObject *iface, DWORD index, IMediaBuffer *buffer,
                DWORD flags, REFERENCE_TIME timestamp, REFERENCE_TIME length)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d, %p, %x, %s, %s\n", This, index, buffer, flags, wine_dbgstr_longlong(timestamp), wine_dbgstr_longlong(length));
    return E_NOTIMPL;
}

static HRESULT WINAPI echo_mediaobj_ProcessOutput(IMediaObject *iface, DWORD flags, DWORD count,
                DMO_OUTPUT_DATA_BUFFER *buffers, DWORD *status)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %x, %d, %p, %p\n", This, flags, count, buffers, status);
    return E_NOTIMPL;
}

static HRESULT WINAPI echo_mediaobj_Lock(IMediaObject *iface, LONG lock)
{
    struct dmo_echofx *This = impl_from_IMediaObject(iface);
    FIXME("%p, %d\n", This, lock);
    return E_NOTIMPL;
}

static const IMediaObjectVtbl echo_mediaobjectVtbl =
{
    echo_mediaobj_QueryInterface,
    echo_mediaobj_AddRef,
    echo_mediaobj_Release,
    echo_mediaobj_GetStreamCount,
    echo_mediaobj_GetInputStreamInfo,
    echo_mediaobj_GetOutputStreamInfo,
    echo_mediaobj_GetInputType,
    echo_mediaobj_GetOutputType,
    echo_mediaobj_SetInputType,
    echo_mediaobj_SetOutputType,
    echo_mediaobj_GetInputCurrentType,
    echo_mediaobj_GetOutputCurrentType,
    echo_mediaobj_GetInputSizeInfo,
    echo_mediaobj_GetOutputSizeInfo,
    echo_mediaobj_GetInputMaxLatency,
    echo_mediaobj_SetInputMaxLatency,
    echo_mediaobj_Flush,
    echo_mediaobj_Discontinuity,
    echo_mediaobj_AllocateStreamingResources,
    echo_mediaobj_FreeStreamingResources,
    echo_mediaobj_GetInputStatus,
    echo_mediaobj_ProcessInput,
    echo_mediaobj_ProcessOutput,
    echo_mediaobj_Lock
};

static HRESULT WINAPI echo_mediainplace_QueryInterface(IMediaObjectInPlace *iface, REFIID riid, void **obj)
{
    struct dmo_echofx *This = impl_from_IMediaObjectInPlace(iface);
    return IDirectSoundFXEcho_QueryInterface(&This->IDirectSoundFXEcho_iface, riid, obj);
}

static ULONG WINAPI echo_mediainplace_AddRef(IMediaObjectInPlace *iface)
{
    struct dmo_echofx *This = impl_from_IMediaObjectInPlace(iface);
    return IDirectSoundFXEcho_AddRef(&This->IDirectSoundFXEcho_iface);
}

static ULONG WINAPI echo_mediainplace_Release(IMediaObjectInPlace *iface)
{
    struct dmo_echofx *This = impl_from_IMediaObjectInPlace(iface);
    return IDirectSoundFXEcho_Release(&This->IDirectSoundFXEcho_iface);
}

static HRESULT WINAPI echo_mediainplace_Process(IMediaObjectInPlace *iface, ULONG size, BYTE *data, REFERENCE_TIME start, DWORD flags)
{
    struct dmo_echofx *This = impl_from_IMediaObjectInPlace(iface);
    static BOOL once = 0;
    if(!once++)
        FIXME("%p, %d, %p, %s, %x\n", This, size, data, wine_dbgstr_longlong(start), flags);
    else
        TRACE("%p, %d, %p, %s, %x\n", This, size, data, wine_dbgstr_longlong(start), flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI echo_mediainplace_Clone(IMediaObjectInPlace *iface, IMediaObjectInPlace **object)
{
    struct dmo_echofx *This = impl_from_IMediaObjectInPlace(iface);
    FIXME("%p, %p\n", This, object);
    return E_NOTIMPL;
}

static HRESULT WINAPI echo_mediainplace_GetLatency(IMediaObjectInPlace *iface, REFERENCE_TIME *latency)
{
    struct dmo_echofx *This = impl_from_IMediaObjectInPlace(iface);
    FIXME("%p, %p\n", This, latency);
    return E_NOTIMPL;
}

static const IMediaObjectInPlaceVtbl echo_mediainplaceVtbl =
{
    echo_mediainplace_QueryInterface,
    echo_mediainplace_AddRef,
    echo_mediainplace_Release,
    echo_mediainplace_Process,
    echo_mediainplace_Clone,
    echo_mediainplace_GetLatency
};

static HRESULT WINAPI echofx_QueryInterface(IDirectSoundFXEcho *iface, REFIID riid, void **ppv)
{
    struct dmo_echofx *This = impl_from_IDirectSoundFXEcho(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirectSoundFXEcho))
    {
        *ppv = &This->IDirectSoundFXEcho_iface;
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

static ULONG WINAPI echofx_AddRef(IDirectSoundFXEcho *iface)
{
    struct dmo_echofx *This = impl_from_IDirectSoundFXEcho(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI echofx_Release(IDirectSoundFXEcho *iface)
{
    struct dmo_echofx *This = impl_from_IDirectSoundFXEcho(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        heap_free(This);
    }
    return ref;
}

static HRESULT WINAPI echofx_SetAllParameters(IDirectSoundFXEcho *iface, const DSFXEcho *echo)
{
    struct dmo_echofx *This = impl_from_IDirectSoundFXEcho(iface);

    TRACE("(%p) %p\n", This, echo);

    if(!echo)
        return E_POINTER;

    /* Out of Range values */
    if( (echo->fWetDryMix < DSFXECHO_WETDRYMIX_MIN || echo->fWetDryMix > DSFXECHO_WETDRYMIX_MAX) ||
        (echo->fFeedback < DSFXECHO_FEEDBACK_MIN   || echo->fFeedback > DSFXECHO_FEEDBACK_MAX) ||
        (echo->fLeftDelay < DSFXECHO_LEFTDELAY_MIN || echo->fLeftDelay > DSFXECHO_LEFTDELAY_MAX) ||
        (echo->fRightDelay < DSFXECHO_RIGHTDELAY_MIN || echo->fRightDelay > DSFXECHO_RIGHTDELAY_MAX) ||
        (echo->lPanDelay != DSFXECHO_PANDELAY_MIN && echo->lPanDelay != DSFXECHO_PANDELAY_MAX) )
    {
        return E_INVALIDARG;
    }

    This->params = *echo;

    return S_OK;
}

static HRESULT WINAPI echofx_GetAllParameters(IDirectSoundFXEcho *iface, DSFXEcho *echo)
{
    struct dmo_echofx *This = impl_from_IDirectSoundFXEcho(iface);

    TRACE("(%p) %p\n", This, echo);

    if(!echo)
        return E_INVALIDARG;

    *echo = This->params;

    return S_OK;
}

static const struct IDirectSoundFXEchoVtbl echofxVtbl =
{
    echofx_QueryInterface,
    echofx_AddRef,
    echofx_Release,
    echofx_SetAllParameters,
    echofx_GetAllParameters
};

HRESULT WINAPI EchoFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    struct dmo_echofx *object;
    HRESULT ret;

    TRACE("(%p, %s, %p)\n", outer, debugstr_guid(riid), ppv);

    *ppv = NULL;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IDirectSoundFXEcho_iface.lpVtbl = &echofxVtbl;
    object->IMediaObject_iface.lpVtbl = &echo_mediaobjectVtbl;
    object->IMediaObjectInPlace_iface.lpVtbl = &echo_mediainplaceVtbl;
    object->ref = 1;

    object->params.fWetDryMix  =  50.0f;
    object->params.fFeedback   =  50.0f;
    object->params.fLeftDelay  = 500.0f;
    object->params.fRightDelay = 500.0f;
    object->params.lPanDelay   =   0;

    ret = echofx_QueryInterface(&object->IDirectSoundFXEcho_iface, riid, ppv);
    echofx_Release(&object->IDirectSoundFXEcho_iface);

    return ret;
}

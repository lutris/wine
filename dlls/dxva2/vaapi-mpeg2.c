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

#include "config.h"
#include <stdarg.h>
#include <assert.h>
#include "windef.h"
#include "winbase.h"

#include "wine/debug.h"

#define COBJMACROS
#include "d3d9.h"
#include "dxva2api.h"
#include "dxva.h"
#include "dxva2_private.h"

#ifdef HAVE_VAAPI

WINE_DEFAULT_DEBUG_CHANNEL(dxva2);

#define MAX_SLICES 1024

static inline UINT estimate_maximum_slice_size( UINT width, UINT height )
{
    UINT size = width * (height + 8);
    return max(size, 1241600);
}

typedef struct
{
    IWineVideoDecoder IWineVideoDecoder_iface;
    LONG refCount;
    IWineVideoService *service;

    /* video attributes */
    UINT width;
    UINT height;
    D3DFORMAT format;
    DWORD maxSliceSize;
    VAImage vaImage;

    /* surfaces used by this decoder */
    UINT surfaceCount;
    VASurfaceID *surfaces;
    UINT currentSurface;

    /* configuration and context of the decoder */
    VAConfigID config;
    VAContextID context;

    /* slice buffer id */
    VABufferID vaBitstream;
    DXVA_PictureParameters d3dPictureParam;
    DXVA_QmatrixData d3dQMatrix;
    DXVA_SliceInfo d3dSliceInfo[MAX_SLICES];
} WineVideoDecoderMPEG2Impl;

/* caller has to hold the vaapi_lock */
static HRESULT process_picture_parameters( WineVideoDecoderMPEG2Impl *This, const DXVA2_DecodeBufferDesc *desc )
{
    VADisplay va_display = IWineVideoService_VADisplay(This->service);
    VAPictureParameterBufferMPEG2 params;
    VABufferID vaPictureParam;
    VAStatus status;

    if (desc->DataSize != sizeof(This->d3dPictureParam))
        FIXME("unexpected picture parameter buffer size %u\n", desc->DataSize);

    memset(&params, 0, sizeof(params));

    params.horizontal_size = This->width;
    params.vertical_size   = This->height;

    /* forward reference */
    if (This->d3dPictureParam.wForwardRefPictureIndex < This->surfaceCount)
        params.forward_reference_picture = This->surfaces[This->d3dPictureParam.wForwardRefPictureIndex];
    else if (This->d3dPictureParam.wForwardRefPictureIndex == 0xFFFF)
        params.forward_reference_picture = VA_INVALID_ID;
    else
    {
        FIXME("invalid forward surface reference index\n");
        params.forward_reference_picture = VA_INVALID_ID;
    }

    /* backward reference */
    if (This->d3dPictureParam.wBackwardRefPictureIndex < This->surfaceCount)
        params.backward_reference_picture = This->surfaces[This->d3dPictureParam.wBackwardRefPictureIndex];
    else if (This->d3dPictureParam.wBackwardRefPictureIndex == 0xFFFF)
        params.backward_reference_picture = VA_INVALID_ID;
    else
    {
        FIXME("invalid backward surface reference index\n");
        params.backward_reference_picture = VA_INVALID_ID;
    }

    /* picture_coding_type can have the following values :
     *   1 - I Frame - not references to other images
     *   2 - P Frame - reference to previous image
     *   3 - B Frame - reference to previous and next image */
    if (This->d3dPictureParam.bPicIntra)
        params.picture_coding_type = 1;
    else if (This->d3dPictureParam.bPicBackwardPrediction)
        params.picture_coding_type = 3;
    else
        params.picture_coding_type = 2;

    params.f_code = This->d3dPictureParam.wBitstreamFcodes;

    /* see http://msdn.microsoft.com/en-us/library/windows/hardware/ff564012(v=vs.85).aspx */
    params.picture_coding_extension.value = 0;
    params.picture_coding_extension.bits.intra_dc_precision         = (This->d3dPictureParam.wBitstreamPCEelements >> 14) & 3;
    params.picture_coding_extension.bits.picture_structure          = (This->d3dPictureParam.wBitstreamPCEelements >> 12) & 3;
    params.picture_coding_extension.bits.top_field_first            = (This->d3dPictureParam.wBitstreamPCEelements >> 11) & 1;
    params.picture_coding_extension.bits.frame_pred_frame_dct       = (This->d3dPictureParam.wBitstreamPCEelements >> 10) & 1;
    params.picture_coding_extension.bits.concealment_motion_vectors = (This->d3dPictureParam.wBitstreamPCEelements >> 9) & 1;
    params.picture_coding_extension.bits.q_scale_type               = (This->d3dPictureParam.wBitstreamPCEelements >> 8) & 1;
    params.picture_coding_extension.bits.intra_vlc_format           = (This->d3dPictureParam.wBitstreamPCEelements >> 7) & 1;
    params.picture_coding_extension.bits.alternate_scan             = (This->d3dPictureParam.wBitstreamPCEelements >> 6) & 1;
    params.picture_coding_extension.bits.repeat_first_field         = (This->d3dPictureParam.wBitstreamPCEelements >> 5) & 1;
    params.picture_coding_extension.bits.progressive_frame          = (This->d3dPictureParam.wBitstreamPCEelements >> 3) & 1;
    params.picture_coding_extension.bits.is_first_field             = !This->d3dPictureParam.bSecondField;

    status = pvaCreateBuffer(va_display, This->context, VAPictureParameterBufferType,
                             sizeof(params), 1, &params, &vaPictureParam);
    if (status != VA_STATUS_SUCCESS)
    {
        ERR("failed to create picture parameter buffer: %s (0x%x)\n", pvaErrorStr(status), status);
        return E_FAIL;
    }

    status = pvaRenderPicture(va_display, This->context, &vaPictureParam, 1);
    pvaDestroyBuffer(va_display, vaPictureParam);

    if (status == VA_STATUS_SUCCESS)
        return S_OK;

    ERR("failed to process picture parameter buffer: %s (0x%x)\n", pvaErrorStr(status), status);
    return E_FAIL;
}

/* caller has to hold the vaapi_lock */
static HRESULT process_quantization_matrix( WineVideoDecoderMPEG2Impl *This, const DXVA2_DecodeBufferDesc *desc )
{
    VADisplay va_display = IWineVideoService_VADisplay(This->service);
    VAIQMatrixBufferMPEG2 matrix;
    VABufferID vaIQMatrix;
    VAStatus status;
    unsigned int i;

    if (desc->DataSize != sizeof(This->d3dQMatrix))
        FIXME("unexpected quantization matrix buffer size %u\n", desc->DataSize);

    memset(&matrix, 0, sizeof(matrix));

    matrix.load_intra_quantiser_matrix            = This->d3dQMatrix.bNewQmatrix[0];
    matrix.load_non_intra_quantiser_matrix        = This->d3dQMatrix.bNewQmatrix[1];
    matrix.load_chroma_intra_quantiser_matrix     = This->d3dQMatrix.bNewQmatrix[2];
    matrix.load_chroma_non_intra_quantiser_matrix = This->d3dQMatrix.bNewQmatrix[3];

    /* In theory we could just copy the matrix, but
     * "The matrix consists of [...] unsigned words (in which only the lower 8 bits of each word are used [...])"
     * (see: http://msdn.microsoft.com/en-us/library/windows/hardware/ff564034(v=vs.85).aspx) */
    for (i = 0; i < 64; i++)
    {
        matrix.intra_quantiser_matrix[i]            = This->d3dQMatrix.Qmatrix[0][i];
        matrix.non_intra_quantiser_matrix[i]        = This->d3dQMatrix.Qmatrix[1][i];
        matrix.chroma_intra_quantiser_matrix[i]     = This->d3dQMatrix.Qmatrix[2][i];
        matrix.chroma_non_intra_quantiser_matrix[i] = This->d3dQMatrix.Qmatrix[3][i];
    }

    status = pvaCreateBuffer(va_display, This->context, VAIQMatrixBufferType,
                             sizeof(matrix), 1, &matrix, &vaIQMatrix);
    if (status != VA_STATUS_SUCCESS)
    {
        ERR("failed to create quantization matrix buffer: %s (0x%x)\n", pvaErrorStr(status), status);
        return E_FAIL;
    }

    status = pvaRenderPicture(va_display, This->context, &vaIQMatrix, 1);
    pvaDestroyBuffer(va_display, vaIQMatrix);

    if (status == VA_STATUS_SUCCESS)
        return S_OK;

    ERR("failed to process quantization matrix buffer: %s (0x%x)\n", pvaErrorStr(status), status);
    return E_FAIL;
}

/* caller has to hold the vaapi_lock */
static HRESULT process_slice_control_buffer( WineVideoDecoderMPEG2Impl *This, const DXVA2_DecodeBufferDesc *desc )
{
    VADisplay va_display = IWineVideoService_VADisplay(This->service);
    VASliceParameterBufferMPEG2 *slice = NULL;
    VABufferID vaSliceInfo;
    VAStatus status;
    unsigned int sliceCount, i;

    /* check for valid parameters */
    if (!desc->DataSize || desc->DataSize % sizeof(DXVA_SliceInfo) != 0)
    {
        ERR("slice control buffer size is invalid (got %u, expected multiple of %lu)\n",
            desc->DataSize, (SIZE_T)sizeof(DXVA_SliceInfo));
        return E_FAIL;
    }

    sliceCount = desc->DataSize / sizeof(DXVA_SliceInfo);

    status = pvaCreateBuffer(va_display, This->context, VASliceParameterBufferType,
                             sizeof(VASliceParameterBufferMPEG2), sliceCount, NULL, &vaSliceInfo);
    if (status != VA_STATUS_SUCCESS)
    {
        ERR("failed to create slice control buffer: %s (0x%x)\n", pvaErrorStr(status), status);
        return E_FAIL;
    }

    status = pvaMapBuffer(va_display, vaSliceInfo, (void **)&slice);
    if (status != VA_STATUS_SUCCESS)
    {
        ERR("failed to map slice control buffer: %s (0x%x)\n", pvaErrorStr(status), status);
        goto err;
    }

    for (i = 0; i < sliceCount; i++)
    {
        slice[i].slice_data_size    = This->d3dSliceInfo[i].dwSliceBitsInBuffer / 8;
        slice[i].slice_data_offset  = This->d3dSliceInfo[i].dwSliceDataLocation;

        switch (This->d3dSliceInfo[i].wBadSliceChopping)
        {
            case 0: slice[i].slice_data_flag = VA_SLICE_DATA_FLAG_ALL; break;
            case 1: slice[i].slice_data_flag = VA_SLICE_DATA_FLAG_BEGIN; break;
            case 2: slice[i].slice_data_flag = VA_SLICE_DATA_FLAG_END; break;
            case 3: slice[i].slice_data_flag = VA_SLICE_DATA_FLAG_MIDDLE; break;
            default:
            {
                ERR("invalid wBadSliceChopping value %d - assuming VA_SLICE_DATA_FLAG_ALL\n",
                    This->d3dSliceInfo[i].wBadSliceChopping);
                slice[i].slice_data_flag = VA_SLICE_DATA_FLAG_ALL;
                break;
            }
        }

        slice[i].macroblock_offset         = This->d3dSliceInfo[i].wMBbitOffset;
        slice[i].slice_horizontal_position = This->d3dSliceInfo[i].wHorizontalPosition;
        slice[i].slice_vertical_position   = This->d3dSliceInfo[i].wVerticalPosition;
        slice[i].quantiser_scale_code      = This->d3dSliceInfo[i].wQuantizerScaleCode;
        slice[i].intra_slice_flag          = (This->d3dSliceInfo[i].wMBbitOffset > 38); /* magic */
    }

    if (pvaUnmapBuffer(va_display, vaSliceInfo) != VA_STATUS_SUCCESS)
        goto err;

    status = pvaRenderPicture(va_display, This->context, &vaSliceInfo, 1);
    if (status == VA_STATUS_SUCCESS)
    {
        pvaDestroyBuffer(va_display, vaSliceInfo);
        return S_OK;
    }

    ERR("failed to process slice control buffer: %s (0x%x)\n", pvaErrorStr(status), status);

err:
    pvaDestroyBuffer(va_display, vaSliceInfo);
    return E_FAIL;
}

/* caller has to hold the vaapi_lock */
static HRESULT process_data_buffer( WineVideoDecoderMPEG2Impl *This, const DXVA2_DecodeBufferDesc *desc )
{
    VADisplay va_display = IWineVideoService_VADisplay(This->service);
    VAStatus status;
    HRESULT hr = S_OK;

    if (This->vaBitstream == VA_INVALID_ID)
        return E_FAIL;

    status = pvaRenderPicture(va_display, This->context, &This->vaBitstream, 1);
    if (status != VA_STATUS_SUCCESS)
    {
        ERR("failed to process slice buffer: %s (0x%x)\n", pvaErrorStr(status), status);
        hr = E_FAIL;
    }

    pvaDestroyBuffer(va_display, This->vaBitstream);
    This->vaBitstream = VA_INVALID_ID;
    return hr;
}

static inline WineVideoDecoderMPEG2Impl *impl_from_IWineVideoDecoder( IWineVideoDecoder *iface )
{
    return CONTAINING_RECORD(iface, WineVideoDecoderMPEG2Impl, IWineVideoDecoder_iface);
}

/*****************************************************************************
 * IWineVideoDecoder interface
 */

static HRESULT WINAPI WineVideoDecoderMPEG2_QueryInterface( IWineVideoDecoder *iface, REFIID riid, void **ppv )
{
    WineVideoDecoderMPEG2Impl *This = impl_from_IWineVideoDecoder(iface);
    TRACE("(%p/%p)->(%s, %p)\n", iface, This, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IWineVideoDecoder))
        *ppv = &This->IWineVideoDecoder_iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI WineVideoDecoderMPEG2_AddRef( IWineVideoDecoder *iface )
{
    WineVideoDecoderMPEG2Impl *This = impl_from_IWineVideoDecoder(iface);
    ULONG refCount = InterlockedIncrement(&This->refCount);

    TRACE("(%p)->() AddRef from %d\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI WineVideoDecoderMPEG2_Release( IWineVideoDecoder *iface )
{
    WineVideoDecoderMPEG2Impl *This = impl_from_IWineVideoDecoder(iface);
    ULONG refCount = InterlockedDecrement(&This->refCount);

    TRACE("(%p)->() Release from %d\n", This, refCount + 1);

    if (!refCount)
    {
        VADisplay va_display = IWineVideoService_VADisplay(This->service);
        TRACE("Destroying\n");

        vaapi_lock();

        if (This->vaBitstream != VA_INVALID_ID)
            pvaDestroyBuffer(va_display, This->vaBitstream);

        pvaDestroySurfaces(va_display, This->surfaces, This->surfaceCount);
        HeapFree(GetProcessHeap(), 0, This->surfaces);

        pvaDestroyImage(va_display, This->vaImage.image_id);

        pvaDestroyContext(va_display, This->context);
        pvaDestroyConfig(va_display, This->config);

        vaapi_unlock();

        IWineVideoService_Release(This->service);
        CoTaskMemFree(This);
    }

    return refCount;
}

static HRESULT WINAPI WineVideoDecoderMPEG2_LockBuffer( IWineVideoDecoder *iface, UINT type, void **buffer,
                                                        UINT *size )
{
    WineVideoDecoderMPEG2Impl *This = impl_from_IWineVideoDecoder(iface);
    VADisplay va_display = IWineVideoService_VADisplay(This->service);
    VAStatus status;
    HRESULT hr = E_FAIL;
    void *buf;

    TRACE("(%p, %u, %p, %p)\n", This, type, buffer, size);

    if (type == DXVA2_PictureParametersBufferType)
    {
        *buffer = &This->d3dPictureParam;
        *size = sizeof(This->d3dPictureParam);
        return S_OK;
    }
    else if(type == DXVA2_InverseQuantizationMatrixBufferType)
    {
        *buffer = &This->d3dQMatrix;
        *size = sizeof(This->d3dQMatrix);
        return S_OK;
    }
    else if (type == DXVA2_SliceControlBufferType)
    {
        *buffer = &This->d3dSliceInfo;
        *size = sizeof(This->d3dSliceInfo);
        return S_OK;
    }
    else if (type != DXVA2_BitStreamDateBufferType)
        return E_INVALIDARG;

    vaapi_lock();

    /* reuse existing vaSlice buffer if any */
    if (This->vaBitstream == VA_INVALID_ID)
    {
        status = pvaCreateBuffer(va_display, This->context, VASliceDataBufferType,
                                 This->maxSliceSize, 1, NULL, &This->vaBitstream);
        if (status != VA_STATUS_SUCCESS)
        {
            ERR("failed to create slice buffer: %s (0x%x)\n", pvaErrorStr(status), status);
            goto out;
        }
    }

    status = pvaMapBuffer(va_display, This->vaBitstream, &buf);
    if (status != VA_STATUS_SUCCESS)
    {
        ERR("failed to map slice buffer: %s (0x%x)\n", pvaErrorStr(status), status);
    }
    else
    {
        *buffer = buf;
        *size   = This->maxSliceSize;
        hr = S_OK;
    }

out:
    vaapi_unlock();
    return hr;
}

static HRESULT WINAPI WineVideoDecoderMPEG2_UnlockBuffer( IWineVideoDecoder *iface, UINT type )
{
    WineVideoDecoderMPEG2Impl *This = impl_from_IWineVideoDecoder(iface);
    VADisplay va_display = IWineVideoService_VADisplay(This->service);
    VAStatus status;
    HRESULT hr = E_FAIL;

    TRACE("(%p, %u,)\n", This, type);

    if (type == DXVA2_PictureParametersBufferType ||
        type == DXVA2_InverseQuantizationMatrixBufferType ||
        type == DXVA2_SliceControlBufferType)
    {
        return S_OK;
    }
    else if (type != DXVA2_BitStreamDateBufferType)
        return E_INVALIDARG;

    vaapi_lock();

    if (This->vaBitstream == VA_INVALID_ID)
    {
        ERR("no slice buffer allocated\n");
        goto out;
    }

    status = pvaUnmapBuffer(va_display, This->vaBitstream);
    if (status != VA_STATUS_SUCCESS)
    {
        ERR("failed to unmap slice buffer: %s (0x%x)\n", pvaErrorStr(status), status);
    }
    else hr = S_OK;

out:
    vaapi_unlock();
    return hr;
}

static HRESULT WINAPI WineVideoDecoderMPEG2_ExecuteBuffer( IWineVideoDecoder *iface, DXVA2_DecodeBufferDesc *pictureParam,
        DXVA2_DecodeBufferDesc *qMatrix, DXVA2_DecodeBufferDesc *sliceInfo, DXVA2_DecodeBufferDesc *bitStream )
{
    WineVideoDecoderMPEG2Impl *This = impl_from_IWineVideoDecoder(iface);
    HRESULT hr;

    TRACE("(%p, %p, %p, %p, %p)\n", This, pictureParam, qMatrix, sliceInfo, bitStream);

    if (!pictureParam || !qMatrix || !sliceInfo || !bitStream)
    {
        FIXME("not enough buffers to decode picture\n");
        return E_FAIL;
    }

    vaapi_lock();

    hr = process_picture_parameters(This, pictureParam);

    if (SUCCEEDED(hr))
        hr = process_quantization_matrix(This, qMatrix);

    if (SUCCEEDED(hr))
        hr = process_slice_control_buffer(This, sliceInfo);

    if (SUCCEEDED(hr))
        hr = process_data_buffer(This, bitStream);

    vaapi_unlock();
    return hr;
}

static HRESULT WINAPI WineVideoDecoderMPEG2_BeginFrame( IWineVideoDecoder *iface, UINT surfaceIndex )
{
    WineVideoDecoderMPEG2Impl *This = impl_from_IWineVideoDecoder(iface);
    VADisplay va_display = IWineVideoService_VADisplay(This->service);
    VAStatus status;
    HRESULT hr = E_FAIL;

    TRACE("(%p, %d)\n", This, surfaceIndex);

    if (surfaceIndex >= This->surfaceCount)
        return E_INVALIDARG;

    vaapi_lock();

    status = pvaBeginPicture(va_display, This->context, This->surfaces[surfaceIndex]);
    if (status != VA_STATUS_SUCCESS)
    {
        ERR("failed to begin picture: %s (0x%x)\n", pvaErrorStr(status), status);
    }
    else
    {
        This->currentSurface = surfaceIndex;
        hr = S_OK;
    }

    vaapi_unlock();
    return hr;
}

static HRESULT WINAPI WineVideoDecoderMPEG2_EndFrame( IWineVideoDecoder *iface )
{
    WineVideoDecoderMPEG2Impl *This = impl_from_IWineVideoDecoder(iface);
    VADisplay va_display = IWineVideoService_VADisplay(This->service);
    VAStatus status;
    HRESULT hr = E_FAIL;

    TRACE("(%p)\n", This);

    vaapi_lock();

    status = pvaEndPicture(va_display, This->context);
    if (status != VA_STATUS_SUCCESS)
    {
        ERR("ending picture failed: %s (0x%x)\n", pvaErrorStr(status), status);
    }
    else hr = S_OK;

    vaapi_unlock();
    return hr;
}

static HRESULT WINAPI WineVideoDecoderMPEG2_LockImage( IWineVideoDecoder *iface, WineVideoImage *image )
{
    WineVideoDecoderMPEG2Impl *This = impl_from_IWineVideoDecoder(iface);
    VADisplay va_display = IWineVideoService_VADisplay(This->service);
    VAStatus status;
    HRESULT hr = E_FAIL;

    TRACE("(%p, %p)\n", This, image);

    vaapi_lock();

    pvaSyncSurface(va_display, This->surfaces[This->currentSurface]);

    status = pvaGetImage(va_display, This->surfaces[This->currentSurface], 0, 0,
                         This->width, This->height, This->vaImage.image_id);
    if (status != VA_STATUS_SUCCESS)
    {
        ERR("failed to get image: %s (0x%x)\n", pvaErrorStr(status), status);
        goto out;
    }

    status = pvaMapBuffer(va_display, This->vaImage.buf, &image->buffer);
    if (status != VA_STATUS_SUCCESS)
    {
        ERR("failed to map image buffer: %s (0x%x)\n", pvaErrorStr(status), status);
    }
    else
    {
        image->format     = This->format;
        image->width      = This->vaImage.width;
        image->height     = This->vaImage.height;
        image->planeCount = This->vaImage.num_planes;
        image->offsets    = This->vaImage.offsets;
        image->pitches    = This->vaImage.pitches;
        hr = S_OK;
    }

out:
    vaapi_unlock();
    return hr;
}

static HRESULT WINAPI WineVideoDecoderMPEG2_UnlockImage( IWineVideoDecoder *iface )
{
    WineVideoDecoderMPEG2Impl *This = impl_from_IWineVideoDecoder(iface);
    VADisplay va_display = IWineVideoService_VADisplay(This->service);
    VAStatus status;
    HRESULT hr = E_FAIL;

    TRACE("(%p)\n", This);

    vaapi_lock();

    status = pvaUnmapBuffer(va_display, This->vaImage.buf);
    if (status != VA_STATUS_SUCCESS)
    {
        ERR("failed to unmap image buffer: %s (0x%x)\n", pvaErrorStr(status), status);
    }
    else hr = S_OK;

    vaapi_unlock();
    return hr;
}

static const IWineVideoDecoderVtbl WineVideoDecoderMPEG2_VTable =
{
    WineVideoDecoderMPEG2_QueryInterface,
    WineVideoDecoderMPEG2_AddRef,
    WineVideoDecoderMPEG2_Release,
    WineVideoDecoderMPEG2_LockBuffer,
    WineVideoDecoderMPEG2_UnlockBuffer,
    WineVideoDecoderMPEG2_ExecuteBuffer,
    WineVideoDecoderMPEG2_BeginFrame,
    WineVideoDecoderMPEG2_EndFrame,
    WineVideoDecoderMPEG2_LockImage,
    WineVideoDecoderMPEG2_UnlockImage
};

HRESULT vaapi_mpeg2decoder_create( IWineVideoService *service, const DXVA2_VideoDesc *videoDesc,
                                   DXVA2_ConfigPictureDecode *config, UINT numSurfaces, IWineVideoDecoder **decoder )
{
    struct vaapi_format *format;
    struct vaapi_profile *profile;
    WineVideoDecoderMPEG2Impl *mpeg2decoder;
    VAConfigAttrib codecAttrib;
    VADisplay va_display;
    VAStatus status;

    if (!service || !videoDesc || !config || !decoder)
        return E_INVALIDARG;

    va_display = IWineVideoService_VADisplay(service);
    *decoder = NULL;

    /* MPEG2 B frames can reference up to 2 previously decoded frames */
    if (numSurfaces < 3)
        WARN("decoder initialized with less than 3 frames\n");

    format  = vaapi_lookup_d3dformat(videoDesc->Format);
    profile = vaapi_lookup_guid(&DXVA2_ModeMPEG2_VLD);
    if (!format || !profile)
        return E_INVALIDARG;

    if (!vaapi_is_format_supported(va_display, profile, format))
        return E_INVALIDARG;

    if (videoDesc->InputSampleFreq.Numerator * videoDesc->OutputFrameFreq.Denominator !=
        videoDesc->OutputFrameFreq.Numerator * videoDesc->InputSampleFreq.Denominator)
    {
        FIXME("changing the framerate is not supported\n");
        return E_INVALIDARG;
    }

    mpeg2decoder = CoTaskMemAlloc(sizeof(*mpeg2decoder));
    if (!mpeg2decoder)
        return E_OUTOFMEMORY;

    memset(mpeg2decoder, 0, sizeof(*mpeg2decoder));

    mpeg2decoder->IWineVideoDecoder_iface.lpVtbl = &WineVideoDecoderMPEG2_VTable;
    mpeg2decoder->refCount          = 1;
    mpeg2decoder->service           = service;

    mpeg2decoder->width             = videoDesc->SampleWidth;
    mpeg2decoder->height            = videoDesc->SampleHeight;
    mpeg2decoder->format            = videoDesc->Format;
    mpeg2decoder->maxSliceSize      = estimate_maximum_slice_size(videoDesc->SampleWidth, videoDesc->SampleHeight);
    memset(&mpeg2decoder->vaImage, 0, sizeof(mpeg2decoder->vaImage));
    mpeg2decoder->vaImage.image_id  = VA_INVALID_ID;

    mpeg2decoder->surfaceCount      = numSurfaces;
    mpeg2decoder->surfaces          = NULL;
    mpeg2decoder->currentSurface    = 0;

    mpeg2decoder->config            = 0;
    mpeg2decoder->context           = 0;

    mpeg2decoder->vaBitstream       = VA_INVALID_ID;

    vaapi_lock();

    codecAttrib.type  = VAConfigAttribRTFormat;
    codecAttrib.value = format->vaformat;

    status = pvaCreateConfig(va_display, profile->profile, profile->entryPoint,
                             &codecAttrib, 1, &mpeg2decoder->config);
    if (status != VA_STATUS_SUCCESS)
    {
        ERR("failed to create decoder config: %s (0x%x)\n", pvaErrorStr(status), status);
        goto err;
    }

    if (!vaapi_create_surfaces(va_display, format, mpeg2decoder->width, mpeg2decoder->height,
                               &mpeg2decoder->vaImage, numSurfaces, &mpeg2decoder->surfaces))
    {
        ERR("Failed to create image or surfaces\n");
        goto err;
    }

    status = pvaCreateContext(va_display, mpeg2decoder->config, mpeg2decoder->width, mpeg2decoder->height,
                              VA_PROGRESSIVE, mpeg2decoder->surfaces, numSurfaces, &mpeg2decoder->context);
    if (status != VA_STATUS_SUCCESS)
    {
        ERR("failed to create context: %s (0x%x)\n", pvaErrorStr(status), status);
        goto err;
    }

    vaapi_unlock();

    IWineVideoService_AddRef(service);

    *decoder = &mpeg2decoder->IWineVideoDecoder_iface;
    return S_OK;

err:
     if (mpeg2decoder->surfaces)
    {
        pvaDestroySurfaces(va_display, mpeg2decoder->surfaces, mpeg2decoder->surfaceCount);
        HeapFree(GetProcessHeap(), 0, mpeg2decoder->surfaces);
    }

    if (mpeg2decoder->vaImage.image_id != VA_INVALID_ID)
        pvaDestroyImage(va_display, mpeg2decoder->vaImage.image_id);

    if (mpeg2decoder->config)
        pvaDestroyConfig(va_display, mpeg2decoder->config);

    vaapi_unlock();
    CoTaskMemFree(mpeg2decoder);
    return E_FAIL;
}

#endif /* HAVE_VAAPI */

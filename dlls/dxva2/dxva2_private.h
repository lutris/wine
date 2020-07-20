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

#include "dxva2api.h"
#include "d3d9.h"
#include "backend.h"
#ifdef HAVE_VAAPI
# undef Status
# include <va/va_x11.h>
# include <va/va_drm.h>
#endif

extern HRESULT videoservice_create( IDirect3DDevice9 *device, REFIID riid, void **ppv ) DECLSPEC_HIDDEN;
extern HRESULT devicemanager_create( UINT *resetToken, void **ppv ) DECLSPEC_HIDDEN;
extern HRESULT processor_software_create( IDirectXVideoProcessorService *processor_service, IDirect3DDevice9 *device,
                                          const DXVA2_VideoDesc *pVideoDesc, D3DFORMAT RenderTargetFormat,
                                          UINT MaxNumSubStreams, IDirectXVideoProcessor **processor ) DECLSPEC_HIDDEN;
extern HRESULT genericdecoder_create( IDirectXVideoDecoderService *videodecoder, const DXVA2_VideoDesc *videoDesc,
                                      DXVA2_ConfigPictureDecode *config, IDirect3DSurface9 **decoderRenderTargets,
                                      UINT numSurfaces, IWineVideoDecoder *backend, IDirectXVideoDecoder **decode ) DECLSPEC_HIDDEN;

/*****************************************************************************
 * vaapi backend
 */

/* public */
extern BOOL config_vaapi_enabled DECLSPEC_HIDDEN;
extern BOOL config_vaapi_drm DECLSPEC_HIDDEN;
extern char config_vaapi_drm_path[MAX_PATH] DECLSPEC_HIDDEN;
extern IWineVideoService *vaapi_videoservice_create( void ) DECLSPEC_HIDDEN;

/* internal */
#ifdef HAVE_VAAPI

#define MAKE_FUNCPTR(f) extern typeof(f) * p##f DECLSPEC_HIDDEN
MAKE_FUNCPTR(XCloseDisplay);
MAKE_FUNCPTR(XOpenDisplay);
MAKE_FUNCPTR(vaBeginPicture);
MAKE_FUNCPTR(vaCreateBuffer);
MAKE_FUNCPTR(vaCreateConfig);
MAKE_FUNCPTR(vaCreateContext);
MAKE_FUNCPTR(vaCreateImage);
MAKE_FUNCPTR(vaCreateSurfaces);
MAKE_FUNCPTR(vaDestroyBuffer);
MAKE_FUNCPTR(vaDestroyConfig);
MAKE_FUNCPTR(vaDestroyContext);
MAKE_FUNCPTR(vaDestroyImage);
MAKE_FUNCPTR(vaDestroySurfaces);
MAKE_FUNCPTR(vaEndPicture);
MAKE_FUNCPTR(vaErrorStr);
MAKE_FUNCPTR(vaGetConfigAttributes);
MAKE_FUNCPTR(vaGetDisplay);
MAKE_FUNCPTR(vaGetImage);
MAKE_FUNCPTR(vaInitialize);
MAKE_FUNCPTR(vaMapBuffer);
MAKE_FUNCPTR(vaMaxNumEntrypoints);
MAKE_FUNCPTR(vaMaxNumProfiles);
MAKE_FUNCPTR(vaQueryConfigEntrypoints);
MAKE_FUNCPTR(vaQueryConfigProfiles);
MAKE_FUNCPTR(vaQuerySurfaceAttributes);
MAKE_FUNCPTR(vaRenderPicture);
MAKE_FUNCPTR(vaSyncSurface);
MAKE_FUNCPTR(vaTerminate);
MAKE_FUNCPTR(vaUnmapBuffer);
#undef MAKE_FUNCPTR

struct vaapi_profile
{
    const int profile;
    const int entryPoint;
    const GUID* guid;
    BOOL supported;
};

struct vaapi_format
{
    const D3DFORMAT d3dformat;
    const unsigned int vafourcc;
    const unsigned int vaformat;
    const BOOL fallback;
    const unsigned int bits;
};

typedef struct
{
    IWineVideoService IWineVideoService_iface;
    LONG refCount;

    /* libraries */
    void *va_handle;
    void *va_x11_handle;
    void *va_drm_handle;
    void *x11_handle;

    /* display / drm connection */
    Display *x11_display;
    VADisplay va_display;
    int drm_fd;
} WineVideoServiceImpl;

static inline WineVideoServiceImpl *impl_from_IWineVideoService( IWineVideoService *iface )
{
    return CONTAINING_RECORD(iface, WineVideoServiceImpl, IWineVideoService_iface);
}

static inline VADisplay IWineVideoService_VADisplay( IWineVideoService *iface )
{
    WineVideoServiceImpl *This = impl_from_IWineVideoService(iface);
    return This->va_display;
}

extern void vaapi_lock( void ) DECLSPEC_HIDDEN;
extern void vaapi_unlock( void ) DECLSPEC_HIDDEN;

extern struct vaapi_profile *vaapi_lookup_profile( int profile, int entryPoint ) DECLSPEC_HIDDEN;
extern struct vaapi_profile *vaapi_lookup_guid( const GUID *guid ) DECLSPEC_HIDDEN;
extern struct vaapi_format *vaapi_lookup_d3dformat( D3DFORMAT d3dformat ) DECLSPEC_HIDDEN;

extern BOOL vaapi_is_format_supported( VADisplay va_display, struct vaapi_profile *profile,
                                       struct vaapi_format *format ) DECLSPEC_HIDDEN;
extern BOOL vaapi_create_surfaces( VADisplay va_display, const struct vaapi_format *format, UINT width, UINT height,
                                   VAImage *vaImage, UINT numSurfaces, VASurfaceID **surfaceList ) DECLSPEC_HIDDEN;

extern HRESULT vaapi_mpeg2decoder_create( IWineVideoService *backend, const DXVA2_VideoDesc *videoDesc,
                                          DXVA2_ConfigPictureDecode *config, UINT numSurfaces,
                                          IWineVideoDecoder **decoder ) DECLSPEC_HIDDEN;
extern HRESULT vaapi_h264decoder_create( IWineVideoService *service, const DXVA2_VideoDesc *videoDesc,
                                         DXVA2_ConfigPictureDecode *config, UINT numSurfaces,
                                         IWineVideoDecoder **decoder ) DECLSPEC_HIDDEN;

#endif /* HAVE_VAAPI */

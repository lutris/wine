/*
 * Copyright 2014-2015 Michael Müller for Pipelight
 * Copyright 2015 Sebastian Lackner for Pipelight
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
#include "wine/port.h"
#include <stdarg.h>
#include <assert.h>
#include "windef.h"
#include "winbase.h"

#include "wine/library.h"
#include "wine/debug.h"

#define COBJMACROS
#include "d3d9.h"
#include "dxva2api.h"
#include "dxva.h"
#include "dxva2_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxva2);

#ifdef HAVE_VAAPI

#define MAKE_FUNCPTR(f) typeof(f) * p##f = NULL
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
MAKE_FUNCPTR(vaGetDisplayDRM);
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

#define LOAD_FUNCPTR(f) \
    if(!(p##f = dlsym(handle, #f))) \
    { \
        WARN("Can't find symbol %s.\n", #f); \
        goto error; \
    }

static void *load_libva( void )
{
    void *handle = dlopen(SONAME_LIBVA, RTLD_NOW);
    if (!handle)
    {
        FIXME("Wine cannot find the %s library.\n", SONAME_LIBVA);
        return FALSE;
    }

    LOAD_FUNCPTR(vaBeginPicture);
    LOAD_FUNCPTR(vaCreateBuffer);
    LOAD_FUNCPTR(vaCreateConfig);
    LOAD_FUNCPTR(vaCreateContext);
    LOAD_FUNCPTR(vaCreateImage);
    LOAD_FUNCPTR(vaCreateSurfaces);
    LOAD_FUNCPTR(vaDestroyBuffer);
    LOAD_FUNCPTR(vaDestroyConfig);
    LOAD_FUNCPTR(vaDestroyContext);
    LOAD_FUNCPTR(vaDestroyImage);
    LOAD_FUNCPTR(vaDestroySurfaces);
    LOAD_FUNCPTR(vaEndPicture);
    LOAD_FUNCPTR(vaErrorStr);
    LOAD_FUNCPTR(vaGetConfigAttributes);
    LOAD_FUNCPTR(vaGetImage);
    LOAD_FUNCPTR(vaInitialize);
    LOAD_FUNCPTR(vaMapBuffer);
    LOAD_FUNCPTR(vaMaxNumEntrypoints);
    LOAD_FUNCPTR(vaMaxNumProfiles);
    LOAD_FUNCPTR(vaQueryConfigEntrypoints);
    LOAD_FUNCPTR(vaQueryConfigProfiles);
    LOAD_FUNCPTR(vaQuerySurfaceAttributes);
    LOAD_FUNCPTR(vaRenderPicture);
    LOAD_FUNCPTR(vaSyncSurface);
    LOAD_FUNCPTR(vaTerminate);
    LOAD_FUNCPTR(vaUnmapBuffer);
    return handle;

error:
    dlclose(handle);
    return NULL;
}

static void *load_libva_x11( void )
{
    void *handle = dlopen(SONAME_LIBVA_X11, RTLD_NOW);
    if (!handle)
    {
        FIXME("Wine cannot find the %s library.\n", SONAME_LIBVA_X11);
        return FALSE;
    }

    LOAD_FUNCPTR(vaGetDisplay);
    return handle;

error:
    dlclose(handle);
    return NULL;
}

static void *load_libva_drm( void )
{
    void *handle = dlopen(SONAME_LIBVA_DRM, RTLD_NOW);
    if (!handle)
    {
        FIXME("Wine cannot find the %s library.\n", SONAME_LIBVA_DRM);
        return FALSE;
    }

    LOAD_FUNCPTR(vaGetDisplayDRM);
    return handle;

error:
    dlclose(handle);
    return NULL;
}

static void *load_libx11( void )
{
    void *handle = dlopen(SONAME_LIBX11, RTLD_NOW);
    if (!handle)
    {
        FIXME("Wine cannot find the %s library.\n", SONAME_LIBX11);
        return FALSE;
    }

    LOAD_FUNCPTR(XCloseDisplay);
    LOAD_FUNCPTR(XOpenDisplay);
    return handle;

error:
    dlclose(handle);
    return NULL;
}

#undef LOAD_FUNCPTR

static CRITICAL_SECTION vaapi_section;
static CRITICAL_SECTION_DEBUG vaapi_section_debug =
{
    0, 0, &vaapi_section,
    {&vaapi_section_debug.ProcessLocksList, &vaapi_section_debug.ProcessLocksList},
      0, 0, {(DWORD_PTR)(__FILE__ ": vaapi_section")}
};
static CRITICAL_SECTION vaapi_section = {&vaapi_section_debug, -1, 0, 0, 0, 0};

static WineVideoServiceImpl *vaapi_videoservice = NULL;

void vaapi_lock( void )
{
    EnterCriticalSection(&vaapi_section);
}

void vaapi_unlock( void )
{
    LeaveCriticalSection(&vaapi_section);
}

static struct vaapi_profile profile_table[] =
{
    /* MPEG2 */
    {VAProfileMPEG2Main,            VAEntrypointVLD,    &DXVA2_ModeMPEG2_VLD},
    {VAProfileMPEG2Main,            VAEntrypointMoComp, &DXVA2_ModeMPEG2_MoComp},
    {VAProfileMPEG2Main,            VAEntrypointIDCT,   &DXVA2_ModeMPEG2_IDCT},

    /* VC1 */
    {VAProfileVC1Advanced,          VAEntrypointVLD,    &DXVA2_ModeVC1_VLD},
    {VAProfileVC1Advanced,          VAEntrypointIDCT,   &DXVA2_ModeVC1_IDCT},
    {VAProfileVC1Advanced,          VAEntrypointMoComp, &DXVA2_ModeVC1_MoComp},

    /* H264 */
    {VAProfileH264High,             VAEntrypointVLD,    &DXVA2_ModeH264_E},
    {VAProfileH264High,             VAEntrypointIDCT,   &DXVA2_ModeH264_C},
    {VAProfileH264High,             VAEntrypointMoComp, &DXVA2_ModeH264_A},

    /* TODO: MPEG4, ... */
};

struct vaapi_profile *vaapi_lookup_profile( int profile, int entryPoint )
{
    unsigned int i;
    for (i = 0; i < sizeof(profile_table)/sizeof(profile_table[0]); i++)
    {
        if (profile_table[i].profile == profile && profile_table[i].entryPoint == entryPoint)
            return &profile_table[i];
    }
    return NULL;
}

struct vaapi_profile *vaapi_lookup_guid( const GUID *guid )
{
    unsigned int i;
    for (i = 0; i < sizeof(profile_table)/sizeof(profile_table[0]); i++)
    {
        if (IsEqualGUID(profile_table[i].guid, guid))
            return &profile_table[i];
    }
    return NULL;
}

static struct vaapi_format format_table[] =
{
    {MAKEFOURCC('N','V','1','1'), VA_FOURCC_NV11, VA_RT_FORMAT_YUV420, FALSE, 12},
    {MAKEFOURCC('N','V','1','2'), VA_FOURCC_NV12, VA_RT_FORMAT_YUV420, TRUE , 12},
    {MAKEFOURCC('A','I','4','4'), VA_FOURCC_AI44, 0,                   FALSE,  0}, /* TODO */
    {0,                           VA_FOURCC_RGBA, VA_RT_FORMAT_RGB32,  FALSE, 32}, /* TODO */
    {0,                           VA_FOURCC_RGBX, VA_RT_FORMAT_RGB32,  FALSE, 32}, /* TODO */
    {0,                           VA_FOURCC_BGRA, VA_RT_FORMAT_RGB32,  FALSE, 32}, /* TODO */
    {0,                           VA_FOURCC_BGRX, VA_RT_FORMAT_RGB32,  FALSE, 32}, /* TODO */
    {D3DFMT_A8R8G8B8,             VA_FOURCC_ARGB, VA_RT_FORMAT_RGB32,  FALSE, 32},
    {D3DFMT_X8R8G8B8,             VA_FOURCC_XRGB, VA_RT_FORMAT_RGB32,  FALSE, 32},
    {D3DFMT_UYVY,                 VA_FOURCC_UYVY, VA_RT_FORMAT_YUV422, FALSE, 16},
    {D3DFMT_YUY2,                 VA_FOURCC_YUY2, VA_RT_FORMAT_YUV422, FALSE, 16},
    {MAKEFOURCC('A','Y','U','V'), VA_FOURCC_AYUV, VA_RT_FORMAT_YUV444, FALSE, 32},
    {MAKEFOURCC('Y','V','1','2'), VA_FOURCC_YV12, VA_RT_FORMAT_YUV420, FALSE, 12},
    {0,                           VA_FOURCC_P208, 0,                   FALSE,  0}, /* TODO */
    {MAKEFOURCC('I','Y','U','V'), VA_FOURCC_IYUV, VA_RT_FORMAT_YUV411, FALSE, 12},
    {MAKEFOURCC('Y','V','2','4'), VA_FOURCC_YV24, VA_RT_FORMAT_YUV444, FALSE, 24}, /* TODO */
    {0,                           VA_FOURCC_YV32, 0,                   FALSE, 32}, /* TODO */
    {0,                           VA_FOURCC_Y800, 0,                   FALSE,  8}, /* TODO */
    {MAKEFOURCC('I','M','C','3'), VA_FOURCC_IMC3, 0,                   FALSE, 12},
    {0,                           VA_FOURCC_411P, 0,                   FALSE, 12}, /* TODO */
    {0,                           VA_FOURCC_422H, VA_RT_FORMAT_YUV422, FALSE, 16}, /* TODO */
    {0,                           VA_FOURCC_422V, VA_RT_FORMAT_YUV422, FALSE, 16}, /* TODO */
    {0,                           VA_FOURCC_444P, 0,                   FALSE, 24}, /* TODO */
    {0,                           VA_FOURCC_RGBP, VA_RT_FORMAT_RGBP,   FALSE, 16}, /* TODO */
    {0,                           VA_FOURCC_BGRP, VA_RT_FORMAT_RGBP,   FALSE, 16}, /* TODO */
    {0,                           VA_FOURCC_411R, 0,                   FALSE, 12}, /* TODO */
};

struct vaapi_format *vaapi_lookup_d3dformat( D3DFORMAT d3dformat )
{
    unsigned int i;
    for (i = 0; i < sizeof(format_table)/sizeof(format_table[0]); i++)
    {
        if (format_table[i].d3dformat == d3dformat)
            return &format_table[i];
    }
    return NULL;
}


/* caller has to hold the vaapi_lock */
static BOOL enumerate_vaprofiles( VADisplay va_display )
{
    int numProfiles, numEntryPoints;
    VAEntrypoint *entryPoints = NULL;
    VAProfile *profiles;
    unsigned int i, j;

    for (i = 0; i < sizeof(profile_table)/sizeof(profile_table[0]); i++)
        profile_table[i].supported = FALSE;

    numProfiles    = pvaMaxNumProfiles(va_display);
    numEntryPoints = pvaMaxNumEntrypoints(va_display);

    if (!numProfiles || !numEntryPoints)
        return FALSE;

    profiles = HeapAlloc(GetProcessHeap(), 0, sizeof(VAProfile) * numProfiles);
    if (!profiles)
        return FALSE;

    entryPoints = HeapAlloc(GetProcessHeap(), 0, sizeof(VAEntrypoint) * numEntryPoints);
    if (!entryPoints)
    {
        HeapFree(GetProcessHeap(), 0, profiles);
        return FALSE;
    }

    if (pvaQueryConfigProfiles(va_display, profiles, &numProfiles) != VA_STATUS_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, profiles);
        HeapFree(GetProcessHeap(), 0, entryPoints);
        return FALSE;
    }

    for (i = 0; i < numProfiles; i++)
    {
        if (pvaQueryConfigEntrypoints(va_display, profiles[i], entryPoints, &numEntryPoints) != VA_STATUS_SUCCESS)
            continue;

        for (j = 0; j < numEntryPoints; j++)
        {
            struct vaapi_profile *profile = vaapi_lookup_profile(profiles[i], entryPoints[j]);
            if (profile)
                profile->supported = TRUE;
            else
                WARN("missing decoder translation for format %d/%d\n", profiles[i], entryPoints[i]);
        }
    }

    HeapFree(GetProcessHeap(), 0, profiles);
    HeapFree(GetProcessHeap(), 0, entryPoints);
    return TRUE;
}

/* caller has to hold the vaapi_lock */
BOOL vaapi_is_format_supported( VADisplay va_display, struct vaapi_profile *profile, struct vaapi_format *format )
{
    unsigned int numSurfaceAttribs = 0;
    VASurfaceAttrib *surfaceAttribs;
    VAConfigAttrib attrib;
    VAConfigID config;
    VAStatus status;
    unsigned int i;
    BOOL ret = FALSE;

    attrib.type  = VAConfigAttribRTFormat;
    attrib.value = format->vaformat;
    if (pvaCreateConfig(va_display, profile->profile, profile->entryPoint, &attrib, 1, &config) != VA_STATUS_SUCCESS)
        return FALSE;

    status = pvaQuerySurfaceAttributes(va_display, config, NULL, &numSurfaceAttribs);
    if (status == VA_STATUS_ERROR_UNIMPLEMENTED ||
        status == VA_STATUS_ERROR_INVALID_PARAMETER)
    {
        pvaDestroyConfig(va_display, config);
        return format->fallback;
    }

    if (status == VA_STATUS_SUCCESS && numSurfaceAttribs != 0)
    {
        surfaceAttribs = HeapAlloc(GetProcessHeap(), 0, sizeof(VASurfaceAttrib) * numSurfaceAttribs);
        if (surfaceAttribs)
        {
            if (pvaQuerySurfaceAttributes(va_display, config, surfaceAttribs, &numSurfaceAttribs) == VA_STATUS_SUCCESS)
            {
                for (i = 0; i < numSurfaceAttribs; i++)
                {
                    if (surfaceAttribs[i].type == VASurfaceAttribPixelFormat &&
                        surfaceAttribs[i].value.value.i == format->vafourcc)
                    {
                        ret = TRUE;
                        break;
                    }
                }
            }
            HeapFree(GetProcessHeap(), 0, surfaceAttribs);
        }
    }

    pvaDestroyConfig(va_display, config);
    return ret;
}

/* caller has to hold the vaapi_lock */
BOOL vaapi_create_surfaces( VADisplay va_display, const struct vaapi_format *format, unsigned int width,
                            unsigned int height, VAImage *vaImage, UINT numSurfaces, VASurfaceID **surfaceList )
{
    VASurfaceAttrib surfaceAttrib;
    VAImageFormat imageFormat;
    VASurfaceID *surfaces;
    VAStatus status;

    memset(&imageFormat, 0, sizeof(imageFormat));
    imageFormat.fourcc         = format->vafourcc;
    imageFormat.byte_order     = VA_LSB_FIRST;
    imageFormat.bits_per_pixel = format->bits;

    status = pvaCreateImage(va_display, &imageFormat, width, height, vaImage);
    if (status != VA_STATUS_SUCCESS)
    {
        ERR("failed to create image: %s (0x%x)\n", pvaErrorStr(status), status);
        return FALSE;
    }

    surfaces = HeapAlloc(GetProcessHeap(), 0, sizeof(VASurfaceID) * numSurfaces);
    if (!surfaces)
    {
        pvaDestroyImage(va_display, vaImage->image_id);
        vaImage->image_id = VA_INVALID_ID;
        return FALSE;
    }

    /* TODO: round width/height values ? */
    surfaceAttrib.type  = VASurfaceAttribPixelFormat;
    surfaceAttrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
    surfaceAttrib.value.value.i = format->vafourcc;

    TRACE("format=%x width=%u height=%u num=%u\n", format->d3dformat, width, height, numSurfaces);
    status = pvaCreateSurfaces(va_display, format->vaformat, width, height,
                               surfaces, numSurfaces, &surfaceAttrib, 1);
    if (status != VA_STATUS_SUCCESS)
    {
        /* The vdpau backend doesn't seem to allow setting the surface picture type.
         * We can only hope that it matches the expected one. */
        status = pvaCreateSurfaces(va_display, format->vaformat, width, height,
                                   surfaces, numSurfaces, NULL, 0);
    }

    if (status == VA_STATUS_SUCCESS)
    {
        *surfaceList = surfaces;
        return TRUE;
    }

    ERR("failed to create surfaces: %s (0x%x)\n", pvaErrorStr(status), status);
    HeapFree(GetProcessHeap(), 0, surfaces);
    pvaDestroyImage(va_display, vaImage->image_id);
    vaImage->image_id = VA_INVALID_ID;
    return FALSE;
}

/*****************************************************************************
 * IWineVideoService vaapi interface
 */

static HRESULT WINAPI WineVideoService_QueryInterface( IWineVideoService *iface, REFIID riid, LPVOID *ppv )
{
    WineVideoServiceImpl *This = impl_from_IWineVideoService(iface);
    TRACE("(%p/%p)->(%s, %p)\n", iface, This, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IWineVideoService))
        *ppv = &This->IWineVideoService_iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI WineVideoService_AddRef( IWineVideoService *iface )
{
    WineVideoServiceImpl *This = impl_from_IWineVideoService(iface);
    ULONG refCount = InterlockedIncrement(&This->refCount);

    TRACE("(%p)->() AddRef from %d\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI WineVideoService_Release( IWineVideoService *iface )
{
    WineVideoServiceImpl *This = impl_from_IWineVideoService(iface);
    ULONG refCount;

    vaapi_lock();
    refCount = InterlockedDecrement(&This->refCount);
    if (!refCount)
    {
        assert(vaapi_videoservice == This);
        vaapi_videoservice = NULL;
    }
    vaapi_unlock();

    TRACE("(%p)->() Release from %d\n", This, refCount + 1);

    if (!refCount)
    {
        TRACE("Destroying\n");

        vaapi_lock();
        pvaTerminate(This->va_display);
        vaapi_unlock();

        if (This->x11_display)
            pXCloseDisplay(This->x11_display);
        if (This->x11_handle)
            dlclose(This->x11_handle);
        if (This->drm_fd > 0)
            close(This->drm_fd);
        if (This->va_drm_handle)
            dlclose(This->va_drm_handle);
        if (This->va_x11_handle)
            dlclose(This->va_x11_handle);
        if (This->va_handle)
            dlclose(This->va_handle);

        CoTaskMemFree(This);
    }

    return refCount;
}

static HRESULT WINAPI WineVideoService_GetDecoderDeviceGuids(IWineVideoService *iface, UINT *count, GUID **guids)
{
    WineVideoServiceImpl *This = impl_from_IWineVideoService(iface);
    unsigned int i, numProfiles = 0;
    GUID *buf;

    TRACE("(%p, %p, %p)\n", This, count, guids);

    buf = CoTaskMemAlloc(sizeof(GUID) * (sizeof(profile_table)/sizeof(profile_table[0])));
    if (!buf)
        return E_OUTOFMEMORY;

    for (i = 0; i < sizeof(profile_table)/sizeof(profile_table[0]); i++)
    {
        if (profile_table[i].supported)
            memcpy(&buf[numProfiles++], profile_table[i].guid, sizeof(GUID));
    }

    if (!numProfiles)
    {
        CoTaskMemFree(buf);
        return E_FAIL;
    }

    *count = numProfiles;
    *guids = buf;
    return S_OK;
}

static HRESULT WINAPI WineVideoService_GetDecoderRenderTargets( IWineVideoService *iface, REFGUID guid, UINT *count,
                                                                D3DFORMAT **formats )
{
    WineVideoServiceImpl *This = impl_from_IWineVideoService(iface);
    unsigned int numFormats = 0, i;
    struct vaapi_profile *profile;
    VAConfigAttrib attrib;
    VAStatus status;
    D3DFORMAT *buf;

    TRACE("(%p, %s, %p, %p)\n", This, debugstr_guid(guid), count, formats);

    profile = vaapi_lookup_guid(guid);
    if (!profile || !profile->supported)
        return D3DERR_INVALIDCALL;

    buf = CoTaskMemAlloc(sizeof(D3DFORMAT) * (sizeof(format_table)/sizeof(format_table[0])));
    if (!buf)
        return E_OUTOFMEMORY;

    vaapi_lock();

    attrib.type  = VAConfigAttribRTFormat;
    attrib.value = 0;

    status = pvaGetConfigAttributes(This->va_display, profile->profile, profile->entryPoint, &attrib, 1);
    if (status != VA_STATUS_SUCCESS)
    {
        ERR("failed to get config attributes: %s (0x%x)\n", pvaErrorStr(status), status);
    }
    else
    {
        for (i = 0; i < sizeof(format_table)/sizeof(format_table[0]); i++)
        {
            /* skip TODOs */
            if (!format_table[i].d3dformat || !format_table[i].vafourcc || !format_table[i].vaformat)
                continue;
            /* check if the main format is supported (i.e. RGB, YUV420, ...) */
            if (!(attrib.value & format_table[i].vaformat))
                continue;
            if (vaapi_is_format_supported(This->va_display, profile, &format_table[i]))
                buf[numFormats++] = format_table[i].d3dformat;
        }
    }

    vaapi_unlock();

    if (!numFormats)
    {
        CoTaskMemFree(buf);
        return E_FAIL;
    }

    *count   = numFormats;
    *formats = buf;
    return S_OK;
}

static BOOL is_h264_codec( REFGUID guid )
{
    return (IsEqualGUID(guid, &DXVA2_ModeH264_A) ||
            IsEqualGUID(guid, &DXVA2_ModeH264_B) ||
            IsEqualGUID(guid, &DXVA2_ModeH264_C) ||
            IsEqualGUID(guid, &DXVA2_ModeH264_D) ||
            IsEqualGUID(guid, &DXVA2_ModeH264_E) ||
            IsEqualGUID(guid, &DXVA2_ModeH264_F));
}

static HRESULT WINAPI WineVideoService_GetDecoderConfigurations( IWineVideoService *iface, REFGUID guid,
                                                                 const DXVA2_VideoDesc *videoDesc, IUnknown *reserved,
                                                                 UINT *count, DXVA2_ConfigPictureDecode **configs )
{
    WineVideoServiceImpl *This = impl_from_IWineVideoService(iface);
    DXVA2_ConfigPictureDecode *config;
    const struct vaapi_profile *profile;

    FIXME("(%p/%p)->(%s, %p, %p, %p, %p): semi-stub\n",
          iface, This, debugstr_guid(guid), videoDesc, reserved, count, configs);

    if (!guid || !videoDesc || !count || !configs)
        return E_INVALIDARG;

    profile = vaapi_lookup_guid(guid);
    if (!profile || !profile->supported)
        return D3DERR_INVALIDCALL;

    config = CoTaskMemAlloc(sizeof(*config));
    if (!config)
        return E_OUTOFMEMORY;

    /* TODO: Query decoder instead of using hardcoded values */
    memcpy(&config->guidConfigBitstreamEncryption, &DXVA2_NoEncrypt, sizeof(GUID));
    memcpy(&config->guidConfigMBcontrolEncryption, &DXVA2_NoEncrypt, sizeof(GUID));
    memcpy(&config->guidConfigResidDiffEncryption, &DXVA2_NoEncrypt, sizeof(GUID));

    config->ConfigBitstreamRaw             = 1;
    config->ConfigMBcontrolRasterOrder     = is_h264_codec(guid) ? 0 : 1;
    config->ConfigResidDiffHost            = 0; /* FIXME */
    config->ConfigSpatialResid8            = 0; /* FIXME */
    config->ConfigResid8Subtraction        = 0; /* FIXME */
    config->ConfigSpatialHost8or9Clipping  = 0; /* FIXME */
    config->ConfigSpatialResidInterleaved  = 0; /* FIXME */
    config->ConfigIntraResidUnsigned       = 0; /* FIXME */
    config->ConfigResidDiffAccelerator     = 0;
    config->ConfigHostInverseScan          = 0;
    config->ConfigSpecificIDCT             = 1;
    config->Config4GroupedCoefs            = 0;
    config->ConfigDecoderSpecific          = 0;

    if (IsEqualGUID(guid, &DXVA2_ModeMPEG2_VLD))
        config->ConfigMinRenderTargetBuffCount = 3;
    else if(is_h264_codec(guid))
        config->ConfigMinRenderTargetBuffCount = 16;
    else
    {
        FIXME("ConfigMinRenderTargetBuffCount unknown for codec %s, falling back to 16\n", debugstr_guid(guid));
        config->ConfigMinRenderTargetBuffCount = 16;
    }

    *count   = 1;
    *configs = config;
    return S_OK;
}

static HRESULT WINAPI WineVideoService_CreateVideoDecoder( IWineVideoService *iface, REFGUID guid, const DXVA2_VideoDesc *videoDesc,
                                                           DXVA2_ConfigPictureDecode *config, UINT numSurfaces, IWineVideoDecoder **decoder )
{
    WineVideoServiceImpl *This = impl_from_IWineVideoService(iface);

    FIXME("(%p/%p)->(%s, %p, %p, %u, %p): semi-stub\n",
          iface, This, debugstr_guid(guid), videoDesc, config, numSurfaces, decoder);

    if (IsEqualGUID(guid, &DXVA2_ModeMPEG2_VLD))
        return vaapi_mpeg2decoder_create(iface, videoDesc, config, numSurfaces, decoder);
    if (IsEqualGUID(guid, &DXVA2_ModeH264_E))
        return vaapi_h264decoder_create(iface, videoDesc, config, numSurfaces, decoder);

    return E_FAIL;
}

static const IWineVideoServiceVtbl WineVideoService_VTable =
{
    WineVideoService_QueryInterface,
    WineVideoService_AddRef,
    WineVideoService_Release,
    WineVideoService_GetDecoderDeviceGuids,
    WineVideoService_GetDecoderRenderTargets,
    WineVideoService_GetDecoderConfigurations,
    WineVideoService_CreateVideoDecoder
};

IWineVideoService *vaapi_videoservice_create( void )
{
    WineVideoServiceImpl *videoservice;
    int major, minor;

    if (!config_vaapi_enabled)
    {
        FIXME("Vaapi backend disabled via registry\n");
        return NULL;
    }

    vaapi_lock();

    /* existing vaapi videoservice backend, increment refcount */
    videoservice = vaapi_videoservice;
    if (videoservice)
    {
        WineVideoService_AddRef(&videoservice->IWineVideoService_iface);
        vaapi_unlock();
        return &videoservice->IWineVideoService_iface;
    }

    videoservice = CoTaskMemAlloc(sizeof(*videoservice));
    if (!videoservice)
        goto err;

    videoservice->IWineVideoService_iface.lpVtbl = &WineVideoService_VTable;
    videoservice->refCount      = 1;

    videoservice->va_handle     = NULL;
    videoservice->va_x11_handle = NULL;
    videoservice->va_drm_handle = NULL;
    videoservice->x11_handle    = NULL;

    videoservice->x11_display   = NULL;
    videoservice->va_display    = NULL;
    videoservice->drm_fd        = -1;

    videoservice->va_handle = load_libva();
    if (!videoservice->va_handle)
        goto err;

    if (config_vaapi_drm)
    {
        TRACE("Using VAAPI in DRM mode.\n");

        videoservice->va_drm_handle = load_libva_drm();
        if (!videoservice->va_drm_handle)
            goto err;

        videoservice->drm_fd = open(config_vaapi_drm_path, O_RDWR);
        if (videoservice->drm_fd < 0)
        {
            FIXME("Failed to open device %s\n", config_vaapi_drm_path);
            goto err;
        }

        videoservice->va_display = pvaGetDisplayDRM(videoservice->drm_fd);
        if (!videoservice->va_display)
            goto err;
    }
    else
    {
        TRACE("Using VAAPI in X11 mode.\n");

        videoservice->va_x11_handle = load_libva_x11();
        if (!videoservice->va_x11_handle)
            goto err;

        videoservice->x11_handle = load_libx11();
        if (!videoservice->x11_handle)
            goto err;

        videoservice->x11_display = pXOpenDisplay(NULL);
        if (!videoservice->x11_display)
            goto err;

        videoservice->va_display = pvaGetDisplay(videoservice->x11_display);
        if (!videoservice->va_display)
            goto err;
    }

    if (pvaInitialize(videoservice->va_display, &major, &minor) != VA_STATUS_SUCCESS)
        goto err;

    if (!enumerate_vaprofiles(videoservice->va_display))
        goto err;

    /* remember the interface for future requests */
    vaapi_videoservice = videoservice;
    vaapi_unlock();
    return &videoservice->IWineVideoService_iface;

err:
    if (videoservice->va_display)
        pvaTerminate(videoservice->va_display);

    vaapi_unlock();

    if (videoservice->x11_display)
        pXCloseDisplay(videoservice->x11_display);
    if (videoservice->x11_handle)
        dlclose(videoservice->x11_handle);
    if (videoservice->drm_fd > 0)
        close(videoservice->drm_fd);
    if (videoservice->va_drm_handle)
        dlclose(videoservice->va_drm_handle);
    if (videoservice->va_x11_handle)
        dlclose(videoservice->va_x11_handle);
    if (videoservice->va_handle)
        dlclose(videoservice->va_handle);

    CoTaskMemFree(videoservice);
    return NULL;
}

#else

IWineVideoService *vaapi_videoservice_create( void )
{
    FIXME("Wine compiled without vaapi support, GPU decoding not available.\n");
    return NULL;
}

#endif /* HAVE_VAAPI */

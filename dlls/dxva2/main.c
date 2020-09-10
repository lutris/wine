/*
 * Copyright 2014 Michael Müller for Pipelight
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

#define INITGUID
#include "d3d9.h"
#include "dxva2api.h"
#include "dxva2_private.h"
#include "physicalmonitorenumerationapi.h"
#include "lowlevelmonitorconfigurationapi.h"
#include "highlevelmonitorconfigurationapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxva2);

BOOL config_vaapi_enabled = FALSE;
BOOL config_vaapi_drm = FALSE;
char config_vaapi_drm_path[MAX_PATH] = "";

BOOL WINAPI CapabilitiesRequestAndCapabilitiesReply( HMONITOR monitor, LPSTR buffer, DWORD length )
{
    FIXME("(%p, %p, %d): stub\n", monitor, buffer, length);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

HRESULT WINAPI DXVA2CreateDirect3DDeviceManager9( UINT *resetToken, IDirect3DDeviceManager9 **dxvManager )
{
    TRACE("(%p, %p)\n", resetToken, dxvManager);

    return devicemanager_create( resetToken, (void **)dxvManager );
}

HRESULT WINAPI DXVA2CreateVideoService( IDirect3DDevice9 *device, REFIID riid, void **ppv )
{
    TRACE("(%p, %s, %p)\n", device, debugstr_guid(riid), ppv);

    return videoservice_create( device, riid, ppv );
}

BOOL WINAPI DegaussMonitor( HMONITOR monitor )
{
    FIXME("(%p): stub\n", monitor);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI DestroyPhysicalMonitor( HMONITOR monitor )
{
    FIXME("(%p): stub\n", monitor);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI DestroyPhysicalMonitors( DWORD arraySize, LPPHYSICAL_MONITOR array )
{
    FIXME("(0x%x, %p): stub\n", arraySize, array);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetCapabilitiesStringLength( HMONITOR monitor, LPDWORD length )
{
    FIXME("(%p, %p): stub\n", monitor, length);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorBrightness( HMONITOR monitor, LPDWORD minimum, LPDWORD current, LPDWORD maximum )
{
    FIXME("(%p, %p, %p, %p): stub\n", monitor, minimum, current, maximum);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorCapabilities( HMONITOR monitor, LPDWORD capabilities, LPDWORD temperatures )
{
    FIXME("(%p, %p, %p): stub\n", monitor, capabilities, temperatures);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOL WINAPI GetMonitorColorTemperature( HMONITOR monitor, LPMC_COLOR_TEMPERATURE temperature )
{
    FIXME("(%p, %p): stub\n", monitor, temperature);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorContrast( HMONITOR monitor, LPDWORD minimum, LPDWORD current, LPDWORD maximum )
{
    FIXME("(%p, %p, %p, %p): stub\n", monitor, minimum, current, maximum);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorDisplayAreaPosition( HMONITOR monitor, MC_POSITION_TYPE type, LPDWORD minimum,
                                           LPDWORD current, LPDWORD maximum )
{
    FIXME("(%p, 0x%x, %p, %p, %p): stub\n", monitor, type, minimum, current, maximum);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorDisplayAreaSize( HMONITOR monitor, MC_SIZE_TYPE type, LPDWORD minimum,
                                       LPDWORD current, LPDWORD maximum )
{
    FIXME("(%p, 0x%x, %p, %p, %p): stub\n", monitor, type, minimum, current, maximum);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorRedGreenOrBlueDrive( HMONITOR monitor, MC_DRIVE_TYPE type, LPDWORD minimum,
                                           LPDWORD current, LPDWORD maximum )
{
    FIXME("(%p, 0x%x, %p, %p, %p): stub\n", monitor, type, minimum, current, maximum);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorRedGreenOrBlueGain( HMONITOR monitor, MC_GAIN_TYPE type, LPDWORD minimum,
                                          LPDWORD current, LPDWORD maximum )
{
    FIXME("(%p, 0x%x, %p, %p, %p): stub\n", monitor, type, minimum, current, maximum);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorTechnologyType( HMONITOR monitor, LPMC_DISPLAY_TECHNOLOGY_TYPE type )
{
    FIXME("(%p, %p): stub\n", monitor, type);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetNumberOfPhysicalMonitorsFromHMONITOR( HMONITOR monitor, LPDWORD number )
{
    FIXME("(%p, %p): stub\n", monitor, number);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

HRESULT WINAPI GetNumberOfPhysicalMonitorsFromIDirect3DDevice9( IDirect3DDevice9 *device, LPDWORD number )
{
    FIXME("(%p, %p): stub\n", device, number);

    return E_NOTIMPL;
}

BOOL WINAPI GetPhysicalMonitorsFromHMONITOR( HMONITOR monitor, DWORD arraySize, LPPHYSICAL_MONITOR array )
{
    FIXME("(%p, 0x%x, %p): stub\n", monitor, arraySize, array);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

HRESULT WINAPI GetPhysicalMonitorsFromIDirect3DDevice9( IDirect3DDevice9 *device, DWORD arraySize, LPPHYSICAL_MONITOR array )
{
    FIXME("(%p, 0x%x, %p): stub\n", device, arraySize, array);

    return E_NOTIMPL;
}

BOOL WINAPI GetTimingReport( HMONITOR monitor, LPMC_TIMING_REPORT timingReport )
{
    FIXME("(%p, %p): stub\n", monitor, timingReport);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetVCPFeatureAndVCPFeatureReply( HMONITOR monitor, BYTE vcpCode, LPMC_VCP_CODE_TYPE pvct,
                                             LPDWORD current, LPDWORD maximum )
{
    FIXME("(%p, 0x%02x, %p, %p, %p): stub\n", monitor, vcpCode, pvct, current, maximum);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

HRESULT WINAPI OPMGetVideoOutputsFromHMONITOR( HMONITOR monitor, /* OPM_VIDEO_OUTPUT_SEMANTICS */ int vos,
                                               ULONG *numVideoOutputs, /* IOPMVideoOutput */ void ***videoOutputs )
{
    FIXME("(%p, 0x%x, %p, %p): stub\n", monitor, vos, numVideoOutputs, videoOutputs);

    return E_NOTIMPL;
}

HRESULT WINAPI OPMGetVideoOutputsFromIDirect3DDevice9Object( IDirect3DDevice9 *device, /* OPM_VIDEO_OUTPUT_SEMANTICS */ int vos,
                                                             ULONG *numVideoOutputs,  /* IOPMVideoOutput */ void ***videoOutputs )
{
    FIXME("(%p, 0x%x, %p, %p): stub\n", device, vos, numVideoOutputs, videoOutputs);

    return E_NOTIMPL;
}

BOOL WINAPI RestoreMonitorFactoryColorDefaults( HMONITOR monitor )
{
    FIXME("(%p): stub\n", monitor);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI RestoreMonitorFactoryDefaults( HMONITOR monitor )
{
    FIXME("(%p): stub\n", monitor);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SaveCurrentMonitorSettings( HMONITOR monitor )
{
    FIXME("(%p): stub\n", monitor);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SaveCurrentSettings( HMONITOR monitor )
{
    FIXME("(%p): stub\n", monitor);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetMonitorBrightness( HMONITOR monitor, DWORD brightness )
{
    FIXME("(%p, 0x%x): stub\n", monitor, brightness);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetMonitorColorTemperature( HMONITOR monitor, MC_COLOR_TEMPERATURE temperature )
{
    FIXME("(%p, 0x%x): stub\n", monitor, temperature);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetMonitorContrast( HMONITOR monitor, DWORD contrast )
{
    FIXME("(%p, 0x%x): stub\n", monitor, contrast);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetMonitorDisplayAreaPosition( HMONITOR monitor, MC_POSITION_TYPE type, DWORD position )
{
    FIXME("(%p, 0x%x, 0x%x): stub\n", monitor, type, position);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetMonitorDisplayAreaSize( HMONITOR monitor, MC_SIZE_TYPE type, DWORD size )
{
    FIXME("(%p, 0x%x, 0x%x): stub\n", monitor, type, size);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetMonitorRedGreenOrBlueDrive( HMONITOR monitor, MC_DRIVE_TYPE type, DWORD drive )
{
    FIXME("(%p, 0x%x, 0x%x): stub\n", monitor, type, drive);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetMonitorRedGreenOrBlueGain( HMONITOR monitor, MC_GAIN_TYPE type, DWORD gain )
{
    FIXME("(%p, 0x%x, 0x%x): stub\n", monitor, type, gain);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetVCPFeature( HMONITOR monitor, BYTE vcpCode, DWORD value )
{
    FIXME("(%p, 0x%02x, 0x%x): stub\n", monitor, vcpCode, value);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

static BOOL get_app_key( HKEY *defkey, HKEY *appkey )
{
    char buffer[MAX_PATH+16];
    DWORD len;

    *appkey = 0;

    /* @@ Wine registry key: HKCU\Software\Wine\DXVA2 */
    if (RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\DXVA2", defkey))
        *defkey = 0;

    len = GetModuleFileNameA(0, buffer, MAX_PATH);
    if (len && len < MAX_PATH)
    {
        HKEY tmpkey;

        /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\DXVA2 */
        if (!RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\AppDefaults", &tmpkey))
        {
            char *p, *appname = buffer;
            if ((p = strrchr(appname, '/'))) appname = p + 1;
            if ((p = strrchr(appname, '\\'))) appname = p + 1;
            strcat(appname, "\\DXVA2");

            if (RegOpenKeyA(tmpkey, appname, appkey)) *appkey = 0;
            RegCloseKey(tmpkey);
        }
    }

    return *defkey || *appkey;
}

static BOOL get_config_key( HKEY defkey, HKEY appkey, const char *name, char *buffer, DWORD size )
{
    if (appkey && !RegQueryValueExA( appkey, name, 0, NULL, (LPBYTE)buffer, &size ))
        return TRUE;

    if (defkey && !RegQueryValueExA( defkey, name, 0, NULL, (LPBYTE)buffer, &size ))
        return TRUE;

    return FALSE;
}

static void dxva2_init( void )
{
    HKEY defkey, appkey;
    char buffer[MAX_PATH];

    if (!get_app_key(&defkey, &appkey))
        return;

    if (get_config_key(defkey, appkey, "backend", buffer, sizeof(buffer)))
        config_vaapi_enabled = !strcmp(buffer, "va");

    if (get_config_key(defkey, appkey, "va_mode", buffer, sizeof(buffer)))
        config_vaapi_drm = !strcmp(buffer, "drm");

    if (!get_config_key(defkey, appkey, "va_drm_device", config_vaapi_drm_path, sizeof(config_vaapi_drm_path)))
        strcpy(config_vaapi_drm_path, "/dev/dri/card0");

    if (defkey) RegCloseKey(defkey);
    if (appkey) RegCloseKey(appkey);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%x,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            dxva2_init();
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

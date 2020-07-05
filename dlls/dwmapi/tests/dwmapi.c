/*
 * Unit tests for dwmapi
 *
 * Copyright 2018 Louis Lenders
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
 *
 */

#include "dwmapi.h"

#include "wine/test.h"

static HRESULT (WINAPI *pDwmIsCompositionEnabled)(BOOL*);
static HRESULT (WINAPI *pDwmEnableComposition)(UINT);
static HRESULT (WINAPI *pDwmGetTransportAttributes)(BOOL*,BOOL*,DWORD*);

BOOL dwmenabled;

static void test_isdwmenabled(void)
{
    HRESULT res;
    BOOL ret;

    ret = -1;
    res = pDwmIsCompositionEnabled(&ret);
    ok((res == S_OK && ret == TRUE) || (res == S_OK && ret == FALSE), "got %x and %d\n", res, ret);

    if (res == S_OK && ret == TRUE)
        dwmenabled = TRUE;
    else
        dwmenabled = FALSE;
    /*tested on win7 by enabling/disabling DWM service via services.msc*/
    if (dwmenabled)
    {
        res = pDwmEnableComposition(DWM_EC_DISABLECOMPOSITION); /* try disable and reenable dwm*/
        ok(res == S_OK, "got %x expected S_OK\n", res);

        ret = -1;
        res = pDwmIsCompositionEnabled(&ret);
        ok((res == S_OK && ret == FALSE) /*wvista win7*/ || (res == S_OK && ret == TRUE) /*>win7*/, "got %x and %d\n", res, ret);

        res = pDwmEnableComposition(DWM_EC_ENABLECOMPOSITION);
        ok(res == S_OK, "got %x\n", res);

        ret = -1;
        res = pDwmIsCompositionEnabled(&ret);
        todo_wine ok(res == S_OK && ret == TRUE, "got %x and %d\n", res, ret);
    }
    else
    {
        res = pDwmEnableComposition(DWM_EC_ENABLECOMPOSITION); /*cannot enable DWM composition this way*/
        ok(res == S_OK /*win7 testbot*/ || res == DWM_E_COMPOSITIONDISABLED /*win7 laptop*/, "got %x\n", res);
        if (winetest_debug > 1)
            trace("returning %x\n", res);

        ret = -1;
        res = pDwmIsCompositionEnabled(&ret);
        ok(res == S_OK && ret == FALSE, "got %x  and %d\n", res, ret);
    }
}

static void test_dwm_get_transport_attributes(void)
{
    BOOL isremoting, isconnected;
    DWORD generation;
    HRESULT res;

    res = pDwmGetTransportAttributes(&isremoting, &isconnected, &generation);
    if (dwmenabled)
        ok(res == S_OK, "got %x\n", res);
    else
    {
        ok(res == S_OK /*win7 testbot*/ || res == DWM_E_COMPOSITIONDISABLED /*win7 laptop*/, "got %x\n", res);
        if (winetest_debug > 1)
            trace("returning %x\n", res);
    }
}

START_TEST(dwmapi)
{
    HMODULE hmod = LoadLibraryA("dwmapi.dll");

    if (!hmod)
    {
        trace("dwmapi not found, skipping tests\n");
        return;
    }

    pDwmIsCompositionEnabled = (void *)GetProcAddress(hmod, "DwmIsCompositionEnabled");
    pDwmEnableComposition = (void *)GetProcAddress(hmod, "DwmEnableComposition");
    pDwmGetTransportAttributes = (void *)GetProcAddress(hmod, "DwmGetTransportAttributes");

    test_isdwmenabled();
    test_dwm_get_transport_attributes();
}

/*
 * Copyright 2016 Michael Müller
 * Copyright 2017 Jacek Caban for CodeWeavers
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
#include "uiautomation.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uiautomation);

static HRESULT WINAPI dummy_QueryInterface(IUnknown *iface, REFIID iid, void **ppv)
{
    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (!IsEqualIID(&IID_IUnknown, iid))
    {
        FIXME("Unknown interface: %s\n", debugstr_guid(iid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    *ppv = iface;
    IUnknown_AddRef((IUnknown *)*ppv);
    return S_OK;
}

static ULONG WINAPI dummy_AddRef(IUnknown *iface)
{
    FIXME("(%p): stub\n", iface);
    return 1;
}

static ULONG WINAPI dummy_Release(IUnknown *iface)
{
    FIXME("(%p): stub\n", iface);
    return 1;
}

static const IUnknownVtbl dummy_Vtbl =
{
    dummy_QueryInterface,
    dummy_AddRef,
    dummy_Release,
};

static IUnknown dummy = { &dummy_Vtbl };

/***********************************************************************
 *          UiaClientsAreListening (uiautomationcore.@)
 */
BOOL WINAPI UiaClientsAreListening(void)
{
    FIXME("(): stub\n");
    return FALSE;
}

/***********************************************************************
 *          UiaGetReservedMixedAttributeValue (uiautomationcore.@)
 */
HRESULT WINAPI UiaGetReservedMixedAttributeValue(IUnknown **value)
{
    FIXME("(%p): stub!\n", value);
    *value = &dummy;
    return S_OK;
}

/***********************************************************************
 *          UiaGetReservedNotSupportedValue (uiautomationcore.@)
 */
HRESULT WINAPI UiaGetReservedNotSupportedValue(IUnknown **value)
{
    FIXME("(%p): stub!\n", value);
    *value = &dummy;
    return S_OK;
}

/***********************************************************************
 *          UiaLookupId (uiautomationcore.@)
 */
int WINAPI UiaLookupId(enum AutomationIdentifierType type, const GUID *guid)
{
    FIXME("(%d, %s) stub!\n", type, debugstr_guid(guid));
    return 1;
}

/***********************************************************************
 *          UiaReturnRawElementProvider (uiautomationcore.@)
 */
LRESULT WINAPI UiaReturnRawElementProvider(HWND hwnd, WPARAM wParam,
        LPARAM lParam, IRawElementProviderSimple *elprov)
{
    FIXME("(%p, %lx, %lx, %p): stub!\n", hwnd, wParam, lParam, elprov);
    return 0;
}

/***********************************************************************
 *          UiaRaiseAutomationEvent (uiautomationcore.@)
 */
HRESULT WINAPI UiaRaiseAutomationEvent(IRawElementProviderSimple *provider, EVENTID id)
{
    FIXME("(%p, %d): stub\n", provider, id);
    return S_OK;
}

void WINAPI UiaRegisterProviderCallback(UiaProviderCallback *callback)
{
    FIXME("(%p): stub\n", callback);
}

HRESULT WINAPI UiaHostProviderFromHwnd(HWND hwnd, IRawElementProviderSimple **provider)
{
    FIXME("(%p, %p): stub\n", hwnd, provider);
    return E_NOTIMPL;
}

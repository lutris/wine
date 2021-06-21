#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winstring.h"
#include "wine/debug.h"
#include "wine/heap.h"

#include "objbase.h"
#include "initguid.h"

#include "activation.h"
#define WIDL_using_Windows_Networking_Connectivity
#define WIDL_using_Windows_Foundation_Collections
#include "windows.networking.connectivity.h"

WINE_DEFAULT_DEBUG_CHANNEL(network);

static const char *debugstr_hstring(HSTRING hstr)
{
    const WCHAR *str;
    UINT32 len;
    if (hstr && !((ULONG_PTR)hstr >> 16)) return "(invalid)";
    str = WindowsGetStringRawBuffer(hstr, &len);
    return wine_dbgstr_wn(str, len);
}

struct windows_networking_connectivity
{
    IActivationFactory IActivationFactory_iface;
    INetworkInformationStatics INetworkInformationStatics_iface;
    IVectorView_ConnectionProfile IVectorView_iface;
    LONG refcount;
};

static inline struct windows_networking_connectivity *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct windows_networking_connectivity, IActivationFactory_iface);
}

static inline struct windows_networking_connectivity *impl_from_INetworkInformationStatics(INetworkInformationStatics *iface)
{
    return CONTAINING_RECORD(iface, struct windows_networking_connectivity, INetworkInformationStatics_iface);
}

static inline struct windows_networking_connectivity *impl_from_IVectorView_ConnectionProfile(IVectorView_ConnectionProfile *iface)
{
    return CONTAINING_RECORD(iface, struct windows_networking_connectivity, IVectorView_iface);
}

static HRESULT STDMETHODCALLTYPE vector_view_QueryInterface(IVectorView_ConnectionProfile *iface, REFIID iid, void **object)
{
    TRACE("iface %p, iid %s, object %p stub!\n", iface, debugstr_guid(iid), object);
    IUnknown_AddRef(iface);
    *object = iface;
    return S_OK;
}

static ULONG STDMETHODCALLTYPE vector_view_AddRef(IVectorView_ConnectionProfile *iface)
{
    struct windows_networking_connectivity *impl = impl_from_IVectorView_ConnectionProfile(iface);
    ULONG rc = InterlockedIncrement(&impl->refcount);
    TRACE("%p increasing refcount to %u.\n", impl, rc);
    return rc;
}

static ULONG STDMETHODCALLTYPE vector_view_Release(IVectorView_ConnectionProfile *iface)
{
    struct windows_networking_connectivity *impl = impl_from_IVectorView_ConnectionProfile(iface);
    ULONG rc = InterlockedDecrement(&impl->refcount);
    TRACE("%p decreasing refcount to %u.\n", impl, rc);
    return rc;
}

static HRESULT STDMETHODCALLTYPE vector_view_GetIids(IVectorView_ConnectionProfile *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE vector_view_GetRuntimeClassName(IVectorView_ConnectionProfile *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE vector_view_GetTrustLevel(IVectorView_ConnectionProfile *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE vector_view_GetAt(IVectorView_ConnectionProfile *iface, ULONG index, IConnectionProfile **out_value)
{
    FIXME("iface %p, index %#x, out_value %p stub!\n", iface, index, out_value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE vector_view_get_Size(
    IVectorView_ConnectionProfile *iface, ULONG *out_value)
{
    FIXME("iface %p, out_value %p stub!\n", iface, out_value);
    *out_value = 0;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE vector_view_IndexOf(
    IVectorView_ConnectionProfile *iface, IConnectionProfile *value, ULONG *index, BOOLEAN *out_value)
{
    FIXME("iface %p, value %p, index %p, out_value %p stub!\n", iface, value, index, out_value);
    *out_value = FALSE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE vector_view_GetMany(
    IVectorView_ConnectionProfile *iface, UINT32 start_index, UINT32 size, IConnectionProfile **items, UINT32 *out_value)
{
    FIXME("iface %p, start_index %#x, items %p, out_value %p stub!\n", iface, start_index, items, out_value);
    *out_value = 0;
    return S_OK;
}

static const struct IVectorView_ConnectionProfileVtbl vector_view_vtbl =
{
    vector_view_QueryInterface,
    vector_view_AddRef,
    vector_view_Release,
    vector_view_GetIids,
    vector_view_GetRuntimeClassName,
    vector_view_GetTrustLevel,
    vector_view_GetAt,
    vector_view_get_Size,
    vector_view_IndexOf,
    vector_view_GetMany
};

static HRESULT STDMETHODCALLTYPE network_information_statics_QueryInterface(
        INetworkInformationStatics *iface, REFIID iid, void **object)
{
    TRACE("iface %p, iid %s, object %p stub!\n", iface, debugstr_guid(iid), object);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_INetworkInformationStatics))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE network_information_statics_AddRef(
        INetworkInformationStatics *iface)
{
    struct windows_networking_connectivity *impl = impl_from_INetworkInformationStatics(iface);
    ULONG rc = InterlockedIncrement(&impl->refcount);
    TRACE("%p increasing refcount to %u.\n", impl, rc);
    return rc;
}

static ULONG STDMETHODCALLTYPE network_information_statics_Release(
        INetworkInformationStatics *iface)
{
    struct windows_networking_connectivity *impl = impl_from_INetworkInformationStatics(iface);
    ULONG rc = InterlockedDecrement(&impl->refcount);
    TRACE("%p decreasing refcount to %u.\n", impl, rc);
    return rc;
}

static HRESULT STDMETHODCALLTYPE network_information_statics_GetIids(
        INetworkInformationStatics *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE network_information_statics_GetRuntimeClassName(
        INetworkInformationStatics *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE network_information_statics_GetTrustLevel(
        INetworkInformationStatics *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE network_information_statics_GetConnectionProfiles(INetworkInformationStatics *iface, __FIVectorView_1_Windows__CNetworking__CConnectivity__CConnectionProfile **value)
{
    struct windows_networking_connectivity *impl = impl_from_INetworkInformationStatics(iface);
    FIXME("iface %p, %p stub!\n", iface, value);
    *value = &impl->IVectorView_iface;
    return S_OK;
}

struct connection_profile
{
    IConnectionProfile IConnectionProfile_iface;
    LONG ref;
};

static inline struct connection_profile *impl_from_IConnectionProfile(IConnectionProfile *iface)
{
    return CONTAINING_RECORD(iface, struct connection_profile, IConnectionProfile_iface);
}

static HRESULT WINAPI connection_profile_QueryInterface(IConnectionProfile *iface, REFIID riid, void **object)
{
    TRACE("iface %p, iid %s, object %p stub!\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IInspectable) ||
        IsEqualGUID(riid, &IID_IConnectionProfile))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));
    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI connection_profile_AddRef(IConnectionProfile *iface)
{
    struct connection_profile *impl = impl_from_IConnectionProfile(iface);
    ULONG rc = InterlockedIncrement(&impl->ref);
    TRACE("%p increasing refcount to %u.\n", impl, rc);
    return rc;
}

static ULONG WINAPI connection_profile_Release(IConnectionProfile *iface)
{
    struct connection_profile *impl = impl_from_IConnectionProfile(iface);
    ULONG rc = InterlockedIncrement(&impl->ref);
    TRACE("%p increasing refcount to %u.\n", impl, rc);
    return rc;
}

static HRESULT WINAPI connection_profile_GetIids(IConnectionProfile *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_profile_GetRuntimeClassName(IConnectionProfile *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_profile_GetTrustLevel(IConnectionProfile *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_profile_get_ProfileName(IConnectionProfile *iface, HSTRING *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_profile_GetNetworkConnectivityLevel(IConnectionProfile *iface,
        enum __x_ABI_CWindows_CNetworking_CConnectivity_CNetworkConnectivityLevel *value)
{
    struct connection_profile *impl = impl_from_IConnectionProfile(iface);
    FIXME("iface %p, value %p stub!\n", impl, value);
    *value = NetworkConnectivityLevel_InternetAccess;
    return S_OK;
}

static HRESULT WINAPI connection_profile_GetNetworkNames(IConnectionProfile *iface, __FIVectorView_1_HSTRING **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_profile_GetConnectionCost(IConnectionProfile *iface,
        __x_ABI_CWindows_CNetworking_CConnectivity_CIConnectionCost **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_profile_GetDataPlanStatus(IConnectionProfile *iface,
        __x_ABI_CWindows_CNetworking_CConnectivity_CIDataPlanStatus **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_profile_get_NetworkAdapter(IConnectionProfile *iface,
        __x_ABI_CWindows_CNetworking_CConnectivity_CINetworkAdapter **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_profile_GetLocalUsage(IConnectionProfile *iface,
        struct __x_ABI_CWindows_CFoundation_CDateTime start, struct __x_ABI_CWindows_CFoundation_CDateTime end,
        __x_ABI_CWindows_CNetworking_CConnectivity_CIDataUsage **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_profile_GetLocalUsagePerRoamingStates(IConnectionProfile *iface,
        struct __x_ABI_CWindows_CFoundation_CDateTime start, struct __x_ABI_CWindows_CFoundation_CDateTime end,
        enum __x_ABI_CWindows_CNetworking_CConnectivity_CRoamingStates states,
        __x_ABI_CWindows_CNetworking_CConnectivity_CIDataUsage **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI connection_profile_get_NetworkSecuritySettings(IConnectionProfile *iface,
        __x_ABI_CWindows_CNetworking_CConnectivity_CINetworkSecuritySettings **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

struct __x_ABI_CWindows_CNetworking_CConnectivity_CIConnectionProfileVtbl connection_vtbl =
{
    connection_profile_QueryInterface,
    connection_profile_AddRef,
    connection_profile_Release,
    connection_profile_GetIids,
    connection_profile_GetRuntimeClassName,
    connection_profile_GetTrustLevel,
    connection_profile_get_ProfileName,
    connection_profile_GetNetworkConnectivityLevel,
    connection_profile_GetNetworkNames,
    connection_profile_GetConnectionCost,
    connection_profile_GetDataPlanStatus,
    connection_profile_get_NetworkAdapter,
    connection_profile_GetLocalUsage,
    connection_profile_GetLocalUsagePerRoamingStates,
    connection_profile_get_NetworkSecuritySettings
};

static HRESULT STDMETHODCALLTYPE network_information_statics_GetInternetConnectionProfile(INetworkInformationStatics *iface, IConnectionProfile **value)
{
    struct windows_networking_connectivity *impl = impl_from_INetworkInformationStatics(iface);
    struct connection_profile *profile;

    FIXME("iface %p, %p stub!\n", impl, value);

    profile = heap_alloc(sizeof(struct connection_profile));
    if (!profile)
        return E_OUTOFMEMORY;

    profile->IConnectionProfile_iface.lpVtbl = &connection_vtbl;
    profile->ref = 1;

    *value = &profile->IConnectionProfile_iface;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE network_information_statics_GetLanIdentifiers(INetworkInformationStatics *iface, __FIVectorView_1_Windows__CNetworking__CConnectivity__CLanIdentifier **value)
{
    FIXME("iface %p, %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE network_information_statics_GetHostNames(INetworkInformationStatics *iface, DWORD **value)
{
    FIXME("iface %p, %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE network_information_statics_GetProxyConfigurationAsync(INetworkInformationStatics *iface, char *name, DWORD **value)
{
    FIXME("iface %p, %p, %p stub!\n", iface, name, value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE network_information_statics_GetSortedEndpointPairs(INetworkInformationStatics *iface, DWORD* destinationList, DWORD sortOptions, DWORD **value)
{
    FIXME("iface %p, %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE network_information_statics_eventadd_NetworkStatusChanged(
    INetworkInformationStatics *iface, __x_ABI_CWindows_CNetworking_CConnectivity_CINetworkStatusChangedEventHandler *value, EventRegistrationToken* token)
{
    FIXME("iface %p, value %p, token %p stub!\n", iface, value, token);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE network_information_statics_eventremove_NetworkStatusChanged(
    INetworkInformationStatics *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %#I64x stub!\n", iface, token.value);
    return S_OK;
}

static const struct INetworkInformationStaticsVtbl network_information_statics_vtbl =
{
    network_information_statics_QueryInterface,
    network_information_statics_AddRef,
    network_information_statics_Release,
    /* IInspectable methods */
    network_information_statics_GetIids,
    network_information_statics_GetRuntimeClassName,
    network_information_statics_GetTrustLevel,
    /* INetworkInformationStatics methods */
    network_information_statics_GetConnectionProfiles,
    network_information_statics_GetInternetConnectionProfile,
    network_information_statics_GetLanIdentifiers,
    network_information_statics_GetHostNames,
    network_information_statics_GetProxyConfigurationAsync,
    network_information_statics_GetSortedEndpointPairs,
    network_information_statics_eventadd_NetworkStatusChanged,
    network_information_statics_eventremove_NetworkStatusChanged,
};

static HRESULT STDMETHODCALLTYPE windows_networking_connectivity_QueryInterface(
        IActivationFactory *iface, REFIID iid, void **object)
{
    struct windows_networking_connectivity *impl = impl_from_IActivationFactory(iface);
    TRACE("iface %p, iid %s, object %p stub!\n", iface, debugstr_guid(iid), object);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IUnknown_AddRef(iface);
        *object = &impl->INetworkInformationStatics_iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_INetworkInformationStatics))
    {
        IUnknown_AddRef(iface);
        *object = &impl->INetworkInformationStatics_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE windows_networking_connectivity_AddRef(
        IActivationFactory *iface)
{
    struct windows_networking_connectivity *impl = impl_from_IActivationFactory(iface);
    ULONG rc = InterlockedIncrement(&impl->refcount);
    TRACE("%p increasing refcount to %u.\n", impl, rc);
    return rc;
}

static ULONG STDMETHODCALLTYPE windows_networking_connectivity_Release(
        IActivationFactory *iface)
{
    struct windows_networking_connectivity *impl = impl_from_IActivationFactory(iface);
    ULONG rc = InterlockedDecrement(&impl->refcount);
    TRACE("%p decreasing refcount to %u.\n", impl, rc);
    return rc;
}

static HRESULT STDMETHODCALLTYPE windows_networking_connectivity_GetIids(
        IActivationFactory *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE windows_networking_connectivity_GetRuntimeClassName(
        IActivationFactory *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE windows_networking_connectivity_GetTrustLevel(
        IActivationFactory *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE windows_networking_connectivity_ActivateInstance(
        IActivationFactory *iface, IInspectable **instance)
{
    FIXME("iface %p, instance %p stub!\n", iface, instance);
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl activation_factory_vtbl =
{
    windows_networking_connectivity_QueryInterface,
    windows_networking_connectivity_AddRef,
    windows_networking_connectivity_Release,
    /* IInspectable methods */
    windows_networking_connectivity_GetIids,
    windows_networking_connectivity_GetRuntimeClassName,
    windows_networking_connectivity_GetTrustLevel,
    /* IActivationFactory methods */
    windows_networking_connectivity_ActivateInstance,
};

static struct windows_networking_connectivity windows_networking_connectivity =
{
    {&activation_factory_vtbl},
    {&network_information_statics_vtbl},
    {&vector_view_vtbl},
    1
};

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, LPVOID *object)
{
    FIXME("clsid %s, riid %s, object %p stub!\n", debugstr_guid(clsid), debugstr_guid(riid), object);
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory(HSTRING classid, IActivationFactory **factory)
{
    TRACE("classid %s, factory %p.\n", debugstr_hstring(classid), factory);
    *factory = &windows_networking_connectivity.IActivationFactory_iface;
    IUnknown_AddRef(*factory);
    return S_OK;
}

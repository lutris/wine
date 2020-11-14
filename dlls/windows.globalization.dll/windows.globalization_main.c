#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winstring.h"
#include "wine/debug.h"
#include "objbase.h"

#include "initguid.h"
#include "activation.h"

#define WIDL_USING_IVECTORVIEW_1_HSTRING
#define WIDL_USING_WINDOWS_GLOBALIZATION_DAYOFWEEK
#define WIDL_USING_WINDOWS_SYSTEM_USERPROFILE_IGLOBALIZATIONPREFERENCESSTATICS
#include "windows.foundation.h"
#include "windows.globalization.h"
#include "windows.system.userprofile.h"

WINE_DEFAULT_DEBUG_CHANNEL(locale);

static const char *debugstr_hstring(HSTRING hstr)
{
    const WCHAR *str;
    UINT32 len;
    if (hstr && !((ULONG_PTR)hstr >> 16)) return "(invalid)";
    str = WindowsGetStringRawBuffer(hstr, &len);
    return wine_dbgstr_wn(str, len);
}

struct hstring_vector
{
    IVectorView_HSTRING IVectorView_HSTRING_iface;
    LONG ref;

    ULONG count;
    HSTRING values[0];
};

static inline struct hstring_vector *impl_from_IVectorView_HSTRING(IVectorView_HSTRING *iface)
{
    return CONTAINING_RECORD(iface, struct hstring_vector, IVectorView_HSTRING_iface);
}

static HRESULT STDMETHODCALLTYPE hstring_vector_QueryInterface(
        IVectorView_HSTRING *iface, REFIID iid, void **out)
{
    struct hstring_vector *This = impl_from_IVectorView_HSTRING(iface);

    FIXME("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (!out) return E_INVALIDARG;

    *out = NULL;
    if (!IsEqualIID(&IID_IUnknown, iid) &&
        !IsEqualIID(&IID_IInspectable, iid) &&
        !IsEqualIID(&IID_IVectorView_HSTRING, iid))
    {
        FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    *out = &This->IVectorView_HSTRING_iface;
    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG STDMETHODCALLTYPE hstring_vector_AddRef(
        IVectorView_HSTRING *iface)
{
    struct hstring_vector *This = impl_from_IVectorView_HSTRING(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    FIXME("iface %p -> ref %u.\n", iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE hstring_vector_Release(
        IVectorView_HSTRING *iface)
{
    struct hstring_vector *This = impl_from_IVectorView_HSTRING(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    FIXME("iface %p -> ref %u.\n", iface, ref);
    if (ref == 0)
    {
        while (This->count--) WindowsDeleteString(This->values[This->count]);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static HRESULT STDMETHODCALLTYPE hstring_vector_GetIids(
        IVectorView_HSTRING *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE hstring_vector_GetRuntimeClassName(
        IVectorView_HSTRING *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE hstring_vector_GetTrustLevel(
        IVectorView_HSTRING *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE hstring_vector_GetAt(
    IVectorView_HSTRING *iface, ULONG index, HSTRING *value)
{
    struct hstring_vector *This = impl_from_IVectorView_HSTRING(iface);

    FIXME("iface %p, index %#x, value %p stub!\n", iface, index, value);

    if (index >= This->count) return E_BOUNDS;
    return WindowsDuplicateString(This->values[index], value);
}

static HRESULT STDMETHODCALLTYPE hstring_vector_get_Size(
    IVectorView_HSTRING *iface, ULONG *value)
{
    struct hstring_vector *This = impl_from_IVectorView_HSTRING(iface);

    FIXME("iface %p, value %p stub!\n", iface, value);

    *value = This->count;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE hstring_vector_IndexOf(
    IVectorView_HSTRING *iface, HSTRING element, ULONG *index, BOOLEAN *value)
{
    FIXME("iface %p, element %p, index %p, value %p stub!\n", iface, element, index, value);
    *value = FALSE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE hstring_vector_GetMany(
    IVectorView_HSTRING *iface, ULONG start_index, HSTRING *items, UINT *count)
{
    struct hstring_vector *This = impl_from_IVectorView_HSTRING(iface);
    HRESULT hr;
    ULONG i;

    FIXME("iface %p, start_index %#x, items %p, count %p stub!\n", iface, start_index, items, count);

    for (i = start_index; i < This->count; ++i)
        if (FAILED(hr = WindowsDuplicateString(This->values[i], items + i - start_index)))
            return hr;
    *count = This->count - start_index;

    return S_OK;
}

static const struct IVectorView_HSTRINGVtbl hstring_vector_vtbl =
{
    hstring_vector_QueryInterface,
    hstring_vector_AddRef,
    hstring_vector_Release,
    /* IInspectable methods */
    hstring_vector_GetIids,
    hstring_vector_GetRuntimeClassName,
    hstring_vector_GetTrustLevel,
    /* IVectorView<HSTRING> methods */
    hstring_vector_GetAt,
    hstring_vector_get_Size,
    hstring_vector_IndexOf,
    hstring_vector_GetMany,
};

static HRESULT hstring_vector_create(HSTRING *values, SIZE_T count, IVectorView_HSTRING **out)
{
    struct hstring_vector *This;

    if (!(This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This) + count * sizeof(HSTRING)))) return E_OUTOFMEMORY;
    This->ref = 1;

    This->IVectorView_HSTRING_iface.lpVtbl = &hstring_vector_vtbl;
    This->count = count;
    memcpy(This->values, values, count * sizeof(HSTRING));

    *out = &This->IVectorView_HSTRING_iface;
    return S_OK;
}

struct windows_globalization
{
    IActivationFactory IActivationFactory_iface;
    IGlobalizationPreferencesStatics IGlobalizationPreferencesStatics_iface;
    LONG ref;
};

static inline struct windows_globalization *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct windows_globalization, IActivationFactory_iface);
}

static inline struct windows_globalization *impl_from_IGlobalizationPreferencesStatics(IGlobalizationPreferencesStatics *iface)
{
    return CONTAINING_RECORD(iface, struct windows_globalization, IGlobalizationPreferencesStatics_iface);
}

static HRESULT STDMETHODCALLTYPE globalization_preferences_QueryInterface(
        IGlobalizationPreferencesStatics *iface, REFIID iid, void **object)
{
    FIXME("iface %p, iid %s, object %p stub!\n", iface, debugstr_guid(iid), object);

    if (IsEqualGUID(iid, &IID_IAgileObject))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE globalization_preferences_AddRef(
        IGlobalizationPreferencesStatics *iface)
{
    struct windows_globalization *impl = impl_from_IGlobalizationPreferencesStatics(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    FIXME("iface %p -> ref %u.\n", iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE globalization_preferences_Release(
        IGlobalizationPreferencesStatics *iface)
{
    struct windows_globalization *impl = impl_from_IGlobalizationPreferencesStatics(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    FIXME("iface %p -> ref %u.\n", iface, ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE globalization_preferences_GetIids(
        IGlobalizationPreferencesStatics *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE globalization_preferences_GetRuntimeClassName(
        IGlobalizationPreferencesStatics *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE globalization_preferences_GetTrustLevel(
        IGlobalizationPreferencesStatics *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE globalization_preferences_get_Calendars(IGlobalizationPreferencesStatics *iface,
        IVectorView_HSTRING **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return hstring_vector_create(NULL, 0, value);
}

static HRESULT STDMETHODCALLTYPE globalization_preferences_get_Clocks(IGlobalizationPreferencesStatics *iface,
        IVectorView_HSTRING **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return hstring_vector_create(NULL, 0, value);
}

static HRESULT STDMETHODCALLTYPE globalization_preferences_get_Currencies(IGlobalizationPreferencesStatics *iface,
        IVectorView_HSTRING **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return hstring_vector_create(NULL, 0, value);
}

static HRESULT STDMETHODCALLTYPE globalization_preferences_get_Languages(IGlobalizationPreferencesStatics *iface,
        IVectorView_HSTRING **value)
{
    HSTRING hstring;
    UINT32 length;
    WCHAR locale_w[LOCALE_NAME_MAX_LENGTH], *tmp;

    FIXME("iface %p, value %p semi-stub!\n", iface, value);

    GetSystemDefaultLocaleName(locale_w, LOCALE_NAME_MAX_LENGTH);

    if ((tmp = wcsrchr(locale_w, '_'))) *tmp = 0;
    if ((tmp = wcschr(locale_w, '-')) && (wcslen(tmp) <= 3 || (tmp = wcschr(tmp + 1, '-')))) *tmp = 0;
    length = wcslen(locale_w);

    FIXME("returning language %s\n", debugstr_w(locale_w));

    WindowsCreateString(locale_w, length, &hstring);
    return hstring_vector_create(&hstring, 1, value);
}

static HRESULT STDMETHODCALLTYPE globalization_preferences_get_HomeGeographicRegion(IGlobalizationPreferencesStatics *iface,
        HSTRING* value)
{
    UINT32 length;
    WCHAR locale_w[LOCALE_NAME_MAX_LENGTH], *tmp;
    const WCHAR *country;

    TRACE("iface %p, value %p stub!\n", iface, value);

    GetSystemDefaultLocaleName(locale_w, LOCALE_NAME_MAX_LENGTH);

    if ((tmp = wcsrchr(locale_w, '_'))) *tmp = 0;
    if (!(tmp = wcschr(locale_w, '-')) || (wcslen(tmp) > 3 && !(tmp = wcschr(tmp + 1, '-')))) country = L"US";
    else country = tmp;
    length = wcslen(country);

    TRACE("returning country %s\n", debugstr_w(country));

    return WindowsCreateString(country, length, value);
}

static HRESULT STDMETHODCALLTYPE globalization_preferences_get_WeekStartsOn(IGlobalizationPreferencesStatics *iface,
        enum DayOfWeek* value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static const struct IGlobalizationPreferencesStaticsVtbl globalization_preferences_vtbl =
{
    globalization_preferences_QueryInterface,
    globalization_preferences_AddRef,
    globalization_preferences_Release,
    /* IInspectable methods */
    globalization_preferences_GetIids,
    globalization_preferences_GetRuntimeClassName,
    globalization_preferences_GetTrustLevel,
    /* IGlobalizationPreferencesStatics methods */
    globalization_preferences_get_Calendars,
    globalization_preferences_get_Clocks,
    globalization_preferences_get_Currencies,
    globalization_preferences_get_Languages,
    globalization_preferences_get_HomeGeographicRegion,
    globalization_preferences_get_WeekStartsOn,
};

static HRESULT STDMETHODCALLTYPE windows_globalization_QueryInterface(
        IActivationFactory *iface, REFIID iid, void **out)
{
    struct windows_globalization *impl = impl_from_IActivationFactory(iface);
    TRACE("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IUnknown_AddRef(iface);
        *out = &impl->IActivationFactory_iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IGlobalizationPreferencesStatics))
    {
        IUnknown_AddRef(iface);
        *out = &impl->IGlobalizationPreferencesStatics_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE windows_globalization_AddRef(
        IActivationFactory *iface)
{
    struct windows_globalization *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %u.\n", iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE windows_globalization_Release(
        IActivationFactory *iface)
{
    struct windows_globalization *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %u.\n", iface, ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE windows_globalization_GetIids(
        IActivationFactory *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE windows_globalization_GetRuntimeClassName(
        IActivationFactory *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE windows_globalization_GetTrustLevel(
        IActivationFactory *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE windows_globalization_ActivateInstance(
        IActivationFactory *iface, IInspectable **instance)
{
    FIXME("iface %p, instance %p stub!\n", iface, instance);
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl activation_factory_vtbl =
{
    windows_globalization_QueryInterface,
    windows_globalization_AddRef,
    windows_globalization_Release,
    /* IInspectable methods */
    windows_globalization_GetIids,
    windows_globalization_GetRuntimeClassName,
    windows_globalization_GetTrustLevel,
    /* IActivationFactory methods */
    windows_globalization_ActivateInstance,
};

static struct windows_globalization windows_globalization =
{
    {&activation_factory_vtbl},
    {&globalization_preferences_vtbl},
    0
};

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    FIXME("clsid %s, riid %s, out %p stub!\n", debugstr_guid(clsid), debugstr_guid(riid), out);
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory(HSTRING classid, IActivationFactory **factory)
{
    TRACE("classid %s, factory %p.\n", debugstr_hstring(classid), factory);
    *factory = &windows_globalization.IActivationFactory_iface;
    IUnknown_AddRef(*factory);
    return S_OK;
}

#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winstring.h"
#include "wine/debug.h"
#include "objbase.h"

#include "initguid.h"
#include "activation.h"

#define WIDL_USING_IVECTORVIEW_1_WINDOWS_GAMING_INPUT_GAMEPAD
#define WIDL_USING_IVECTORVIEW_1_WINDOWS_GAMING_INPUT_RAWGAMECONTROLLER
#define WIDL_USING_IEVENTHANDLER_1_WINDOWS_GAMING_INPUT_GAMEPAD
#define WIDL_USING_IEVENTHANDLER_1_WINDOWS_GAMING_INPUT_RAWGAMECONTROLLER
#define WIDL_USING_WINDOWS_GAMING_INPUT_IGAMEPAD
#define WIDL_USING_WINDOWS_GAMING_INPUT_IGAMEPADSTATICS
#define WIDL_USING_WINDOWS_GAMING_INPUT_IRAWGAMECONTROLLERSTATICS
#define WIDL_USING_WINDOWS_GAMING_INPUT_IGAMECONTROLLER
#define WIDL_USING_WINDOWS_GAMING_INPUT_IRAWGAMECONTROLLER
#define WIDL_USING_WINDOWS_GAMING_INPUT_RAWGAMECONTROLLER
#include "windows.foundation.h"
#include "windows.gaming.input.h"

WINE_DEFAULT_DEBUG_CHANNEL(input);

static const char *debugstr_hstring(HSTRING hstr)
{
    const WCHAR *str;
    UINT32 len;
    if (hstr && !((ULONG_PTR)hstr >> 16)) return "(invalid)";
    str = WindowsGetStringRawBuffer(hstr, &len);
    return wine_dbgstr_wn(str, len);
}

struct windows_gaming_input
{
    IActivationFactory IActivationFactory_iface;
    IGamepadStatics IGamepadStatics_iface;
    IRawGameControllerStatics IRawGameControllerStatics_iface;
    IVectorView_Gamepad IVectorView_Gamepad_iface;
    IVectorView_RawGameController IVectorView_RawGameController_iface;
    LONG ref;
};

static inline struct windows_gaming_input *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct windows_gaming_input, IActivationFactory_iface);
}

static inline struct windows_gaming_input *impl_from_IGamepadStatics(IGamepadStatics *iface)
{
    return CONTAINING_RECORD(iface, struct windows_gaming_input, IGamepadStatics_iface);
}

static inline struct windows_gaming_input *impl_from_IRawGameControllerStatics(IRawGameControllerStatics *iface)
{
    return CONTAINING_RECORD(iface, struct windows_gaming_input, IRawGameControllerStatics_iface);
}

static inline struct windows_gaming_input *impl_from_IVectorView_Gamepad(IVectorView_Gamepad *iface)
{
    return CONTAINING_RECORD(iface, struct windows_gaming_input, IVectorView_Gamepad_iface);
}

static inline struct windows_gaming_input *impl_from_IVectorView_RawGameController(IVectorView_RawGameController *iface)
{
    return CONTAINING_RECORD(iface, struct windows_gaming_input, IVectorView_RawGameController_iface);
}

static HRESULT STDMETHODCALLTYPE vector_view_gamepad_QueryInterface(
        IVectorView_Gamepad *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IVectorView_Gamepad))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE vector_view_gamepad_AddRef(
        IVectorView_Gamepad *iface)
{
    struct windows_gaming_input *impl = impl_from_IVectorView_Gamepad(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %u.\n", iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE vector_view_gamepad_Release(
        IVectorView_Gamepad *iface)
{
    struct windows_gaming_input *impl = impl_from_IVectorView_Gamepad(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %u.\n", iface, ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE vector_view_gamepad_GetIids(
        IVectorView_Gamepad *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE vector_view_gamepad_GetRuntimeClassName(
        IVectorView_Gamepad *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE vector_view_gamepad_GetTrustLevel(
        IVectorView_Gamepad *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE vector_view_gamepad_GetAt(
    IVectorView_Gamepad *iface, ULONG index, IGamepad **value)
{
    FIXME("iface %p, index %#x, value %p stub!\n", iface, index, value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE vector_view_gamepad_get_Size(
    IVectorView_Gamepad *iface, ULONG *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    *value = 0;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE vector_view_gamepad_IndexOf(
    IVectorView_Gamepad *iface, IGamepad *element, ULONG *index, BOOLEAN *value)
{
    FIXME("iface %p, element %p, index %p, value %p stub!\n", iface, element, index, value);
    *value = FALSE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE vector_view_gamepad_GetMany(
    IVectorView_Gamepad *iface, ULONG start_index, IGamepad **items, UINT *value)
{
    FIXME("iface %p, start_index %#x, items %p, value %p stub!\n", iface, start_index, items, value);
    *value = 0;
    return S_OK;
}

static const struct IVectorView_GamepadVtbl vector_view_gamepad_vtbl =
{
    vector_view_gamepad_QueryInterface,
    vector_view_gamepad_AddRef,
    vector_view_gamepad_Release,
    /* IInspectable methods */
    vector_view_gamepad_GetIids,
    vector_view_gamepad_GetRuntimeClassName,
    vector_view_gamepad_GetTrustLevel,
    /* IVectorView<Gamepad> methods */
    vector_view_gamepad_GetAt,
    vector_view_gamepad_get_Size,
    vector_view_gamepad_IndexOf,
    vector_view_gamepad_GetMany,
};

static HRESULT STDMETHODCALLTYPE vector_view_raw_game_controller_QueryInterface(
        IVectorView_RawGameController *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IVectorView_RawGameController))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE vector_view_raw_game_controller_AddRef(
        IVectorView_RawGameController *iface)
{
    struct windows_gaming_input *impl = impl_from_IVectorView_RawGameController(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %u.\n", iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE vector_view_raw_game_controller_Release(
        IVectorView_RawGameController *iface)
{
    struct windows_gaming_input *impl = impl_from_IVectorView_RawGameController(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %u.\n", iface, ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE vector_view_raw_game_controller_GetIids(
        IVectorView_RawGameController *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE vector_view_raw_game_controller_GetRuntimeClassName(
        IVectorView_RawGameController *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE vector_view_raw_game_controller_GetTrustLevel(
        IVectorView_RawGameController *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE vector_view_raw_game_controller_GetAt(
    IVectorView_RawGameController *iface, ULONG index, IRawGameController **value)
{
    FIXME("iface %p, index %#x, value %p stub!\n", iface, index, value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE vector_view_raw_game_controller_get_Size(
    IVectorView_RawGameController *iface, ULONG *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    *value = 0;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE vector_view_raw_game_controller_IndexOf(
    IVectorView_RawGameController *iface, IRawGameController *element, ULONG *index, BOOLEAN *value)
{
    FIXME("iface %p, element %p, index %p, value %p stub!\n", iface, element, index, value);
    *value = FALSE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE vector_view_raw_game_controller_GetMany(
    IVectorView_RawGameController *iface, ULONG start_index, IRawGameController **items, UINT *value)
{
    FIXME("iface %p, start_index %#x, items %p, value %p stub!\n", iface, start_index, items, value);
    *value = 0;
    return S_OK;
}

static const struct IVectorView_RawGameControllerVtbl vector_view_raw_game_controller_vtbl =
{
    vector_view_raw_game_controller_QueryInterface,
    vector_view_raw_game_controller_AddRef,
    vector_view_raw_game_controller_Release,
    /* IInspectable methods */
    vector_view_raw_game_controller_GetIids,
    vector_view_raw_game_controller_GetRuntimeClassName,
    vector_view_raw_game_controller_GetTrustLevel,
    /* IVectorView<RawGameController> methods */
    vector_view_raw_game_controller_GetAt,
    vector_view_raw_game_controller_get_Size,
    vector_view_raw_game_controller_IndexOf,
    vector_view_raw_game_controller_GetMany,
};

static HRESULT STDMETHODCALLTYPE gamepad_statics_QueryInterface(
        IGamepadStatics *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IAgileObject))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE gamepad_statics_AddRef(
        IGamepadStatics *iface)
{
    struct windows_gaming_input *impl = impl_from_IGamepadStatics(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %u.\n", iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE gamepad_statics_Release(
        IGamepadStatics *iface)
{
    struct windows_gaming_input *impl = impl_from_IGamepadStatics(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %u.\n", iface, ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE gamepad_statics_GetIids(
        IGamepadStatics *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE gamepad_statics_GetRuntimeClassName(
        IGamepadStatics *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE gamepad_statics_GetTrustLevel(
        IGamepadStatics *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE gamepad_statics_add_GamepadAdded(
    IGamepadStatics *iface, IEventHandler_Gamepad *value, EventRegistrationToken* token)
{
    FIXME("iface %p, value %p, token %p stub!\n", iface, value, token);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE gamepad_statics_remove_GamepadAdded(
    IGamepadStatics *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %#I64x stub!\n", iface, token.value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE gamepad_statics_add_GamepadRemoved(
    IGamepadStatics *iface, IEventHandler_Gamepad *value, EventRegistrationToken* token)
{
    FIXME("iface %p, value %p, token %p stub!\n", iface, value, token);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE gamepad_statics_remove_GamepadRemoved(
    IGamepadStatics *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %#I64x stub!\n", iface, token.value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE gamepad_statics_get_Gamepads(
    IGamepadStatics *iface, IVectorView_Gamepad **value)
{
    struct windows_gaming_input *impl = impl_from_IGamepadStatics(iface);
    FIXME("iface %p, value %p stub!\n", iface, value);
    *value = &impl->IVectorView_Gamepad_iface;
    return S_OK;
}

static const struct IGamepadStaticsVtbl gamepad_statics_vtbl =
{
    gamepad_statics_QueryInterface,
    gamepad_statics_AddRef,
    gamepad_statics_Release,
    /* IInspectable methods */
    gamepad_statics_GetIids,
    gamepad_statics_GetRuntimeClassName,
    gamepad_statics_GetTrustLevel,
    /* IGamepadStatics methods */
    gamepad_statics_add_GamepadAdded,
    gamepad_statics_remove_GamepadAdded,
    gamepad_statics_add_GamepadRemoved,
    gamepad_statics_remove_GamepadRemoved,
    gamepad_statics_get_Gamepads,
};

static HRESULT STDMETHODCALLTYPE raw_game_controller_statics_QueryInterface(
        IRawGameControllerStatics *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IAgileObject))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE raw_game_controller_statics_AddRef(
        IRawGameControllerStatics *iface)
{
    struct windows_gaming_input *impl = impl_from_IRawGameControllerStatics(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %u.\n", iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE raw_game_controller_statics_Release(
        IRawGameControllerStatics *iface)
{
    struct windows_gaming_input *impl = impl_from_IRawGameControllerStatics(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %u.\n", iface, ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE raw_game_controller_statics_GetIids(
        IRawGameControllerStatics *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE raw_game_controller_statics_GetRuntimeClassName(
        IRawGameControllerStatics *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE raw_game_controller_statics_GetTrustLevel(
        IRawGameControllerStatics *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE raw_game_controller_statics_add_RawGameControllerAdded(
    IRawGameControllerStatics *iface, IEventHandler_RawGameController *value, EventRegistrationToken* token)
{
    FIXME("iface %p, value %p, token %p stub!\n", iface, value, token);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE raw_game_controller_statics_remove_RawGameControllerAdded(
    IRawGameControllerStatics *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %#I64x stub!\n", iface, token.value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE raw_game_controller_statics_add_RawGameControllerRemoved(
    IRawGameControllerStatics *iface, IEventHandler_RawGameController *value, EventRegistrationToken* token)
{
    FIXME("iface %p, value %p, token %p stub!\n", iface, value, token);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE raw_game_controller_statics_remove_RawGameControllerRemoved(
    IRawGameControllerStatics *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %#I64x stub!\n", iface, token.value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE raw_game_controller_statics_get_RawGameControllers(
    IRawGameControllerStatics *iface, IVectorView_RawGameController **value)
{
    struct windows_gaming_input *impl = impl_from_IRawGameControllerStatics(iface);
    FIXME("iface %p, value %p stub!\n", iface, value);
    *value = &impl->IVectorView_RawGameController_iface;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE raw_game_controller_statics_FromGameController(
    IRawGameControllerStatics *iface, IGameController *game_controller, IRawGameController **value)
{
    FIXME("iface %p, game_controller %p, value %p stub!\n", iface, game_controller, value);
    return E_NOTIMPL;
}

static const struct IRawGameControllerStaticsVtbl raw_game_controller_statics_vtbl =
{
    raw_game_controller_statics_QueryInterface,
    raw_game_controller_statics_AddRef,
    raw_game_controller_statics_Release,
    /* IInspectable methods */
    raw_game_controller_statics_GetIids,
    raw_game_controller_statics_GetRuntimeClassName,
    raw_game_controller_statics_GetTrustLevel,
    /* IRawGameControllerStatics methods */
    raw_game_controller_statics_add_RawGameControllerAdded,
    raw_game_controller_statics_remove_RawGameControllerAdded,
    raw_game_controller_statics_add_RawGameControllerRemoved,
    raw_game_controller_statics_remove_RawGameControllerRemoved,
    raw_game_controller_statics_get_RawGameControllers,
    raw_game_controller_statics_FromGameController,
};

static HRESULT STDMETHODCALLTYPE windows_gaming_input_QueryInterface(
        IActivationFactory *iface, REFIID iid, void **out)
{
    struct windows_gaming_input *impl = impl_from_IActivationFactory(iface);
    TRACE("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IUnknown_AddRef(iface);
        *out = &impl->IActivationFactory_iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IGamepadStatics))
    {
        IUnknown_AddRef(iface);
        *out = &impl->IGamepadStatics_iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IRawGameControllerStatics))
    {
        IUnknown_AddRef(iface);
        *out = &impl->IRawGameControllerStatics_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE windows_gaming_input_AddRef(
        IActivationFactory *iface)
{
    struct windows_gaming_input *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %u.\n", iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE windows_gaming_input_Release(
        IActivationFactory *iface)
{
    struct windows_gaming_input *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %u.\n", iface, ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE windows_gaming_input_GetIids(
        IActivationFactory *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE windows_gaming_input_GetRuntimeClassName(
        IActivationFactory *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE windows_gaming_input_GetTrustLevel(
        IActivationFactory *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE windows_gaming_input_ActivateInstance(
        IActivationFactory *iface, IInspectable **instance)
{
    FIXME("iface %p, instance %p stub!\n", iface, instance);
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl activation_factory_vtbl =
{
    windows_gaming_input_QueryInterface,
    windows_gaming_input_AddRef,
    windows_gaming_input_Release,
    /* IInspectable methods */
    windows_gaming_input_GetIids,
    windows_gaming_input_GetRuntimeClassName,
    windows_gaming_input_GetTrustLevel,
    /* IActivationFactory methods */
    windows_gaming_input_ActivateInstance,
};

static struct windows_gaming_input windows_gaming_input =
{
    {&activation_factory_vtbl},
    {&gamepad_statics_vtbl},
    {&raw_game_controller_statics_vtbl},
    {&vector_view_gamepad_vtbl},
    {&vector_view_raw_game_controller_vtbl},
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
    *factory = &windows_gaming_input.IActivationFactory_iface;
    IUnknown_AddRef(*factory);
    return S_OK;
}

#include "windef.h"

#include "mfidl.h"
#include "mfapi.h"
#include "mfobjects.h"

#include "wine/list.h"

/* helper sub-object that handles ansyncronous nature of handlers */

struct handler;
typedef HRESULT (*p_create_object_callback)(struct handler *handler, WCHAR *url, IMFByteStream *stream, DWORD flags, IPropertyStore *props,
                                            IUnknown **out_object, MF_OBJECT_TYPE *out_obj_type);

struct handler
{
    IMFAsyncCallback IMFAsyncCallback_iface;
    struct list results;
    CRITICAL_SECTION cs;
    p_create_object_callback create_object;
};

void handler_construct(struct handler *handler, p_create_object_callback create_object_callback);
void handler_destruct(struct handler *handler);
HRESULT handler_begin_create_object(struct handler *handler, IMFByteStream *stream,
        const WCHAR *url, DWORD flags, IPropertyStore *props, IUnknown **cancel_cookie,
        IMFAsyncCallback *callback, IUnknown *state);
HRESULT handler_end_create_object(struct handler *handler, IMFAsyncResult *result,
        MF_OBJECT_TYPE *obj_type, IUnknown **object);
HRESULT handler_cancel_object_creation(struct handler *handler, IUnknown *cancel_cookie);
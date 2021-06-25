#if 0
#pragma makedep unix
#endif

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "ntsecapi.h"
#include "bcrypt.h"

#include "bcrypt_internal.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#if defined(HAVE_COMMONCRYPTO_COMMONCRYPTOR_H) && MAC_OS_X_VERSION_MAX_ALLOWED >= 1080 || defined(HAVE_GNUTLS_CIPHER_INIT)
WINE_DEFAULT_DEBUG_CHANNEL(bcrypt);

static NTSTATUS CDECL key_set_property( struct key *key, const WCHAR *prop, UCHAR *value, ULONG size, ULONG flags )
{
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_symmetric_init( struct key *key )
{
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static void CDECL key_symmetric_vector_reset( struct key *key )
{
    FIXME( "not implemented\n" );
}

static NTSTATUS CDECL key_symmetric_set_auth_data( struct key *key, UCHAR *auth_data, ULONG len )
{
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_symmetric_encrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output, ULONG output_len  )
{
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_symmetric_decrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output, ULONG output_len )
{
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_symmetric_get_tag( struct key *key, UCHAR *tag, ULONG len )
{
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static void CDECL key_symmetric_destroy( struct key *key )
{
    FIXME( "not implemented\n" );
}

static NTSTATUS CDECL key_asymmetric_init( struct key *key )
{
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_asymmetric_sign( struct key *key, void *padding, UCHAR *input, ULONG input_len, UCHAR *output,
                                           ULONG output_len, ULONG *ret_len, ULONG flags )
{
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_asymmetric_verify( struct key *key, void *padding, UCHAR *hash, ULONG hash_len,
                                             UCHAR *signature, ULONG signature_len, DWORD flags )
{
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_export_dsa_capi( struct key *key, UCHAR *buf, ULONG len, ULONG *ret_len )
{
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_export_ecc( struct key *key, UCHAR *output, ULONG len, ULONG *ret_len )
{
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_import_dsa_capi( struct key *key, UCHAR *buf, ULONG len )
{
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_import_ecc( struct key *key, UCHAR *input, ULONG len )
{
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_asymmetric_generate( struct key *key )
{
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_asymmetric_duplicate( struct key *key_orig, struct key *key_copy )
{
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static void CDECL key_asymmetric_destroy( struct key *key )
{
    FIXME( "not implemented\n" );
}

static NTSTATUS CDECL key_asymmetric_decrypt( struct key *key, UCHAR *input, ULONG input_len,
        UCHAR *output, ULONG *output_len )
{
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_import_rsa( struct key *key, UCHAR *input, ULONG input_len )
{
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_compute_secret_ecc (unsigned char *privkey_in, struct key *pubkey_in, struct secret *secret)
{
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static struct key_funcs key_funcs =
{
    key_set_property,
    key_symmetric_init,
    key_symmetric_vector_reset,
    key_symmetric_set_auth_data,
    key_symmetric_encrypt,
    key_symmetric_decrypt,
    key_symmetric_get_tag,
    key_symmetric_destroy,
    key_asymmetric_init,
    key_asymmetric_generate,
    key_asymmetric_decrypt,
    key_asymmetric_duplicate,
    key_asymmetric_sign,
    key_asymmetric_verify,
    key_asymmetric_destroy,
    key_export_dsa_capi,
    key_export_ecc,
    key_import_dsa_capi,
    key_import_ecc,
    key_import_rsa,
    key_compute_secret_ecc,
};

NTSTATUS CDECL __wine_init_unix_lib( HMODULE module, DWORD reason, const void *ptr_in, void *ptr_out )
{
    struct key_funcs *gnutls_funcs = gnutls_lib_init(reason);
    struct key_funcs *macos_funcs = macos_lib_init(reason);
    struct key_funcs *gcrypt_funcs = gcrypt_lib_init(reason);

    if (reason == DLL_PROCESS_ATTACH)
    {
#define RESOLVE_FUNC(name) \
        if (macos_funcs && macos_funcs->key_##name) \
            key_funcs.key_##name = macos_funcs->key_##name; \
        if (gnutls_funcs && gnutls_funcs->key_##name) \
            key_funcs.key_##name = gnutls_funcs->key_##name; \
        if (gcrypt_funcs && gcrypt_funcs->key_##name) \
            key_funcs.key_##name = gcrypt_funcs->key_##name;

        RESOLVE_FUNC(set_property)
        RESOLVE_FUNC(symmetric_init)
        RESOLVE_FUNC(symmetric_vector_reset)
        RESOLVE_FUNC(symmetric_set_auth_data)
        RESOLVE_FUNC(symmetric_encrypt)
        RESOLVE_FUNC(symmetric_decrypt)
        RESOLVE_FUNC(symmetric_get_tag)
        RESOLVE_FUNC(symmetric_destroy)
        RESOLVE_FUNC(asymmetric_init)
        RESOLVE_FUNC(asymmetric_generate)
        RESOLVE_FUNC(asymmetric_decrypt)
        RESOLVE_FUNC(asymmetric_duplicate)
        RESOLVE_FUNC(asymmetric_sign)
        RESOLVE_FUNC(asymmetric_verify)
        RESOLVE_FUNC(asymmetric_destroy)
        RESOLVE_FUNC(export_dsa_capi)
        RESOLVE_FUNC(export_ecc)
        RESOLVE_FUNC(import_dsa_capi)
        RESOLVE_FUNC(import_ecc)
        RESOLVE_FUNC(import_rsa)
        RESOLVE_FUNC(compute_secret_ecc)

#undef RESOLVE_FUNC

        *(struct key_funcs **)ptr_out = &key_funcs;
    }

    return STATUS_SUCCESS;
}

#endif

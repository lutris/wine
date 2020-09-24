/*
 * Copyright 2017 Nikolay Sivov
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"

#undef INITGUID
#include <guiddef.h>
#include "mfidl.h"

#include "wine/debug.h"

#include "mf_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

static LONG next_node_id;
static TOPOID next_topology_id;

struct node_stream
{
    IMFMediaType *preferred_type;
    struct topology_node *connection;
    DWORD connection_stream;
};

struct node_streams
{
    struct node_stream *streams;
    size_t size;
    size_t count;
};

struct topology_node
{
    IMFTopologyNode IMFTopologyNode_iface;
    LONG refcount;
    IMFAttributes *attributes;
    MF_TOPOLOGY_TYPE node_type;
    TOPOID id;
    IUnknown *object;
    IMFMediaType *input_type; /* Only for tee nodes. */
    struct node_streams inputs;
    struct node_streams outputs;
    CRITICAL_SECTION cs;
};

struct topology
{
    IMFTopology IMFTopology_iface;
    LONG refcount;
    IMFAttributes *attributes;
    struct
    {
        struct topology_node **nodes;
        size_t size;
        size_t count;
    } nodes;
    TOPOID id;
};

struct topology_loader
{
    IMFTopoLoader IMFTopoLoader_iface;
    LONG refcount;
};

struct seq_source
{
    IMFSequencerSource IMFSequencerSource_iface;
    IMFMediaSourceTopologyProvider IMFMediaSourceTopologyProvider_iface;
    LONG refcount;
};

static inline struct topology *impl_from_IMFTopology(IMFTopology *iface)
{
    return CONTAINING_RECORD(iface, struct topology, IMFTopology_iface);
}

static struct topology_node *impl_from_IMFTopologyNode(IMFTopologyNode *iface)
{
    return CONTAINING_RECORD(iface, struct topology_node, IMFTopologyNode_iface);
}

static const IMFTopologyNodeVtbl topologynodevtbl;

static struct topology_node *unsafe_impl_from_IMFTopologyNode(IMFTopologyNode *iface)
{
    if (!iface || iface->lpVtbl != &topologynodevtbl)
        return NULL;
    return impl_from_IMFTopologyNode(iface);
}

static HRESULT create_topology_node(MF_TOPOLOGY_TYPE node_type, struct topology_node **node);
static HRESULT topology_node_connect_output(struct topology_node *node, DWORD output_index,
        struct topology_node *connection, DWORD input_index);

static struct topology *unsafe_impl_from_IMFTopology(IMFTopology *iface);

static struct topology_loader *impl_from_IMFTopoLoader(IMFTopoLoader *iface)
{
    return CONTAINING_RECORD(iface, struct topology_loader, IMFTopoLoader_iface);
}

static struct seq_source *impl_from_IMFSequencerSource(IMFSequencerSource *iface)
{
    return CONTAINING_RECORD(iface, struct seq_source, IMFSequencerSource_iface);
}

static struct seq_source *impl_from_IMFMediaSourceTopologyProvider(IMFMediaSourceTopologyProvider *iface)
{
    return CONTAINING_RECORD(iface, struct seq_source, IMFMediaSourceTopologyProvider_iface);
}

static HRESULT topology_node_reserve_streams(struct node_streams *streams, DWORD index)
{
    if (!mf_array_reserve((void **)&streams->streams, &streams->size, index + 1, sizeof(*streams->streams)))
        return E_OUTOFMEMORY;

    if (index >= streams->count)
    {
        memset(&streams->streams[streams->count], 0, (index - streams->count + 1) * sizeof(*streams->streams));
        streams->count = index + 1;
    }

    return S_OK;
}

static HRESULT WINAPI topology_QueryInterface(IMFTopology *iface, REFIID riid, void **out)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFTopology) ||
            IsEqualIID(riid, &IID_IMFAttributes) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &topology->IMFTopology_iface;
    }
    else
    {
        FIXME("(%s, %p)\n", debugstr_guid(riid), out);
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*out);
    return S_OK;
}

static ULONG WINAPI topology_AddRef(IMFTopology *iface)
{
    struct topology *topology = impl_from_IMFTopology(iface);
    ULONG refcount = InterlockedIncrement(&topology->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static HRESULT topology_node_disconnect_output(struct topology_node *node, DWORD output_index)
{
    struct topology_node *connection = NULL;
    struct node_stream *stream;
    DWORD connection_stream;
    HRESULT hr = S_OK;

    EnterCriticalSection(&node->cs);

    if (output_index < node->outputs.count)
    {
        stream = &node->outputs.streams[output_index];

        if (stream->connection)
        {
            connection = stream->connection;
            connection_stream = stream->connection_stream;
            stream->connection = NULL;
            stream->connection_stream = 0;
        }
        else
            hr = MF_E_NOT_FOUND;
    }
    else
        hr = E_INVALIDARG;

    LeaveCriticalSection(&node->cs);

    if (connection)
    {
        EnterCriticalSection(&connection->cs);

        if (connection_stream < connection->inputs.count)
        {
            stream = &connection->inputs.streams[connection_stream];

            if (stream->connection)
            {
                stream->connection = NULL;
                stream->connection_stream = 0;
            }
        }

        LeaveCriticalSection(&connection->cs);

        IMFTopologyNode_Release(&connection->IMFTopologyNode_iface);
        IMFTopologyNode_Release(&node->IMFTopologyNode_iface);
    }

    return hr;
}

static void topology_node_disconnect(struct topology_node *node)
{
    struct node_stream *stream;
    size_t i;

    for (i = 0; i < node->outputs.count; ++i)
        topology_node_disconnect_output(node, i);

    for (i = 0; i < node->inputs.count; ++i)
    {
        stream = &node->inputs.streams[i];
        if (stream->connection)
            topology_node_disconnect_output(stream->connection, stream->connection_stream);
    }
}

static void topology_clear(struct topology *topology)
{
    size_t i;

    for (i = 0; i < topology->nodes.count; ++i)
    {
        topology_node_disconnect(topology->nodes.nodes[i]);
        IMFTopologyNode_Release(&topology->nodes.nodes[i]->IMFTopologyNode_iface);
    }
    heap_free(topology->nodes.nodes);
    topology->nodes.nodes = NULL;
    topology->nodes.count = 0;
    topology->nodes.size = 0;
}

static ULONG WINAPI topology_Release(IMFTopology *iface)
{
    struct topology *topology = impl_from_IMFTopology(iface);
    ULONG refcount = InterlockedDecrement(&topology->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (topology->attributes)
            IMFAttributes_Release(topology->attributes);
        topology_clear(topology);
        heap_free(topology);
    }

    return refcount;
}

static HRESULT WINAPI topology_GetItem(IMFTopology *iface, REFGUID key, PROPVARIANT *value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetItem(topology->attributes, key, value);
}

static HRESULT WINAPI topology_GetItemType(IMFTopology *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), type);

    return IMFAttributes_GetItemType(topology->attributes, key, type);
}

static HRESULT WINAPI topology_CompareItem(IMFTopology *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), value, result);

    return IMFAttributes_CompareItem(topology->attributes, key, value, result);
}

static HRESULT WINAPI topology_Compare(IMFTopology *iface, IMFAttributes *theirs, MF_ATTRIBUTES_MATCH_TYPE type,
        BOOL *result)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %p, %d, %p.\n", iface, theirs, type, result);

    return IMFAttributes_Compare(topology->attributes, theirs, type, result);
}

static HRESULT WINAPI topology_GetUINT32(IMFTopology *iface, REFGUID key, UINT32 *value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetUINT32(topology->attributes, key, value);
}

static HRESULT WINAPI topology_GetUINT64(IMFTopology *iface, REFGUID key, UINT64 *value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetUINT64(topology->attributes, key, value);
}

static HRESULT WINAPI topology_GetDouble(IMFTopology *iface, REFGUID key, double *value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetDouble(topology->attributes, key, value);
}

static HRESULT WINAPI topology_GetGUID(IMFTopology *iface, REFGUID key, GUID *value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetGUID(topology->attributes, key, value);
}

static HRESULT WINAPI topology_GetStringLength(IMFTopology *iface, REFGUID key, UINT32 *length)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), length);

    return IMFAttributes_GetStringLength(topology->attributes, key, length);
}

static HRESULT WINAPI topology_GetString(IMFTopology *iface, REFGUID key, WCHAR *value,
        UINT32 size, UINT32 *length)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p, %d, %p.\n", iface, debugstr_guid(key), value, size, length);

    return IMFAttributes_GetString(topology->attributes, key, value, size, length);
}

static HRESULT WINAPI topology_GetAllocatedString(IMFTopology *iface, REFGUID key,
        WCHAR **value, UINT32 *length)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), value, length);

    return IMFAttributes_GetAllocatedString(topology->attributes, key, value, length);
}

static HRESULT WINAPI topology_GetBlobSize(IMFTopology *iface, REFGUID key, UINT32 *size)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), size);

    return IMFAttributes_GetBlobSize(topology->attributes, key, size);
}

static HRESULT WINAPI topology_GetBlob(IMFTopology *iface, REFGUID key, UINT8 *buf,
        UINT32 bufsize, UINT32 *blobsize)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p, %d, %p.\n", iface, debugstr_guid(key), buf, bufsize, blobsize);

    return IMFAttributes_GetBlob(topology->attributes, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI topology_GetAllocatedBlob(IMFTopology *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), buf, size);

    return IMFAttributes_GetAllocatedBlob(topology->attributes, key, buf, size);
}

static HRESULT WINAPI topology_GetUnknown(IMFTopology *iface, REFGUID key, REFIID riid, void **ppv)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_guid(key), debugstr_guid(riid), ppv);

    return IMFAttributes_GetUnknown(topology->attributes, key, riid, ppv);
}

static HRESULT WINAPI topology_SetItem(IMFTopology *iface, REFGUID key, REFPROPVARIANT value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetItem(topology->attributes, key, value);
}

static HRESULT WINAPI topology_DeleteItem(IMFTopology *iface, REFGUID key)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s.\n", topology, debugstr_guid(key));

    return IMFAttributes_DeleteItem(topology->attributes, key);
}

static HRESULT WINAPI topology_DeleteAllItems(IMFTopology *iface)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_DeleteAllItems(topology->attributes);
}

static HRESULT WINAPI topology_SetUINT32(IMFTopology *iface, REFGUID key, UINT32 value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %d.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetUINT32(topology->attributes, key, value);
}

static HRESULT WINAPI topology_SetUINT64(IMFTopology *iface, REFGUID key, UINT64 value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), wine_dbgstr_longlong(value));

    return IMFAttributes_SetUINT64(topology->attributes, key, value);
}

static HRESULT WINAPI topology_SetDouble(IMFTopology *iface, REFGUID key, double value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %f.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetDouble(topology->attributes, key, value);
}

static HRESULT WINAPI topology_SetGUID(IMFTopology *iface, REFGUID key, REFGUID value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), debugstr_guid(value));

    return IMFAttributes_SetGUID(topology->attributes, key, value);
}

static HRESULT WINAPI topology_SetString(IMFTopology *iface, REFGUID key, const WCHAR *value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), debugstr_w(value));

    return IMFAttributes_SetString(topology->attributes, key, value);
}

static HRESULT WINAPI topology_SetBlob(IMFTopology *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p, %d.\n", iface, debugstr_guid(key), buf, size);

    return IMFAttributes_SetBlob(topology->attributes, key, buf, size);
}

static HRESULT WINAPI topology_SetUnknown(IMFTopology *iface, REFGUID key, IUnknown *unknown)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), unknown);

    return IMFAttributes_SetUnknown(topology->attributes, key, unknown);
}

static HRESULT WINAPI topology_LockStore(IMFTopology *iface)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_LockStore(topology->attributes);
}

static HRESULT WINAPI topology_UnlockStore(IMFTopology *iface)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_UnlockStore(topology->attributes);
}

static HRESULT WINAPI topology_GetCount(IMFTopology *iface, UINT32 *count)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %p.\n", iface, count);

    return IMFAttributes_GetCount(topology->attributes, count);
}

static HRESULT WINAPI topology_GetItemByIndex(IMFTopology *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %u, %p, %p.\n", iface, index, key, value);

    return IMFAttributes_GetItemByIndex(topology->attributes, index, key, value);
}

static HRESULT WINAPI topology_CopyAllItems(IMFTopology *iface, IMFAttributes *dest)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %p.\n", iface, dest);

    return IMFAttributes_CopyAllItems(topology->attributes, dest);
}

static HRESULT WINAPI topology_GetTopologyID(IMFTopology *iface, TOPOID *id)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %p.\n", iface, id);

    if (!id)
        return E_POINTER;

    *id = topology->id;

    return S_OK;
}

static HRESULT topology_get_node_by_id(const struct topology *topology, TOPOID id, struct topology_node **node)
{
    size_t i = 0;

    for (i = 0; i < topology->nodes.count; ++i)
    {
        if (topology->nodes.nodes[i]->id == id)
        {
            *node = topology->nodes.nodes[i];
            return S_OK;
        }
    }

    return MF_E_NOT_FOUND;
}

static HRESULT topology_add_node(struct topology *topology, struct topology_node *node)
{
    struct topology_node *match;

    if (!node)
        return E_POINTER;

    if (SUCCEEDED(topology_get_node_by_id(topology, node->id, &match)))
        return E_INVALIDARG;

    if (!mf_array_reserve((void **)&topology->nodes.nodes, &topology->nodes.size, topology->nodes.count + 1,
            sizeof(*topology->nodes.nodes)))
    {
        return E_OUTOFMEMORY;
    }

    topology->nodes.nodes[topology->nodes.count++] = node;
    IMFTopologyNode_AddRef(&node->IMFTopologyNode_iface);

    return S_OK;
}

static HRESULT WINAPI topology_AddNode(IMFTopology *iface, IMFTopologyNode *node_iface)
{
    struct topology *topology = impl_from_IMFTopology(iface);
    struct topology_node *node = unsafe_impl_from_IMFTopologyNode(node_iface);

    TRACE("%p, %p.\n", iface, node_iface);

    return topology_add_node(topology, node);
}

static HRESULT WINAPI topology_RemoveNode(IMFTopology *iface, IMFTopologyNode *node)
{
    struct topology *topology = impl_from_IMFTopology(iface);
    size_t i, count;

    TRACE("%p, %p.\n", iface, node);

    for (i = 0; i < topology->nodes.count; ++i)
    {
        if (&topology->nodes.nodes[i]->IMFTopologyNode_iface == node)
        {
            topology_node_disconnect(topology->nodes.nodes[i]);
            IMFTopologyNode_Release(&topology->nodes.nodes[i]->IMFTopologyNode_iface);
            count = topology->nodes.count - i - 1;
            if (count)
            {
                memmove(&topology->nodes.nodes[i], &topology->nodes.nodes[i + 1],
                        count * sizeof(*topology->nodes.nodes));
            }
            topology->nodes.count--;
            return S_OK;
        }
    }

    return E_INVALIDARG;
}

static HRESULT WINAPI topology_GetNodeCount(IMFTopology *iface, WORD *count)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %p.\n", iface, count);

    if (!count)
        return E_POINTER;

    *count = topology->nodes.count;

    return S_OK;
}

static HRESULT WINAPI topology_GetNode(IMFTopology *iface, WORD index, IMFTopologyNode **node)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %u, %p.\n", iface, index, node);

    if (!node)
        return E_POINTER;

    if (index >= topology->nodes.count)
        return MF_E_INVALIDINDEX;

    *node = &topology->nodes.nodes[index]->IMFTopologyNode_iface;
    IMFTopologyNode_AddRef(*node);

    return S_OK;
}

static HRESULT WINAPI topology_Clear(IMFTopology *iface)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p.\n", iface);

    topology_clear(topology);
    return S_OK;
}

static HRESULT WINAPI topology_CloneFrom(IMFTopology *iface, IMFTopology *src)
{
    struct topology *topology = impl_from_IMFTopology(iface);
    struct topology *src_topology = unsafe_impl_from_IMFTopology(src);
    struct topology_node *node;
    size_t i, j;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, src);

    topology_clear(topology);

    /* Clone nodes. */
    for (i = 0; i < src_topology->nodes.count; ++i)
    {
        if (FAILED(hr = create_topology_node(src_topology->nodes.nodes[i]->node_type, &node)))
        {
            WARN("Failed to create a node, hr %#x.\n", hr);
            break;
        }

        if (SUCCEEDED(hr = IMFTopologyNode_CloneFrom(&node->IMFTopologyNode_iface,
                &src_topology->nodes.nodes[i]->IMFTopologyNode_iface)))
        {
            topology_add_node(topology, node);
        }

        IMFTopologyNode_Release(&node->IMFTopologyNode_iface);
    }

    /* Clone connections. */
    for (i = 0; i < src_topology->nodes.count; ++i)
    {
        const struct node_streams *outputs = &src_topology->nodes.nodes[i]->outputs;

        for (j = 0; j < outputs->count; ++j)
        {
            DWORD input_index = outputs->streams[j].connection_stream;
            TOPOID id = outputs->streams[j].connection->id;

            /* Skip node lookup in destination topology, assuming same node order. */
            if (SUCCEEDED(hr = topology_get_node_by_id(topology, id, &node)))
                topology_node_connect_output(topology->nodes.nodes[i], j, node, input_index);
        }
    }

    /* Copy attributes and id. */
    hr = IMFTopology_CopyAllItems(src, (IMFAttributes *)&topology->IMFTopology_iface);
    if (SUCCEEDED(hr))
        topology->id = src_topology->id;

    return S_OK;
}

static HRESULT WINAPI topology_GetNodeByID(IMFTopology *iface, TOPOID id, IMFTopologyNode **ret)
{
    struct topology *topology = impl_from_IMFTopology(iface);
    struct topology_node *node;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, ret);

    if (SUCCEEDED(hr = topology_get_node_by_id(topology, id, &node)))
    {
        *ret = &node->IMFTopologyNode_iface;
        IMFTopologyNode_AddRef(*ret);
    }
    else
        *ret = NULL;

    return hr;
}

static HRESULT topology_get_node_collection(const struct topology *topology, MF_TOPOLOGY_TYPE node_type,
        IMFCollection **collection)
{
    HRESULT hr;
    size_t i;

    if (!collection)
        return E_POINTER;

    if (FAILED(hr = MFCreateCollection(collection)))
        return hr;

    for (i = 0; i < topology->nodes.count; ++i)
    {
        if (topology->nodes.nodes[i]->node_type == node_type)
        {
            if (FAILED(hr = IMFCollection_AddElement(*collection,
                    (IUnknown *)&topology->nodes.nodes[i]->IMFTopologyNode_iface)))
            {
                IMFCollection_Release(*collection);
                *collection = NULL;
                break;
            }
        }
    }

    return hr;
}

static HRESULT WINAPI topology_GetSourceNodeCollection(IMFTopology *iface, IMFCollection **collection)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %p.\n", iface, collection);

    return topology_get_node_collection(topology, MF_TOPOLOGY_SOURCESTREAM_NODE, collection);
}

static HRESULT WINAPI topology_GetOutputNodeCollection(IMFTopology *iface, IMFCollection **collection)
{
    struct topology *topology = impl_from_IMFTopology(iface);

    TRACE("%p, %p.\n", iface, collection);

    return topology_get_node_collection(topology, MF_TOPOLOGY_OUTPUT_NODE, collection);
}

static const IMFTopologyVtbl topologyvtbl =
{
    topology_QueryInterface,
    topology_AddRef,
    topology_Release,
    topology_GetItem,
    topology_GetItemType,
    topology_CompareItem,
    topology_Compare,
    topology_GetUINT32,
    topology_GetUINT64,
    topology_GetDouble,
    topology_GetGUID,
    topology_GetStringLength,
    topology_GetString,
    topology_GetAllocatedString,
    topology_GetBlobSize,
    topology_GetBlob,
    topology_GetAllocatedBlob,
    topology_GetUnknown,
    topology_SetItem,
    topology_DeleteItem,
    topology_DeleteAllItems,
    topology_SetUINT32,
    topology_SetUINT64,
    topology_SetDouble,
    topology_SetGUID,
    topology_SetString,
    topology_SetBlob,
    topology_SetUnknown,
    topology_LockStore,
    topology_UnlockStore,
    topology_GetCount,
    topology_GetItemByIndex,
    topology_CopyAllItems,
    topology_GetTopologyID,
    topology_AddNode,
    topology_RemoveNode,
    topology_GetNodeCount,
    topology_GetNode,
    topology_Clear,
    topology_CloneFrom,
    topology_GetNodeByID,
    topology_GetSourceNodeCollection,
    topology_GetOutputNodeCollection,
};

static struct topology *unsafe_impl_from_IMFTopology(IMFTopology *iface)
{
    if (!iface || iface->lpVtbl != &topologyvtbl)
        return NULL;
    return impl_from_IMFTopology(iface);
}

static TOPOID topology_generate_id(void)
{
    TOPOID old;

    do
    {
        old = next_topology_id;
    }
    while (InterlockedCompareExchange64((LONG64 *)&next_topology_id, old + 1, old) != old);

    return next_topology_id;
}

/***********************************************************************
 *      MFCreateTopology (mf.@)
 */
HRESULT WINAPI MFCreateTopology(IMFTopology **topology)
{
    struct topology *object;
    HRESULT hr;

    TRACE("%p.\n", topology);

    if (!topology)
        return E_POINTER;

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFTopology_iface.lpVtbl = &topologyvtbl;
    object->refcount = 1;

    hr = MFCreateAttributes(&object->attributes, 0);
    if (FAILED(hr))
    {
        IMFTopology_Release(&object->IMFTopology_iface);
        return hr;
    }

    object->id = topology_generate_id();

    *topology = &object->IMFTopology_iface;

    return S_OK;
}

static HRESULT WINAPI topology_node_QueryInterface(IMFTopologyNode *iface, REFIID riid, void **out)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFTopologyNode) ||
            IsEqualIID(riid, &IID_IMFAttributes) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &node->IMFTopologyNode_iface;
        IMFTopologyNode_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *out = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI topology_node_AddRef(IMFTopologyNode *iface)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);
    ULONG refcount = InterlockedIncrement(&node->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI topology_node_Release(IMFTopologyNode *iface)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);
    ULONG refcount = InterlockedDecrement(&node->refcount);
    unsigned int i;

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (node->object)
            IUnknown_Release(node->object);
        if (node->input_type)
            IMFMediaType_Release(node->input_type);
        for (i = 0; i < node->inputs.count; ++i)
        {
            if (node->inputs.streams[i].preferred_type)
                IMFMediaType_Release(node->inputs.streams[i].preferred_type);
        }
        for (i = 0; i < node->outputs.count; ++i)
        {
            if (node->outputs.streams[i].preferred_type)
                IMFMediaType_Release(node->outputs.streams[i].preferred_type);
        }
        heap_free(node->inputs.streams);
        heap_free(node->outputs.streams);
        IMFAttributes_Release(node->attributes);
        DeleteCriticalSection(&node->cs);
        heap_free(node);
    }

    return refcount;
}

static HRESULT WINAPI topology_node_GetItem(IMFTopologyNode *iface, REFGUID key, PROPVARIANT *value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetItem(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_GetItemType(IMFTopologyNode *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), type);

    return IMFAttributes_GetItemType(node->attributes, key, type);
}

static HRESULT WINAPI topology_node_CompareItem(IMFTopologyNode *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), value, result);

    return IMFAttributes_CompareItem(node->attributes, key, value, result);
}

static HRESULT WINAPI topology_node_Compare(IMFTopologyNode *iface, IMFAttributes *theirs,
        MF_ATTRIBUTES_MATCH_TYPE type, BOOL *result)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %p, %d, %p.\n", iface, theirs, type, result);

    return IMFAttributes_Compare(node->attributes, theirs, type, result);
}

static HRESULT WINAPI topology_node_GetUINT32(IMFTopologyNode *iface, REFGUID key, UINT32 *value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetUINT32(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_GetUINT64(IMFTopologyNode *iface, REFGUID key, UINT64 *value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetUINT64(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_GetDouble(IMFTopologyNode *iface, REFGUID key, double *value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetDouble(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_GetGUID(IMFTopologyNode *iface, REFGUID key, GUID *value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_GetGUID(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_GetStringLength(IMFTopologyNode *iface, REFGUID key, UINT32 *length)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), length);

    return IMFAttributes_GetStringLength(node->attributes, key, length);
}

static HRESULT WINAPI topology_node_GetString(IMFTopologyNode *iface, REFGUID key, WCHAR *value,
        UINT32 size, UINT32 *length)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p, %d, %p.\n", iface, debugstr_guid(key), value, size, length);

    return IMFAttributes_GetString(node->attributes, key, value, size, length);
}

static HRESULT WINAPI topology_node_GetAllocatedString(IMFTopologyNode *iface, REFGUID key,
        WCHAR **value, UINT32 *length)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), value, length);

    return IMFAttributes_GetAllocatedString(node->attributes, key, value, length);
}

static HRESULT WINAPI topology_node_GetBlobSize(IMFTopologyNode *iface, REFGUID key, UINT32 *size)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), size);

    return IMFAttributes_GetBlobSize(node->attributes, key, size);
}

static HRESULT WINAPI topology_node_GetBlob(IMFTopologyNode *iface, REFGUID key, UINT8 *buf,
        UINT32 bufsize, UINT32 *blobsize)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p, %d, %p.\n", iface, debugstr_guid(key), buf, bufsize, blobsize);

    return IMFAttributes_GetBlob(node->attributes, key, buf, bufsize, blobsize);
}

static HRESULT WINAPI topology_node_GetAllocatedBlob(IMFTopologyNode *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p, %p.\n", iface, debugstr_guid(key), buf, size);

    return IMFAttributes_GetAllocatedBlob(node->attributes, key, buf, size);
}

static HRESULT WINAPI topology_node_GetUnknown(IMFTopologyNode *iface, REFGUID key, REFIID riid, void **ppv)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %s, %p.\n", iface, debugstr_guid(key), debugstr_guid(riid), ppv);

    return IMFAttributes_GetUnknown(node->attributes, key, riid, ppv);
}

static HRESULT WINAPI topology_node_SetItem(IMFTopologyNode *iface, REFGUID key, REFPROPVARIANT value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetItem(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_DeleteItem(IMFTopologyNode *iface, REFGUID key)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s.\n", iface, debugstr_guid(key));

    return IMFAttributes_DeleteItem(node->attributes, key);
}

static HRESULT WINAPI topology_node_DeleteAllItems(IMFTopologyNode *iface)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_DeleteAllItems(node->attributes);
}

static HRESULT WINAPI topology_node_SetUINT32(IMFTopologyNode *iface, REFGUID key, UINT32 value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %d.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetUINT32(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_SetUINT64(IMFTopologyNode *iface, REFGUID key, UINT64 value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), wine_dbgstr_longlong(value));

    return IMFAttributes_SetUINT64(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_SetDouble(IMFTopologyNode *iface, REFGUID key, double value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %f.\n", iface, debugstr_guid(key), value);

    return IMFAttributes_SetDouble(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_SetGUID(IMFTopologyNode *iface, REFGUID key, REFGUID value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), debugstr_guid(value));

    return IMFAttributes_SetGUID(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_SetString(IMFTopologyNode *iface, REFGUID key, const WCHAR *value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_guid(key), debugstr_w(value));

    return IMFAttributes_SetString(node->attributes, key, value);
}

static HRESULT WINAPI topology_node_SetBlob(IMFTopologyNode *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p, %d.\n", iface, debugstr_guid(key), buf, size);

    return IMFAttributes_SetBlob(node->attributes, key, buf, size);
}

static HRESULT WINAPI topology_node_SetUnknown(IMFTopologyNode *iface, REFGUID key, IUnknown *unknown)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(key), unknown);

    return IMFAttributes_SetUnknown(node->attributes, key, unknown);
}

static HRESULT WINAPI topology_node_LockStore(IMFTopologyNode *iface)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_LockStore(node->attributes);
}

static HRESULT WINAPI topology_node_UnlockStore(IMFTopologyNode *iface)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p.\n", iface);

    return IMFAttributes_UnlockStore(node->attributes);
}

static HRESULT WINAPI topology_node_GetCount(IMFTopologyNode *iface, UINT32 *count)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %p.\n", iface, count);

    return IMFAttributes_GetCount(node->attributes, count);
}

static HRESULT WINAPI topology_node_GetItemByIndex(IMFTopologyNode *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %u, %p, %p.\n", iface, index, key, value);

    return IMFAttributes_GetItemByIndex(node->attributes, index, key, value);
}

static HRESULT WINAPI topology_node_CopyAllItems(IMFTopologyNode *iface, IMFAttributes *dest)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %p.\n", iface, dest);

    return IMFAttributes_CopyAllItems(node->attributes, dest);
}

static HRESULT topology_node_set_object(struct topology_node *node, IUnknown *object)
{
    static const GUID *iids[3] = { &IID_IPersist, &IID_IPersistStorage, &IID_IPersistPropertyBag };
    IPersist *persist = NULL;
    BOOL has_object_id;
    GUID object_id;
    unsigned int i;
    HRESULT hr;

    has_object_id = IMFAttributes_GetGUID(node->attributes, &MF_TOPONODE_TRANSFORM_OBJECTID, &object_id) == S_OK;

    if (object && !has_object_id)
    {
        for (i = 0; i < ARRAY_SIZE(iids); ++i)
        {
            persist = NULL;
            if (SUCCEEDED(hr = IUnknown_QueryInterface(object, iids[i], (void **)&persist)))
                break;
        }

        if (persist)
        {
            if (FAILED(hr = IPersist_GetClassID(persist, &object_id)))
            {
                IPersist_Release(persist);
                persist = NULL;
            }
        }
    }

    EnterCriticalSection(&node->cs);

    if (node->object)
        IUnknown_Release(node->object);
    node->object = object;
    if (node->object)
        IUnknown_AddRef(node->object);

    if (persist)
        IMFAttributes_SetGUID(node->attributes, &MF_TOPONODE_TRANSFORM_OBJECTID, &object_id);

    LeaveCriticalSection(&node->cs);

    if (persist)
        IPersist_Release(persist);

    return S_OK;
}

static HRESULT WINAPI topology_node_SetObject(IMFTopologyNode *iface, IUnknown *object)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %p.\n", iface, object);

    return topology_node_set_object(node, object);
}

static HRESULT WINAPI topology_node_GetObject(IMFTopologyNode *iface, IUnknown **object)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %p.\n", iface, object);

    if (!object)
        return E_POINTER;

    EnterCriticalSection(&node->cs);

    *object = node->object;
    if (*object)
        IUnknown_AddRef(*object);

    LeaveCriticalSection(&node->cs);

    return *object ? S_OK : E_FAIL;
}

static HRESULT WINAPI topology_node_GetNodeType(IMFTopologyNode *iface, MF_TOPOLOGY_TYPE *node_type)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %p.\n", iface, node_type);

    *node_type = node->node_type;

    return S_OK;
}

static HRESULT WINAPI topology_node_GetTopoNodeID(IMFTopologyNode *iface, TOPOID *id)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %p.\n", iface, id);

    *id = node->id;

    return S_OK;
}

static HRESULT WINAPI topology_node_SetTopoNodeID(IMFTopologyNode *iface, TOPOID id)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %s.\n", iface, wine_dbgstr_longlong(id));

    node->id = id;

    return S_OK;
}

static HRESULT WINAPI topology_node_GetInputCount(IMFTopologyNode *iface, DWORD *count)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %p.\n", iface, count);

    *count = node->inputs.count;

    return S_OK;
}

static HRESULT WINAPI topology_node_GetOutputCount(IMFTopologyNode *iface, DWORD *count)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %p.\n", iface, count);

    *count = node->outputs.count;

    return S_OK;
}

static void topology_node_set_stream_type(struct node_stream *stream, IMFMediaType *mediatype)
{
    if (stream->preferred_type)
        IMFMediaType_Release(stream->preferred_type);
    stream->preferred_type = mediatype;
    if (stream->preferred_type)
        IMFMediaType_AddRef(stream->preferred_type);
}

static HRESULT topology_node_connect_output(struct topology_node *node, DWORD output_index,
        struct topology_node *connection, DWORD input_index)
{
    struct node_stream *stream;
    HRESULT hr;

    if (node->node_type == MF_TOPOLOGY_OUTPUT_NODE || connection->node_type == MF_TOPOLOGY_SOURCESTREAM_NODE)
         return E_FAIL;

    EnterCriticalSection(&node->cs);
    EnterCriticalSection(&connection->cs);

    topology_node_disconnect_output(node, output_index);
    if (input_index < connection->inputs.count)
    {
        stream = &connection->inputs.streams[input_index];
        if (stream->connection)
            topology_node_disconnect_output(stream->connection, stream->connection_stream);
    }

    hr = topology_node_reserve_streams(&node->outputs, output_index);
    if (SUCCEEDED(hr))
    {
        size_t old_count = connection->inputs.count;
        hr = topology_node_reserve_streams(&connection->inputs, input_index);
        if (SUCCEEDED(hr) && !old_count && connection->input_type)
        {
            topology_node_set_stream_type(connection->inputs.streams, connection->input_type);
            IMFMediaType_Release(connection->input_type);
            connection->input_type = NULL;
        }
    }

    if (SUCCEEDED(hr))
    {
        node->outputs.streams[output_index].connection = connection;
        IMFTopologyNode_AddRef(&node->outputs.streams[output_index].connection->IMFTopologyNode_iface);
        node->outputs.streams[output_index].connection_stream = input_index;
        connection->inputs.streams[input_index].connection = node;
        IMFTopologyNode_AddRef(&connection->inputs.streams[input_index].connection->IMFTopologyNode_iface);
        connection->inputs.streams[input_index].connection_stream = output_index;
    }

    LeaveCriticalSection(&connection->cs);
    LeaveCriticalSection(&node->cs);

    return hr;
}

static HRESULT WINAPI topology_node_ConnectOutput(IMFTopologyNode *iface, DWORD output_index,
        IMFTopologyNode *peer, DWORD input_index)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);
    struct topology_node *connection = unsafe_impl_from_IMFTopologyNode(peer);

    TRACE("%p, %u, %p, %u.\n", iface, output_index, peer, input_index);

    if (!connection)
    {
        WARN("External node implementations are not supported.\n");
        return E_UNEXPECTED;
    }

    return topology_node_connect_output(node, output_index, connection, input_index);
}

static HRESULT WINAPI topology_node_DisconnectOutput(IMFTopologyNode *iface, DWORD output_index)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);

    TRACE("%p, %u.\n", iface, output_index);

    return topology_node_disconnect_output(node, output_index);
}

static HRESULT WINAPI topology_node_GetInput(IMFTopologyNode *iface, DWORD input_index, IMFTopologyNode **ret,
        DWORD *output_index)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %p, %p.\n", iface, input_index, ret, output_index);

    EnterCriticalSection(&node->cs);

    if (input_index < node->inputs.count)
    {
        const struct node_stream *stream = &node->inputs.streams[input_index];

        if (stream->connection)
        {
            *ret = &stream->connection->IMFTopologyNode_iface;
            IMFTopologyNode_AddRef(*ret);
            *output_index = stream->connection_stream;
        }
        else
            hr = MF_E_NOT_FOUND;
    }
    else
        hr = E_INVALIDARG;

    LeaveCriticalSection(&node->cs);

    return hr;
}

static HRESULT WINAPI topology_node_GetOutput(IMFTopologyNode *iface, DWORD output_index, IMFTopologyNode **ret,
        DWORD *input_index)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %p, %p.\n", iface, output_index, ret, input_index);

    EnterCriticalSection(&node->cs);

    if (output_index < node->outputs.count)
    {
        const struct node_stream *stream = &node->outputs.streams[output_index];

        if (stream->connection)
        {
            *ret = &stream->connection->IMFTopologyNode_iface;
            IMFTopologyNode_AddRef(*ret);
            *input_index = stream->connection_stream;
        }
        else
            hr = MF_E_NOT_FOUND;
    }
    else
        hr = E_INVALIDARG;

    LeaveCriticalSection(&node->cs);

    return hr;
}

static HRESULT WINAPI topology_node_SetOutputPrefType(IMFTopologyNode *iface, DWORD index, IMFMediaType *mediatype)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %p.\n", iface, index, mediatype);

    EnterCriticalSection(&node->cs);

    if (node->node_type != MF_TOPOLOGY_OUTPUT_NODE)
    {
        if (SUCCEEDED(hr = topology_node_reserve_streams(&node->outputs, index)))
            topology_node_set_stream_type(&node->outputs.streams[index], mediatype);
    }
    else
        hr = E_NOTIMPL;

    LeaveCriticalSection(&node->cs);

    return hr;
}

static HRESULT topology_node_get_pref_type(struct node_streams *streams, unsigned int index, IMFMediaType **mediatype)
{
    *mediatype = streams->streams[index].preferred_type;
    if (*mediatype)
    {
        IMFMediaType_AddRef(*mediatype);
        return S_OK;
    }

    return E_FAIL;
}

static HRESULT WINAPI topology_node_GetOutputPrefType(IMFTopologyNode *iface, DWORD index, IMFMediaType **mediatype)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %p.\n", iface, index, mediatype);

    EnterCriticalSection(&node->cs);

    if (index < node->outputs.count)
        hr = topology_node_get_pref_type(&node->outputs, index, mediatype);
    else
        hr = E_INVALIDARG;

    LeaveCriticalSection(&node->cs);

    return hr;
}

static HRESULT WINAPI topology_node_SetInputPrefType(IMFTopologyNode *iface, DWORD index, IMFMediaType *mediatype)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %p.\n", iface, index, mediatype);

    EnterCriticalSection(&node->cs);

    switch (node->node_type)
    {
        case MF_TOPOLOGY_TEE_NODE:
            if (index)
            {
                hr = MF_E_INVALIDTYPE;
                break;
            }
            if (node->inputs.count)
                topology_node_set_stream_type(&node->inputs.streams[index], mediatype);
            else
            {
                if (node->input_type)
                    IMFMediaType_Release(node->input_type);
                node->input_type = mediatype;
                if (node->input_type)
                    IMFMediaType_AddRef(node->input_type);
            }
            break;
        case MF_TOPOLOGY_SOURCESTREAM_NODE:
            hr = E_NOTIMPL;
            break;
        default:
            if (SUCCEEDED(hr = topology_node_reserve_streams(&node->inputs, index)))
                topology_node_set_stream_type(&node->inputs.streams[index], mediatype);
    }

    LeaveCriticalSection(&node->cs);

    return hr;
}

static HRESULT WINAPI topology_node_GetInputPrefType(IMFTopologyNode *iface, DWORD index, IMFMediaType **mediatype)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u, %p.\n", iface, index, mediatype);

    EnterCriticalSection(&node->cs);

    if (index < node->inputs.count)
    {
        hr = topology_node_get_pref_type(&node->inputs, index, mediatype);
    }
    else if (node->node_type == MF_TOPOLOGY_TEE_NODE && node->input_type)
    {
        *mediatype = node->input_type;
        IMFMediaType_AddRef(*mediatype);
    }
    else
        hr = E_INVALIDARG;

    LeaveCriticalSection(&node->cs);

    return hr;
}

static HRESULT WINAPI topology_node_CloneFrom(IMFTopologyNode *iface, IMFTopologyNode *src_node)
{
    struct topology_node *node = impl_from_IMFTopologyNode(iface);
    MF_TOPOLOGY_TYPE node_type;
    IMFMediaType *mediatype;
    IUnknown *object;
    DWORD count, i;
    TOPOID topoid;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, src_node);

    if (FAILED(hr = IMFTopologyNode_GetNodeType(src_node, &node_type)))
        return hr;

    if (node->node_type != node_type)
        return MF_E_INVALIDREQUEST;

    if (FAILED(hr = IMFTopologyNode_GetTopoNodeID(src_node, &topoid)))
        return hr;

    object = NULL;
    IMFTopologyNode_GetObject(src_node, &object);

    EnterCriticalSection(&node->cs);

    hr = IMFTopologyNode_CopyAllItems(src_node, node->attributes);

    if (SUCCEEDED(hr))
        hr = topology_node_set_object(node, object);

    if (SUCCEEDED(hr))
        node->id = topoid;

    if (SUCCEEDED(IMFTopologyNode_GetInputCount(src_node, &count)))
    {
        for (i = 0; i < count; ++i)
        {
            if (SUCCEEDED(IMFTopologyNode_GetInputPrefType(src_node, i, &mediatype)))
            {
                IMFTopologyNode_SetInputPrefType(iface, i, mediatype);
                IMFMediaType_Release(mediatype);
            }
        }
    }

    if (SUCCEEDED(IMFTopologyNode_GetOutputCount(src_node, &count)))
    {
        for (i = 0; i < count; ++i)
        {
            if (SUCCEEDED(IMFTopologyNode_GetOutputPrefType(src_node, i, &mediatype)))
            {
                IMFTopologyNode_SetOutputPrefType(iface, i, mediatype);
                IMFMediaType_Release(mediatype);
            }
        }
    }

    LeaveCriticalSection(&node->cs);

    if (object)
        IUnknown_Release(object);

    return hr;
}

static const IMFTopologyNodeVtbl topologynodevtbl =
{
    topology_node_QueryInterface,
    topology_node_AddRef,
    topology_node_Release,
    topology_node_GetItem,
    topology_node_GetItemType,
    topology_node_CompareItem,
    topology_node_Compare,
    topology_node_GetUINT32,
    topology_node_GetUINT64,
    topology_node_GetDouble,
    topology_node_GetGUID,
    topology_node_GetStringLength,
    topology_node_GetString,
    topology_node_GetAllocatedString,
    topology_node_GetBlobSize,
    topology_node_GetBlob,
    topology_node_GetAllocatedBlob,
    topology_node_GetUnknown,
    topology_node_SetItem,
    topology_node_DeleteItem,
    topology_node_DeleteAllItems,
    topology_node_SetUINT32,
    topology_node_SetUINT64,
    topology_node_SetDouble,
    topology_node_SetGUID,
    topology_node_SetString,
    topology_node_SetBlob,
    topology_node_SetUnknown,
    topology_node_LockStore,
    topology_node_UnlockStore,
    topology_node_GetCount,
    topology_node_GetItemByIndex,
    topology_node_CopyAllItems,
    topology_node_SetObject,
    topology_node_GetObject,
    topology_node_GetNodeType,
    topology_node_GetTopoNodeID,
    topology_node_SetTopoNodeID,
    topology_node_GetInputCount,
    topology_node_GetOutputCount,
    topology_node_ConnectOutput,
    topology_node_DisconnectOutput,
    topology_node_GetInput,
    topology_node_GetOutput,
    topology_node_SetOutputPrefType,
    topology_node_GetOutputPrefType,
    topology_node_SetInputPrefType,
    topology_node_GetInputPrefType,
    topology_node_CloneFrom,
};

static HRESULT create_topology_node(MF_TOPOLOGY_TYPE node_type, struct topology_node **node)
{
    HRESULT hr;

    *node = heap_alloc_zero(sizeof(**node));
    if (!*node)
        return E_OUTOFMEMORY;

    (*node)->IMFTopologyNode_iface.lpVtbl = &topologynodevtbl;
    (*node)->refcount = 1;
    (*node)->node_type = node_type;
    hr = MFCreateAttributes(&(*node)->attributes, 0);
    if (FAILED(hr))
    {
        heap_free(*node);
        return hr;
    }
    (*node)->id = ((TOPOID)GetCurrentProcessId() << 32) | InterlockedIncrement(&next_node_id);
    InitializeCriticalSection(&(*node)->cs);

    return S_OK;
}

/***********************************************************************
 *      MFCreateTopologyNode (mf.@)
 */
HRESULT WINAPI MFCreateTopologyNode(MF_TOPOLOGY_TYPE node_type, IMFTopologyNode **node)
{
    struct topology_node *object;
    HRESULT hr;

    TRACE("%d, %p.\n", node_type, node);

    if (!node)
        return E_POINTER;

    hr = create_topology_node(node_type, &object);
    if (SUCCEEDED(hr))
        *node = &object->IMFTopologyNode_iface;

    return hr;
}

/***********************************************************************
 *      MFGetTopoNodeCurrentType (mf.@)
 */
HRESULT WINAPI MFGetTopoNodeCurrentType(IMFTopologyNode *node, DWORD stream, BOOL output, IMFMediaType **type)
{
    IMFMediaTypeHandler *type_handler;
    MF_TOPOLOGY_TYPE node_type;
    IMFStreamSink *stream_sink;
    IMFStreamDescriptor *sd;
    IMFTransform *transform;
    UINT32 primary_output;
    IUnknown *object;
    HRESULT hr;

    TRACE("%p, %u, %d, %p.\n", node, stream, output, type);

    if (FAILED(hr = IMFTopologyNode_GetNodeType(node, &node_type)))
        return hr;

    switch (node_type)
    {
        case MF_TOPOLOGY_OUTPUT_NODE:
            if (FAILED(hr = IMFTopologyNode_GetObject(node, &object)))
                return hr;

            hr = IUnknown_QueryInterface(object, &IID_IMFStreamSink, (void **)&stream_sink);
            IUnknown_Release(object);
            if (SUCCEEDED(hr))
            {
                hr = IMFStreamSink_GetMediaTypeHandler(stream_sink, &type_handler);
                IMFStreamSink_Release(stream_sink);

                if (SUCCEEDED(hr))
                {
                    hr = IMFMediaTypeHandler_GetCurrentMediaType(type_handler, type);
                    IMFMediaTypeHandler_Release(type_handler);
                }
            }
            break;
        case MF_TOPOLOGY_SOURCESTREAM_NODE:
            if (FAILED(hr = IMFTopologyNode_GetUnknown(node, &MF_TOPONODE_STREAM_DESCRIPTOR, &IID_IMFStreamDescriptor,
                    (void **)&sd)))
            {
                return hr;
            }

            hr = IMFStreamDescriptor_GetMediaTypeHandler(sd, &type_handler);
            IMFStreamDescriptor_Release(sd);
            if (SUCCEEDED(hr))
            {
                hr = IMFMediaTypeHandler_GetCurrentMediaType(type_handler, type);
                IMFMediaTypeHandler_Release(type_handler);
            }
            break;
        case MF_TOPOLOGY_TRANSFORM_NODE:
            if (FAILED(hr = IMFTopologyNode_GetObject(node, &object)))
                return hr;

            hr = IUnknown_QueryInterface(object, &IID_IMFTransform, (void **)&transform);
            IUnknown_Release(object);
            if (SUCCEEDED(hr))
            {
                if (output)
                    hr = IMFTransform_GetOutputCurrentType(transform, stream, type);
                else
                    hr = IMFTransform_GetInputCurrentType(transform, stream, type);
                IMFTransform_Release(transform);
            }
            break;
        case MF_TOPOLOGY_TEE_NODE:
            if (SUCCEEDED(hr = IMFTopologyNode_GetInputPrefType(node, 0, type)))
                break;

            if (FAILED(IMFTopologyNode_GetUINT32(node, &MF_TOPONODE_PRIMARYOUTPUT, &primary_output)))
                primary_output = 0;

            hr = IMFTopologyNode_GetOutputPrefType(node, primary_output, type);
            break;
        default:
            ;
    }

    return hr;
}

static HRESULT WINAPI topology_loader_QueryInterface(IMFTopoLoader *iface, REFIID riid, void **out)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(riid, &IID_IMFTopoLoader) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        IMFTopoLoader_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI topology_loader_AddRef(IMFTopoLoader *iface)
{
    struct topology_loader *loader = impl_from_IMFTopoLoader(iface);
    ULONG refcount = InterlockedIncrement(&loader->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI topology_loader_Release(IMFTopoLoader *iface)
{
    struct topology_loader *loader = impl_from_IMFTopoLoader(iface);
    ULONG refcount = InterlockedDecrement(&loader->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        heap_free(loader);
    }

    return refcount;
}

static void topology_loader_add_branch(struct topology *topology, IMFTopologyNode *first, IMFTopologyNode *last)
{
    IMFTopology *full_topo = &topology->IMFTopology_iface;
    IMFTopologyNode *in, *out;
    DWORD index;

    in = first;
    IMFTopology_AddNode(full_topo, in);
    while (SUCCEEDED(IMFTopologyNode_GetOutput(in, 0, &out, &index)))
    {
        IMFTopology_AddNode(full_topo, out);
        in = out;
    }
}

struct available_output_type
{
    IMFMediaType *type;
    IMFTransform *transform;
};

static HRESULT topology_loader_enumerate_output_types(GUID *category, IMFMediaType *input_type, HRESULT (*new_type)(struct available_output_type *, void *), void *context)
{
    MFT_REGISTER_TYPE_INFO mft_typeinfo;
    GUID major_type, subtype;
    IMFActivate **activates;
    UINT32 num_activates;
    HRESULT hr;

    if (FAILED(hr = IMFMediaType_GetMajorType(input_type, &major_type)))
        return hr;

    if (FAILED(hr = IMFMediaType_GetGUID(input_type, &MF_MT_SUBTYPE, &subtype)))
        return hr;

    mft_typeinfo.guidMajorType = major_type;
    mft_typeinfo.guidSubtype = subtype;

    if (FAILED(hr = MFTEnumEx(*category, MFT_ENUM_FLAG_ALL, &mft_typeinfo, NULL, &activates, &num_activates)))
        return hr;

    hr = E_FAIL;

    for (unsigned int i = 0; i < num_activates; i++)
    {
        IMFTransform *mft;

        IMFActivate_ActivateObject(activates[i], &IID_IMFTransform, (void**) &mft);

        if (SUCCEEDED(hr = IMFTransform_SetInputType(mft, 0, input_type, 0)))
        {
            struct available_output_type avail = {.transform = mft};
            unsigned int output_count = 0;

            while (SUCCEEDED(IMFTransform_GetOutputAvailableType(mft, 0, output_count++, &avail.type)))
            {
                if (SUCCEEDED(hr = new_type(&avail, context)))
                {
                    IMFActivate_ShutdownObject(activates[i]);
                    return hr;
                }
            }
        }

        IMFActivate_ShutdownObject(activates[i]);
    }

    return hr;
}

struct connect_to_sink_context
{
    IMFTopologyNode *src, *sink;
    IMFMediaTypeHandler *sink_mth;
};

HRESULT connect_to_sink(struct available_output_type *type, void *context)
{
    IMFTopologyNode *node;
    struct connect_to_sink_context *ctx = context;

    if (SUCCEEDED(IMFMediaTypeHandler_IsMediaTypeSupported(ctx->sink_mth, type->type, NULL)))
    {
        MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &node);
        IMFTopologyNode_SetObject(node, (IUnknown *) type->transform);
        IMFTopologyNode_ConnectOutput(ctx->src, 0, node, 0);
        IMFTopologyNode_ConnectOutput(node, 0, ctx->sink, 0);

        IMFMediaTypeHandler_SetCurrentMediaType(ctx->sink_mth, type->type);
        IMFTransform_SetOutputType(type->transform, 0, type->type, 0);

        return S_OK;
    }
    return E_FAIL;
}

struct connect_to_converter_context
{
    struct connect_to_sink_context sink_ctx;
    GUID *converter_category;
};

HRESULT connect_to_converter(struct available_output_type *type, void *context)
{
    struct connect_to_converter_context *ctx = context;
    struct connect_to_sink_context sink_ctx;
    IMFTopologyNode *node;

    MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &node);
    IMFTopologyNode_SetObject(node, (IUnknown *) type->transform);

    sink_ctx = ctx->sink_ctx;
    sink_ctx.src = node;
    if (SUCCEEDED(topology_loader_enumerate_output_types(ctx->converter_category, type->type, connect_to_sink, &sink_ctx)))
    {
        IMFTopologyNode_ConnectOutput(ctx->sink_ctx.src, 0, node, 0);

        IMFTransform_SetOutputType(type->transform, 0, type->type, 0);

        return S_OK;
    }
    IMFTopologyNode_Release(node);
    return E_FAIL;
}

static HRESULT topology_loader_resolve_branch(IMFTopologyNode *src, IMFMediaType *mediatype, IMFTopologyNode *sink, MF_CONNECT_METHOD method)
{
    struct connect_to_converter_context convert_ctx;
    struct connect_to_sink_context sink_ctx;
    GUID major_type, decode_cat, convert_cat;
    IMFStreamSink *streamsink;
    IMFMediaTypeHandler *mth;
    HRESULT hr;

    TRACE("method %u\n", method);

    IMFTopologyNode_GetObject(sink, (IUnknown **)&streamsink);
    IMFStreamSink_GetMediaTypeHandler(streamsink, &mth);

    if (SUCCEEDED(hr = IMFMediaTypeHandler_IsMediaTypeSupported(mth, mediatype, NULL)))
    {
        IMFMediaTypeHandler_SetCurrentMediaType(mth, mediatype);
        return IMFTopologyNode_ConnectOutput(src, 0, sink, 0);
    }

    if (FAILED(hr = IMFMediaType_GetMajorType(mediatype, &major_type)))
        return hr;

    if (IsEqualGUID(&major_type, &MFMediaType_Audio))
    {
        decode_cat = MFT_CATEGORY_AUDIO_DECODER;
        convert_cat = MFT_CATEGORY_AUDIO_EFFECT;
    }
    else if (IsEqualGUID(&major_type, &MFMediaType_Video))
    {
        decode_cat = MFT_CATEGORY_VIDEO_DECODER;
        convert_cat = MFT_CATEGORY_VIDEO_EFFECT;
    }
    else
        return E_FAIL;

    sink_ctx.src = src;
    sink_ctx.sink = sink;
    sink_ctx.sink_mth = mth;

    convert_ctx.sink_ctx = sink_ctx;
    convert_ctx.converter_category = &convert_cat;

    hr = E_FAIL;

    if (method & MF_CONNECT_ALLOW_CONVERTER)
    {
        if (SUCCEEDED(hr = topology_loader_enumerate_output_types(&convert_cat, mediatype, connect_to_sink, &sink_ctx)))
            goto done;
    }

    /* 2 = decoder flag */
    if (method & 2)
    {
        if (method & MF_CONNECT_ALLOW_CONVERTER)
        {
            if (SUCCEEDED(hr = topology_loader_enumerate_output_types(&decode_cat, mediatype, connect_to_converter, &convert_ctx)))
                goto done;
        }
        else
        {
            if (SUCCEEDED(hr = topology_loader_enumerate_output_types(&decode_cat, mediatype, connect_to_sink, &sink_ctx)))
                goto done;
        }
    }

    done:
    IMFMediaTypeHandler_Release(mth);
    IMFStreamSink_Release(streamsink);
    return hr;
}

static HRESULT topology_loader_resolve_partial_topology(struct topology_node *src, struct topology_node *sink, struct topology *topology, struct topology **full_topology)
{
    UINT32 method, src_method, sink_method, enum_src_types, streamid;
    IMFMediaTypeHandler *mth_src, *mth_sink;
    IMFTopologyNode *clone_src, *clone_sink;
    IMFMediaType **src_mediatypes;
    IMFStreamDescriptor *desc;
    IMFAttributes *attrs_sink;
    IMFAttributes *attrs_src;
    IMFStreamSink *strm_sink;
    IMFMediaType *mtype_src;
    DWORD num_media_types;
    HRESULT hr;
    int i;

    attrs_sink = sink->attributes;
    attrs_src = src->attributes;
    if (FAILED(hr = IMFAttributes_GetUnknown(attrs_src, &MF_TOPONODE_STREAM_DESCRIPTOR, &IID_IMFStreamDescriptor, (void **)&desc)))
        return hr;
    strm_sink = (IMFStreamSink *)sink->object;

    if (FAILED(hr = IMFStreamDescriptor_GetMediaTypeHandler(desc, &mth_src)))
    {
        IMFStreamDescriptor_Release(desc);
        return hr;
    }
    if (FAILED(hr = IMFStreamSink_GetMediaTypeHandler(strm_sink, &mth_sink)))
    {
        IMFStreamDescriptor_Release(desc);
        IMFMediaTypeHandler_Release(mth_src);
        return hr;
    }

    hr = IMFTopology_GetUINT32(&topology->IMFTopology_iface, &MF_TOPOLOGY_ENUMERATE_SOURCE_TYPES, &enum_src_types);

    mtype_src = NULL;
    if (FAILED(hr) || !enum_src_types)
    {
        num_media_types = 1;
        enum_src_types = 0;
        if (FAILED(hr = IMFMediaTypeHandler_GetCurrentMediaType(mth_src, &mtype_src)))
            if (FAILED(hr = IMFMediaTypeHandler_GetMediaTypeByIndex(mth_src, 0, &mtype_src)))
            {
                IMFMediaTypeHandler_Release(mth_src);
                IMFMediaTypeHandler_Release(mth_sink);
                IMFStreamDescriptor_Release(desc);
                return hr;
            }
    }
    else
        IMFMediaTypeHandler_GetMediaTypeCount(mth_src, &num_media_types);

    src_mediatypes = heap_alloc(sizeof(IMFMediaType *) * num_media_types);

    if (mtype_src)
        src_mediatypes[0] = mtype_src;
    else
        for (i = 0; i < num_media_types; i++)
            IMFMediaTypeHandler_GetMediaTypeByIndex(mth_src, i, &src_mediatypes[i]);


    MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &clone_src);
    MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &clone_sink);
    IMFTopologyNode_CloneFrom(clone_src, &src->IMFTopologyNode_iface);
    IMFTopologyNode_CloneFrom(clone_sink, &sink->IMFTopologyNode_iface);

    if (FAILED(IMFTopologyNode_GetUINT32(clone_sink, &MF_TOPONODE_STREAMID, &streamid)))
        IMFTopologyNode_SetUINT32(clone_sink, &MF_TOPONODE_STREAMID, 0);

    if (FAILED(IMFAttributes_GetUINT32(attrs_sink, &MF_TOPONODE_CONNECT_METHOD, &sink_method)))
        sink_method = MF_CONNECT_ALLOW_DECODER;

    if (enum_src_types)
    {
        hr = IMFAttributes_GetUINT32(attrs_src, &MF_TOPONODE_CONNECT_METHOD, &src_method);
        if (hr == S_OK && src_method != MF_CONNECT_RESOLVE_INDEPENDENT_OUTPUTTYPES)
        {
            for (method = MF_CONNECT_DIRECT; method <= sink_method; method++)
                for (i = 0; i < num_media_types; i++)
                    if (SUCCEEDED(topology_loader_resolve_branch(clone_src, src_mediatypes[i], clone_sink, method)))
                    {
                        topology_loader_add_branch(*full_topology, clone_src, clone_sink);
                        heap_free(src_mediatypes);
                        return S_OK;
                    }
        }
        else
        {
            for (i = 0; i < num_media_types; i++)
                for (method = MF_CONNECT_DIRECT; method <= sink_method; method++)
                    if (SUCCEEDED(topology_loader_resolve_branch(clone_src, src_mediatypes[i], clone_sink, method)))
                    {
                        topology_loader_add_branch(*full_topology, clone_src, clone_sink);
                        heap_free(src_mediatypes);
                        return S_OK;
                    }
        }
    }
    else
    {
        for (method = MF_CONNECT_DIRECT; method <= sink_method; method++)
            if (SUCCEEDED(topology_loader_resolve_branch(clone_src, src_mediatypes[0], clone_sink, method)))
            {
                TRACE("Successfully connected nodes with method %u\n", method);
                topology_loader_add_branch(*full_topology, clone_src, clone_sink);
                heap_free(src_mediatypes);
                return S_OK;
            }
    }

    heap_free(src_mediatypes);
    return MF_E_TOPO_UNSUPPORTED;
}

static HRESULT WINAPI topology_loader_Load(IMFTopoLoader *iface, IMFTopology *input_topology,
        IMFTopology **output_topology, IMFTopology *current_topology)
{
    struct topology *topology = unsafe_impl_from_IMFTopology(input_topology);
    struct topology_node *(*node_pairs)[2];
    int num_connections;
    IMFStreamSink *sink;
    HRESULT hr;
    int i, idx;

    FIXME("%p, %p, %p, %p.\n", iface, input_topology, output_topology, current_topology);

    if (current_topology)
        FIXME("Current topology instance is ignored.\n");

    if (!topology || topology->nodes.count < 2)
        return MF_E_TOPO_UNSUPPORTED;

    num_connections = 0;
    for (i = 0; i < topology->nodes.count; i++)
    {
        struct topology_node *node = topology->nodes.nodes[i];

        if (node->node_type == MF_TOPOLOGY_SOURCESTREAM_NODE)
        {
            if (node->outputs.count && node->outputs.streams->connection)
                num_connections++;
        }
    }

    if (!num_connections)
        return MF_E_TOPO_UNSUPPORTED;

    node_pairs = heap_alloc_zero(sizeof(struct topology_node *[2]) * num_connections);

    idx = 0;
    for (i = 0; i < topology->nodes.count; ++i)
    {
        struct topology_node *node = topology->nodes.nodes[i];

        if (node->node_type == MF_TOPOLOGY_SOURCESTREAM_NODE)
        {
            if (node->outputs.count && node->outputs.streams->connection)
            {
                node_pairs[idx][0] = node;
                if (node->outputs.streams->connection->node_type == MF_TOPOLOGY_TRANSFORM_NODE)
                {
                    struct topology_node *sink = node->outputs.streams->connection;

                    while (sink && sink->node_type != MF_TOPOLOGY_OUTPUT_NODE && sink->outputs.count)
                        sink = sink->outputs.streams->connection;
                    if (!sink || !sink->outputs.count)
                    {
                        FIXME("Check for MF_CONNECT_AS_OPTIONAL and MF_CONNECT_AS_OPTIONAL_BRANCH flags.\n");
                        heap_free(node_pairs);
                        return MF_E_TOPO_UNSUPPORTED;
                    }
                    node_pairs[idx][1] = sink;
                }
                else if (node->outputs.streams->connection->node_type == MF_TOPOLOGY_OUTPUT_NODE)
                    node_pairs[idx][1] = node->outputs.streams->connection;
                else {
                    FIXME("Tee nodes currently unhandled.\n");
                    heap_free(node_pairs);
                    return MF_E_TOPO_UNSUPPORTED;
                }
                idx++;
            }
        }
    }

    /* all sinks must be activated */
    for (i = 0; i < num_connections; i++)
    {
        if (FAILED(IUnknown_QueryInterface(node_pairs[i][1]->object, &IID_IMFStreamSink, (void **)&sink)))
        {
            FIXME("Check for MF_CONNECT_AS_OPTIONAL and MF_CONNECT_AS_OPTIONAL_BRANCH flags.\n");
            heap_free(node_pairs);
            return MF_E_TOPO_SINK_ACTIVATES_UNSUPPORTED;
        }
        IMFStreamSink_Release(sink);
    }

    if (FAILED(hr = MFCreateTopology(output_topology)))
        return hr;

    /* resolve each branch */
    for (i = 0; i < num_connections; i++)
    {
        struct topology_node *src = node_pairs[i][0];
        struct topology_node *sink = node_pairs[i][1];
        struct topology *full_topology = unsafe_impl_from_IMFTopology(*output_topology);

        if (FAILED(hr = topology_loader_resolve_partial_topology(src, sink, topology, &full_topology)))
        {
            ERR("Failed to resolve connection between %p and %p. %x\n", src, sink, hr);
            heap_free(node_pairs);
            return hr;
        }
    }

    heap_free(node_pairs);
    return S_OK;
}

static const IMFTopoLoaderVtbl topologyloadervtbl =
{
    topology_loader_QueryInterface,
    topology_loader_AddRef,
    topology_loader_Release,
    topology_loader_Load,
};

/***********************************************************************
 *      MFCreateTopoLoader (mf.@)
 */
HRESULT WINAPI MFCreateTopoLoader(IMFTopoLoader **loader)
{
    struct topology_loader *object;

    TRACE("%p.\n", loader);

    if (!loader)
        return E_POINTER;

    object = heap_alloc(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFTopoLoader_iface.lpVtbl = &topologyloadervtbl;
    object->refcount = 1;

    *loader = &object->IMFTopoLoader_iface;

    return S_OK;
}

static HRESULT WINAPI seq_source_QueryInterface(IMFSequencerSource *iface, REFIID riid, void **out)
{
    struct seq_source *seq_source = impl_from_IMFSequencerSource(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), out);

    *out = NULL;

    if (IsEqualIID(riid, &IID_IMFSequencerSource) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &seq_source->IMFSequencerSource_iface;
    }
    else if (IsEqualIID(riid, &IID_IMFMediaSourceTopologyProvider))
    {
        *out = &seq_source->IMFMediaSourceTopologyProvider_iface;
    }
    else
    {
        WARN("Unimplemented %s.\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    if (*out)
        IUnknown_AddRef((IUnknown *)*out);

    return S_OK;
}

static ULONG WINAPI seq_source_AddRef(IMFSequencerSource *iface)
{
    struct seq_source *seq_source = impl_from_IMFSequencerSource(iface);
    ULONG refcount = InterlockedIncrement(&seq_source->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI seq_source_Release(IMFSequencerSource *iface)
{
    struct seq_source *seq_source = impl_from_IMFSequencerSource(iface);
    ULONG refcount = InterlockedDecrement(&seq_source->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        heap_free(seq_source);
    }

    return refcount;
}

static HRESULT WINAPI seq_source_AppendTopology(IMFSequencerSource *iface, IMFTopology *topology,
        DWORD flags, MFSequencerElementId *id)
{
    FIXME("%p, %p, %x, %p.\n", iface, topology, flags, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI seq_source_DeleteTopology(IMFSequencerSource *iface, MFSequencerElementId id)
{
    FIXME("%p, %#x.\n", iface, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI seq_source_GetPresentationContext(IMFSequencerSource *iface,
        IMFPresentationDescriptor *descriptor, MFSequencerElementId *id, IMFTopology **topology)
{
    FIXME("%p, %p, %p, %p.\n", iface, descriptor, id, topology);

    return E_NOTIMPL;
}

static HRESULT WINAPI seq_source_UpdateTopology(IMFSequencerSource *iface, MFSequencerElementId id,
        IMFTopology *topology)
{
    FIXME("%p, %#x, %p.\n", iface, id, topology);

    return E_NOTIMPL;
}

static HRESULT WINAPI seq_source_UpdateTopologyFlags(IMFSequencerSource *iface, MFSequencerElementId id, DWORD flags)
{
    FIXME("%p, %#x, %#x.\n", iface, id, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI seq_source_topology_provider_QueryInterface(IMFMediaSourceTopologyProvider *iface, REFIID riid,
        void **obj)
{
    struct seq_source *seq_source = impl_from_IMFMediaSourceTopologyProvider(iface);
    return IMFSequencerSource_QueryInterface(&seq_source->IMFSequencerSource_iface, riid, obj);
}

static ULONG WINAPI seq_source_topology_provider_AddRef(IMFMediaSourceTopologyProvider *iface)
{
    struct seq_source *seq_source = impl_from_IMFMediaSourceTopologyProvider(iface);
    return IMFSequencerSource_AddRef(&seq_source->IMFSequencerSource_iface);
}

static ULONG WINAPI seq_source_topology_provider_Release(IMFMediaSourceTopologyProvider *iface)
{
    struct seq_source *seq_source = impl_from_IMFMediaSourceTopologyProvider(iface);
    return IMFSequencerSource_Release(&seq_source->IMFSequencerSource_iface);
}

static HRESULT WINAPI seq_source_topology_provider_GetMediaSourceTopology(IMFMediaSourceTopologyProvider *iface,
        IMFPresentationDescriptor *pd, IMFTopology **topology)
{
    FIXME("%p, %p, %p.\n", iface, pd, topology);

    return E_NOTIMPL;
}

static const IMFMediaSourceTopologyProviderVtbl seq_source_topology_provider_vtbl =
{
    seq_source_topology_provider_QueryInterface,
    seq_source_topology_provider_AddRef,
    seq_source_topology_provider_Release,
    seq_source_topology_provider_GetMediaSourceTopology,
};

static const IMFSequencerSourceVtbl seqsourcevtbl =
{
    seq_source_QueryInterface,
    seq_source_AddRef,
    seq_source_Release,
    seq_source_AppendTopology,
    seq_source_DeleteTopology,
    seq_source_GetPresentationContext,
    seq_source_UpdateTopology,
    seq_source_UpdateTopologyFlags,
};

/***********************************************************************
 *      MFCreateSequencerSource (mf.@)
 */
HRESULT WINAPI MFCreateSequencerSource(IUnknown *reserved, IMFSequencerSource **seq_source)
{
    struct seq_source *object;

    TRACE("%p, %p.\n", reserved, seq_source);

    if (!seq_source)
        return E_POINTER;

    object = heap_alloc(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFSequencerSource_iface.lpVtbl = &seqsourcevtbl;
    object->IMFMediaSourceTopologyProvider_iface.lpVtbl = &seq_source_topology_provider_vtbl;
    object->refcount = 1;

    *seq_source = &object->IMFSequencerSource_iface;

    return S_OK;
}

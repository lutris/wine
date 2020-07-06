/*
 * Copyright (C) 2015 Michael Müller
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
#include "wingdi.h"
#include "winuser.h"
#include "shlwapi.h"
#include "winerror.h"
#include "nvapi.h"
#include "d3d9.h"
#include "d3d11.h"

#include "wine/test.h"

static void* (CDECL *pnvapi_QueryInterface)(unsigned int offset);
static NvAPI_Status (CDECL *pNvAPI_Initialize)(void);
static NvAPI_Status (CDECL *pNvAPI_GetDisplayDriverVersion)(NvDisplayHandle hNvDisplay, NV_DISPLAY_DRIVER_VERSION *pVersion);
static NvAPI_Status (CDECL *pNvAPI_unknown1)(void* param0);
static NvAPI_Status (CDECL *pNvAPI_unknown2)(NvPhysicalGpuHandle gpuHandle, void *param1);
static NvAPI_Status (CDECL *pNvAPI_unknown3)(void *param0, void *param1);
static NvAPI_Status (CDECL *pNvAPI_EnumLogicalGPUs_unknown)(NvLogicalGpuHandle nvGPUHandle[NVAPI_MAX_LOGICAL_GPUS], NvU32 *pGpuCount);
static NvAPI_Status (CDECL *pNvAPI_EnumLogicalGPUs)(NvLogicalGpuHandle nvGPUHandle[NVAPI_MAX_LOGICAL_GPUS], NvU32 *pGpuCount);
static NvAPI_Status (CDECL *pNvAPI_GetPhysicalGPUsFromLogicalGPU)(NvLogicalGpuHandle hLogicalGPU, NvPhysicalGpuHandle hPhysicalGPU[NVAPI_MAX_PHYSICAL_GPUS], NvU32 *pGpuCount);
static NvAPI_Status (CDECL *pNvAPI_EnumPhysicalGPUs)(NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32 *pGpuCount);
static NvAPI_Status (CDECL* pNvAPI_GPU_GetFullName)(NvPhysicalGpuHandle hPhysicalGpu, NvAPI_ShortString szName);
static NvAPI_Status (CDECL* pNvAPI_DISP_GetGDIPrimaryDisplayId)(NvU32* displayId);
static NvAPI_Status (CDECL* pNvAPI_EnumNvidiaDisplayHandle)(NvU32 thisEnum, NvDisplayHandle *pNvDispHandle);
static NvAPI_Status (CDECL* pNvAPI_SYS_GetDriverAndBranchVersion)(NvU32* pDriverVersion, NvAPI_ShortString szBuildBranchString);
static NvAPI_Status (CDECL* pNvAPI_D3D_GetCurrentSLIState)(IUnknown *pDevice, NV_GET_CURRENT_SLI_STATE *pSliState);
static NvAPI_Status (CDECL* pNvAPI_GetLogicalGPUFromDisplay)(NvDisplayHandle hNvDisp, NvLogicalGpuHandle *pLogicalGPU);
static NvAPI_Status (CDECL* pNvAPI_D3D11_SetDepthBoundsTest)(IUnknown*, NvU32, float, float);
static NvAPI_Status (CDECL* pNvAPI_D3D11_CreateDevice)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL*, UINT, UINT,
                                                       ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**, NVAPI_DEVICE_FEATURE_LEVEL*);

static const struct
{
    unsigned int offset;
    void **function;
}
function_list[] =
{
    {0x0150E828, (void**) &pNvAPI_Initialize},
    {0xF951A4D1, (void**) &pNvAPI_GetDisplayDriverVersion},
    {0x5786cc6e, (void**) &pNvAPI_unknown1},
    {0x6533ea3e, (void**) &pNvAPI_unknown2},
    {0x5380ad1a, (void**) &pNvAPI_unknown3},
    {0xfb9bc2ab, (void**) &pNvAPI_EnumLogicalGPUs_unknown},
    {0x48b3ea59, (void**) &pNvAPI_EnumLogicalGPUs},
    {0xaea3fa32, (void**) &pNvAPI_GetPhysicalGPUsFromLogicalGPU},
    {0xe5ac921f, (void**) &pNvAPI_EnumPhysicalGPUs},
    {0xceee8e9f, (void**) &pNvAPI_GPU_GetFullName},
    {0x1e9d8a31, (void**) &pNvAPI_DISP_GetGDIPrimaryDisplayId},
    {0x9abdd40d, (void**) &pNvAPI_EnumNvidiaDisplayHandle},
    {0x2926aaad, (void**) &pNvAPI_SYS_GetDriverAndBranchVersion},
    {0x4b708b54, (void**) &pNvAPI_D3D_GetCurrentSLIState},
    {0xee1370cf, (void**) &pNvAPI_GetLogicalGPUFromDisplay},
    {0x7aaf7a04, (void**) &pNvAPI_D3D11_SetDepthBoundsTest},
    {0x6a16d3a0, (void**) &pNvAPI_D3D11_CreateDevice},
};

static BOOL init(void)
{
    int i;
    #ifdef __x86_64__
    HMODULE nvapi = LoadLibraryA("nvapi64.dll");
    #else
    HMODULE nvapi = LoadLibraryA("nvapi.dll");
    #endif

    if (!nvapi)
    {
        skip("Could not load nvapi.dll\n");
        return FALSE;
    }

    pnvapi_QueryInterface = (void *)GetProcAddress(nvapi, "nvapi_QueryInterface");
    if (!pnvapi_QueryInterface)
    {
        win_skip("Failed to get entry point for nvapi_QueryInterface.\n");
        return FALSE;
    }

    for (i = 0; i < sizeof(function_list) / sizeof(function_list[0]); i++)
        *function_list[i].function = pnvapi_QueryInterface(function_list[i].offset);

    if (!pNvAPI_Initialize)
    {
        win_skip("Failed to get entry point for NvAPI_Initialize.\n");
        return FALSE;
    }

    if (pNvAPI_Initialize())
    {
        skip("Failed to initialize nvapi.\n");
        return FALSE;
    }

    return TRUE;
}

static void test_GetDisplayDriverVersion(void)
{
    NV_DISPLAY_DRIVER_VERSION version;
    NvAPI_Status status;
    DISPLAY_DEVICEA dinfo;
    char *gpuName;

    if (!pNvAPI_GetDisplayDriverVersion)
    {
        win_skip("NvAPI_GetDisplayDriverVersion export not found.\n");
        return;
    }

    status = pNvAPI_GetDisplayDriverVersion(0, NULL);
    ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);

    memset(&version, 0, sizeof(version));
    version.version = NV_DISPLAY_DRIVER_VERSION_VER;

    status = pNvAPI_GetDisplayDriverVersion((void *)0xdeadbeef, &version);
    ok(status == NVAPI_INVALID_HANDLE, "Expected status NVAPI_INVALID_HANDLE, got %d\n", status);

    memset(&dinfo, 0, sizeof(dinfo));
    dinfo.cb = sizeof(dinfo);
    ok(EnumDisplayDevicesA(NULL, 0, &dinfo, 0), "Failed to get monitor info\n");

    trace("Device name: %s\n", (char*)&dinfo.DeviceName);
    trace("Device string: %s\n", (char*)&dinfo.DeviceString);

    gpuName = (char*)&dinfo.DeviceString;
    StrTrimA(gpuName, " \t\r\n");

    if (!strncmp(gpuName, "NVIDIA ", 7))
        gpuName += 7;

    memset(&version, 0, sizeof(version));
    version.version = NV_DISPLAY_DRIVER_VERSION_VER;

    status = pNvAPI_GetDisplayDriverVersion(0, &version);
    ok(!status, "Expected status NVAPI_OK, got %d\n", status);

    trace("Driver version: %u\n", (unsigned int)version.drvVersion);
    trace("Change list num: %u\n", (unsigned int)version.bldChangeListNum);
    trace("Build string: %s\n", version.szBuildBranchString);
    trace("Adapter string: %s\n", version.szAdapterString);

    todo_wine ok(!strcmp(version.szAdapterString, gpuName), "Expected device name %s, got %s\n",
                 gpuName, version.szAdapterString);
}

static void test_unknown1(void)
{
    NV_UNKNOWN_1 test;
    NvAPI_Status status;
    int i;

    if (!pNvAPI_unknown1)
    {
        win_skip("pNvAPI_unknown1 export not found.\n");
        return;
    }

    status = pNvAPI_unknown1(NULL);
    ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);

    memset(&test, 0, sizeof(test));
    test.version = NV_UNKNOWN_1_VER;
    status = pNvAPI_unknown1(&test);

    ok(!status, "Expected status NVAPI_OK, got %d\n", status);
    ok(test.gpu_count > 0, "Expected gpu_count > 0, got %d\n", test.gpu_count);

    for (i = 0; i < test.gpu_count; i++)
    {
        ok(test.gpus[i].gpuHandle != NULL, "Expected gpus[%d].gpuHandle != 0, got %p\n", i, test.gpus[i].gpuHandle);
        ok(test.gpus[i].unknown2 != 0, "Expected gpus[%d].unknown2 != 0, got %d\n", i, test.gpus[i].unknown2);
    }

    for (; i < sizeof(test.gpus) / sizeof(test.gpus[0]); i++)
    {
        ok(test.gpus[i].gpuHandle == NULL, "Expected gpus[%d].gpuHandle == NULL, got %p\n", i, test.gpus[i].gpuHandle);
        ok(test.gpus[i].unknown2 == 0, "Expected gpus[%d].unknown2 == 0, got %d\n", i, test.gpus[i].unknown2);
    }
}

static void test_unknown2(void)
{
    NV_UNKNOWN_1 test;
    NvAPI_Status status;
    NvPhysicalGpuHandle test2 = NULL;

    if (!pNvAPI_unknown1)
    {
        win_skip("pNvAPI_unknown1 export not found.\n");
        return;
    }

    if (!pNvAPI_unknown2)
    {
        win_skip("pNvAPI_unknown1 export not found.\n");
        return;
    }

    status = pNvAPI_unknown2(NULL, NULL);
    ok(status == NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE, "Expected status NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE, got %d\n", status);

    memset(&test, 0, sizeof(test));
    test.version = NV_UNKNOWN_1_VER;
    status = pNvAPI_unknown1(&test);
    ok(!status, "Expected status NVAPI_OK, got %d\n", status);

    status = pNvAPI_unknown2(test.gpus[0].gpuHandle, NULL);
    ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);

    status = pNvAPI_unknown2(NULL, &test2);
    ok(status == NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE, "Expected status NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE, got %d\n", status);

    test2 = NULL;
    status = pNvAPI_unknown2((void *)0xdeadbeef, &test2);
    ok(!status, "Expected status NVAPI_OK, got %d\n", status);
    ok(test2 == (void*)0xffffffff, "Expected handle 0xffffffff, got %p\n", test2);

    test2 = NULL;
    status = pNvAPI_unknown2(test.gpus[0].gpuHandle, &test2);
    ok(!status, "Expected status NVAPI_OK, got %d\n", status);
    ok(test2 == test.gpus[0].gpuHandle, "Expected handle %p, got %p\n", test.gpus[0].gpuHandle, test2);
}

static void test_unknown3(void)
{
    NV_UNKNOWN_1 test;
    NvAPI_Status status;
    NvPhysicalGpuHandle test2 = NULL;
    NvPhysicalGpuHandle test3 = NULL;

    if (!pNvAPI_unknown1)
    {
        win_skip("pNvAPI_unknown1 export not found.\n");
        return;
    }

    if (!pNvAPI_unknown2)
    {
        win_skip("pNvAPI_unknown1 export not found.\n");
        return;
    }

    if (!pNvAPI_unknown3)
    {
        win_skip("pNvAPI_unknown1 export not found.\n");
        return;
    }

    status = pNvAPI_unknown3(NULL, NULL);
    ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);

    memset(&test, 0, sizeof(test));
    test.version = NV_UNKNOWN_1_VER;
    status = pNvAPI_unknown1(&test);
    ok(!status, "Expected status NVAPI_OK, got %d\n", status);

    status = pNvAPI_unknown2(test.gpus[0].gpuHandle, &test2);
    ok(!status, "Expected status NVAPI_OK, got %d\n", status);

    status = pNvAPI_unknown3(test2, NULL);
    ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);

    status = pNvAPI_unknown3(NULL, &test3);
    ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);

    test3 = NULL;
    status = pNvAPI_unknown3((void *)0xdeadbeef, &test3);
    ok(!status, "Expected status NVAPI_OK, got %d\n", status);
    ok(test3 == (void*)0xffffffff, "Expected handle 0xffffffff, got %p\n", test3);

    test3 = NULL;
    status = pNvAPI_unknown3(test2, &test3);
    ok(!status, "Expected status NVAPI_OK, got %d\n", status);
    ok(test2 == test3, "Expected handle %p, got %p\n", test2, test3);
}

static void test_NvAPI_EnumLogicalGPUs(void)
{
    NvLogicalGpuHandle gpuHandle1[NVAPI_MAX_LOGICAL_GPUS];
    NvLogicalGpuHandle gpuHandle2[NVAPI_MAX_LOGICAL_GPUS];
    NvAPI_Status status;
    NvU32 count1, count2;
    int i;

    if (!pNvAPI_EnumLogicalGPUs_unknown)
    {
        win_skip("NvAPI_EnumLogicalGPUs_unknown export not found.\n");
        return;
    }

    if (!pNvAPI_EnumLogicalGPUs)
    {
        win_skip("NvAPI_EnumLogicalGPUs export not found.\n");
        return;
    }

    status = pNvAPI_EnumLogicalGPUs_unknown(NULL, NULL);
    ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);

    status = pNvAPI_EnumLogicalGPUs_unknown((void*)0xdeadbeef, NULL);
    ok(status == NVAPI_INVALID_POINTER, "Expected status NVAPI_INVALID_POINTER, got %d\n", status);

    status = pNvAPI_EnumLogicalGPUs_unknown(NULL, (void*)0xdeadbeef);
    ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);

    status = pNvAPI_EnumLogicalGPUs(NULL, NULL);
    ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);

    status = pNvAPI_EnumLogicalGPUs((void*)0xdeadbeef, NULL);
    ok(status == NVAPI_INVALID_POINTER, "Expected status NVAPI_INVALID_POINTER, got %d\n", status);

    status = pNvAPI_EnumLogicalGPUs(NULL, (void*)0xdeadbeef);
    ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);

    memset(gpuHandle1, 0, sizeof(gpuHandle1));

    status = pNvAPI_EnumLogicalGPUs_unknown(gpuHandle1, &count1);
    ok(status == NVAPI_OK, "Expected status NVAPI_OK, got %d\n", status);
    ok(count1 > 0, "Expected count1 > 0, got %d\n", count1);
    for (i = 0; i < count1; i++)
        ok(gpuHandle1[i] != NULL, "Expected gpuHandle1[%d] not be NULL, got %p\n", i, gpuHandle1[i]);

    status = pNvAPI_EnumLogicalGPUs(gpuHandle2, &count2);
    ok(status == NVAPI_OK, "Expected status NVAPI_OK, got %d\n", status);
    ok(count2 > 0, "Expected count2 > 0, got %d\n", count2);
    ok(count1 == count2, "Expected count1 == count2, got %d != %d\n", count1, count2);
    for (i = 0; i < count2; i++)
        ok(gpuHandle1[i] == gpuHandle2[i], "Expected gpuHandle1[i] == gpuHandle2[i], got %p != %p\n", gpuHandle1[i], gpuHandle2[i]);
}

static void test_NvAPI_GetPhysicalGPUsFromLogicalGPU(void)
{
    NvLogicalGpuHandle gpus_logical[NVAPI_MAX_LOGICAL_GPUS];
    NvPhysicalGpuHandle gpus_physical[NVAPI_MAX_PHYSICAL_GPUS];
    NvAPI_Status status;
    NvU32 count1, count2;
    int i;

    if (!pNvAPI_EnumLogicalGPUs)
    {
        win_skip("NvAPI_EnumLogicalGPUs export not found.\n");
        return;
    }

    if (!pNvAPI_GetPhysicalGPUsFromLogicalGPU)
    {
        win_skip("NvAPI_GetPhysicalGPUsFromLogicalGPU export not found.\n");
        return;
    }

    status = pNvAPI_EnumLogicalGPUs_unknown(gpus_logical, &count1);
    ok(status == NVAPI_OK, "Expected status NVAPI_OK, got %d\n", status);
    ok(count1 > 0, "Expected count1 > 0, got %d\n", count1);

    status = pNvAPI_GetPhysicalGPUsFromLogicalGPU(NULL, NULL, NULL);
    ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);

    status = pNvAPI_GetPhysicalGPUsFromLogicalGPU(gpus_logical[0], NULL, NULL);
    ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);

    status = pNvAPI_GetPhysicalGPUsFromLogicalGPU(gpus_logical[0], gpus_physical, NULL);
    ok(status == NVAPI_INVALID_POINTER, "Expected status NVAPI_INVALID_POINTER, got %d\n", status);

    status = pNvAPI_GetPhysicalGPUsFromLogicalGPU(gpus_logical[0], NULL, &count2);
    ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);

    status = pNvAPI_GetPhysicalGPUsFromLogicalGPU(NULL, gpus_physical, &count2);
    ok(status == NVAPI_EXPECTED_LOGICAL_GPU_HANDLE, "Expected status NVAPI_EXPECTED_LOGICAL_GPU_HANDLE, got %d\n", status);

    status = pNvAPI_GetPhysicalGPUsFromLogicalGPU((void*) 0xdeadbeef, gpus_physical, &count2);
    ok(status == NVAPI_EXPECTED_LOGICAL_GPU_HANDLE, "Expected status NVAPI_EXPECTED_LOGICAL_GPU_HANDLE, got %d\n", status);

    status = pNvAPI_GetPhysicalGPUsFromLogicalGPU(gpus_logical[0], gpus_physical, &count2);
    ok(status == NVAPI_OK, "Expected status NVAPI_OK, got %d\n", status);
    ok(count2 > 0, "Expected count1 > 0, got %d\n", count2);
    for (i = 0; i < count2; i++)
        ok(gpus_physical[i] != NULL, "Expected gpus_physical[%d] not be NULL, got %p\n", i, gpus_physical[i]);
}


static void test_NvAPI_EnumPhysicalGPUs(void)
{
    NvLogicalGpuHandle gpuHandle[NVAPI_MAX_PHYSICAL_GPUS];
    NvAPI_Status status;
    NvU32 count;
    int i;

    if (!pNvAPI_EnumPhysicalGPUs)
    {
        win_skip("NvAPI_EnumLogicalGPUs export not found.\n");
        return;
    }

    status = pNvAPI_EnumPhysicalGPUs(NULL, NULL);
    ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);

    status = pNvAPI_EnumPhysicalGPUs((void*)0xdeadbeef, NULL);
    ok(status == NVAPI_INVALID_POINTER, "Expected status NVAPI_INVALID_POINTER, got %d\n", status);

    status = pNvAPI_EnumPhysicalGPUs(NULL, (void*)0xdeadbeef);
    ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);

    memset(gpuHandle, 0, sizeof(gpuHandle));

    status = pNvAPI_EnumPhysicalGPUs(gpuHandle, &count);
    ok(status == NVAPI_OK, "Expected status NVAPI_OK, got %d\n", status);
    ok(count > 0, "Expected count > 0, got %d\n", count);
    for (i = 0; i < count; i++)
        ok(gpuHandle[i] != NULL, "Expected gpuHandle[%d] != NULL\n", i);
}

static void test_NvAPI_GPU_GetFullName(void)
{
    NvLogicalGpuHandle gpuHandle[NVAPI_MAX_PHYSICAL_GPUS];
    NvAPI_Status status;
    NvU32 count;
    NvAPI_ShortString str;

    if (!pNvAPI_EnumPhysicalGPUs)
    {
        win_skip("NvAPI_EnumLogicalGPUs export not found.\n");
        return;
    }

    if (!pNvAPI_GPU_GetFullName)
    {
        win_skip("NvAPI_GPU_GetFullName export not found.\n");
        return;
    }

    memset(gpuHandle, 0, sizeof(gpuHandle));

    status = pNvAPI_EnumPhysicalGPUs(gpuHandle, &count);
    ok(status == NVAPI_OK, "Expected status NVAPI_OK, got %d\n", status);
    ok(count > 0, "Expected count > 0, got %d\n", count);

    status = pNvAPI_GPU_GetFullName(NULL, NULL);
    ok(status == NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE, "Expected status NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE, got %d\n", status);

    if (0) /* crashes on windows */
    {
        status = pNvAPI_GPU_GetFullName(gpuHandle[0], NULL);
        ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);
    }

    status = pNvAPI_GPU_GetFullName(NULL, str);
    ok(status == NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE, "Expected status NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE, got %d\n", status);

    status = pNvAPI_GPU_GetFullName((void*)0xdeadbeef, str);
    ok(status == NVAPI_INVALID_HANDLE, "Expected status NVAPI_INVALID_HANDLE, got %d\n", status);

    memset(str, 0, sizeof(str));
    status = pNvAPI_GPU_GetFullName(gpuHandle[0], str);
    ok(status == NVAPI_OK, "Expected status NVAPI_OK, got %d\n", status);
    ok(str[0] != 0, "Expected non emptry string\n");
    trace("GPU-0 name: %s\n", str);
}

static void test_NvAPI_DISP_GetGDIPrimaryDisplayId(void)
{
    NvAPI_Status status;
    NvU32 disp;

    if (!pNvAPI_DISP_GetGDIPrimaryDisplayId)
    {
        win_skip("NvAPI_DISP_GetGDIPrimaryDisplayId export not found.\n");
        return;
    }

    status = pNvAPI_DISP_GetGDIPrimaryDisplayId(NULL);
    ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);

    disp = 0;
    status = pNvAPI_DISP_GetGDIPrimaryDisplayId(&disp);
    ok(status == NVAPI_OK, "Expected status NVAPI_OK, got %d\n", status);
    ok(disp != 0, "Expected disp to be non null\n");
}

static void test_NvAPI_EnumNvidiaDisplayHandle(void)
{
    NvAPI_Status status;
    NvDisplayHandle disp;
    int i = 0;

    if (!pNvAPI_EnumNvidiaDisplayHandle)
    {
        win_skip("NvAPI_EnumNvidiaDisplayHandle export not found.\n");
        return;
    }

    status = pNvAPI_EnumNvidiaDisplayHandle(0, NULL);
    ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);

    disp = NULL;
    status = pNvAPI_EnumNvidiaDisplayHandle(i, &disp);
    ok(status == NVAPI_OK, "Expected status NVAPI_OK, got %d\n", status);
    ok(disp != NULL, "Expected disp to be non null\n");

    while (!pNvAPI_EnumNvidiaDisplayHandle(i, &disp))
        i++;

    disp = NULL;
    status = pNvAPI_EnumNvidiaDisplayHandle(i, &disp);
    ok(status == NVAPI_END_ENUMERATION, "Expected status NVAPI_END_ENUMERATION, got %d\n", status);
    ok(disp == NULL, "Expected disp to be null\n");

    disp = NULL;
    status = pNvAPI_EnumNvidiaDisplayHandle(NVAPI_MAX_DISPLAYS - 1, &disp);
    ok(status == NVAPI_END_ENUMERATION, "Expected status NVAPI_END_ENUMERATION, got %d\n", status);
    ok(disp == NULL, "Expected disp to be null\n");

    disp = NULL;
    status = pNvAPI_EnumNvidiaDisplayHandle(NVAPI_MAX_DISPLAYS, &disp);
    ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);
    ok(disp == NULL, "Expected disp to be null\n");
}

static void test_NvAPI_SYS_GetDriverAndBranchVersion(void)
{
    NvAPI_Status status;
    NvU32 version;
    NvAPI_ShortString branch;

    if (!pNvAPI_SYS_GetDriverAndBranchVersion)
    {
        win_skip("NvAPI_SYS_GetDriverAndBranchVersion export not found.\n");
        return;
    }

    status = pNvAPI_SYS_GetDriverAndBranchVersion(NULL, NULL);
    ok(status == NVAPI_INVALID_POINTER, "Expected status NVAPI_INVALID_POINTER, got %d\n", status);

    status = pNvAPI_SYS_GetDriverAndBranchVersion(NULL, branch);
    ok(status == NVAPI_INVALID_POINTER, "Expected status NVAPI_INVALID_POINTER, got %d\n", status);

    if (0) /* crashes on windows */
    {
        status = pNvAPI_SYS_GetDriverAndBranchVersion(&version, NULL);
        ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);
    }

    version = 0;
    memset(branch, 0, sizeof(branch));
    status = pNvAPI_SYS_GetDriverAndBranchVersion(&version, branch);
    ok(status == NVAPI_OK, "Expected status NVAPI_OK, got %d\n", status);
    ok(version != 0, "Expected non null version\n");
    ok(branch[0] != 0, "Expected non emptry branch string\n");

    trace("Version: %d\n", version);
    trace("Branch: %s\n", branch);
}

static void test_NvAPI_GetLogicalGPUFromDisplay(void)
{
    NvAPI_Status status;
    NvDisplayHandle disp;
    NvLogicalGpuHandle gpuHandle;

    if (!pNvAPI_GetLogicalGPUFromDisplay)
    {
        win_skip("NvAPI_SYS_GetDriverAndBranchVersion export not found.\n");
        return;
    }

    if (!pNvAPI_EnumNvidiaDisplayHandle)
    {
        win_skip("NvAPI_EnumNvidiaDisplayHandle export not found.\n");
        return;
    }

    disp = NULL;
    status = pNvAPI_EnumNvidiaDisplayHandle(0, &disp);
    ok(status == NVAPI_OK, "Expected status NVAPI_OK, got %d\n", status);
    ok(disp != NULL, "Expected disp to be non null\n");

    status = pNvAPI_GetLogicalGPUFromDisplay(NULL, NULL);
    ok(status == NVAPI_INVALID_POINTER, "Expected status NVAPI_INVALID_POINTER, got %d\n", status);

    status = pNvAPI_GetLogicalGPUFromDisplay(disp, NULL);
    ok(status == NVAPI_INVALID_POINTER, "Expected status NVAPI_INVALID_POINTER, got %d\n", status);

    status = pNvAPI_GetLogicalGPUFromDisplay((void*)0xdeadbeef, &gpuHandle);
    ok(status == NVAPI_NVIDIA_DEVICE_NOT_FOUND, "Expected status NVAPI_NVIDIA_DEVICE_NOT_FOUND, got %d\n", status);

    gpuHandle = NULL;
    status = pNvAPI_GetLogicalGPUFromDisplay(NULL, &gpuHandle);
    ok(status == NVAPI_OK, "Expected status NVAPI_OK, got %d\n", status);
    ok(gpuHandle != NULL, "Expected gpuHandle to be non null\n");

    gpuHandle = NULL;
    status = pNvAPI_GetLogicalGPUFromDisplay(disp, &gpuHandle);
    ok(status == NVAPI_OK, "Expected status NVAPI_OK, got %d\n", status);
    ok(gpuHandle != NULL, "Expected gpuHandle to be non null\n");
}

static IDirect3DDevice9 *create_device(IDirect3D9 *d3d9, HWND focus_window)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};
    IDirect3DDevice9 *device;
    DWORD behavior_flags = D3DCREATE_HARDWARE_VERTEXPROCESSING;

    if (!d3d9 || !focus_window)
        return NULL;

    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.hDeviceWindow = focus_window;
    present_parameters.Windowed = TRUE;
    present_parameters.EnableAutoDepthStencil = FALSE;

    if (SUCCEEDED(IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            behavior_flags, &present_parameters, &device)))
        return device;

    return NULL;
}

static void test_NvAPI_D3D_GetCurrentSLIState(void)
{
    HWND window;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    NV_GET_CURRENT_SLI_STATE sli_state;
    NvAPI_Status status;

    if (!pNvAPI_D3D_GetCurrentSLIState)
    {
        win_skip("NvAPI_D3D_GetCurrentSLIState export not found.\n");
        return;
    }

    status = pNvAPI_D3D_GetCurrentSLIState(NULL, NULL);
    ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);

    status = pNvAPI_D3D_GetCurrentSLIState(NULL, &sli_state);
    ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);

    memset(&sli_state, 0, sizeof(sli_state));
    sli_state.version = NV_GET_CURRENT_SLI_STATE_VER;
    status = pNvAPI_D3D_GetCurrentSLIState(NULL, &sli_state);
    ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);

    window = CreateWindowA("d3d9_nvapi_test_wc", "d3d9_nvapi_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    ok(window != NULL, "Failed to create a window.\n");

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(d3d != NULL, "Failed to create a D3D object.\n");

    if (!(device = create_device(d3d, window)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        goto cleanup;
    }

    status = pNvAPI_D3D_GetCurrentSLIState((IUnknown*)device, NULL);
    ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);

    memset(&sli_state, 0, sizeof(sli_state));
    status = pNvAPI_D3D_GetCurrentSLIState((IUnknown*)device, &sli_state);
    ok(status == NVAPI_INCOMPATIBLE_STRUCT_VERSION, "Expected status NVAPI_INCOMPATIBLE_STRUCT_VERSION, got %d\n", status);

    memset(&sli_state, 0, sizeof(sli_state));
    sli_state.version = NV_GET_CURRENT_SLI_STATE_VER;
    status = pNvAPI_D3D_GetCurrentSLIState((IUnknown*)device, &sli_state);
    ok(status == NVAPI_OK || status == NVAPI_NO_ACTIVE_SLI_TOPOLOGY,
       "Expected status NVAPI_OK or NVAPI_NO_ACTIVE_SLI_TOPOLOGY, got %d\n", status);

    if (status == NVAPI_OK)
    {
        trace("maxNumAFRGroups: %u\n", sli_state.maxNumAFRGroups);
        trace("numAFRGroups: %u\n", sli_state.numAFRGroups);
        trace("currentAFRIndex: %u\n", sli_state.currentAFRIndex);
        trace("nextFrameAFRIndex: %u\n", sli_state.nextFrameAFRIndex);
        trace("previousFrameAFRIndex: %u\n", sli_state.previousFrameAFRIndex);
        trace("bIsCurAFRGroupNew: %u\n", sli_state.bIsCurAFRGroupNew);
    }

cleanup:
    if (device)
    {
        UINT refcount = IDirect3DDevice9_Release(device);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    if (d3d) IDirect3D9_Release(d3d);
    if (window) DestroyWindow(window);
}

static void test_NvAPI_D3D11_SetDepthBoundsTest(void)
{
    NvAPI_Status status;

    if (!pNvAPI_D3D11_SetDepthBoundsTest)
    {
        win_skip("NvAPI_D3D11_SetDepthBoundsTest export not found.\n");
        return;
    }

    status = pNvAPI_D3D11_SetDepthBoundsTest(NULL, 0, 0.0, 0.0);
    ok(status == NVAPI_INVALID_ARGUMENT, "Expected status NVAPI_INVALID_ARGUMENT, got %d\n", status);
}

static void test_NvAPI_D3D11_CreateDevice(void)
{
    NVAPI_DEVICE_FEATURE_LEVEL level_nvapi;
    D3D_FEATURE_LEVEL level_d3d;
    ID3D11Device *device;
    NvAPI_Status status;

    if (!pNvAPI_D3D11_CreateDevice)
    {
        win_skip("NvAPI_D3D11_CreateDevice export not found.\n");
        return;
    }

    status = pNvAPI_D3D11_CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0,
                                       D3D11_SDK_VERSION, &device, &level_d3d, NULL, &level_nvapi);
    ok(status == NVAPI_OK, "Expected status NVAPI_OK, got %d\n", status);

    switch (level_d3d)
    {
        case D3D_FEATURE_LEVEL_9_1:
        case D3D_FEATURE_LEVEL_9_2:
        case D3D_FEATURE_LEVEL_9_3:
            ok(level_nvapi == NVAPI_DEVICE_FEATURE_LEVEL_NULL, "Expected level NVAPI_DEVICE_FEATURE_LEVEL_NULL, got %d\n", level_nvapi);
            break;
        case D3D_FEATURE_LEVEL_10_0:
            ok(level_nvapi == NVAPI_DEVICE_FEATURE_LEVEL_10_0 || level_nvapi == NVAPI_DEVICE_FEATURE_LEVEL_10_0_PLUS,
               "Expected level NVAPI_DEVICE_FEATURE_LEVEL_10_0(_PLUS), got %d\n", level_nvapi);
            break;
        case D3D_FEATURE_LEVEL_10_1:
            ok(level_nvapi == NVAPI_DEVICE_FEATURE_LEVEL_10_1, "Expected level NVAPI_DEVICE_FEATURE_LEVEL_10_1, got %d\n", level_nvapi);
            break;
        case D3D_FEATURE_LEVEL_11_0:
        default:
            ok(level_nvapi == NVAPI_DEVICE_FEATURE_LEVEL_11_0, "Expected level NVAPI_DEVICE_FEATURE_LEVEL_11_0, got %d\n", level_nvapi);
            break;
    }

    if (status == NVAPI_OK)
        ID3D11Device_Release(device);

    level_d3d = D3D_FEATURE_LEVEL_9_1;
    status = pNvAPI_D3D11_CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, &level_d3d, 1,
                                       D3D11_SDK_VERSION, &device, NULL, NULL, &level_nvapi);
    ok(status == NVAPI_OK, "Expected status NVAPI_OK, got %d\n", status);
    ok(level_nvapi == NVAPI_DEVICE_FEATURE_LEVEL_NULL, "Expected level NVAPI_DEVICE_FEATURE_LEVEL_NULL, got %d\n", level_nvapi);

    if (status == NVAPI_OK)
        ID3D11Device_Release(device);
}

START_TEST( nvapi )
{
    WNDCLASSA wc = {0};

    if (!init())
        return;

    test_GetDisplayDriverVersion();
    test_unknown1();
    test_unknown2();
    test_unknown3();
    test_NvAPI_EnumLogicalGPUs();
    test_NvAPI_GetPhysicalGPUsFromLogicalGPU();
    test_NvAPI_EnumPhysicalGPUs();
    test_NvAPI_GPU_GetFullName();
    test_NvAPI_DISP_GetGDIPrimaryDisplayId();
    test_NvAPI_EnumNvidiaDisplayHandle();
    test_NvAPI_SYS_GetDriverAndBranchVersion();
    test_NvAPI_GetLogicalGPUFromDisplay();
    test_NvAPI_D3D11_SetDepthBoundsTest();
    test_NvAPI_D3D11_CreateDevice();

    /* d3d9 tests */
    wc.lpfnWndProc = DefWindowProcA;
    wc.lpszClassName = "d3d9_nvapi_test_wc";
    RegisterClassA(&wc);

    test_NvAPI_D3D_GetCurrentSLIState();
}

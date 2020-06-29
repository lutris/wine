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

#ifndef __WINE_NVAPI_H
#define __WINE_NVAPI_H

#include "pshpack8.h"

typedef unsigned char NvU8;
typedef unsigned int NvU32;

#define NvAPI_Status int

#define NVAPI_OK 0
#define NVAPI_ERROR -1
#define NVAPI_INVALID_ARGUMENT -5
#define NVAPI_NVIDIA_DEVICE_NOT_FOUND -6
#define NVAPI_END_ENUMERATION -7
#define NVAPI_INVALID_HANDLE -8
#define NVAPI_INCOMPATIBLE_STRUCT_VERSION -9
#define NVAPI_INVALID_POINTER -14
#define NVAPI_EXPECTED_LOGICAL_GPU_HANDLE -100
#define NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE -101
#define NVAPI_NO_ACTIVE_SLI_TOPOLOGY -113
#define NVAPI_STEREO_NOT_INITIALIZED -140
#define NVAPI_UNREGISTERED_RESOURCE -170

#define NVAPI_SHORT_STRING_MAX 64
#define NVAPI_PHYSICAL_GPUS 32
#define NVAPI_MAX_PHYSICAL_GPUS 64
#define NVAPI_MAX_LOGICAL_GPUS 64
#define NVAPI_ADVANCED_DISPLAY_HEADS 4
#define NVAPI_MAX_DISPLAYS (NVAPI_PHYSICAL_GPUS * NVAPI_ADVANCED_DISPLAY_HEADS)

typedef char NvAPI_ShortString[NVAPI_SHORT_STRING_MAX];

#define MAKE_NVAPI_VERSION(type,version) (NvU32)(sizeof(type) | ((version)<<16))

typedef void *NvPhysicalGpuHandle;
typedef void *NvLogicalGpuHandle;
typedef void *NvDisplayHandle;
typedef void *StereoHandle;
typedef void *NVDX_ObjectHandle;

typedef enum
{
    NVAPI_DEVICE_FEATURE_LEVEL_NULL      = -1,
    NVAPI_DEVICE_FEATURE_LEVEL_10_0      = 0,
    NVAPI_DEVICE_FEATURE_LEVEL_10_0_PLUS = 1,
    NVAPI_DEVICE_FEATURE_LEVEL_10_1      = 2,
    NVAPI_DEVICE_FEATURE_LEVEL_11_0      = 3,
} NVAPI_DEVICE_FEATURE_LEVEL;

typedef struct
{
    NvU32              version;
    NvU32              drvVersion;
    NvU32              bldChangeListNum;
    NvAPI_ShortString  szBuildBranchString;
    NvAPI_ShortString  szAdapterString;
} NV_DISPLAY_DRIVER_VERSION;

#define NV_DISPLAY_DRIVER_VERSION_VER MAKE_NVAPI_VERSION(NV_DISPLAY_DRIVER_VERSION, 1)

typedef struct
{
    NvU32 version;
    NvU32 maxNumAFRGroups;
    NvU32 numAFRGroups;
    NvU32 currentAFRIndex;
    NvU32 nextFrameAFRIndex;
    NvU32 previousFrameAFRIndex;
    NvU32 bIsCurAFRGroupNew;
} NV_GET_CURRENT_SLI_STATE_V1;

typedef struct
{
    NvU32 version;
    NvU32 maxNumAFRGroups;
    NvU32 numAFRGroups;
    NvU32 currentAFRIndex;
    NvU32 nextFrameAFRIndex;
    NvU32 previousFrameAFRIndex;
    NvU32 bIsCurAFRGroupNew;
    NvU32 numVRSLIGpus;
} NV_GET_CURRENT_SLI_STATE_V2;

#define NV_GET_CURRENT_SLI_STATE_VER1 MAKE_NVAPI_VERSION(NV_GET_CURRENT_SLI_STATE_V1, 1)
#define NV_GET_CURRENT_SLI_STATE_VER2 MAKE_NVAPI_VERSION(NV_GET_CURRENT_SLI_STATE_V2, 1)

#define NV_GET_CURRENT_SLI_STATE_VER NV_GET_CURRENT_SLI_STATE_VER2
#define NV_GET_CURRENT_SLI_STATE     NV_GET_CURRENT_SLI_STATE_V2

/* undocumented stuff */
typedef struct
{
    NvU32 version;
    NvU32 gpu_count;
    struct
    {
        NvPhysicalGpuHandle gpuHandle;
        NvU32 unknown2;
    } gpus[8];
}NV_UNKNOWN_1;

#define NV_UNKNOWN_1_VER MAKE_NVAPI_VERSION(NV_UNKNOWN_1, 1)

#include "poppack.h"

#endif /* __WINE_NVAPI_H */

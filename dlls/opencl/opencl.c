/*
 * OpenCL.dll proxy for native OpenCL implementation.
 *
 * Copyright 2010 Peter Urbanec
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

#include "config.h"
#include "wine/port.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(opencl);

#define CL_SILENCE_DEPRECATION
#if defined(HAVE_CL_CL_H)
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#define CL_TARGET_OPENCL_VERSION 220
#include <CL/cl.h>
#elif defined(HAVE_OPENCL_OPENCL_H)
#include <OpenCL/opencl.h>
#endif

/* TODO: Figure out how to provide GL context sharing before enabling OpenGL */
#define OPENCL_WITH_GL 0

#ifndef SONAME_LIBOPENCL
#if defined(HAVE_OPENCL_OPENCL_H)
#define SONAME_LIBOPENCL "libOpenCL.dylib"
#else
#define SONAME_LIBOPENCL "libOpenCL.so"
#endif
#endif

/* Platform API */
static cl_int (*pclGetPlatformIDs)(cl_uint num_entries, cl_platform_id *platforms, cl_uint *num_platforms);
static cl_int (*pclGetPlatformInfo)(cl_platform_id platform, cl_platform_info param_name,
                                    size_t param_value_size, void * param_value, size_t * param_value_size_ret);

/* Device APIs */
static cl_int (*pclGetDeviceIDs)(cl_platform_id platform, cl_device_type device_type,
                                 cl_uint num_entries, cl_device_id * devices, cl_uint * num_devices);
static cl_int (*pclGetDeviceInfo)(cl_device_id device, cl_device_info param_name,
                                  size_t param_value_size, void * param_value, size_t * param_value_size_ret);

/* Context APIs  */
static cl_context (*pclCreateContext)(const cl_context_properties * properties, cl_uint num_devices, const cl_device_id * devices,
                                      void (*pfn_notify)(const char *errinfo, const void *private_info, size_t cb, void *user_data),
                                      void * user_data, cl_int * errcode_ret);
static cl_context (*pclCreateContextFromType)(const cl_context_properties * properties, cl_device_type device_type,
                                              void (*pfn_notify)(const char *errinfo, const void *private_info, size_t cb, void *user_data),
                                              void * user_data, cl_int * errcode_ret);
static cl_int (*pclRetainContext)(cl_context context);
static cl_int (*pclReleaseContext)(cl_context context);
static cl_int (*pclGetContextInfo)(cl_context context, cl_context_info param_name,
                                   size_t param_value_size, void * param_value, size_t * param_value_size_ret);

/* Command Queue APIs */
static cl_command_queue (*pclCreateCommandQueue)(cl_context context, cl_device_id device,
                                                 cl_command_queue_properties properties, cl_int * errcode_ret);
static cl_int (*pclRetainCommandQueue)(cl_command_queue command_queue);
static cl_int (*pclReleaseCommandQueue)(cl_command_queue command_queue);
static cl_int (*pclGetCommandQueueInfo)(cl_command_queue command_queue, cl_command_queue_info param_name,
                                        size_t param_value_size, void * param_value, size_t * param_value_size_ret);
static cl_int (*pclSetCommandQueueProperty)(cl_command_queue command_queue, cl_command_queue_properties properties, cl_bool enable,
                                            cl_command_queue_properties * old_properties);

/* Memory Object APIs  */
static cl_mem (*pclCreateBuffer)(cl_context context, cl_mem_flags flags, size_t size, void * host_ptr, cl_int * errcode_ret);
static cl_mem (*pclCreateImage2D)(cl_context context, cl_mem_flags flags, cl_image_format * image_format,
                                  size_t image_width, size_t image_height, size_t image_row_pitch, void * host_ptr, cl_int * errcode_ret);
static cl_mem (*pclCreateImage3D)(cl_context context, cl_mem_flags flags, cl_image_format * image_format,
                                  size_t image_width, size_t image_height, size_t image_depth, size_t image_row_pitch, size_t image_slice_pitch,
                                  void * host_ptr, cl_int * errcode_ret);
static cl_int (*pclRetainMemObject)(cl_mem memobj);
static cl_int (*pclReleaseMemObject)(cl_mem memobj);
static cl_int (*pclGetSupportedImageFormats)(cl_context context, cl_mem_flags flags, cl_mem_object_type image_type, cl_uint num_entries,
                                             cl_image_format * image_formats, cl_uint * num_image_formats);
static cl_int (*pclGetMemObjectInfo)(cl_mem memobj, cl_mem_info param_name, size_t param_value_size, void * param_value, size_t * param_value_size_ret);
static cl_int (*pclGetImageInfo)(cl_mem image, cl_image_info param_name, size_t param_value_size, void * param_value, size_t * param_value_size_ret);

/* Sampler APIs  */
static cl_sampler (*pclCreateSampler)(cl_context context, cl_bool normalized_coords, cl_addressing_mode addressing_mode,
                                      cl_filter_mode filter_mode, cl_int * errcode_ret);
static cl_int (*pclRetainSampler)(cl_sampler sampler);
static cl_int (*pclReleaseSampler)(cl_sampler sampler);
static cl_int (*pclGetSamplerInfo)(cl_sampler sampler, cl_sampler_info param_name, size_t param_value_size,
                                   void * param_value, size_t * param_value_size_ret);

/* Program Object APIs  */
static cl_program (*pclCreateProgramWithSource)(cl_context context, cl_uint count, const char ** strings,
                                                const size_t * lengths, cl_int * errcode_ret);
static cl_program (*pclCreateProgramWithBinary)(cl_context context, cl_uint num_devices, const cl_device_id * device_list,
                                                const size_t * lengths, const unsigned char ** binaries, cl_int * binary_status,
                                                cl_int * errcode_ret);
static cl_int (*pclRetainProgram)(cl_program program);
static cl_int (*pclReleaseProgram)(cl_program program);
static cl_int (*pclBuildProgram)(cl_program program, cl_uint num_devices, const cl_device_id * device_list, const char * options,
                                 void (*pfn_notify)(cl_program program, void * user_data),
                                 void * user_data);
static cl_int (*pclUnloadCompiler)(void);
static cl_int (*pclGetProgramInfo)(cl_program program, cl_program_info param_name,
                                   size_t param_value_size, void * param_value, size_t * param_value_size_ret);
static cl_int (*pclGetProgramBuildInfo)(cl_program program, cl_device_id device,
                                        cl_program_build_info param_name, size_t param_value_size, void * param_value,
                                        size_t * param_value_size_ret);

/* Kernel Object APIs */
static cl_kernel (*pclCreateKernel)(cl_program program, char * kernel_name, cl_int * errcode_ret);
static cl_int (*pclCreateKernelsInProgram)(cl_program program, cl_uint num_kernels,
                                           cl_kernel * kernels, cl_uint * num_kernels_ret);
static cl_int (*pclRetainKernel)(cl_kernel kernel);
static cl_int (*pclReleaseKernel)(cl_kernel kernel);
static cl_int (*pclSetKernelArg)(cl_kernel kernel, cl_uint arg_index, size_t arg_size, void * arg_value);
static cl_int (*pclGetKernelInfo)(cl_kernel kernel, cl_kernel_info param_name,
                                  size_t param_value_size, void * param_value, size_t * param_value_size_ret);
static cl_int (*pclGetKernelWorkGroupInfo)(cl_kernel kernel, cl_device_id device,
                                           cl_kernel_work_group_info param_name, size_t param_value_size,
                                           void * param_value, size_t * param_value_size_ret);
/* Event Object APIs  */
static cl_int (*pclWaitForEvents)(cl_uint num_events, cl_event * event_list);
static cl_int (*pclGetEventInfo)(cl_event event, cl_event_info param_name, size_t param_value_size,
                                 void * param_value, size_t * param_value_size_ret);
static cl_int (*pclRetainEvent)(cl_event event);
static cl_int (*pclReleaseEvent)(cl_event event);

/* Profiling APIs  */
static cl_int (*pclGetEventProfilingInfo)(cl_event event, cl_profiling_info param_name, size_t param_value_size,
                                           void * param_value, size_t * param_value_size_ret);

/* Flush and Finish APIs */
static cl_int (*pclFlush)(cl_command_queue command_queue);
static cl_int (*pclFinish)(cl_command_queue command_queue);

/* Enqueued Commands APIs */
static cl_int (*pclEnqueueReadBuffer)(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read,
                                      size_t offset, size_t cb, void * ptr,
                                      cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event);
static cl_int (*pclEnqueueWriteBuffer)(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write,
                                       size_t offset, size_t cb, const void * ptr,
                                       cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event);
static cl_int (*pclEnqueueCopyBuffer)(cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer,
                                      size_t src_offset, size_t dst_offset, size_t cb,
                                      cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event);
static cl_int (*pclEnqueueReadImage)(cl_command_queue command_queue, cl_mem image, cl_bool blocking_read,
                                     const size_t * origin, const size_t * region,
                                     size_t row_pitch, size_t slice_pitch, void * ptr,
                                     cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event);
static cl_int (*pclEnqueueWriteImage)(cl_command_queue command_queue, cl_mem image, cl_bool blocking_write,
                                      const size_t * origin, const size_t * region,
                                      size_t input_row_pitch, size_t input_slice_pitch, const void * ptr,
                                      cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event);
static cl_int (*pclEnqueueCopyImage)(cl_command_queue command_queue, cl_mem src_image, cl_mem dst_image,
                                      size_t * src_origin, size_t * dst_origin, size_t * region,
                                      cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event);
static cl_int (*pclEnqueueCopyImageToBuffer)(cl_command_queue command_queue, cl_mem src_image, cl_mem dst_buffer,
                                             size_t * src_origin, size_t * region, size_t dst_offset,
                                             cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event);
static cl_int (*pclEnqueueCopyBufferToImage)(cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_image,
                                             size_t src_offset, size_t * dst_origin, size_t * region,
                                             cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event);
static void * (*pclEnqueueMapBuffer)(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_map,
                                     cl_map_flags map_flags, size_t offset, size_t cb,
                                     cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event, cl_int * errcode_ret);
static void * (*pclEnqueueMapImage)(cl_command_queue command_queue, cl_mem image, cl_bool blocking_map,
                                    cl_map_flags map_flags, size_t * origin, size_t * region,
                                    size_t * image_row_pitch, size_t * image_slice_pitch,
                                    cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event, cl_int * errcode_ret);
static cl_int (*pclEnqueueUnmapMemObject)(cl_command_queue command_queue, cl_mem memobj, void * mapped_ptr,
                                          cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event);
static cl_int (*pclEnqueueNDRangeKernel)(cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim,
                                         size_t * global_work_offset, size_t * global_work_size, size_t * local_work_size,
                                         cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event);
static cl_int (*pclEnqueueTask)(cl_command_queue command_queue, cl_kernel kernel,
                                cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event);
static cl_int (*pclEnqueueNativeKernel)(cl_command_queue command_queue,
                                        void (*user_func)(void *args),
                                        void * args, size_t cb_args,
                                        cl_uint num_mem_objects, const cl_mem * mem_list, const void ** args_mem_loc,
                                        cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event);
static cl_int (*pclEnqueueMarker)(cl_command_queue command_queue, cl_event * event);
static cl_int (*pclEnqueueWaitForEvents)(cl_command_queue command_queue, cl_uint num_events, cl_event * event_list);
static cl_int (*pclEnqueueBarrier)(cl_command_queue command_queue);

/* Extension function access */
static void * (*pclGetExtensionFunctionAddress)(const char * func_name);

/* OpenCL 1.1 functions */
static cl_mem (*pclCreateSubBuffer)(cl_mem buffer, cl_mem_flags flags,
                                    cl_buffer_create_type buffer_create_type, const void * buffer_create_info, cl_int * errcode_ret);
static cl_event (*pclCreateUserEvent)(cl_context context, cl_int * errcode_ret);
static cl_int (*pclEnqueueCopyBufferRect)(cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer,
                                          const size_t * src_origin, const size_t * dst_origin, const size_t * region,
                                          size_t src_row_pitch, size_t src_slice_pitch,
                                          size_t dst_row_pitch, size_t dst_slice_pitch,
                                          cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event);
static cl_int (*pclEnqueueReadBufferRect)(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read,
                                          const size_t * buffer_origin, const size_t * host_origin, const size_t * region,
                                          size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch,
                                          void * ptr, cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event);
static cl_int (*pclEnqueueWriteBufferRect)(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read,
                                           const size_t * buffer_origin, const size_t * host_origin, const size_t * region,
                                           size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch,
                                           const void * ptr, cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event);
static cl_int (*pclSetEventCallback)(cl_event event, cl_int command_exec_callback_type,
                                     void (*pfn_notify)(cl_event, cl_int, void *), void *user_data);
static cl_int (*pclSetMemObjectDestructorCallback)(cl_mem memobj, void (*pfn_notify)(cl_mem, void*), void *user_data);
static cl_int (*pclSetUserEventStatus)(cl_event event, cl_int execution_status);

/* OpenCL 1.2 functions */
static cl_int (*pclCompileProgram)(cl_program program, cl_uint num_devices, const cl_device_id * device_list, const char * options,
                                   cl_uint num_input_headers, const cl_program * input_headers, const char ** header_include_names,
                                   void (*pfn_notify)(cl_program program, void * user_data),
                                   void * user_data);
static cl_mem (*pclCreateImage)(cl_context context, cl_mem_flags flags,
                                const cl_image_format * image_format, const cl_image_desc * image_desc, void * host_ptr, cl_int * errcode_ret);
static cl_program (*pclCreateProgramWithBuiltInKernels)(cl_context context, cl_uint num_devices, const cl_device_id * device_list,
                                                        const char * kernel_names, cl_int * errcode_ret);
static cl_int (*pclCreateSubDevices)(cl_device_id in_device, const cl_device_partition_property * properties, cl_uint num_entries,
                                     cl_device_id * out_devices, cl_uint * num_devices);
static cl_int (*pclEnqueueBarrierWithWaitList)(cl_command_queue command_queue, cl_uint num_events_in_wait_list,
                                               const cl_event * event_wait_list, cl_event * event);
static cl_int (*pclEnqueueFillBuffer)(cl_command_queue command_queue, cl_mem buffer, const void * pattern, size_t pattern_size, size_t offset, size_t cb,
                                      cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event);
static cl_int (*pclEnqueueFillImage)(cl_command_queue command_queue, cl_mem image, const void * fill_color,
                                     const size_t * origin, const size_t * region,
                                     cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event);
static cl_int (*pclEnqueueMarkerWithWaitList)(cl_command_queue command_queue, cl_uint num_events_in_wait_list,
                                              const cl_event * event_wait_list, cl_event * event);
static cl_int (*pclEnqueueMigrateMemObjects)(cl_command_queue command_queue, cl_uint num_mem_objects, const cl_mem * mem_objects, cl_mem_migration_flags flags,
                                             cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event);
static void * (*pclGetExtensionFunctionAddressForPlatform)(cl_platform_id platform, const char * function_name);
static cl_int (*pclGetKernelArgInfo)(cl_kernel kernel, cl_uint arg_indx, cl_kernel_arg_info param_name,
                                     size_t param_value_size, void * param_value, size_t * param_value_size_ret);
static cl_program (*pclLinkProgram)(cl_context context, cl_uint num_devices, const cl_device_id * device_list, const char * options,
                                    cl_uint num_input_programs, const cl_program * input_programs,
                                    void (* pfn_notify)(cl_program program, void * user_data),
                                    void * user_data, cl_int * errcode_ret);
static cl_int (*pclReleaseDevice)(cl_device_id device);
static cl_int (*pclRetainDevice)(cl_device_id device);
static cl_int (*pclUnloadPlatformCompiler)(cl_platform_id platform);


static BOOL init_opencl(void);
static BOOL load_opencl_func(void);

static void * opencl_handle = NULL;


/***********************************************************************
 *      DllMain [Internal]
 *
 * Initializes the internal 'opencl.dll'.
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD reason, LPVOID reserved)
{
    TRACE("opencl.dll: %p,%x,%p\n", hinstDLL, reason, reserved);

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        if (init_opencl())
            load_opencl_func();
        break;

    case DLL_PROCESS_DETACH:
        if (reserved) break;
        if (opencl_handle) dlclose(opencl_handle);
    }

    return TRUE;
}


/***********************************************************************
 *      init_opencl [Internal]
 *
 * Initializes OpenCL library.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
static BOOL init_opencl(void)
{
#ifdef SONAME_LIBOPENCL
    char error[256];

    opencl_handle = dlopen(SONAME_LIBOPENCL, RTLD_NOW);
    if (opencl_handle != NULL)
    {
        TRACE("Opened library %s\n", SONAME_LIBOPENCL);
        return TRUE;
    }
    else
        ERR("Failed to open library %s: %s\n", SONAME_LIBOPENCL, error);
#else
    ERR("OpenCL is needed but support was not included at build time\n");
#endif
    return FALSE;
}


/***********************************************************************
 *      load_opencl_func [Internal]
 *
 * Populate function table.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
static BOOL load_opencl_func(void)
{


    if (opencl_handle == NULL)
        return FALSE;

#define LOAD_FUNCPTR(f) \
    if (!(p##f = dlsym(opencl_handle, #f))) \
    WARN("%s not found in %s\n", #f, SONAME_LIBOPENCL);

    /* Platform API */
    LOAD_FUNCPTR(clGetPlatformIDs);
    LOAD_FUNCPTR(clGetPlatformInfo);
    /* Device APIs */
    LOAD_FUNCPTR(clGetDeviceIDs);
    LOAD_FUNCPTR(clGetDeviceInfo);
    /* Context APIs  */
    LOAD_FUNCPTR(clCreateContext);
    LOAD_FUNCPTR(clCreateContextFromType);
    LOAD_FUNCPTR(clRetainContext);
    LOAD_FUNCPTR(clReleaseContext);
    LOAD_FUNCPTR(clGetContextInfo);
    /* Command Queue APIs */
    LOAD_FUNCPTR(clCreateCommandQueue);
    LOAD_FUNCPTR(clRetainCommandQueue);
    LOAD_FUNCPTR(clReleaseCommandQueue);
    LOAD_FUNCPTR(clGetCommandQueueInfo);
    LOAD_FUNCPTR(clSetCommandQueueProperty);
    /* Memory Object APIs  */
    LOAD_FUNCPTR(clCreateBuffer);
    LOAD_FUNCPTR(clCreateImage2D);
    LOAD_FUNCPTR(clCreateImage3D);
    LOAD_FUNCPTR(clRetainMemObject);
    LOAD_FUNCPTR(clReleaseMemObject);
    LOAD_FUNCPTR(clGetSupportedImageFormats);
    LOAD_FUNCPTR(clGetMemObjectInfo);
    LOAD_FUNCPTR(clGetImageInfo);
    /* Sampler APIs  */
    LOAD_FUNCPTR(clCreateSampler);
    LOAD_FUNCPTR(clRetainSampler);
    LOAD_FUNCPTR(clReleaseSampler);
    LOAD_FUNCPTR(clGetSamplerInfo);
    /* Program Object APIs  */
    LOAD_FUNCPTR(clCreateProgramWithSource);
    LOAD_FUNCPTR(clCreateProgramWithBinary);
    LOAD_FUNCPTR(clRetainProgram);
    LOAD_FUNCPTR(clReleaseProgram);
    LOAD_FUNCPTR(clBuildProgram);
    LOAD_FUNCPTR(clUnloadCompiler);
    LOAD_FUNCPTR(clGetProgramInfo);
    LOAD_FUNCPTR(clGetProgramBuildInfo);
    /* Kernel Object APIs */
    LOAD_FUNCPTR(clCreateKernel);
    LOAD_FUNCPTR(clCreateKernelsInProgram);
    LOAD_FUNCPTR(clRetainKernel);
    LOAD_FUNCPTR(clReleaseKernel);
    LOAD_FUNCPTR(clSetKernelArg);
    LOAD_FUNCPTR(clGetKernelInfo);
    LOAD_FUNCPTR(clGetKernelWorkGroupInfo);
    /* Event Object APIs  */
    LOAD_FUNCPTR(clWaitForEvents);
    LOAD_FUNCPTR(clGetEventInfo);
    LOAD_FUNCPTR(clRetainEvent);
    LOAD_FUNCPTR(clReleaseEvent);
    /* Profiling APIs  */
    LOAD_FUNCPTR(clGetEventProfilingInfo);
    /* Flush and Finish APIs */
    LOAD_FUNCPTR(clFlush);
    LOAD_FUNCPTR(clFinish);
    /* Enqueued Commands APIs */
    LOAD_FUNCPTR(clEnqueueReadBuffer);
    LOAD_FUNCPTR(clEnqueueWriteBuffer);
    LOAD_FUNCPTR(clEnqueueCopyBuffer);
    LOAD_FUNCPTR(clEnqueueReadImage);
    LOAD_FUNCPTR(clEnqueueWriteImage);
    LOAD_FUNCPTR(clEnqueueCopyImage);
    LOAD_FUNCPTR(clEnqueueCopyImageToBuffer);
    LOAD_FUNCPTR(clEnqueueCopyBufferToImage);
    LOAD_FUNCPTR(clEnqueueMapBuffer);
    LOAD_FUNCPTR(clEnqueueMapImage);
    LOAD_FUNCPTR(clEnqueueUnmapMemObject);
    LOAD_FUNCPTR(clEnqueueNDRangeKernel);
    LOAD_FUNCPTR(clEnqueueTask);
    LOAD_FUNCPTR(clEnqueueNativeKernel);
    LOAD_FUNCPTR(clEnqueueMarker);
    LOAD_FUNCPTR(clEnqueueWaitForEvents);
    LOAD_FUNCPTR(clEnqueueBarrier);
    /* Extension function access */
    LOAD_FUNCPTR(clGetExtensionFunctionAddress);

    /* OpenCL 1.1 functions */
#ifdef CL_VERSION_1_1
    LOAD_FUNCPTR(clCreateSubBuffer);
    LOAD_FUNCPTR(clCreateUserEvent);
    LOAD_FUNCPTR(clEnqueueCopyBufferRect);
    LOAD_FUNCPTR(clEnqueueReadBufferRect);
    LOAD_FUNCPTR(clEnqueueWriteBufferRect);
    LOAD_FUNCPTR(clSetEventCallback);
    LOAD_FUNCPTR(clSetMemObjectDestructorCallback);
    LOAD_FUNCPTR(clSetUserEventStatus);
#endif

    /* OpenCL 1.2 functions */
#ifdef CL_VERSION_1_2
    LOAD_FUNCPTR(clCompileProgram);
    /*LOAD_FUNCPTR(clCreateFromGLTexture);*/
    LOAD_FUNCPTR(clCreateImage);
    LOAD_FUNCPTR(clCreateProgramWithBuiltInKernels);
    LOAD_FUNCPTR(clCreateSubDevices);
    LOAD_FUNCPTR(clEnqueueBarrierWithWaitList);
    LOAD_FUNCPTR(clEnqueueFillBuffer);
    LOAD_FUNCPTR(clEnqueueFillImage);
    LOAD_FUNCPTR(clEnqueueMarkerWithWaitList);
    LOAD_FUNCPTR(clEnqueueMigrateMemObjects);
    LOAD_FUNCPTR(clGetExtensionFunctionAddressForPlatform);
    LOAD_FUNCPTR(clGetKernelArgInfo);
    LOAD_FUNCPTR(clLinkProgram);
    LOAD_FUNCPTR(clReleaseDevice);
    LOAD_FUNCPTR(clRetainDevice);
    LOAD_FUNCPTR(clUnloadPlatformCompiler);
#endif

#undef LOAD_FUNCPTR

    return TRUE;
}


/*---------------------------------------------------------------*/
/* Platform API */

cl_int WINAPI wine_clGetPlatformIDs(cl_uint num_entries, cl_platform_id *platforms, cl_uint *num_platforms)
{
    cl_int ret;
    TRACE("(%d, %p, %p)\n", num_entries, platforms, num_platforms);
    if (!pclGetPlatformIDs) return CL_INVALID_VALUE;
    ret = pclGetPlatformIDs(num_entries, platforms, num_platforms);
    TRACE("(%d, %p, %p)=%d\n", num_entries, platforms, num_platforms, ret);
    return ret;
}

cl_int WINAPI wine_clGetPlatformInfo(cl_platform_id platform, cl_platform_info param_name,
                                     SIZE_T param_value_size, void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("(%p, 0x%x, %ld, %p, %p)\n", platform, param_name, param_value_size, param_value, param_value_size_ret);

    if (!pclGetPlatformInfo) return CL_INVALID_VALUE;

    /* Hide all extensions.
     * TODO: Add individual extension support as needed.
     */
/*    if (param_name == CL_PLATFORM_EXTENSIONS)
    {
        ret = CL_INVALID_VALUE;

        if (param_value && param_value_size > 0)
        {
            char *exts = (char *) param_value;
            exts[0] = '\0';
            ret = CL_SUCCESS;
        }

        if (param_value_size_ret)
        {
            *param_value_size_ret = 1;
            ret = CL_SUCCESS;
        }
    }
    else*/
    {
        ret = pclGetPlatformInfo(platform, param_name, param_value_size, param_value, param_value_size_ret);
    }

    TRACE("(%p, 0x%x, %ld, %p, %p)=%d\n", platform, param_name, param_value_size, param_value, param_value_size_ret, ret);
    return ret;
}


/*---------------------------------------------------------------*/
/* Device APIs */

cl_int WINAPI wine_clGetDeviceIDs(cl_platform_id platform, cl_device_type device_type,
                                  cl_uint num_entries, cl_device_id * devices, cl_uint * num_devices)
{
    cl_int ret;
    TRACE("(%p, 0x%lx, %d, %p, %p)\n", platform, (long unsigned int)device_type, num_entries, devices, num_devices);
    if (!pclGetDeviceIDs) return CL_INVALID_VALUE;
    ret = pclGetDeviceIDs(platform, device_type, num_entries, devices, num_devices);
    TRACE("(%p, 0x%lx, %d, %p, %p)=%d\n", platform, (long unsigned int)device_type, num_entries, devices, num_devices, ret);
    return ret;
}

cl_int WINAPI wine_clGetDeviceInfo(cl_device_id device, cl_device_info param_name,
                                   SIZE_T param_value_size, void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("(%p, 0x%x, %ld, %p, %p)\n",device, param_name, param_value_size, param_value, param_value_size_ret);

    if (!pclGetDeviceInfo) return CL_INVALID_VALUE;

    /* Hide all extensions.
     * TODO: Add individual extension support as needed.
     */
/*    if (param_name == CL_DEVICE_EXTENSIONS)
    {
        ret = CL_INVALID_VALUE;

        if (param_value && param_value_size > 0)
        {
            char *exts = (char *) param_value;
            exts[0] = '\0';
            ret = CL_SUCCESS;
        }

        if (param_value_size_ret)
        {
            *param_value_size_ret = 1;
            ret = CL_SUCCESS;
        }
    }
    else*/
    {
        ret = pclGetDeviceInfo(device, param_name, param_value_size, param_value, param_value_size_ret);
    }

    /* Filter out the CL_EXEC_NATIVE_KERNEL flag */
    if (param_name == CL_DEVICE_EXECUTION_CAPABILITIES)
    {
        cl_device_exec_capabilities *caps = (cl_device_exec_capabilities *) param_value;
        *caps &= ~CL_EXEC_NATIVE_KERNEL;
    }

    TRACE("(%p, 0x%x, %ld, %p, %p)=%d\n",device, param_name, param_value_size, param_value, param_value_size_ret, ret);
    return ret;
}

cl_int WINAPI wine_clCreateSubDevices(cl_device_id in_device, const cl_device_partition_property * properties, cl_uint num_entries,
                                      cl_device_id * out_devices, cl_uint * num_devices)
{
    cl_int ret;
    TRACE("(%p, %p, %d, %p, %p)\n", in_device, properties, num_entries, out_devices, num_devices);
    if (!pclCreateSubDevices) return CL_INVALID_VALUE;
    ret = pclCreateSubDevices(in_device, properties, num_entries, out_devices, num_devices);
    TRACE("(%p, %p, %d, %p, %p)=%d\n", in_device, properties, num_entries, out_devices, num_devices, ret);
    return ret;
}

cl_int WINAPI wine_clRetainDevice(cl_device_id device)
{
    cl_int ret;
    TRACE("(%p)\n", device);
    if (!pclRetainDevice) return CL_INVALID_DEVICE;
    ret = pclRetainDevice(device);
    TRACE("(%p)=%d\n", device, ret);
    return ret;

}

cl_int WINAPI wine_clReleaseDevice(cl_device_id device)
{
    cl_int ret;
    TRACE("(%p)\n", device);
    if (!pclReleaseDevice) return CL_INVALID_DEVICE;
    ret = pclReleaseDevice(device);
    TRACE("(%p)=%d\n", device, ret);
    return ret;
}


/*---------------------------------------------------------------*/
/* Context APIs  */

typedef struct
{
    void WINAPI (*pfn_notify)(const char *errinfo, const void *private_info, size_t cb, void *user_data);
    void *user_data;
} CONTEXT_CALLBACK;

static void context_fn_notify(const char *errinfo, const void *private_info, size_t cb, void *user_data)
{
    CONTEXT_CALLBACK *ccb;
    TRACE("(%s, %p, %ld, %p)\n", errinfo, private_info, (SIZE_T)cb, user_data);
    ccb = (CONTEXT_CALLBACK *) user_data;
    if(ccb->pfn_notify) ccb->pfn_notify(errinfo, private_info, cb, ccb->user_data);
    TRACE("Callback COMPLETED\n");
}

cl_context WINAPI wine_clCreateContext(const cl_context_properties * properties, cl_uint num_devices, const cl_device_id * devices,
                                       void WINAPI (*pfn_notify)(const char *errinfo, const void *private_info, size_t cb, void *user_data),
                                       void * user_data, cl_int * errcode_ret)
{
    cl_context ret;
    CONTEXT_CALLBACK *ccb;
    TRACE("(%p, %d, %p, %p, %p, %p)\n", properties, num_devices, devices, pfn_notify, user_data, errcode_ret);
    if (!pclCreateContext)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return NULL;
    }
    /* FIXME: The CONTEXT_CALLBACK structure is currently leaked.
     * Pointers to callback redirectors should be remembered and free()d when the context is destroyed.
     * The problem is determining when a context is being destroyed. clReleaseContext only decrements
     * the use count for a context, its destruction can come much later and therefore there is a risk
     * that the callback could be invoked after the user_data memory has been free()d.
     */
    ccb = HeapAlloc(GetProcessHeap(), 0, sizeof(CONTEXT_CALLBACK));
    ccb->pfn_notify = pfn_notify;
    ccb->user_data = user_data;
    ret = pclCreateContext(properties, num_devices, devices, context_fn_notify, ccb, errcode_ret);
    TRACE("(%p, %d, %p, %p, %p, %p (%d)))=%p\n", properties, num_devices, devices, &pfn_notify, user_data, errcode_ret, errcode_ret ? *errcode_ret : 0, ret);
    return ret;
}

cl_context WINAPI wine_clCreateContextFromType(const cl_context_properties * properties, cl_device_type device_type,
                                               void WINAPI (*pfn_notify)(const char *errinfo, const void *private_info, size_t cb, void *user_data),
                                               void * user_data, cl_int * errcode_ret)
{
    cl_context ret;
    CONTEXT_CALLBACK *ccb;
    TRACE("(%p, 0x%lx, %p, %p, %p)\n", properties, (long unsigned int)device_type, pfn_notify, user_data, errcode_ret);
    if (!pclCreateContextFromType)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return NULL;
    }
    /* FIXME: The CONTEXT_CALLBACK structure is currently leaked.
     * Pointers to callback redirectors should be remembered and free()d when the context is destroyed.
     * The problem is determining when a context is being destroyed. clReleaseContext only decrements
     * the use count for a context, its destruction can come much later and therefore there is a risk
     * that the callback could be invoked after the user_data memory has been free()d.
     */
    ccb = HeapAlloc(GetProcessHeap(), 0, sizeof(CONTEXT_CALLBACK));
    ccb->pfn_notify = pfn_notify;
    ccb->user_data = user_data;
    ret = pclCreateContextFromType(properties, device_type, context_fn_notify, ccb, errcode_ret);
    TRACE("(%p, 0x%lx, %p, %p, %p (%d)))=%p\n", properties, (long unsigned int)device_type, pfn_notify, user_data, errcode_ret, errcode_ret ? *errcode_ret : 0, ret);
    return ret;
}

cl_int WINAPI wine_clRetainContext(cl_context context)
{
    cl_int ret;
    TRACE("(%p)\n", context);
    if (!pclRetainContext) return CL_INVALID_VALUE;
    ret = pclRetainContext(context);
    TRACE("(%p)=%d\n", context, ret);
    return ret;
}

cl_int WINAPI wine_clReleaseContext(cl_context context)
{
    cl_int ret;
    TRACE("(%p)\n", context);
    if (!pclReleaseContext) return CL_INVALID_VALUE;
    ret = pclReleaseContext(context);
    TRACE("(%p)=%d\n", context, ret);
    return ret;
}

cl_int WINAPI wine_clGetContextInfo(cl_context context, cl_context_info param_name,
                                    SIZE_T param_value_size, void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("(%p, 0x%x, %ld, %p, %p)\n", context, param_name, param_value_size, param_value, param_value_size_ret);
    if (!pclGetContextInfo) return CL_INVALID_VALUE;
    ret = pclGetContextInfo(context, param_name, param_value_size, param_value, param_value_size_ret);
    TRACE("(%p, 0x%x, %ld, %p, %p)=%d\n", context, param_name, param_value_size, param_value, param_value_size_ret, ret);
    return ret;
}


/*---------------------------------------------------------------*/
/* Command Queue APIs */

cl_command_queue WINAPI wine_clCreateCommandQueue(cl_context context, cl_device_id device,
                                                  cl_command_queue_properties properties, cl_int * errcode_ret)
{
    cl_command_queue ret;
    TRACE("(%p, %p, 0x%lx, %p)\n", context, device, (long unsigned int)properties, errcode_ret);
    if (!pclCreateCommandQueue)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return NULL;
    }
    ret = pclCreateCommandQueue(context, device, properties, errcode_ret);
    TRACE("(%p, %p, 0x%lx, %p)=%p\n", context, device, (long unsigned int)properties, errcode_ret, ret);
    return ret;
}

cl_int WINAPI wine_clRetainCommandQueue(cl_command_queue command_queue)
{
    cl_int ret;
    TRACE("(%p)\n", command_queue);
    if (!pclRetainCommandQueue) return CL_INVALID_VALUE;
    ret = pclRetainCommandQueue(command_queue);
    TRACE("(%p)=%d\n", command_queue, ret);
    return ret;
}

cl_int WINAPI wine_clReleaseCommandQueue(cl_command_queue command_queue)
{
    cl_int ret;
    TRACE("(%p)\n", command_queue);
    if (!pclReleaseCommandQueue) return CL_INVALID_VALUE;
    ret = pclReleaseCommandQueue(command_queue);
    TRACE("(%p)=%d\n", command_queue, ret);
    return ret;
}

cl_int WINAPI wine_clGetCommandQueueInfo(cl_command_queue command_queue, cl_command_queue_info param_name,
                                         SIZE_T param_value_size, void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("%p, %d, %ld, %p, %p\n", command_queue, param_name, param_value_size, param_value, param_value_size_ret);
    if (!pclGetCommandQueueInfo) return CL_INVALID_VALUE;
    ret = pclGetCommandQueueInfo(command_queue, param_name, param_value_size, param_value, param_value_size_ret);
    return ret;
}

cl_int WINAPI wine_clSetCommandQueueProperty(cl_command_queue command_queue, cl_command_queue_properties properties, cl_bool enable,
                                             cl_command_queue_properties * old_properties)
{
    FIXME("(%p, 0x%lx, %d, %p): deprecated\n", command_queue, (long unsigned int)properties, enable, old_properties);
    return CL_INVALID_QUEUE_PROPERTIES;
}


/*---------------------------------------------------------------*/
/* Memory Object APIs  */

cl_mem WINAPI wine_clCreateBuffer(cl_context context, cl_mem_flags flags, size_t size, void * host_ptr, cl_int * errcode_ret)
{
    cl_mem ret;
    TRACE("\n");
    if (!pclCreateBuffer)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return NULL;
    }
    ret = pclCreateBuffer(context, flags, size, host_ptr, errcode_ret);
    return ret;
}

cl_mem WINAPI wine_clCreateSubBuffer(cl_mem buffer, cl_mem_flags flags,
                                     cl_buffer_create_type buffer_create_type, const void * buffer_create_info, cl_int * errcode_ret)
{
    cl_mem ret;
    TRACE("\n");
    if (!pclCreateSubBuffer)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return NULL;
    }
    ret = pclCreateSubBuffer(buffer, flags, buffer_create_type, buffer_create_info, errcode_ret);
    return ret;
}

cl_mem WINAPI wine_clCreateImage(cl_context context, cl_mem_flags flags,
                                 const cl_image_format * image_format, const cl_image_desc * image_desc, void * host_ptr, cl_int * errcode_ret)
{
    cl_mem ret;
    TRACE("\n");
    if (!pclCreateImage)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return NULL;
    }
    ret = pclCreateImage(context, flags, image_format, image_desc, host_ptr, errcode_ret);
    return ret;
}

cl_mem WINAPI wine_clCreateImage2D(cl_context context, cl_mem_flags flags, cl_image_format * image_format,
                                   size_t image_width, size_t image_height, size_t image_row_pitch, void * host_ptr, cl_int * errcode_ret)
{
    cl_mem ret;
    TRACE("\n");
    if (!pclCreateImage2D)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return NULL;
    }
    ret = pclCreateImage2D(context, flags, image_format, image_width, image_height, image_row_pitch, host_ptr, errcode_ret);
    return ret;
}

cl_mem WINAPI wine_clCreateImage3D(cl_context context, cl_mem_flags flags, cl_image_format * image_format,
                                   size_t image_width, size_t image_height, size_t image_depth, size_t image_row_pitch, size_t image_slice_pitch,
                                   void * host_ptr, cl_int * errcode_ret)
{
    cl_mem ret;
    TRACE("\n");
    if (!pclCreateImage3D)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return NULL;
    }
    ret = pclCreateImage3D(context, flags, image_format, image_width, image_height, image_depth, image_row_pitch, image_slice_pitch, host_ptr, errcode_ret);
    return ret;
}

cl_int WINAPI wine_clRetainMemObject(cl_mem memobj)
{
    cl_int ret;
    TRACE("(%p)\n", memobj);
    if (!pclRetainMemObject) return CL_INVALID_VALUE;
    ret = pclRetainMemObject(memobj);
    TRACE("(%p)=%d\n", memobj, ret);
    return ret;
}

cl_int WINAPI wine_clReleaseMemObject(cl_mem memobj)
{
    cl_int ret;
    TRACE("(%p)\n", memobj);
    if (!pclReleaseMemObject) return CL_INVALID_VALUE;
    ret = pclReleaseMemObject(memobj);
    TRACE("(%p)=%d\n", memobj, ret);
    return ret;
}

cl_int WINAPI wine_clGetSupportedImageFormats(cl_context context, cl_mem_flags flags, cl_mem_object_type image_type, cl_uint num_entries,
                                              cl_image_format * image_formats, cl_uint * num_image_formats)
{
    cl_int ret;
    TRACE("\n");
    if (!pclGetSupportedImageFormats) return CL_INVALID_VALUE;
    ret = pclGetSupportedImageFormats(context, flags, image_type, num_entries, image_formats, num_image_formats);
    return ret;
}

cl_int WINAPI wine_clGetMemObjectInfo(cl_mem memobj, cl_mem_info param_name, size_t param_value_size, void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("\n");
    if (!pclGetMemObjectInfo) return CL_INVALID_VALUE;
    ret = pclGetMemObjectInfo(memobj, param_name, param_value_size, param_value, param_value_size_ret);
    return ret;
}

cl_int WINAPI wine_clGetImageInfo(cl_mem image, cl_image_info param_name, size_t param_value_size, void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("\n");
    if (!pclGetImageInfo) return CL_INVALID_VALUE;
    ret = pclGetImageInfo(image, param_name, param_value_size, param_value, param_value_size_ret);
    return ret;
}

typedef struct
{
    void WINAPI (*pfn_notify)(cl_mem memobj, void* user_data);
    void *user_data;
} MEM_CALLBACK;

static void mem_fn_notify(cl_mem memobj, void* user_data)
{
    MEM_CALLBACK *mcb;
    FIXME("(%p, %p)\n", memobj, user_data);
    mcb = (MEM_CALLBACK *) user_data;
    mcb->pfn_notify(memobj, mcb->user_data);
    HeapFree(GetProcessHeap(), 0, mcb);
    FIXME("Callback COMPLETED\n");
}

cl_int WINAPI wine_clSetMemObjectDestructorCallback(cl_mem memobj, void WINAPI (*pfn_notify)(cl_mem, void*), void *user_data)
{
    /* FIXME: Based on PROGRAM_CALLBACK/program_fn_notify function. I'm not sure about this. */
    cl_int ret;
    FIXME("(%p, %p, %p)\n", memobj, pfn_notify, user_data);
    if (!pclSetMemObjectDestructorCallback) return CL_INVALID_VALUE;
    if(pfn_notify)
    {
        /* When pfn_notify is provided, clSetMemObjectDestructorCallback is asynchronous */
        MEM_CALLBACK *mcb;
        mcb = HeapAlloc(GetProcessHeap(), 0, sizeof(MEM_CALLBACK));
        mcb->pfn_notify = pfn_notify;
        mcb->user_data = user_data;
        ret = pclSetMemObjectDestructorCallback(memobj, mem_fn_notify, user_data);
    }
    else
    {
        /* When pfn_notify is NULL, clSetMemObjectDestructorCallback is synchronous */
        ret = pclSetMemObjectDestructorCallback(memobj, NULL, user_data);
    }
    FIXME("(%p, %p, %p)=%d\n", memobj, pfn_notify, user_data, ret);
    return ret;
}


/*---------------------------------------------------------------*/
/* Sampler APIs  */

cl_sampler WINAPI wine_clCreateSampler(cl_context context, cl_bool normalized_coords, cl_addressing_mode addressing_mode,
                                       cl_filter_mode filter_mode, cl_int * errcode_ret)
{
    cl_sampler ret;
    TRACE("\n");
    if (!pclCreateSampler)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return NULL;
    }
    ret = pclCreateSampler(context, normalized_coords, addressing_mode, filter_mode, errcode_ret);
    return ret;
}

cl_int WINAPI wine_clRetainSampler(cl_sampler sampler)
{
    cl_int ret;
    TRACE("\n");
    if (!pclRetainSampler) return CL_INVALID_VALUE;
    ret = pclRetainSampler(sampler);
    return ret;
}

cl_int WINAPI wine_clReleaseSampler(cl_sampler sampler)
{
    cl_int ret;
    TRACE("\n");
    if (!pclReleaseSampler) return CL_INVALID_VALUE;
    ret = pclReleaseSampler(sampler);
    return ret;
}

cl_int WINAPI wine_clGetSamplerInfo(cl_sampler sampler, cl_sampler_info param_name, size_t param_value_size,
                                    void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("\n");
    if (!pclGetSamplerInfo) return CL_INVALID_VALUE;
    ret = pclGetSamplerInfo(sampler, param_name, param_value_size, param_value, param_value_size_ret);
    return ret;
}


/*---------------------------------------------------------------*/
/* Program Object APIs  */

cl_program WINAPI wine_clCreateProgramWithSource(cl_context context, cl_uint count, const char ** strings,
                                                 const size_t * lengths, cl_int * errcode_ret)
{
    cl_program ret;
    TRACE("\n");
    if (!pclCreateProgramWithSource)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return NULL;
    }
    ret = pclCreateProgramWithSource(context, count, strings, lengths, errcode_ret);
    return ret;
}

cl_program WINAPI wine_clCreateProgramWithBinary(cl_context context, cl_uint num_devices, const cl_device_id * device_list,
                                                 const size_t * lengths, const unsigned char ** binaries, cl_int * binary_status,
                                                 cl_int * errcode_ret)
{
    cl_program ret;
    TRACE("\n");
    if (!pclCreateProgramWithBinary)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return NULL;
    }
    ret = pclCreateProgramWithBinary(context, num_devices, device_list, lengths, binaries, binary_status, errcode_ret);
    return ret;
}

cl_program WINAPI wine_clCreateProgramWithBuiltInKernels(cl_context context, cl_uint num_devices, const cl_device_id * device_list,
                                                         const char * kernel_names, cl_int * errcode_ret)
{
    cl_program ret;
    TRACE("\n");
    if (!pclCreateProgramWithBuiltInKernels)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return NULL;
    }
    ret = pclCreateProgramWithBuiltInKernels(context, num_devices, device_list, kernel_names, errcode_ret);
    return ret;
}

cl_int WINAPI wine_clRetainProgram(cl_program program)
{
    cl_int ret;
    TRACE("\n");
    if (!pclRetainProgram) return CL_INVALID_PROGRAM;
    ret = pclRetainProgram(program);
    return ret;
}

cl_int WINAPI wine_clReleaseProgram(cl_program program)
{
    cl_int ret;
    TRACE("\n");
    if (!pclReleaseProgram) return CL_INVALID_PROGRAM;
    ret = pclReleaseProgram(program);
    return ret;
}

typedef struct
{
    void WINAPI (*pfn_notify)(cl_program program, void * user_data);
    void *user_data;
} PROGRAM_CALLBACK;

static void program_fn_notify(cl_program program, void * user_data)
{
    PROGRAM_CALLBACK *pcb;
    TRACE("(%p, %p)\n", program, user_data);
    pcb = (PROGRAM_CALLBACK *) user_data;
    pcb->pfn_notify(program, pcb->user_data);
    HeapFree(GetProcessHeap(), 0, pcb);
    TRACE("Callback COMPLETED\n");
}

cl_int WINAPI wine_clBuildProgram(cl_program program, cl_uint num_devices, const cl_device_id * device_list, const char * options,
                                  void WINAPI (*pfn_notify)(cl_program program, void * user_data),
                                  void * user_data)
{
    cl_int ret;
    TRACE("\n");
    if (!pclBuildProgram) return CL_INVALID_VALUE;
    if(pfn_notify)
    {
        /* When pfn_notify is provided, clBuildProgram is asynchronous */
        PROGRAM_CALLBACK *pcb;
        pcb = HeapAlloc(GetProcessHeap(), 0, sizeof(PROGRAM_CALLBACK));
        pcb->pfn_notify = pfn_notify;
        pcb->user_data = user_data;
        ret = pclBuildProgram(program, num_devices, device_list, options, program_fn_notify, pcb);
    }
    else
    {
        /* When pfn_notify is NULL, clBuildProgram is synchronous */
        ret = pclBuildProgram(program, num_devices, device_list, options, NULL, user_data);
    }
    return ret;
}

cl_int WINAPI wine_clCompileProgram(cl_program program, cl_uint num_devices, const cl_device_id * device_list, const char * options,
                                    cl_uint num_input_headers, const cl_program * input_headers, const char ** header_include_names,
                                    void WINAPI (*pfn_notify)(cl_program program, void * user_data),
                                    void * user_data)
{
    cl_int ret;
    TRACE("\n");
    if (!pclCompileProgram) return CL_INVALID_VALUE;
    if(pfn_notify)
    {
        /* When pfn_notify is provided, clCompileProgram is asynchronous */
        PROGRAM_CALLBACK *pcb;
        pcb = HeapAlloc(GetProcessHeap(), 0, sizeof(PROGRAM_CALLBACK));
        pcb->pfn_notify = pfn_notify;
        pcb->user_data = user_data;
        ret = pclCompileProgram(program, num_devices, device_list, options, num_input_headers, input_headers, header_include_names, program_fn_notify, user_data);
    }
    else
    {
        /* When pfn_notify is NULL, clCompileProgram is synchronous */
        ret = pclCompileProgram(program, num_devices, device_list, options, num_input_headers, input_headers, header_include_names, NULL, user_data);
    }
    return ret;
}

cl_program WINAPI wine_clLinkProgram(cl_context context, cl_uint num_devices, const cl_device_id * device_list, const char * options,
                                     cl_uint num_input_programs, const cl_program * input_programs,
                                     void WINAPI (* pfn_notify)(cl_program program, void * user_data),
                                     void * user_data, cl_int * errcode_ret)
{
    cl_program ret;
    TRACE("\n");
    if (!pclLinkProgram)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return NULL;
    }
    if(pfn_notify)
    {
        /* When pfn_notify is provided, clLinkProgram is asynchronous */
        PROGRAM_CALLBACK *pcb;
        pcb = HeapAlloc(GetProcessHeap(), 0, sizeof(PROGRAM_CALLBACK));
        pcb->pfn_notify = pfn_notify;
        pcb->user_data = user_data;
        ret = pclLinkProgram(context, num_devices, device_list, options, num_input_programs, input_programs, program_fn_notify, user_data, errcode_ret);
    }
    else
    {
        /* When pfn_notify is NULL, clLinkProgram is synchronous */
        ret = pclLinkProgram(context, num_devices, device_list, options, num_input_programs, input_programs, NULL, user_data, errcode_ret);
    }
    return ret;
}

cl_int WINAPI wine_clUnloadCompiler(void)
{
    cl_int ret;
    TRACE("()\n");
    if (!pclUnloadCompiler) return CL_SUCCESS;
    ret = pclUnloadCompiler();
    TRACE("()=%d\n", ret);
    return ret;
}

cl_int WINAPI wine_clUnloadPlatformCompiler(cl_platform_id platform)
{
    cl_int ret;
    TRACE("()\n");
    if (!pclUnloadPlatformCompiler) return CL_SUCCESS;
    ret = pclUnloadPlatformCompiler(platform);
    TRACE("()=%d\n", ret);
    return ret;
}

cl_int WINAPI wine_clGetProgramInfo(cl_program program, cl_program_info param_name,
                                    size_t param_value_size, void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("\n");
    if (!pclGetProgramInfo) return CL_INVALID_VALUE;
    ret = pclGetProgramInfo(program, param_name, param_value_size, param_value, param_value_size_ret);
    return ret;
}

cl_int WINAPI wine_clGetProgramBuildInfo(cl_program program, cl_device_id device,
                                         cl_program_build_info param_name, size_t param_value_size, void * param_value,
                                         size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("\n");
    if (!pclGetProgramBuildInfo) return CL_INVALID_VALUE;
    ret = pclGetProgramBuildInfo(program, device, param_name, param_value_size, param_value, param_value_size_ret);
    return ret;
}


/*---------------------------------------------------------------*/
/* Kernel Object APIs */

cl_kernel WINAPI wine_clCreateKernel(cl_program program, char * kernel_name, cl_int * errcode_ret)
{
    cl_kernel ret;
    TRACE("\n");
    if (!pclCreateKernel)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return NULL;
    }
    ret = pclCreateKernel(program, kernel_name, errcode_ret);
    return ret;
}

cl_int WINAPI wine_clCreateKernelsInProgram(cl_program program, cl_uint num_kernels,
                                            cl_kernel * kernels, cl_uint * num_kernels_ret)
{
    cl_int ret;
    TRACE("\n");
    if (!pclCreateKernelsInProgram) return CL_INVALID_VALUE;
    ret = pclCreateKernelsInProgram(program, num_kernels, kernels, num_kernels_ret);
    return ret;
}

cl_int WINAPI wine_clRetainKernel(cl_kernel kernel)
{
    cl_int ret;
    TRACE("\n");
    if (!pclRetainKernel) return CL_INVALID_KERNEL;
    ret = pclRetainKernel(kernel);
    return ret;
}

cl_int WINAPI wine_clReleaseKernel(cl_kernel kernel)
{
    cl_int ret;
    TRACE("\n");
    if (!pclReleaseKernel) return CL_INVALID_KERNEL;
    ret = pclReleaseKernel(kernel);
    return ret;
}

cl_int WINAPI wine_clSetKernelArg(cl_kernel kernel, cl_uint arg_index, size_t arg_size, void * arg_value)
{
    cl_int ret;
    TRACE("\n");
    if (!pclSetKernelArg) return CL_INVALID_KERNEL;
    ret = pclSetKernelArg(kernel, arg_index, arg_size, arg_value);
    return ret;
}

cl_int WINAPI wine_clGetKernelArgInfo(cl_kernel kernel, cl_uint arg_indx, cl_kernel_arg_info param_name,
                                      size_t param_value_size, void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("\n");
    if (!pclGetKernelArgInfo) return CL_INVALID_VALUE;
    ret = pclGetKernelArgInfo(kernel, arg_indx, param_name, param_value_size, param_value, param_value_size_ret);
    return ret;
}

cl_int WINAPI wine_clGetKernelInfo(cl_kernel kernel, cl_kernel_info param_name,
                                   size_t param_value_size, void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("\n");
    if (!pclGetKernelInfo) return CL_INVALID_VALUE;
    ret = pclGetKernelInfo(kernel, param_name, param_value_size, param_value, param_value_size_ret);
    return ret;
}

cl_int WINAPI wine_clGetKernelWorkGroupInfo(cl_kernel kernel, cl_device_id device,
                                            cl_kernel_work_group_info param_name, size_t param_value_size,
                                            void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("\n");
    if (!pclGetKernelWorkGroupInfo) return CL_INVALID_VALUE;
    ret = pclGetKernelWorkGroupInfo(kernel, device, param_name, param_value_size, param_value, param_value_size_ret);
    return ret;
}


/*---------------------------------------------------------------*/
/* Event Object APIs  */

cl_int WINAPI wine_clWaitForEvents(cl_uint num_events, cl_event * event_list)
{
    cl_int ret;
    TRACE("\n");
    if (!pclWaitForEvents) return CL_INVALID_EVENT;
    ret = pclWaitForEvents(num_events, event_list);
    return ret;
}

cl_int WINAPI wine_clGetEventInfo(cl_event event, cl_event_info param_name, size_t param_value_size,
                                  void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("\n");
    if (!pclGetEventInfo) return CL_INVALID_EVENT;
    ret = pclGetEventInfo(event, param_name, param_value_size, param_value, param_value_size_ret);
    return ret;
}

cl_int WINAPI wine_clRetainEvent(cl_event event)
{
    cl_int ret;
    TRACE("\n");
    if (!pclRetainEvent) return CL_INVALID_EVENT;
    ret = pclRetainEvent(event);
    return ret;
}

cl_int WINAPI wine_clReleaseEvent(cl_event event)
{
    cl_int ret;
    TRACE("\n");
    if (!pclReleaseEvent) return CL_INVALID_EVENT;
    ret = pclReleaseEvent(event);
    return ret;
}

cl_event WINAPI wine_clCreateUserEvent(cl_context context, cl_int * errcode_ret)
{
    cl_event ret;
    TRACE("\n");
    if (!pclCreateUserEvent)
    {
        *errcode_ret = CL_INVALID_CONTEXT;
        return NULL;
    }
    ret = pclCreateUserEvent(context, errcode_ret);
    return ret;
}

typedef struct
{
    void WINAPI (*pfn_notify)(cl_event event, cl_int num, void* user_data);
    void *user_data;
} EVENT_CALLBACK;

static void event_fn_notify(cl_event event, cl_int num, void* user_data)
{
    EVENT_CALLBACK *ecb;
    FIXME("(%p, %d, %p)\n", event, num, user_data);
    ecb = (EVENT_CALLBACK *) user_data;
    ecb->pfn_notify(event, num, ecb->user_data);
    HeapFree(GetProcessHeap(), 0, ecb);
    FIXME("Callback COMPLETED\n");
}

cl_int WINAPI wine_clSetEventCallback(cl_event event, cl_int command_exec_callback_type,
                                      void WINAPI (*pfn_notify)(cl_event, cl_int, void *), void *user_data)
{
    /* FIXME: Based on PROGRAM_CALLBACK/program_fn_notify function. I'm not sure about this. */
    cl_int ret;
    FIXME("(%p, %d, %p, %p)\n", event, command_exec_callback_type, pfn_notify, user_data);
    if (!pclSetEventCallback) return CL_INVALID_EVENT;
    if(pfn_notify)
    {
        /* When pfn_notify is provided, clSetEventCallback is asynchronous */
        EVENT_CALLBACK *ecb;
        ecb = HeapAlloc(GetProcessHeap(), 0, sizeof(EVENT_CALLBACK));
        ecb->pfn_notify = pfn_notify;
        ecb->user_data = user_data;
        ret = pclSetEventCallback(event, command_exec_callback_type, event_fn_notify, user_data);
    }
    else
    {
        /* When pfn_notify is NULL, clSetEventCallback is synchronous */
        ret = pclSetEventCallback(event, command_exec_callback_type, NULL, user_data);
    }
    FIXME("(%p, %d, %p, %p)=%d\n", event, command_exec_callback_type, pfn_notify, user_data, ret);
    return ret;
}

cl_int WINAPI wine_clSetUserEventStatus(cl_event event, cl_int execution_status)
{
    cl_int ret;
    TRACE("\n");
    if (!pclSetUserEventStatus) return CL_INVALID_EVENT;
    ret = pclSetUserEventStatus(event, execution_status);
    return ret;
}


/*---------------------------------------------------------------*/
/* Profiling APIs  */

cl_int WINAPI wine_clGetEventProfilingInfo(cl_event event, cl_profiling_info param_name, size_t param_value_size,
                                           void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("\n");
    if (!pclGetEventProfilingInfo) return CL_INVALID_EVENT;
    ret = pclGetEventProfilingInfo(event, param_name, param_value_size, param_value, param_value_size_ret);
    return ret;
}


/*---------------------------------------------------------------*/
/* Flush and Finish APIs */

cl_int WINAPI wine_clFlush(cl_command_queue command_queue)
{
    cl_int ret;
    TRACE("(%p)\n", command_queue);
    if (!pclFlush) return CL_INVALID_COMMAND_QUEUE;
    ret = pclFlush(command_queue);
    TRACE("(%p)=%d\n", command_queue, ret);
    return ret;
}

cl_int WINAPI wine_clFinish(cl_command_queue command_queue)
{
    cl_int ret;
    TRACE("(%p)\n", command_queue);
    if (!pclFinish) return CL_INVALID_COMMAND_QUEUE;
    ret = pclFinish(command_queue);
    TRACE("(%p)=%d\n", command_queue, ret);
    return ret;
}


/*---------------------------------------------------------------*/
/* Enqueued Commands APIs */

cl_int WINAPI wine_clEnqueueReadBuffer(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read,
                                       size_t offset, size_t cb, void * ptr,
                                       cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    if (!pclEnqueueReadBuffer) return CL_INVALID_VALUE;
    ret = pclEnqueueReadBuffer(command_queue, buffer, blocking_read, offset, cb, ptr, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueReadBufferRect(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read,
                                           const size_t * buffer_origin, const size_t * host_origin, const size_t * region,
                                           size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch,
                                           void * ptr, cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    if (!pclEnqueueReadBufferRect) return CL_INVALID_VALUE;
    ret = pclEnqueueReadBufferRect(command_queue, buffer, blocking_read,
        buffer_origin, host_origin, region,
        buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch,
        ptr, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueWriteBuffer(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write,
                                        size_t offset, size_t cb, const void * ptr,
                                        cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    if (!pclEnqueueWriteBuffer) return CL_INVALID_VALUE;
    ret = pclEnqueueWriteBuffer(command_queue, buffer, blocking_write, offset, cb, ptr, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueWriteBufferRect( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read,
                                            const size_t * buffer_origin, const size_t * host_origin, const size_t * region,
                                            size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch,
                                            const void * ptr, cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    if (!pclEnqueueWriteBufferRect) return CL_INVALID_VALUE;
    ret = pclEnqueueWriteBufferRect(command_queue, buffer, blocking_read,
        buffer_origin, host_origin, region,
        buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch,
        ptr, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueFillBuffer(cl_command_queue command_queue, cl_mem buffer, const void * pattern, size_t pattern_size, size_t offset, size_t cb,
                                       cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    if (!pclEnqueueFillBuffer) return CL_INVALID_VALUE;
    ret = pclEnqueueFillBuffer(command_queue, buffer, pattern, pattern_size, offset, cb, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueCopyBuffer(cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer,
                                       size_t src_offset, size_t dst_offset, size_t cb,
                                       cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    if (!pclEnqueueCopyBuffer) return CL_INVALID_VALUE;
    ret = pclEnqueueCopyBuffer(command_queue, src_buffer, dst_buffer, src_offset, dst_offset, cb, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueCopyBufferRect(cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer,
                                           const size_t * src_origin, const size_t * dst_origin, const size_t * region,
                                           size_t src_row_pitch, size_t src_slice_pitch,
                                           size_t dst_row_pitch, size_t dst_slice_pitch,
                                           cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    if (!pclEnqueueCopyBufferRect) return CL_INVALID_VALUE;
    ret = pclEnqueueCopyBufferRect(command_queue, src_buffer, dst_buffer, src_origin,  dst_origin, region, src_row_pitch, src_slice_pitch, dst_row_pitch, dst_slice_pitch, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueReadImage(cl_command_queue command_queue, cl_mem image, cl_bool blocking_read,
                                      const size_t * origin, const size_t * region,
                                      SIZE_T row_pitch, SIZE_T slice_pitch, void * ptr,
                                      cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("(%p, %p, %d, %p, %p, %ld, %ld, %p, %d, %p, %p)\n", command_queue, image, blocking_read,
          origin, region, row_pitch, slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event);
    if (!pclEnqueueReadImage) return CL_INVALID_VALUE;
    ret = pclEnqueueReadImage(command_queue, image, blocking_read, origin, region, row_pitch, slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event);
    TRACE("(%p, %p, %d, %p, %p, %ld, %ld, %p, %d, %p, %p)=%d\n", command_queue, image, blocking_read,
          origin, region, row_pitch, slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event, ret);
    return ret;
}

cl_int WINAPI wine_clEnqueueWriteImage(cl_command_queue command_queue, cl_mem image, cl_bool blocking_write,
                                       const size_t * origin, const size_t * region,
                                       size_t input_row_pitch, size_t input_slice_pitch, const void * ptr,
                                       cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    if (!pclEnqueueWriteImage) return CL_INVALID_VALUE;
    ret = pclEnqueueWriteImage(command_queue, image, blocking_write, origin, region, input_row_pitch, input_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueFillImage(cl_command_queue command_queue, cl_mem image, const void * fill_color,
                                      const size_t * origin, const size_t * region,
                                      cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    if (!pclEnqueueFillImage) return CL_INVALID_VALUE;
    ret = pclEnqueueFillImage(command_queue, image, fill_color,  origin, region, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueCopyImage(cl_command_queue command_queue, cl_mem src_image, cl_mem dst_image,
                                      size_t * src_origin, size_t * dst_origin, size_t * region,
                                      cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    if (!pclEnqueueCopyImage) return CL_INVALID_VALUE;
    ret = pclEnqueueCopyImage(command_queue, src_image, dst_image, src_origin, dst_origin, region, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueCopyImageToBuffer(cl_command_queue command_queue, cl_mem src_image, cl_mem dst_buffer,
                                              size_t * src_origin, size_t * region, size_t dst_offset,
                                              cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    if (!pclEnqueueCopyImageToBuffer) return CL_INVALID_VALUE;
    ret = pclEnqueueCopyImageToBuffer(command_queue, src_image, dst_buffer, src_origin, region, dst_offset, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueCopyBufferToImage(cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_image,
                                              size_t src_offset, size_t * dst_origin, size_t * region,
                                              cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    if (!pclEnqueueCopyBufferToImage) return CL_INVALID_VALUE;
    ret = pclEnqueueCopyBufferToImage(command_queue, src_buffer, dst_image, src_offset, dst_origin, region, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

void * WINAPI wine_clEnqueueMapBuffer(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_map,
                                      cl_map_flags map_flags, size_t offset, size_t cb,
                                      cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event, cl_int * errcode_ret)
{
    void * ret;
    TRACE("\n");
    if (!pclEnqueueMapBuffer)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return NULL;
    }
    ret = pclEnqueueMapBuffer(command_queue, buffer, blocking_map, map_flags, offset, cb, num_events_in_wait_list, event_wait_list, event, errcode_ret);
    return ret;
}

void * WINAPI wine_clEnqueueMapImage(cl_command_queue command_queue, cl_mem image, cl_bool blocking_map,
                                     cl_map_flags map_flags, size_t * origin, size_t * region,
                                     size_t * image_row_pitch, size_t * image_slice_pitch,
                                     cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event, cl_int * errcode_ret)
{
    void * ret;
    TRACE("\n");
    if (!pclEnqueueMapImage)
    {
        *errcode_ret = CL_INVALID_VALUE;
        return NULL;
    }
    ret = pclEnqueueMapImage(command_queue, image, blocking_map, map_flags, origin, region, image_row_pitch, image_slice_pitch, num_events_in_wait_list, event_wait_list, event, errcode_ret);
    return ret;
}

cl_int WINAPI wine_clEnqueueUnmapMemObject(cl_command_queue command_queue, cl_mem memobj, void * mapped_ptr,
                                           cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    if (!pclEnqueueUnmapMemObject) return CL_INVALID_VALUE;
    ret = pclEnqueueUnmapMemObject(command_queue, memobj, mapped_ptr, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueMigrateMemObjects(cl_command_queue command_queue, cl_uint num_mem_objects, const cl_mem * mem_objects, cl_mem_migration_flags flags,
                                              cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    if (!pclEnqueueMigrateMemObjects) return CL_INVALID_VALUE;
    ret = pclEnqueueMigrateMemObjects(command_queue, num_mem_objects, mem_objects, flags, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueNDRangeKernel(cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim,
                                          size_t * global_work_offset, size_t * global_work_size, size_t * local_work_size,
                                          cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    if (!pclEnqueueNDRangeKernel) return CL_INVALID_VALUE;
    ret = pclEnqueueNDRangeKernel(command_queue, kernel, work_dim, global_work_offset, global_work_size, local_work_size, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueTask(cl_command_queue command_queue, cl_kernel kernel,
                                 cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    if (!pclEnqueueTask) return CL_INVALID_VALUE;
    ret = pclEnqueueTask(command_queue, kernel, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueNativeKernel(cl_command_queue command_queue,
                                         void WINAPI (*user_func)(void *args),
                                         void * args, size_t cb_args,
                                         cl_uint num_mem_objects, const cl_mem * mem_list, const void ** args_mem_loc,
                                         cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)
{
    cl_int ret = CL_INVALID_OPERATION;
    /* FIXME: There appears to be no obvious method for translating the ABI for user_func.
     * There is no opaque user_data structure passed, that could encapsulate the return address.
     * The OpenCL specification seems to indicate that args has an implementation specific
     * structure that cannot be used to stash away a return address for the WINAPI user_func.
     */
#if 0
    ret = clEnqueueNativeKernel(command_queue, user_func, args, cb_args, num_mem_objects, mem_list, args_mem_loc,
                                 num_events_in_wait_list, event_wait_list, event);
#else
    FIXME("not supported due to user_func ABI mismatch\n");
#endif
    return ret;
}

cl_int WINAPI wine_clEnqueueMarker(cl_command_queue command_queue, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    if (!pclEnqueueMarker) return CL_INVALID_VALUE;
    ret = pclEnqueueMarker(command_queue, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueMarkerWithWaitList(cl_command_queue command_queue, cl_uint num_events_in_wait_list,
                                               const cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    if (!pclEnqueueMarkerWithWaitList) return CL_INVALID_COMMAND_QUEUE;
    ret = pclEnqueueMarkerWithWaitList(command_queue, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueWaitForEvents(cl_command_queue command_queue, cl_uint num_events, cl_event * event_list)
{
    cl_int ret;
    TRACE("\n");
    if (!pclEnqueueWaitForEvents) return CL_INVALID_VALUE;
    ret = pclEnqueueWaitForEvents(command_queue, num_events, event_list);
    return ret;
}

cl_int WINAPI wine_clEnqueueBarrier(cl_command_queue command_queue)
{
    cl_int ret;
    TRACE("\n");
    if (!pclEnqueueBarrier) return CL_INVALID_VALUE;
    ret = pclEnqueueBarrier(command_queue);
    return ret;
}

cl_int WINAPI wine_clEnqueueBarrierWithWaitList(cl_command_queue command_queue, cl_uint num_events_in_wait_list,
                                                const cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    if (!pclEnqueueBarrierWithWaitList) return CL_INVALID_COMMAND_QUEUE;
    ret = pclEnqueueBarrierWithWaitList(command_queue, num_events_in_wait_list, event_wait_list, event);
    return ret;
}


/*---------------------------------------------------------------*/
/* Extension function access */

void * WINAPI wine_clGetExtensionFunctionAddressForPlatform(cl_platform_id platform, const char * function_name)
{
    void * ret = NULL;
    TRACE("(%p, %s)\n", platform, function_name);
#if 0
    if (!pclGetExtensionFunctionAddressForPlatform) return NULL;
    ret = pclGetExtensionFunctionAddressForPlatform(platform, function_name);
#else
    FIXME("(%p, %s), extensions support is not implemented\n", platform, function_name);
#endif
    TRACE("(%p, %s)=%p\n", platform, function_name, ret);
    return ret;
}

void * WINAPI wine_clGetExtensionFunctionAddress(const char * func_name)
{
    void * ret = 0;
    TRACE("(%s)\n",func_name);
#if 0
    ret = clGetExtensionFunctionAddress(func_name);
#else
    FIXME("extensions not implemented\n");
#endif
    TRACE("(%s)=%p\n",func_name, ret);
    return ret;
}


#if OPENCL_WITH_GL
/*---------------------------------------------------------------*/
/* Khronos-approved (KHR) OpenCL extensions which have OpenGL dependencies. */

cl_mem WINAPI wine_clCreateFromGLBuffer(cl_context context, cl_mem_flags flags, cl_GLuint bufobj, int * errcode_ret)
{
}

cl_mem WINAPI wine_clCreateFromGLTexture2D(cl_context context, cl_mem_flags flags, cl_GLenum target,
                                           cl_GLint miplevel, cl_GLuint texture, cl_int * errcode_ret)
{
}

cl_mem WINAPI wine_clCreateFromGLTexture3D(cl_context context, cl_mem_flags flags, cl_GLenum target,
                                           cl_GLint miplevel, cl_GLuint texture, cl_int * errcode_ret)
{
}

cl_mem WINAPI wine_clCreateFromGLRenderbuffer(cl_context context, cl_mem_flags flags, cl_GLuint renderbuffer, cl_int * errcode_ret)
{
}

cl_int WINAPI wine_clGetGLObjectInfo(cl_mem memobj, cl_gl_object_type * gl_object_type, cl_GLuint * gl_object_name)
{
}

cl_int WINAPI wine_clGetGLTextureInfo(cl_mem memobj, cl_gl_texture_info param_name, size_t param_value_size,
                                      void * param_value, size_t * param_value_size_ret)
{
}

cl_int WINAPI wine_clEnqueueAcquireGLObjects(cl_command_queue command_queue, cl_uint num_objects, const cl_mem * mem_objects,
                                             cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)
{
}

cl_int WINAPI wine_clEnqueueReleaseGLObjects(cl_command_queue command_queue, cl_uint num_objects, const cl_mem * mem_objects,
                                             cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)
{
}


/*---------------------------------------------------------------*/
/* cl_khr_gl_sharing extension  */

cl_int WINAPI wine_clGetGLContextInfoKHR(const cl_context_properties * properties, cl_gl_context_info param_name,
                                         size_t param_value_size, void * param_value, size_t * param_value_size_ret)
{
}

#endif


#if 0
/*---------------------------------------------------------------*/
/* cl_khr_icd extension */

cl_int WINAPI wine_clIcdGetPlatformIDsKHR(cl_uint num_entries, cl_platform_id * platforms, cl_uint * num_platforms)
{
}
#endif

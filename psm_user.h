/*

  This file is provided under a dual BSD/GPLv2 license.  When using or
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2016 Intel Corporation.

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  Contact Information:
  Intel Corporation, www.intel.com

  BSD LICENSE

  Copyright(c) 2016 Intel Corporation.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/* Copyright (c) 2003-2016 Intel Corporation. All rights reserved. */

#ifndef _PSMI_USER_H
#define _PSMI_USER_H

#include <inttypes.h>
#include <pthread.h>

#include <sched.h>
#include <numa.h>

#include "psm2.h"
#include "psm2_mq.h"

#include "ptl.h"

#include "opa_user.h"
#include "opa_queue.h"

#ifdef PSM_VALGRIND
#include <valgrind/valgrind.h>
#include <valgrind/memcheck.h>
#endif

#include "psm_log.h"

#ifdef PSM_VALGRIND
#define PSM_VALGRIND_REDZONE_SZ	     8
#define PSM_VALGRIND_DEFINE_MQ_RECV(buf, posted_len, recv_len)	do {	\
	    VALGRIND_MAKE_MEM_DEFINED((void *)(buf), (posted_len));	\
	    if ((recv_len) < (posted_len))				\
		VALGRIND_MAKE_MEM_UNDEFINED(				\
		(void *) ((uintptr_t) (buf) + (recv_len)),		\
		(posted_len) - (recv_len));				\
	    } while (0)

#else
#define PSM_VALGRIND_REDZONE_SZ	     0
#define PSM_VALGRIND_DEFINE_MQ_RECV(buf, posted_len, recv_len)
#define VALGRIND_CREATE_MEMPOOL(ARG1,ARG2,ARG3)
#define VALGRIND_MAKE_MEM_DEFINED(ARG1,ARG2)
#define VALGRIND_DESTROY_MEMPOOL(ARG1)
#define VALGRIND_MEMPOOL_ALLOC(ARG1,ARG2,ARG3)
#define VALGRIND_MEMPOOL_FREE(ARG1,ARG2)
#define VALGRIND_MAKE_MEM_NOACCESS(ARG1,ARG2)
#endif

/* Parameters for use in valgrind's "is_zeroed" */
#define PSM_VALGRIND_MEM_DEFINED     1
#define PSM_VALGRIND_MEM_UNDEFINED   0

#define PSMI_LOCK_NO_OWNER	((pthread_t)(-1))

#ifdef PSM_DEBUG
#define PSMI_LOCK_IS_MUTEXLOCK_DEBUG
#else
#define PSMI_LOCK_IS_SPINLOCK
/* #define PSMI_LOCK_IS_MUTEXLOCK */
/* #define PSMI_LOCK_IS_MUTEXLOCK_DEBUG */
/* #define PSMI_PLOCK_IS_NOLOCK */
#endif

#define _PSMI_IN_USER_H
#include "psm_help.h"
#include "psm_error.h"
#include "psm_context.h"
#include "psm_utils.h"
#include "psm_timer.h"
#include "psm_mpool.h"
#include "psm_ep.h"
#include "psm_lock.h"
#include "psm_stats.h"
#include "psm2_mock_testing.h"
#undef _PSMI_IN_USER_H

#define PSMI_VERNO_MAKE(major, minor) ((((major)&0xff)<<8)|((minor)&0xff))
#define PSMI_VERNO  PSMI_VERNO_MAKE(PSM2_VERNO_MAJOR, PSM2_VERNO_MINOR)
#define PSMI_VERNO_GET_MAJOR(verno) (((verno)>>8) & 0xff)
#define PSMI_VERNO_GET_MINOR(verno) (((verno)>>0) & 0xff)

int psmi_verno_client();
int psmi_verno_isinteroperable(uint16_t verno);
int MOCKABLE(psmi_isinitialized)();
MOCK_DCL_EPILOGUE(psmi_isinitialized);

psm2_error_t psmi_poll_internal(psm2_ep_t ep, int poll_amsh);
psm2_error_t psmi_mq_wait_internal(psm2_mq_req_t *ireq);

int psmi_get_current_proc_location();

extern uint32_t non_dw_mul_sdma;
extern psmi_lock_t psmi_creation_lock;

extern psm2_ep_t psmi_opened_endpoint;

/*
 * Default setting for Receive thread
 *
 *   0 disables rcvthread by default
 * 0x1 enables ips receive thread by default
 */
#define PSMI_RCVTHREAD_FLAGS	0x1

/*
 * Define one of these below.
 *
 * Spinlock gives the best performance and makes sense with the progress thread
 * only because the progress thread does a "trylock" and then goes back to
 * sleep in a poll.
 *
 * Mutexlock should be used for experimentation while the more useful
 * mutexlock-debug should be enabled during development to catch potential
 * errors.
 */
#ifdef PSMI_LOCK_IS_SPINLOCK
#define _PSMI_LOCK_INIT(pl)	psmi_spin_init(&((pl).lock))
#define _PSMI_LOCK_TRY(pl)	psmi_spin_trylock(&((pl).lock))
#define _PSMI_LOCK(pl)		psmi_spin_lock(&((pl).lock))
#define _PSMI_UNLOCK(pl)	psmi_spin_unlock(&((pl).lock))
#define _PSMI_LOCK_ASSERT(pl)
#define _PSMI_UNLOCK_ASSERT(pl)
#define PSMI_LOCK_DISABLED	0

#elif defined(PSMI_LOCK_IS_MUTEXLOCK_DEBUG)

PSMI_ALWAYS_INLINE(
int
_psmi_mutex_trylock_inner(pthread_mutex_t *mutex,
			  const char *curloc, pthread_t *lock_owner))
{
	psmi_assert_always_loc(*lock_owner != pthread_self(),
			       curloc);
	int ret = pthread_mutex_trylock(mutex);
	if (ret == 0)
		*lock_owner = pthread_self();
	return ret;
}

PSMI_ALWAYS_INLINE(
int
_psmi_mutex_lock_inner(pthread_mutex_t *mutex,
		       const char *curloc, pthread_t *lock_owner))
{
	psmi_assert_always_loc(*lock_owner != pthread_self(),
			       curloc);
	int ret = pthread_mutex_lock(mutex);
	psmi_assert_always_loc(ret != EDEADLK, curloc);
	*lock_owner = pthread_self();
	return ret;
}

PSMI_ALWAYS_INLINE(
void
_psmi_mutex_unlock_inner(pthread_mutex_t *mutex,
			 const char *curloc, pthread_t *lock_owner))
{
	psmi_assert_always_loc(*lock_owner == pthread_self(),
			       curloc);
	*lock_owner = PSMI_LOCK_NO_OWNER;
	psmi_assert_always_loc(pthread_mutex_unlock(mutex) !=
			       EPERM, curloc);
	return;
}

#define _PSMI_LOCK_INIT(pl)	/* static initialization */
#define _PSMI_LOCK_TRY(pl)							\
	    _psmi_mutex_trylock_inner(&((pl).lock), PSMI_CURLOC,	\
					&((pl).lock_owner))
#define _PSMI_LOCK(pl)								\
	    _psmi_mutex_lock_inner(&((pl).lock), PSMI_CURLOC,	\
                                        &((pl).lock_owner))
#define _PSMI_UNLOCK(pl)							\
	    _psmi_mutex_unlock_inner(&((pl).lock), PSMI_CURLOC,	\
                                        &((pl).lock_owner))
#define _PSMI_LOCK_ASSERT(pl)							\
	    psmi_assert_always(pl.lock_owner == pthread_self());
#define _PSMI_UNLOCK_ASSERT(pl)						\
	    psmi_assert_always(pl.lock_owner != pthread_self());
#define PSMI_LOCK_DISABLED	0

#elif defined(PSMI_LOCK_IS_MUTEXLOCK)
#define _PSMI_LOCK_INIT(pl)	/* static initialization */
#define _PSMI_LOCK_TRY(pl)	pthread_mutex_trylock(&((pl).lock))
#define _PSMI_LOCK(pl)		pthread_mutex_lock(&((pl).lock))
#define _PSMI_UNLOCK(pl)	pthread_mutex_unlock(&((pl).lock))
#define PSMI_LOCK_DISABLED	0
#define _PSMI_LOCK_ASSERT(pl)
#define _PSMI_UNLOCK_ASSERT(pl)

#elif defined(PSMI_PLOCK_IS_NOLOCK)
#define _PSMI_LOCK_TRY(pl)	0	/* 0 *only* so progress thread never succeeds */
#define _PSMI_LOCK(pl)
#define _PSMI_UNLOCK(pl)
#define PSMI_LOCK_DISABLED	1
#define _PSMI_LOCK_ASSERT(pl)
#define _PSMI_UNLOCK_ASSERT(pl)
#else
#error No LOCK lock type declared
#endif

#define PSMI_YIELD(pl)							\
	do { _PSMI_UNLOCK((pl)); sched_yield(); _PSMI_LOCK((pl)); } while (0)

#ifdef PSM2_MOCK_TESTING
/* If this is a mocking tests build, all the operations on the locks
 * are routed through functions which may be mocked, if necessary.  */
void MOCKABLE(psmi_mockable_lock_init)(psmi_lock_t *pl);
MOCK_DCL_EPILOGUE(psmi_mockable_lock_init);

int MOCKABLE(psmi_mockable_lock_try)(psmi_lock_t *pl);
MOCK_DCL_EPILOGUE(psmi_mockable_lock_try);

void MOCKABLE(psmi_mockable_lock)(psmi_lock_t *pl);
MOCK_DCL_EPILOGUE(psmi_mockable_lock);

void MOCKABLE(psmi_mockable_unlock)(psmi_lock_t *pl);
MOCK_DCL_EPILOGUE(psmi_mockable_unlock);

void MOCKABLE(psmi_mockable_lock_assert)(psmi_lock_t *pl);
MOCK_DCL_EPILOGUE(psmi_mockable_lock_assert);

void MOCKABLE(psmi_mockable_unlock_assert)(psmi_lock_t *pl);
MOCK_DCL_EPILOGUE(psmi_mockable_unlock_assert);

#define PSMI_LOCK_INIT(pl)	psmi_mockable_lock_init(&(pl))
#define PSMI_LOCK_TRY(pl)	psmi_mockable_lock_try(&(pl))
#define PSMI_LOCK(pl)		psmi_mockable_lock(&(pl))
#define PSMI_UNLOCK(pl)		psmi_mockable_unlock(&(pl))
#define PSMI_LOCK_ASSERT(pl)	psmi_mockable_lock_assert(&(pl))
#define PSMI_UNLOCK_ASSERT(pl)	psmi_mockable_unlock_assert(&(pl))
#else
#define PSMI_LOCK_INIT(pl)	_PSMI_LOCK_INIT(pl)
#define PSMI_LOCK_TRY(pl)	_PSMI_LOCK_TRY(pl)
#define PSMI_LOCK(pl)		_PSMI_LOCK(pl)
#define PSMI_UNLOCK(pl)		_PSMI_UNLOCK(pl)
#define PSMI_LOCK_ASSERT(pl)	_PSMI_LOCK_ASSERT(pl)
#define PSMI_UNLOCK_ASSERT(pl)	_PSMI_UNLOCK_ASSERT(pl)
#endif

#ifdef PSM_PROFILE
void psmi_profile_block() __attribute__ ((weak));
void psmi_profile_unblock() __attribute__ ((weak));
void psmi_profile_reblock(int did_no_progress) __attribute__ ((weak));

#define PSMI_PROFILE_BLOCK()		psmi_profile_block()
#define PSMI_PROFILE_UNBLOCK()		psmi_profile_unblock()
#define PSMI_PROFILE_REBLOCK(noprog)	psmi_profile_reblock(noprog)
#else
#define PSMI_PROFILE_BLOCK()
#define PSMI_PROFILE_UNBLOCK()
#define PSMI_PROFILE_REBLOCK(noprog)
#endif

#ifdef PSM_CUDA
#include <cuda.h>
#include <cuda_runtime.h>
#include <driver_types.h>

#if CUDART_VERSION < 4010
#error Please update CUDA runtime, required minimum version is 4.1
#endif

extern int is_cuda_enabled;
extern int device_support_gpudirect;
extern int cuda_runtime_version;

extern CUcontext ctxt;
void *psmi_cudart_lib;
void *psmi_cuda_lib;
CUresult (*psmi_cuCtxGetCurrent)(CUcontext *c);
CUresult (*psmi_cuCtxSetCurrent)(CUcontext c);
CUresult (*psmi_cuPointerGetAttribute)(void *data, CUpointer_attribute pa, CUdeviceptr p);
CUresult (*psmi_cuPointerSetAttribute)(void *data, CUpointer_attribute pa, CUdeviceptr p);
cudaError_t (*psmi_cudaRuntimeGetVersion)(int *runtime_version);
cudaError_t (*psmi_cudaGetDeviceCount)(int *n);
cudaError_t (*psmi_cudaGetDeviceProperties)(struct cudaDeviceProp *p, int d);
cudaError_t (*psmi_cudaGetDevice)(int *n);
cudaError_t (*psmi_cudaSetDevice)(int n);
cudaError_t (*psmi_cudaStreamCreate)(cudaStream_t *s);
cudaError_t (*psmi_cudaStreamCreateWithFlags)(cudaStream_t *s, unsigned f);
cudaError_t (*psmi_cudaStreamSynchronize)(cudaStream_t s);
cudaError_t (*psmi_cudaDeviceSynchronize)();
cudaError_t (*psmi_cudaEventCreate)(cudaEvent_t *event);
cudaError_t (*psmi_cudaEventDestroy)(cudaEvent_t event);
cudaError_t (*psmi_cudaEventQuery)(cudaEvent_t event);
cudaError_t (*psmi_cudaEventRecord)(cudaEvent_t event, cudaStream_t stream);
cudaError_t (*psmi_cudaEventSynchronize)(cudaEvent_t event);
cudaError_t (*psmi_cudaMemcpy)(void *dst, const void *src, size_t count, enum cudaMemcpyKind kind);
cudaError_t (*psmi_cudaMemcpyAsync)(void *dst, const void *src, size_t count, enum cudaMemcpyKind kind, cudaStream_t s);
cudaError_t (*psmi_cudaMalloc)(void **devPtr, size_t size);
cudaError_t (*psmi_cudaHostAlloc)(void **devPtr, size_t size, unsigned int flags);
cudaError_t (*psmi_cudaFreeHost)(void *ptr);

cudaError_t (*psmi_cudaIpcGetMemHandle)(cudaIpcMemHandle_t* handle, void* devPtr);
cudaError_t (*psmi_cudaIpcOpenMemHandle)(void** devPtr, cudaIpcMemHandle_t handle, unsigned int  flags);
cudaError_t (*psmi_cudaIpcCloseMemHandle)(void* devPtr);

#define PSMI_CUDA_DRIVER_API_CALL(func, args...) do {			\
		CUresult cudaerr;					\
		cudaerr = psmi_##func(args);				\
		if (cudaerr != CUDA_SUCCESS) {				\
			if (ctxt == NULL)				\
				_HFI_ERROR(				\
				"Check if cuda runtime is initialized"	\
				"before psm2_ep_open call \n");	\
			_HFI_ERROR(					\
				"CUDA failure: %s() (at %s:%d)"	\
				"returned %d\n",			\
				#func, __FILE__, __LINE__, cudaerr);	\
			psmi_handle_error(				\
				PSMI_EP_NORETURN, PSM2_INTERNAL_ERR,	\
				"Error returned from CUDA function.\n");\
		}							\
	} while (0)

#define PSMI_CUDA_CALL(func, args...) do {				\
		cudaError_t cudaerr;					\
		cudaerr = psmi_##func(args);				\
		if (cudaerr != cudaSuccess) {				\
			if (ctxt == NULL)				\
				_HFI_ERROR(				\
				"Check if cuda runtime is initialized"	\
				"before psm2_ep_open call \n");	\
			_HFI_ERROR(					\
				"CUDA failure: %s() (at %s:%d)"	\
				"returned %d\n",			\
				#func, __FILE__, __LINE__, cudaerr);    \
			psmi_handle_error(				\
				PSMI_EP_NORETURN, PSM2_INTERNAL_ERR,	\
				"Error returned from CUDA function.\n");\
		}							\
	} while (0)

#define PSMI_CUDA_CHECK_EVENT(event, cudaerr) do {			\
		cudaerr = psmi_cudaEventQuery(event);			\
		if ((cudaerr != cudaSuccess) &&			\
		    (cudaerr != cudaErrorNotReady)) {			\
			_HFI_ERROR(					\
				"CUDA failure: %s() returned %d\n",	\
				"cudaEventQuery", cudaerr);		\
			psmi_handle_error(				\
				PSMI_EP_NORETURN, PSM2_INTERNAL_ERR,	\
				"Error returned from CUDA function.\n");\
		}							\
	} while (0)



PSMI_ALWAYS_INLINE(
int
_psmi_is_cuda_mem(void *ptr))
{
	CUresult cres;
	CUmemorytype mt;
	cres = psmi_cuPointerGetAttribute(
		&mt, CU_POINTER_ATTRIBUTE_MEMORY_TYPE, (CUdeviceptr) ptr);
	if ((cres == CUDA_SUCCESS) && (mt == CU_MEMORYTYPE_DEVICE))
		return 1;
	else
		return 0;
}

PSMI_ALWAYS_INLINE(
int
_psmi_is_cuda_enabled())
{
	return is_cuda_enabled;
}

#define PSMI_IS_CUDA_ENABLED _psmi_is_cuda_enabled()

#define PSMI_IS_CUDA_MEM(p) _psmi_is_cuda_mem(p)
/* XXX TODO: Getting the gpu page size from driver at init time */
#define PSMI_GPU_PAGESIZE 65536

struct ips_cuda_hostbuf {
	STAILQ_ENTRY(ips_cuda_hostbuf) req_next;
	STAILQ_ENTRY(ips_cuda_hostbuf) next;
	uint32_t size, offset, bytes_read;
	/* This flag indicates whether a chb is
	 * pulled from a mpool or dynamically
	 * allocated using calloc. */
	uint8_t is_tempbuf;
	cudaEvent_t copy_status;
	psm2_mq_req_t req;
	void *host_buf, *gpu_buf;
};

struct ips_cuda_hostbuf_mpool_cb_context {
	unsigned bufsz;
};
void psmi_cuda_hostbuf_alloc_func(int is_alloc, void *context, void *obj);

#define CUDA_HOSTBUFFER_LIMITS {				\
	    .env = "PSM_CUDA_BOUNCEBUFFERS_MAX",		\
	    .descr = "Max CUDA bounce buffers (in MB)",		\
	    .env_level = PSMI_ENVVAR_LEVEL_HIDDEN,		\
	    .minval = 1,					\
	    .maxval = 1<<30,					\
	    .mode[PSMI_MEMMODE_NORMAL]  = {  16, 256 },		\
	    .mode[PSMI_MEMMODE_MINIMAL] = {   1,   1 },		\
	    .mode[PSMI_MEMMODE_LARGE]   = {  32, 512 }		\
	}

#define CUDA_SMALLHOSTBUF_SZ	(256*1024)
#define CUDA_WINDOW_PREFETCH_DEFAULT	2
#define GPUDIRECT_THRESH_RV 3

extern uint32_t gpudirect_send_threshold;
extern uint32_t gpudirect_recv_threshold;

enum psm2_chb_match_type {
	/* Complete data found in a single chb */
	PSMI_CUDA_FULL_MATCH_FOUND = 0,
	/* Data is spread across two chb's */
	PSMI_CUDA_SPLIT_MATCH_FOUND = 1,
	/* Data is only partially prefetched */
	PSMI_CUDA_PARTIAL_MATCH_FOUND = 2,
	PSMI_CUDA_CONTINUE = 3
};
typedef enum psm2_chb_match_type psm2_chb_match_type_t;

#endif /* PSM_CUDA */
#endif /* _PSMI_USER_H */

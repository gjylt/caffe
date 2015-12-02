#include <algorithm>
#include <vector>
#include "caffe/common.hpp"
#include "caffe/util/gpu_memory.hpp"


#ifndef CPU_ONLY
#include "cub/cub/util_allocator.cuh"
#endif

namespace caffe {

#ifndef CPU_ONLY  // CPU-only Caffe.
  static cub::CachingDeviceAllocator* cubAlloc = 0;
#endif

  gpu_memory::PoolMode gpu_memory::mode_   = gpu_memory::NoPool;
  // default is to cache everything, no limit on pool size
  size_t               gpu_memory::poolsize_ = size_t(-1);
  bool                 gpu_memory::debug_ = false;

#ifdef CPU_ONLY  // CPU-only Caffe.
  void gpu_memory::init(const std::vector<int>&, PoolMode, bool) {}
  void gpu_memory::destroy() {}

  const char* gpu_memory::getPoolName()  {
    return "No GPU: CPU Only Memory";
  }
#else
  void gpu_memory::init(const std::vector<int>& gpus,
                        PoolMode m, bool debug) {
    debug_ = debug;
    if (gpus.size() <= 0) {
      // should we report an error here ?
      m = gpu_memory::NoPool;
    }

    switch (m) {
    case CubPool:
      initMEM(gpus, m);
      break;
    default:
      break;
    }
    if (debug)
      std::cout << "gpu_memory initialized with "
                << getPoolName() << ". Poolsize = "
                << (1.0*poolsize_)/(1024.0*1024.0*1024.0) << " G." << std::endl;
  }

  void gpu_memory::destroy() {
    switch (mode_) {
    case CubPool:
      delete cubAlloc;
      cubAlloc = NULL;
      break;
    default:
      break;
    }
    mode_ = NoPool;
  }


  void gpu_memory::allocate(void **ptr, size_t size, cudaStream_t stream) {
    CHECK((ptr) != NULL);
    switch (mode_) {
    case CubPool:
      CUDA_CHECK(cubAlloc->DeviceAllocate(ptr, size, stream));
      break;
    default:
      CUDA_CHECK(cudaMalloc(ptr, size));
      break;
    }
  }

  void gpu_memory::deallocate(void *ptr, cudaStream_t stream) {
    // allow for null pointer deallocation
    if (!ptr)
      return;
    switch (mode_) {
    case CubPool:
      CUDA_CHECK(cubAlloc->DeviceFree(ptr));
      break;
    default:
      CUDA_CHECK(cudaFree(ptr));
      break;
    }
  }

  void gpu_memory::initMEM(const std::vector<int>& gpus, PoolMode m) {
    mode_ = m;
    int initial_device;

    switch ( mode_ ) {
      case CubPool:
        try {
          delete cubAlloc;
          cubAlloc = new cub::CachingDeviceAllocator( 2,
                                                      6,
                                                      16,
                                                      poolsize_,
                                                      false,
                                                      debug_);
        }
        catch (...) {}
        CHECK(cubAlloc);
        break;
      default:
        break;
      }
  }

  const char* gpu_memory::getPoolName()  {
    switch (mode_) {
    case CubPool:
      return "CUB Pool";
    default:
      return "No Pool : Plain CUDA Allocator";
    }
  }

  void gpu_memory::getInfo(size_t *free_mem, size_t *total_mem) {
    CUDA_CHECK(cudaMemGetInfo(free_mem, total_mem));
    switch (mode_) {
    case CubPool:
      int cur_device;
      CUDA_CHECK(cudaGetDevice(&cur_device));
      // Free memory is.
      *free_mem += cubAlloc->cached_bytes[cur_device].free;
      break;
    default:
      break;
    }
  }
#endif  // CPU_ONLY

}  // namespace caffe

///////////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite target app to test memory allocation intercept functionality
//
// This app allocates memory using a variety of methods in each transaction
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/framework/Framework.H>
#include <xpedite/framework/Probes.H>
#include <stdexcept>
#include <cstdlib>
#include <sys/mman.h>
#include <unistd.h>

int txnCount {100};

int main() {
  if(!xpedite::framework::initialize("xpedite-appinfo.txt", true)) { 
    throw std::runtime_error {"failed to init xpedite"}; 
  }

  using Type = int;
  using Pointer = int*;
  constexpr int ALIGNMENT = 2048;

  Pointer ptr;
  for(int i=0; i<txnCount; ++i) {
    XPEDITE_PROBE_SCOPE(Allocation);

    ptr = new Type {};
    delete ptr;

    ptr = new Type[4] {};
    delete[] ptr;

    if((ptr = static_cast<Pointer>(malloc(sizeof(Type))))) {
      free(ptr);
    }

    if((ptr = static_cast<Pointer>(calloc(1, sizeof(Type))))) {
      if((ptr = static_cast<Pointer>(realloc(ptr, 2*sizeof(Type))))) {
        free(ptr);
      }
    }

    if(!posix_memalign(reinterpret_cast<void**>(&ptr), ALIGNMENT, sizeof(Type))) {
      free(ptr);
    }

    auto size = getpagesize();
    if((ptr = static_cast<Pointer>(mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0))) != MAP_FAILED) {
      munmap(ptr, size);
    }
  }
  return 0;
}

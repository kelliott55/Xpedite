///////////////////////////////////////////////////////////////////////////////
//
// Logic to load and search process address space
//
// Segment - represents a block of memory mapped by the target processes
//
// AddressSpace - List of segments in a processes address space
//
// Provides logic to locate code segments containing probes.
// The page protections are updated during probe activation/deactivation.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <sys/mman.h>
#include <cstdint>
#include <string>
#include <vector>
#include <xpedite/util/Errno.H>
#include <xpedite/util/Allocator.H>
#include <xpedite/log/Log.H>

namespace xpedite { namespace util {

  class AddressSpace
  {
    public:

    struct Segment
    {
      using Pointer = unsigned char*; 

      using ConstPointer = const unsigned char*; 

      Segment() = default;

      Segment(Pointer begin_, Pointer end_, bool canRead_, bool canWrite_, bool canExec_)
        : _begin {begin_}, _end {end_},
          _protections {protections(canRead_, canWrite_, canExec_)} {
      }

      Pointer begin() const noexcept { return _begin; }
      Pointer end()   const noexcept { return _end;   }

      bool canRead()  const noexcept { return _protections & PROT_READ;  }
      bool canWrite() const noexcept { return _protections & PROT_WRITE; }
      bool canExec()  const noexcept { return _protections & PROT_EXEC;  }

      explicit operator bool() const noexcept {
        return _begin;
      }

      void logError() {
        util::Errno e;
        XpediteLogCritical << "Xpedite fatal error: failed to change protections for " << toString()
          << " - " << e.asString() << XpediteLogEnd;
      }

      bool makeWritable() {
        if(mprotect(begin(), end()-begin(), PROT_READ | PROT_WRITE | PROT_EXEC)) {
          logError();
          return {};
        }
        return true;
      }

      bool restoreProtections() {
        if(mprotect(begin(), end()-begin(), _protections) != 0) {
          logError();
          return {};
        }
        return true;
      }

      void toString(std::ostringstream&) const;
      std::string toString() const;

      private:

      static constexpr int protectionMask(bool v_) noexcept {
        return v_ ? -1 : 0;
      }

      static constexpr int protections(bool canRead_, bool canWrite_, bool canExec_) noexcept {
        return (protectionMask(canRead_) & PROT_READ) | 
          (protectionMask(canWrite_) & PROT_WRITE)    | 
          (protectionMask(canExec_) & PROT_EXEC);
      }

      Pointer _begin;
      Pointer _end;
      int _protections;
    };

    using Segments = std::vector<Segment>;

    static AddressSpace& get() {
      if(!_instance) {
        _instance = new AddressSpace {};
      }
      return *_instance;
    }

    static Segments load();

    AddressSpace();

    Segment* find(Segment::ConstPointer addr_) noexcept;
    const Segment* find(Segment::ConstPointer addr_) const noexcept;

    std::string toString() const noexcept;

    private:

    mutable Segments _segments;
    static AddressSpace* _instance;
  };

  inline bool operator<(AddressSpace::Segment& lhs_, AddressSpace::Segment& rhs_) {
    return lhs_.begin() < rhs_.begin();
  }

  inline AddressSpace& addressSpace() {
    return AddressSpace::get();
  }
}}

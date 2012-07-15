#pragma once

#include "cache_hash_fn.h"

namespace HybridProtocol_PPMOSI_SS
{

class L2CacheHashFn : public CacheHashFn
{
public:
   L2CacheHashFn(UInt32 cache_size, UInt32 associativity, UInt32 cache_line_size);
   ~L2CacheHashFn();

   UInt32 compute(IntPtr address);

private:
   UInt32 _log_num_sets;
};

}

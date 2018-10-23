
#pragma once

#include <cstddef>
#include "Common/Type.hpp"

const int64_t  c_length_1P 	= 0x4000000000000LL;
const int64_t  c_length_1E 	= 0x10000000000LL;
const int64_t  c_length_64G = 0x1000000000LL;
const int64_t  c_length_16G = 0x400000000LL;
const int64_t  c_length_4G 	= 0x100000000LL;
const int64_t  c_length_1G 	= 0x40000000LL;
const int32_t  c_length_64M = 0x4000000;
const int32_t  c_length_16M = 0x1000000;
const int32_t  c_length_4M 	= 0x400000;
const int32_t  c_length_1M 	= 0x100000;
const int32_t  c_length_64K = 0x10000;
const int32_t  c_length_16K = 0x04000;
const int32_t  c_length_4K 	= 0x01000;
const int32_t  c_length_1K 	= 0x00400;
const int32_t  c_int32_max  = INT32_MAX;
const int64_t  c_int64_max  = INT64_MAX;
const int32_t  c_int32_min  = INT32_MIN;
const int64_t  c_int64_min  = INT64_MIN;
const uint32_t c_uint32_max = UINT32_MAX;
const uint64_t c_uint64_max = UINT64_MAX;

extern const void* c_invalid_ptr;

/** system word size */
const int c_word_size = sizeof(void*);
/** default align length */
const int c_length_align = c_word_size;
/** default page size */
const int c_page_size = c_length_4K;
/** invalid handle */
const int c_invalid_handle = -1;



/*######   Copyright (c) 2017 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com      ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include "byte-permutation.h"
namespace Ext {

BytePermutation<128> Transposition128bytesBy8() {
	return BytePermutation<128>((const uint8_t*)
		"\x00\x08\x10\x18\x20\x28\x30\x38\x40\x48\x50\x58\x60\x68\x70\x78"
		"\x01\x09\x11\x19\x21\x29\x31\x39\x41\x49\x51\x59\x61\x69\x71\x79"
		"\x02\x0A\x12\x1A\x22\x2A\x32\x3A\x42\x4A\x52\x5A\x62\x6A\x72\x7A"
		"\x03\x0B\x13\x1B\x23\x2B\x33\x3B\x43\x4B\x53\x5B\x63\x6B\x73\x7B"
		"\x04\x0C\x14\x1C\x24\x2C\x34\x3C\x44\x4C\x54\x5C\x64\x6C\x74\x7C"
		"\x05\x0D\x15\x1D\x25\x2D\x35\x3D\x45\x4D\x55\x5D\x65\x6D\x75\x7D"
		"\x06\x0E\x16\x1E\x26\x2E\x36\x3E\x46\x4E\x56\x5E\x66\x6E\x76\x7E"
		"\x07\x0F\x17\x1F\x27\x2F\x37\x3F\x47\x4F\x57\x5F\x67\x6F\x77\x7F"
		);
}


#if UCFG_CPU_X86_X64

void Transpose16x8bytes(__m128i to[8], __m128i from[8])
{


}



#endif // UCFG_CPU_X86_X64


} // Ext::

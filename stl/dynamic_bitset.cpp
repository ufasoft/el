/*######   Copyright (c) 2012-2018 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include EXT_HEADER_DYNAMIC_BITSET

namespace ExtSTL {
using namespace Ext;

void dynamic_bitsetBase::replace(size_type pos, bool val) {
	uint8_t& bref = ByteRef(pos);
	bref = bref & ~(1 << (pos & 7)) | (uint8_t(val) << (pos & 7));
}

void dynamic_bitsetBase::reset(size_type pos) {
	ByteRef(pos) &= ~(1 << (pos & 7));
}

void dynamic_bitsetBase::flip(size_type pos) {
	ByteRef(pos) ^= 1 << (pos & 7);
}


}  // ExtSTL::

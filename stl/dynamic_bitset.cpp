#include <el/ext.h>

#include "dynamic_bitset"

namespace ExtSTL {
using namespace Ext;

void dynamic_bitsetBase::replace(size_type pos, bool val) {
	byte& bref = ByteRef(pos);
	bref = bref & ~(1 << (pos & 7)) | (byte(val) << (pos & 7));
}

void dynamic_bitsetBase::reset(size_type pos) {
	ByteRef(pos) &= ~(1 << (pos & 7));
}

void dynamic_bitsetBase::flip(size_type pos) {
	ByteRef(pos) ^= 1 << (pos & 7);
}


}  // ExtSTL::


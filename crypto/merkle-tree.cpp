/*######   Copyright (c) 2012-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include "hash.h"

namespace Ext { namespace Crypto {

int PartialMerkleTreeBase::CalcTreeHeight() const {
	for (int r=0;; ++r)
		if (CalcTreeWidth(r) <= 1)
			return r;
}

void PartialMerkleTreeBase::TraverseAndBuild(int height, size_t pos, const void *ar, const dynamic_bitset<uint8_t>& vMatch) {
    // determine whether this node is the parent of at least one matched txid
    bool fParentOfMatch = false;
    for (size_t p=pos<<height; p < (pos+1)<<height && p<NItems; p++)
        fParentOfMatch |= vMatch[p];
	Bitset.push_back(fParentOfMatch);
    if (height==0 || !fParentOfMatch) {
		AddHash(height, pos, ar);
    } else {
        // otherwise, don't store any hash, but descend into the subtrees
        TraverseAndBuild(height-1, pos*2, ar, vMatch);
        if (pos*2+1 < CalcTreeWidth(height-1))
            TraverseAndBuild(height-1, pos*2+1, ar, vMatch);
    }

}

}} // Ext::Crypto


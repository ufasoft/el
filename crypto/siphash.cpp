/*######   Copyright (c) 2013-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include "hash.h"

using namespace Ext;

namespace Ext { namespace Crypto {

void SipHash2_4::InitHash(void *dst) noexcept {
    uint64_t *v = (uint64_t*)dst;
    v[0] = 0x736f6d6570736575ULL ^ m_key[0];
    v[1] = 0x646f72616e646f6dULL ^ m_key[1];
    v[2] = 0x6c7967656e657261ULL ^ m_key[0];
    v[3] = 0x7465646279746573ULL ^ m_key[1];
}

static void SipSubRound(uint64_t& a, uint64_t& b, int off) {
    b ^= a += exchange(b, _rotl64(b, off));
}

void SipHash2_4::Round(uint64_t v[4]) {
    SipSubRound(v[0], v[1], 13);
    v[0] = _rotl64(v[0], 32);
    SipSubRound(v[2], v[3], 16);
    SipSubRound(v[0], v[3], 21);
    SipSubRound(v[2], v[1], 17);
    v[2] = _rotl64(v[2], 32);
}

hashval SipHash2_4::ComputeHash(Stream& stm) {
    uint64_t v[4];
    InitHash(v);
    uint64_t cnt = 0;
    uint64_t t;
    while (true) {
        t = 0;
        auto c = stm.Read(&t, sizeof t);
        cnt += c;
        t = le64toh(t);
        if (c < 8)
            break;
        v[3] ^= t;
        TwoRounds(v);
        v[0] ^= t;
    }
    t |= cnt << 56;
    v[3] ^= t;
    TwoRounds(v);
    v[0] ^= t;
    v[2] ^= 0xFF;
    FourRounds(v);
    auto r = htole(v[0] ^ v[1] ^ v[2] ^ v[3]);
    return hashval((uint8_t*)&r, 8);
}

}} // Ext::Crypto::

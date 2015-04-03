#pragma once 

#include "util.h"

namespace Ext { namespace Crypto {

void Salsa20Core(uint32_t dst[16], int rounds = 20) noexcept;
void ChaCha20Core(uint32_t dst[16], int rounds = 20) noexcept;

typedef void (*PFN_Salsa)(uint32_t dst[16], int rounds);
void VectorMix(PFN_Salsa pfn, uint32_t x[][16], uint32_t tmp[][16], int r, int rounds = 20) noexcept;


}} // Ext::Crypto::



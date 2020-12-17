#pragma once 

#include "util.h"

namespace Ext { namespace Crypto {

void Salsa20Core(uint32_t dst[16], const uint32_t src[16], int rounds = 20) noexcept;
void ChaCha20Core(uint32_t dst[16], const uint32_t src[16], int rounds = 20) noexcept;



}} // Ext::Crypto::



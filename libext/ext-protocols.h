/*######   Copyright (c) 1997-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

namespace Ext {

uint16_t AFXAPI CalculateWordSum(RCSpan mb, uint32_t sum = 0, bool bComplement = false);



void AFXAPI ReadOneLineFromStream(const Stream& stm, String& beg, Stream *pDupStream = 0);
EXT_API std::vector<String> AFXAPI ReadHttpHeader(const Stream& stm, Stream *pDupStream = 0);


} // Ext::

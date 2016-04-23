/*######   Copyright (c) 2013-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once


#undef getcwd
#define DB_DBM_HSEARCH    0

#pragma warning(push)
#	pragma warning(disable: 4510 4610)
#	include <bdb/dbinc/db_cxx.h>
#pragma warning(pop)

//#include "db-itf.h"

namespace Ext {


} // namespace Ext



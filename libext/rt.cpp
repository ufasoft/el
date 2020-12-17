#include <el/ext.h>

namespace Ext { 
using namespace std;


TimeSpan ThreadBase::get_TotalProcessorTime() const {
#if UCFG_USE_PTHREADS
	timespec ts;
	CCheck(::clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts));
	return ts;
#else
	return UserProcessorTime + PrivilegedProcessorTime;
#endif
}


} // Ext::


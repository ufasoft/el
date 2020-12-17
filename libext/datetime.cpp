/*######   Copyright (c) 1997-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#pragma warning(disable: 4073)
#pragma init_seg(lib)				// to initialize DateTime::MaxValue early

#if UCFG_WIN32
#	include <winsock2.h>		// for timeval
#	include <unknwn.h>

#	include <el/libext/win32/ext-win.h>

#	if UCFG_COM
#		include <el/libext/win32/ext-com.h>
#	endif
#endif

#if UCFG_WDM
struct timeval {
	long    tv_sec;
	long    tv_usec;
};
#endif

#ifdef HAVE_SYS_TIME_H
#	include <sys/time.h>
#endif

#if UCFG_WCE
#	define mktime _mktime64
#endif

using namespace Ext;

namespace Ext {
using namespace std;

#if UCFG_WCE
	const int64_t TimeSpan::TicksPerMillisecond = 10000;
	const int64_t TimeSpan::TicksPerSecond = TimeSpan::TicksPerMillisecond * 1000;
	const int64_t TimeSpan::TicksPerMinute = TimeSpan::TicksPerSecond * 60;
	const int64_t TimeSpan::TicksPerHour = TimeSpan::TicksPerMinute * 60;
	const int64_t TimeSpan::TicksPerDay = TimeSpan::TicksPerHour * 24;

	const int DateTime::DaysPerYear = 365;
	const int DateTime::DaysPer4Years = DateTime::DaysPerYear * 4 + 1;
	const int DateTime::DaysPer100Years = DateTime::DaysPer4Years * 25 - 1;
	const int DateTime::DaysPer400Years = DateTime::DaysPer100Years * 4 + 1;
	const int DateTime::DaysTo1601 = DateTime::DaysPer400Years * 4;
	const int64_t DateTime::FileTimeOffset = DateTime::DaysTo1601 * TimeSpan::TicksPerDay;
#endif

const TimeSpan TimeSpan::MaxValue(numeric_limits<int64_t>::max());
const DateTime DateTime::MaxValue(numeric_limits<int64_t>::max());

const int64_t DateTime::TimevalOffset(621355968000000000LL);


const int64_t Unix_FileTime_Offset = 116444736000000000LL,
               Unix_DateTime_Offset = DateTime::FileTimeOffset + Unix_FileTime_Offset;

#if !UCFG_WDM

const int64_t DateTime::OADateOffset = DateTime(1899, 12, 30).Ticks;

TimeSpan::TimeSpan(const timeval& tv)
	: base(tv.tv_sec*10000000+tv.tv_usec*10)
{
}

void TimeSpan::ToTimeval(timeval& tv) const {
	int64_t cnt = count();
	tv.tv_sec = long(cnt /10000000);
	tv.tv_usec = long((cnt % 10000000)/10);
}

DateTime::DateTime(const timeval& tv) {
	m_ticks = int64_t(tv.tv_sec)*10000000 + Unix_DateTime_Offset+tv.tv_usec*10;
}

DateTime::DateTime(int year, int month, int day, int hour, int minute, int second, int ms) {
#if UCFG_WIN32
	SYSTEMTIME st = { (WORD)year, (WORD)month, 0, (WORD)day, (WORD)hour, (WORD)minute, (WORD)second, (WORD)ms };
	DateTime dt(st);
	_self = dt;
#else
	tm t = {second, minute, hour, day, month - 1, year - 1900};
	_self = t;
	m_ticks += ms * 10000;
#endif
}

DateTime::DateTime(const tm& t) {
	tm _t = t;
	_self = from_time_t(mktime(&_t));
}

#if UCFG_WIN32

DateTime::DateTime(const SYSTEMTIME& st) {
#if UCFG_USE_POSIX
	tm t = { st.wSecond, st.wMinute, st.wHour, st.wDay, st.wMonth-1, st.wYear-1900 };
	_self = t;
#else
	FILETIME ft;
	Win32Check(::SystemTimeToFileTime(&st, &ft));
	_self = ft;
#endif
}

#endif

DateTime DateTime::from_time_t(int64_t epoch) {
	return DateTime(epoch * 10000000 + Unix_DateTime_Offset);
}

String TimeSpan::ToString(int w) const {
	ostringstream os;
	Days days = duration_cast<Days>(*this);
	if (days.count())
		os << days.count() << ".";
	os << setw(2) << setfill('0') << Hours << ":" << setw(2) << setfill('0') << Minutes << ":" << setw(2) << setfill('0') << Seconds;
	int fraction = int(count() % 10000000L);
	if (w || fraction) {
		int full = 10000000;
		os << ".";
		if (!w)
			w = 7;
		if (w > 7)
			w = 7;
		for (int i=0; i<w; i++)
			full /= 10;
		os << setw(w) << setfill('0') << fraction/full;
	}
	return os.str();
}

String DateTime::ToString(DWORD dwFlags, LCID lcid) const {
#if UCFG_USE_POSIX || !UCFG_OLE || UCFG_WDM
	tm t = _self;
	char buf[100];
	strftime(buf, sizeof buf, "%x %X", &t);
	return buf;
#else
	CComBSTR bstr;
	OleCheck(::VarBstrFromDate(ToOADate(), lcid, dwFlags, &bstr));
	return bstr;
#endif
}

String DateTime::ToString(Microseconds) const {
	ostringstream os;
#if UCFG_USE_POSIX || UCFG_WDM
	os << ToString();
#else
	os << ToString(VAR_DATEVALUEONLY, LOCALE_NEUTRAL);
#endif
	os << " " << get_TimeOfDay().ToString(6);
	return os.str();
}

#	if !UCFG_WCE
String DateTime::ToString(RCString format) const {
	tm t = _self;
	char buf[100];
	if (format == "u")
		strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%SZ", &t);
	else
		Throw(E_INVALIDARG);
	return buf;
}
#	endif

#	if UCFG_USE_REGEX

static StaticRegex	s_reDateTimeFormat_u("(\\d\\d\\d\\d)-(\\d\\d)-(\\d\\d) (\\d\\d):(\\d\\d):(\\d\\d)Z"),
					s_reDateTimeFormat_8601("(\\d\\d\\d\\d)-(\\d\\d)-(\\d\\d)T(\\d\\d):(\\d\\d):(\\d\\d)(?:\\.(\\d{1,6}))(?:Z|[+-](\\d\\d)(?::(\\d\\d))?)?");

DateTime AFXAPI DateTime::ParseExact(RCString s, RCString format) {
	cmatch m;
	if (format == nullptr) {
		if (regex_match(s.c_str(), m, *s_reDateTimeFormat_8601)) {
			DateTime dt(stoi(string(m[1])), stoi(string(m[2])), stoi(string(m[3])), stoi(string(m[4])), stoi(string(m[5])), stoi(string(m[6])));
			if (m[7].matched) {
				String si(m[7]);
				int n = atoi(si);
				for (ssize_t i=7-m[7].length(); i--;)
					n *= 10;
				dt += TimeSpan(n);
			}
			return dt;						//!!!TODO adjust timezone
		}
	} else if (format == "u") {
		if (regex_match(s.c_str(), m, *s_reDateTimeFormat_u))
			return DateTime(stoi(string(m[1])), stoi(string(m[2])), stoi(string(m[3])), stoi(string(m[4])), stoi(string(m[5])), stoi(string(m[6])));
	}
	Throw(errc::invalid_argument);
}
#	endif // UCFG_USE_REGEX

#endif   // !UCFG_WDM

DateTime DateTime::FromAsctime(RCString s) {
#if UCFG_WDM
	Throw(E_NOTIMPL);
#else
	char month[4];
	int year, day;
	if (sscanf(s, "%3s %d %d", month, &day, &year) == 3) {
		static const char s_months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
		if (const char *p = strstr(s_months, month))
			return DateTime(year, int(p-s_months)/3+1, day);
	}
	Throw(ExtErr::InvalidInteger);
#endif
}

#if UCFG_WIN32_FULL
typedef VOID (WINAPI *PFN_GetSystemTimePreciseAsFileTime)(LPFILETIME lpSystemTimeAsFileTime);
static DlProcWrap<PFN_GetSystemTimePreciseAsFileTime> s_pfnGetSystemTimePreciseAsFileTime("KERNEL32.DLL", "GetSystemTimePreciseAsFileTime");
static PFN_GetSystemTimePreciseAsFileTime s_pfnGetSystemTimeAsFileTime = s_pfnGetSystemTimePreciseAsFileTime ? s_pfnGetSystemTimePreciseAsFileTime : &::GetSystemTimeAsFileTime;
#endif

int64_t DateTime::SimpleUtc() {
#if UCFG_USE_POSIX
//!!! librt required	timespec ts;
//	CCheck(::clock_gettime(CLOCK_REALTIME, &ts));
//	return from_time_t(ts.tv_sec).m_ticks + ts.tv_nsec/100;

#	ifdef HAVE_SYS_TIME_H
	timeval tv;
	CCheck(::gettimeofday(&tv, 0));
	return from_time_t(tv.tv_sec).m_ticks+tv.tv_usec*10;
#	else
		return from_time_t(time(0)).m_ticks;
#	endif
#elif UCFG_WDM
	LARGE_INTEGER st;
	KeQuerySystemTime(&st);
	return st.QuadPart + s_span1600;
#else

	FILETIME ft;
#	if UCFG_WCE
	SYSTEMTIME st;
	::GetSystemTime(&st);
	Win32Check(::SystemTimeToFileTime(&st, &ft));
#	else
	s_pfnGetSystemTimePreciseAsFileTime(&ft);
#	endif

	return (int64_t&)ft + s_span1600;
#endif
}

CPreciseTimeBase* volatile CPreciseTimeBase::s_pCurrent;

CPreciseTimeBase::CPreciseTimeBase()
	: MAX_PERIOD(128 * 10000000)   // 128 s,  200 s is upper limit
	, CORRECTION_PRECISION(500000)	// .05 s
	, MAX_FREQ_INSTABILITY(10)		// 10 %
{
	m_afCalibrate.clear();
	Reset();
}

void CPreciseTimeBase::AddToList() {
	CPreciseTimeBase * volatile *pPlace = &s_pCurrent;
	while (*pPlace)
		pPlace = &(*pPlace)->m_pNext;
	*pPlace = this;
}

int64_t CPreciseTimeBase::GetFrequency(int64_t stPeriod, int64_t tscPeriod) {
	return tscPeriod > numeric_limits<int64_t>::max()/10000000 || 0==stPeriod ? 0 : tscPeriod*10000000/stPeriod;
}

bool CPreciseTimeBase::Recalibrate(int64_t st, int64_t tsc, int64_t stPeriod, int64_t tscPeriod, bool bResetBase) {
	if (!m_period)
		return false;
	atomic_flag_lock lk(m_afCalibrate, try_to_lock);
	if (lk) {													// Inited
		if (bResetBase)
			ResetBounds();

		int64_t cntPeriod = tscPeriod & m_mask;
		int64_t freq;
		if (stPeriod > 0 && cntPeriod > 0 && (freq = GetFrequency(stPeriod, cntPeriod))) {
			int64_t prevMul = int64_t(m_mul) << (32-m_shift);

			int shift = std::min(int(m_shift), BitLen((uint64_t)freq));
			int32_t mul = int32_t((10000000LL << shift)/freq);
			m_shift = shift;
			m_mul = mul;

			m_minFreq = std::min(m_minFreq, freq);
			m_maxFreq = std::max(m_maxFreq, freq);

			TRC(6, "Fq=" << m_minFreq << " < " << freq << "(" << m_mul << ", " << m_shift << ") < " << m_maxFreq << " Hz Spread: " << (m_maxFreq-m_minFreq)*100/m_maxFreq << " %");

			if ((m_maxFreq - m_minFreq) * (100 / MAX_FREQ_INSTABILITY) > m_maxFreq) {
				TRC(5, "Switching PreciseTime");
				if (s_pCurrent == this) {
					s_pCurrent = m_pNext;
					return true;
				}
			}

			if (bResetBase)
				m_period = std::min(int64_t(m_period)*2, MAX_PERIOD);

		} else if (stPeriod < 0 || cntPeriod < 0) {
			TRC(5, "Some period is Negative. stPeriod: " << stPeriod << "     cntPeriod: " << cntPeriod);
		}

		if (bResetBase) {
			m_stBase = st;
			m_tscBase = tsc;
		}
	}
	return false;
}

#if UCFG_USE_DECLSPEC_THREAD
THREAD_LOCAL int s_cntDisablePreciseTime;
#else
static atomic<int> s_cntDisablePreciseTime;
#endif

class CDisablePreciseTimeKeeper {
public:
	__forceinline CDisablePreciseTimeKeeper() {
		++s_cntDisablePreciseTime;
	}

	__forceinline ~CDisablePreciseTimeKeeper() {
		--s_cntDisablePreciseTime;
	}
};


int64_t CPreciseTimeBase::GetTime(PFNSimpleUtc pfnSimpleUtc) {
	int64_t tsc = GetTicks(),
			st = pfnSimpleUtc();
#if UCFG_TRC
	int64_t tscAfter = GetTicks();
#endif

	CDisablePreciseTimeKeeper disablePreciseTimeKeeper;

	int64_t r = st;
	bool bSwitch = false;
	if (m_stBase) {
		int64_t stPeriod = st-m_stBase,
			tscPeriod = tsc-m_tscBase;
		int64_t ct = m_stBase + (((tscPeriod & m_mask) * m_mul) >> m_shift);
		bool bResetBase = MyAbs(stPeriod) > m_period;
		if (!bResetBase && MyAbs(st-ct) < CORRECTION_PRECISION)
			r = ct;
		else {
			TRC(6, "ST-diff " << stPeriod << "  CT-diff " << (st-(m_stBase+(((tscPeriod & m_mask)*m_mul)>>m_shift)))/10000 << "ms TSC-diff = " << tscAfter-tsc << "   Prev Recalibration: " << stPeriod/10000000 << " s ago");
			bSwitch = Recalibrate(st, tsc, stPeriod, tscPeriod, bResetBase);
		}
	} else {
		m_tscBase = tsc;
		m_stBase = st;  			// Race: this assignment should be last
	}
	if (bSwitch) {
		if (s_pCurrent)
			r = s_pCurrent->GetTime(pfnSimpleUtc);
	} else if (r < m_stPrev) {
		int64_t stPrev = m_stPrev;
		if (stPrev-r > 10*10000000) {
			TRC(6, "Time Anomaly: " << r-m_stPrev << "  stPrev = " << hex << stPrev << " r=" << hex << r);

			m_stPrev = r;
		} else {
			r = m_stPrev = m_stPrev+1;
		}
	} else {
		m_stPrev = r;
	}
	return r;
}

#	if UCFG_CPU_X86_X64
static class CTscPreciseTime : public CPreciseTimeBase {
public:
	CTscPreciseTime() {
		if (CpuInfo().ConstantTsc)
			AddToList();
	}

	int64_t GetTicks() noexcept override {
		return __rdtsc();
	}
} s_tscPreciseTime;

#if defined(_DEBUG) && defined(_MSC_VER)
__declspec(dllexport) void __cdecl Debug_ResetTsc() {
	return s_tscPreciseTime.Reset();
}

__declspec(dllexport) int32_t __cdecl Debug_GetTscMul() {
	return s_tscPreciseTime.m_mul;
}
#endif // _DEBUG && defined(_MSC_VER)

#	endif  // UCFG_CPU_X86_X64

#ifdef _WIN32

static class CPerfCounterPreciseTime : public CPreciseTimeBase {
	typedef CPreciseTimeBase base;
public:
	CPerfCounterPreciseTime() {
		LARGE_INTEGER liFreq;
#if UCFG_WIN32
		if (!::QueryPerformanceFrequency(&liFreq))
			return;
#else
		KeQueryPerformanceCounter(&liFreq);
#endif
		if (liFreq.QuadPart)
			AddToList();
	}

	int64_t GetTicks() noexcept override {
		return System.PerformanceCounter;
	}

	int64_t GetFrequency(int64_t stPeriod, int64_t tscPeriod) override {
		int64_t freq = System.PerformanceFrequency;
		TRC(6, "Declared Freq: " << freq << " Hz");
//!!!?not accurate		return freq;
		return base::GetFrequency(stPeriod, tscPeriod);
	}
} s_perfCounterPreciseTime;


#endif // _WIN32

LocalDateTime DateTime::ToLocalTime() const {
#if UCFG_USE_POSIX
	timeval tv;
	ToTimeval(tv);
	tm g = *gmtime(&tv.tv_sec);
	g.tm_isdst = 0;
	time_t t2 = mktime(&g);
	return LocalDateTime(DateTime(Ticks+int64_t(tv.tv_sec-t2)*10000000));
#elif UCFG_WDM
	LARGE_INTEGER st, lt;
	st.QuadPart = Ticks-FileTimeOffset;
	ExSystemTimeToLocalTime(&st, &lt);
	return LocalDateTime(DateTime(lt.QuadPart+FileTimeOffset));
#else
	FILETIME ft = ToFileTime();
	FileTimeToLocalFileTime(&ft, &ft);
	return LocalDateTime(DateTime(ft));
#endif
}



#ifdef WIN32

DateTime::operator SYSTEMTIME() const {
	SYSTEMTIME st;
	Win32Check(::FileTimeToSystemTime(&ToFileTime(), &st));
	return st;
}

#if UCFG_COM
DATE DateTime::ToOADate() const {
	return double(Ticks-OADateOffset)/TimeSpan::TicksPerDay;
}

DateTime DateTime::FromOADate(DATE date) {
	return DateTime(OADateOffset + int64_t(date*TimeSpan::TicksPerDay));
}

DateTime DateTime::Parse(RCString s, DWORD dwFlags, LCID lcid) {
	DATE date;
	const OLECHAR *pS = s;
	OleCheck(::VarDateFromStr((OLECHAR*)pS, lcid, dwFlags, &date));
	return FromOADate(date);
}
#endif

#	if !UCFG_WCE && UCFG_OLE

DateTime::DateTime(const COleVariant& v) {
	if (v.vt != VT_DATE)
		Throw(E_FAIL);
	_self = FromOADate(v.date);
}


#	endif

#endif

/*!!!D
String DateTime::ToString() const
{
  FILETIME ft;
  (int64_t&)ft = m_ticks-s_span1600;
	if ((int64_t&)ft < 0)
		(int64_t&)ft = 0; //!!!
	ATL::COleDateTime odt(ft);
  return odt.Format();
}
*/


/*!!!D
static timeval FileTimeToTimeval(const FILETIME & ft)
{
  timeval tv;
  tv.tv_sec = long(((int64_t&)ft - 116444736000000000) / 10000000);
  tv.tv_usec = long(((int64_t&)ft % 10000000)/10);
  return tv;
}
*/

#if defined(_WIN32) && defined(_M_IX86)

__declspec(naked) int __fastcall UnsafeDiv64by32(int64_t n, int d, int& r) {
	__asm {
		push	EDX
		mov		EAX, [ESP+8]
		mov		EDX, [ESP+12]
		idiv	ECX
		pop		ECX
		mov		[ECX], EDX
		ret		8
	}
}

#	if !UCFG_WDM
void DateTime::ToTimeval(timeval& tv) const {
	ZeroStruct(tv);
	int r;
	_try {
		tv.tv_sec = UnsafeDiv64by32(Ticks-TimevalOffset, 10000000, r);		//!!! int overflow possible for some dates
		tv.tv_usec = r/10;
	} _except(EXCEPTION_EXECUTE_HANDLER) {
	}
}


#	endif

#	if UCFG_USE_PTHREADS

DateTime::operator timespec() const {
	timespec ts = { 0 };
	int r;
	_try {
		ts.tv_sec = UnsafeDiv64by32(Ticks-TimevalOffset, 10000000, r);		//!!! int overflow possible for some dates
		ts.tv_nsec = r*100;
	} _except(EXCEPTION_EXECUTE_HANDLER) {
	}
	return ts;
}
#	endif

#else // defined(_WIN32) && defined(_M_IX86)

#	if UCFG_USE_PTHREADS

DateTime::operator timespec() const {
	int64_t t = Ticks-TimevalOffset;
	timespec tv;
	tv.tv_sec = long(t/10000000);
	tv.tv_nsec = long((t % 10000000)*100);
	return tv;
}
#	endif

#	if UCFG_WIN32 || UCFG_USE_PTHREADS
void DateTime::ToTimeval(timeval& tv) const {
	int64_t t = Ticks - TimevalOffset;
	tv.tv_sec = long(t / 10000000);
	tv.tv_usec = long((t % 10000000) / 10);
}
#	endif

#endif // defined(_WIN32) && defined(_M_IX86)


void DateTime::ToTm(tm& r) const {
#if UCFG_WDM
	LARGE_INTEGER li;
	li.QuadPart = Ticks - FileTimeOffset;
    TIME_FIELDS tf;
    ::RtlTimeToTimeFields(&li, &tf);
	memset(&r, 0, sizeof r);
    r.tm_sec = tf.Second;
    r.tm_min = tf.Minute;
    r.tm_hour = tf.Hour;
    r.tm_mday = tf.Day;
    r.tm_mon = tf.Month - 1;
    r.tm_year = tf.Year-1900;
    r.tm_wday = tf.Weekday;
 //!!!TODO    r.rm_yday = ;
    return r;
#elif UCFG_MSC_VERSION
	__time64_t t64 = ToUnixTimeSeconds();
	if (errno_t e = _gmtime64_s(&r, &t64))
		Throw(error_code(e, generic_category()));
#else
	gmtime_r(&tim, &r);
#endif
}

void DateTime::ToLocalTm(tm& r) const {
#if UCFG_MSC_VERSION
	__time64_t t64 = ToUnixTimeSeconds();
	if (errno_t e = _localtime64_s(&r, &t64))
		Throw(error_code(e, generic_category()));
#else
	ToLocalTime().ToTm(r);
#endif
}

DateTime LocalDateTime::ToUniversalTime() {
#if UCFG_USE_POSIX
	timeval tv;
	ToTimeval(tv);
	tm g = *gmtime(&tv.tv_sec);
	g.tm_isdst = 0;
	time_t t2 = mktime(&g);
	return DateTime(Ticks-int64_t(tv.tv_sec-t2)*10000000);
#elif UCFG_WDM
	LARGE_INTEGER st, lt;
	lt.QuadPart = Ticks-FileTimeOffset;
	ExLocalTimeToSystemTime(&lt, &st);
	return DateTime(st.QuadPart+FileTimeOffset);
#else
	FILETIME ft = ToFileTime();
	LocalFileTimeToFileTime(&ft, &ft);
	return ft;
#endif
}


#if !UCFG_WDM

TimeZoneInfo TimeZoneInfo::Local() {
	TimeZoneInfo tzi;
#if UCFG_USE_POSIX
	tzi.BaseUtcOffset = TimeSpan(int64_t(timezone)*10000000);
#else
	TIME_ZONE_INFORMATION info;
	Win32Check(TIME_ZONE_ID_INVALID != ::GetTimeZoneInformation(&info));
	tzi.BaseUtcOffset = TimeSpan(-int64_t(info.Bias) * 60*10000000);
#endif
	return tzi;
}

#endif // !UCFG_WDM

int64_t AFXAPI to_time_t(const DateTime& dt) {
	return (dt.Ticks - Unix_DateTime_Offset) / 10000000;
}

Clock::time_point AFXAPI Clock::now() noexcept {
#if !UCFG_USE_POSIX
	if (!s_cntDisablePreciseTime) {
		if (CPreciseTimeBase *preciser = CPreciseTimeBase::s_pCurrent)		// get pointer to avoid Race Condition
			return DateTime(preciser->GetTime(&DateTime::SimpleUtc));
	}
#endif
	return DateTime(DateTime::SimpleUtc());
}


} // Ext::


//From January 1, 1601 (UTC). to January 1, 1970

extern "C" {

#if UCFG_WIN32_FULL
/*
* Returns the difference between gmt and local time in seconds.
* Use gmtime() and localtime() to keep things simple.
*/
__int32 _cdecl gmt2local(time_t t)
{
	int dt, dir;
	struct tm *gmt, *loc;
	struct tm sgmt;

	if (t == 0)
		t = time(NULL);
	gmt = &sgmt;
	*gmt = *gmtime(&t);
	loc = localtime(&t);
	dt = (loc->tm_hour - gmt->tm_hour) * 60 * 60 +
		(loc->tm_min - gmt->tm_min) * 60;

	/*
	* If the year or julian day is different, we span 00:00 GMT
	* and must add or subtract a day. Check the year first to
	* avoid problems when the julian day wraps.
	*/
	dir = loc->tm_year - gmt->tm_year;
	if (dir == 0)
		dir = loc->tm_yday - gmt->tm_yday;
	dt += dir * 24 * 60 * 60;

	return (dt);
}



#endif // UCFG_WIN32_FULL

} // "C"

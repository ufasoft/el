/*######   Copyright (c) 1997-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include <chrono>

struct timeval;

namespace Ext {
using namespace std::chrono;
using std::ratio;
using std::ratio_multiply;

enum Microseconds {
};

extern const int64_t Unix_FileTime_Offset, Unix_DateTime_Offset;


class DateTimeBase {
	typedef DateTimeBase class_type;
protected:
	int64_t m_ticks;
public:
	int64_t get_Ticks() const { return m_ticks; }
	DEFPROP_GET_CONST(int64_t, Ticks);

	void Write(BinaryWriter& wr) const {
		wr << m_ticks;
	}

	void Read(const BinaryReader& rd) {
		m_ticks = rd.ReadInt64();
	}
protected:
	DateTimeBase(int64_t ticks = 0)
		: m_ticks(ticks)
	{}

	friend BinaryReader& AFXAPI operator>>(BinaryReader& rd, DateTimeBase& dt);
};

typedef duration<int, ratio<3600 * 24>> Days;

class TimeSpan : public duration<int64_t, ratio<1, 10000000>> {					//!!!R	: public DateTimeBase {
	typedef duration<int64_t, ratio<1, 10000000>> base;
public:

	typedef TimeSpan class_type;

#if UCFG_WCE
	static const EXT_DATA int64_t TicksPerMillisecond;
	static const EXT_DATA int64_t TicksPerSecond;
	static const EXT_DATA int64_t TicksPerMinute;
	static const EXT_DATA int64_t TicksPerHour;
	static const EXT_DATA int64_t TicksPerDay;
#else
	static const int64_t TicksPerMillisecond = 10000;
	static const int64_t TicksPerSecond = TicksPerMillisecond * 1000;
	static const int64_t TicksPerMinute = TicksPerSecond * 60;
	static const int64_t TicksPerHour = TicksPerMinute * 60;
	static const int64_t TicksPerDay = TicksPerHour * 24;
#endif

	static const EXT_DATA TimeSpan MaxValue;

	explicit TimeSpan(int64_t ticks = 0)
		: base(ticks)
	{}

	template <class T, class U>
	TimeSpan(const std::chrono::duration<T, U>& dur)
		: base(duration_cast<duration<TimeSpan::rep, TimeSpan::period>>(dur).count())
	{}

	TimeSpan(const TimeSpan& span)
		: base(span)
	{}

	TimeSpan(int days, int hours, int minutes, int seconds)
		: base((((days*24+hours)*60+minutes)*60+seconds)*10000000LL)
	{}

#if UCFG_WIN32
	TimeSpan(const FILETIME& ft)
		: base((int64_t&)ft)
	{}
#endif

#ifndef WDM_DRIVER
	TimeSpan(const timeval& tv);
	void ToTimeval(timeval& tv) const;
#endif

#if UCFG_USE_POSIX
	TimeSpan(const timespec& ts)
		: base(int64_t(ts.tv_sec)*10000000+ts.tv_nsec/100)
	{
	}
#endif

	void Write(BinaryWriter& wr) const {
		wr << count();
	}

	void Read(const BinaryReader& rd) {
		base::operator=(base(rd.ReadInt64()));
	}

	TimeSpan operator+(const TimeSpan& span) const { return TimeSpan(count() + span.count()); }
	TimeSpan operator-(const TimeSpan& span) const { return TimeSpan(count() - span.count()); }

	/*

	TimeSpan& operator+=(const TimeSpan& span) { m_ticks+=span.Ticks; return *this;}

	TimeSpan& operator-=(const TimeSpan& span) { m_ticks-=span.Ticks; return *this;}


	double get_TotalSeconds() const { return double(m_ticks)/10000000; }
	DEFPROP_GET_CONST(double, TotalSeconds);

	double get_TotalMinutes() const { return double(m_ticks)/(60LL*10000000); }
	DEFPROP_GET_CONST(double, TotalMinutes);

	double get_TotalHours() const { return double(m_ticks)/(60LL*60*10000000); }
	DEFPROP_GET_CONST(double, TotalHours);

	int get_Days() const { return int(TotalSeconds / (3600 * 24)); }
	DEFPROP_GET_CONST(int, Days);




	int get_Milliseconds() const { return int(Ticks/10000%1000); }
	DEFPROP_GET(int, Milliseconds);
	*/

	int get_Hours() const { return int(count() / 36000000000LL) % 24; }
	DEFPROP_GET(int, Hours);

	int get_Minutes() const { return int(count() / 600000000LL % 60); }
	DEFPROP_GET(int, Minutes);

	int get_Seconds() const { return int(count() / 10000000LL % 60); }
	DEFPROP_GET(int, Seconds);

	String ToString(int w = 0) const;

	static TimeSpan AFXAPI FromMilliseconds(double value) { return TimeSpan(int64_t(value * 10000)); }		//!!! behavior from .NET, where value rounds to milliseconds
	static TimeSpan AFXAPI FromSeconds(double value)	{ return TimeSpan(int64_t(value * 10000000)); }
	static TimeSpan AFXAPI FromMinutes(double value)	{ return FromSeconds(value * 60); }
	static TimeSpan AFXAPI FromHours(double value)		{ return FromSeconds(value * 3600); }
	static TimeSpan AFXAPI FromDays(double value)		{ return FromSeconds(value * 3600*24); }

	friend class DateTime;
};


inline TimeSpan operator*(const TimeSpan& ts, int v) { return TimeSpan(int64_t(ts.count() * v)); }
inline TimeSpan operator*(const TimeSpan& ts, double v) { return TimeSpan(int64_t(ts.count() * v)); }

inline TimeSpan operator/(const TimeSpan& ts, int v) { return TimeSpan(int64_t(ts.count() / v)); }
inline TimeSpan operator/(const TimeSpan& ts, double v) { return TimeSpan(int64_t(ts.count() / v)); }


/*inline bool AFXAPI operator==(const TimeSpan& span1, const TimeSpan& span2) { return span1.Ticks == span2.Ticks; }
inline bool AFXAPI operator!=(const TimeSpan& span1, const TimeSpan& span2) { return !(span1 == span2); }
inline bool AFXAPI operator<(const TimeSpan& span1, const TimeSpan& span2) { return span1.Ticks < span2.Ticks; }
inline bool AFXAPI operator>(const TimeSpan& span1, const TimeSpan& span2) { return span2 < span1; }
inline bool AFXAPI operator<=(const TimeSpan& span1, const TimeSpan& span2) { return span1.Ticks <= span2.Ticks; }
inline bool AFXAPI operator>=(const TimeSpan& span1, const TimeSpan& span2) { return span2 <= span1; }
*/

class DateTime;

inline bool AFXAPI operator<(const DateTime& dt1, const DateTime& dt2);
inline bool AFXAPI operator>(const DateTime& dt1, const DateTime& dt2);
inline bool AFXAPI operator==(const DateTime& dt1, const DateTime& dt2);

ENUM_CLASS(DayOfWeek) {
	Sunday, Monday, Tuesday, Wednesday, Thursday, Friday, Saturday
} END_ENUM_CLASS(DayOfWeek);

class LocalDateTime;

struct Clock;

class DateTime : public DateTimeBase {		//!!! should be : time_point<Clock, ...>
	typedef DateTimeBase base;
public:
	typedef DateTime class_type;
	typedef Clock clock;

	typedef std::chrono::duration<int64_t, ratio<1, 10000000>> duration;
	typedef std::chrono::time_point<system_clock, duration> time_point;

	static const int64_t s_span1600 = ((int64_t)10000000*3600*24)*(365*1600+388); //!!!+day/4; //!!! between 0 C.E. to 1600 C.E. as MS calculates

	static const EXT_DATA DateTime MaxValue;

	DateTime() {
	}

	DateTime(const DateTime& dt)
		: base(dt.m_ticks)
	{}

	explicit DateTime(int64_t ticks)
		: base(ticks)
	{}

	template <typename D>
	DateTime(const std::chrono::time_point<system_clock, D>& tp) {
		typedef ratio_multiply<ratio<10000000>, typename D::period> ticks_t;
		m_ticks = from_time_t(0).Ticks + tp.time_since_epoch().count() * ticks_t::num / ticks_t::den;		//!!!TODO Optimize
	}

	DateTime(const FILETIME& ft)
		: base((int64_t&)ft + FileTimeOffset)
	{
	}

#if !UCFG_WDM

#	if UCFG_WIN32
	DateTime(const SYSTEMTIME& st);
#	endif
	DateTime(const timeval& tv);

	int get_DayOfYear() const { return tm(*this).tm_yday+1; }
	DEFPROP_GET(int, DayOfYear);

#endif

	operator time_point() const {
		return time_point(duration(Ticks - from_time_t(0).Ticks));
	}

	void ToTimeval(timeval& tv) const;
	void ToTm(tm& r) const;	// Optimized
	void ToLocalTm(tm& r) const;	// Optimized
	
	operator tm() const {
		tm r;
		ToTm(r);
		return r;
	}

	static int64_t AFXAPI SimpleUtc();
	static DateTime AFXAPI from_time_t(int64_t epoch);
	int64_t ToUnixTimeSeconds() const { return (Ticks - TimevalOffset) / 10000000; }
	static DateTime AFXAPI FromAsctime(RCString s);

	int get_Hour() const {
#ifdef WIN32
		return SYSTEMTIME(*this).wHour;
#else
		return int(m_ticks/(int64_t(10000)*1000*3600) % 24);
#endif
	}
	DEFPROP_GET(int, Hour);

	int get_Minute() const {
#ifdef WIN32
		return SYSTEMTIME(*this).wMinute;
#else
		return int(m_ticks/(10000*1000*60) % 60);
#endif
	}
	DEFPROP_GET(int, Minute);

	int get_Second() const {
#ifdef WIN32
		return SYSTEMTIME(*this).wSecond;
#else
		return int(m_ticks/(10000*1000) % 60);
#endif
	}
	DEFPROP_GET(int, Second);

	int get_Millisecond() const {
#ifdef WIN32
		return SYSTEMTIME(*this).wMilliseconds;
#else
		return int(m_ticks/10000 % 1000);
#endif
	}
	DEFPROP_GET(int, Millisecond);


#ifdef WIN32

	explicit DateTime(const COleVariant& v);

	FILETIME ToFileTime() const {
		int64_t n = m_ticks - FileTimeOffset;
		return (FILETIME&)n;
	}

	operator FILETIME() const { return ToFileTime(); }

	operator SYSTEMTIME() const;


#	if UCFG_COM
	DATE ToOADate() const;
	static DateTime AFXAPI FromOADate(DATE date);
#	endif

#endif

	DateTime(int year, int month, int day, int hour = 0, int minute = 0, int second = 0, int ms = 0);
	DateTime(const tm& t);

	DateTime operator+(const TimeSpan& span) const { return DateTime(m_ticks + span.count()); }
	DateTime& operator+=(const TimeSpan& span) { return *this = *this + span; }


	template <class T, class U>
	DateTime operator-(const std::chrono::duration<T, U>& dur) const {
		return DateTime(m_ticks - duration_cast<std::chrono::duration<TimeSpan::rep, TimeSpan::period>>(dur).count());
	}

	DateTime& operator-=(const TimeSpan& span) { return *this = *this - span; }

	TimeSpan operator-(const DateTime& dt) const { return TimeSpan(m_ticks - dt.m_ticks); }

	LocalDateTime ToLocalTime() const;

#if UCFG_USE_PTHREADS
	operator timespec() const;
#endif

	int get_Day() const {
#if UCFG_USE_POSIX
		return tm(_self).tm_mday;
#elif !defined(WDM_DRIVER)
		return SYSTEMTIME(*this).wDay;
#else
		Throw(E_FAIL);
#endif
	}
	DEFPROP_GET(int, Day);

	TimeSpan get_TimeOfDay() const { return TimeSpan(Ticks % (24LL*3600*10000000)); }
	DEFPROP_GET_CONST(TimeSpan, TimeOfDay);

	DateTime get_Date() const { return *this - get_TimeOfDay(); }
	DEFPROP_GET_CONST(DateTime, Date);

	Ext::DayOfWeek get_DayOfWeek() const {
#if UCFG_USE_POSIX
		return Ext::DayOfWeek(tm(_self).tm_wday);
#elif !defined(WDM_DRIVER)
		return Ext::DayOfWeek(SYSTEMTIME(*this).wDayOfWeek);
#else
		Throw(E_FAIL);
#endif
	 }
	DEFPROP_GET_CONST(Ext::DayOfWeek, DayOfWeek);

	int get_Month() const {
#if UCFG_USE_POSIX
		return tm(_self).tm_mon+1;
#elif !defined(WDM_DRIVER)
		return SYSTEMTIME(*this).wMonth;
#else
		Throw(E_FAIL);
#endif

	}
	DEFPROP_GET_CONST(int, Month);

	int get_Year() const {
#if UCFG_USE_POSIX
		return tm(_self).tm_year+1900;
#elif !defined(WDM_DRIVER)
		return SYSTEMTIME(*this).wYear;
#else
		Throw(E_FAIL);
#endif
	}
	DEFPROP_GET(int, Year);

	String ToString(DWORD dwFlags = 0, LCID lcid = LANG_USER_DEFAULT) const;
	String ToString(Microseconds) const;
	String ToString(RCString format) const;

	static DateTime AFXAPI Parse(RCString s, DWORD dwFlags = 0, LCID lcid = LANG_USER_DEFAULT);
	static DateTime AFXAPI ParseExact(RCString s, RCString format = nullptr);

#if UCFG_WCE
	static const int DaysPerYear;
	static const int DaysPer4Years;
	static const int DaysPer100Years;	
	static const int DaysPer400Years;
	static const int DaysTo1601;
	static const int64_t FileTimeOffset;
#else
	static const int DaysPerYear = 365;
	static const int DaysPer4Years = DaysPerYear * 4 + 1;
	static const int DaysPer100Years = DaysPer4Years * 25 - 1;
	static const int DaysPer400Years = DaysPer100Years * 4 + 1;
	static const int DaysTo1601 = DaysPer400Years * 4;
	static const int64_t FileTimeOffset = DaysTo1601 * TimeSpan::TicksPerDay;
#endif

private:
	static const int64_t TimevalOffset;
	static const int64_t OADateOffset;
};

inline bool AFXAPI operator<(const DateTime& dt1, const DateTime& dt2) { return dt1.Ticks < dt2.Ticks; }
inline bool AFXAPI operator>(const DateTime& dt1, const DateTime& dt2) { return dt2 < dt1; }
inline bool AFXAPI operator<=(const DateTime& dt1, const DateTime& dt2) { return !(dt2 < dt1); }
inline bool AFXAPI operator>=(const DateTime& dt1, const DateTime& dt2) { return !(dt1 < dt2); }
inline bool AFXAPI operator==(const DateTime& dt1, const DateTime& dt2) { return dt1.Ticks == dt2.Ticks; }
inline bool AFXAPI operator!=(const DateTime& dt1, const DateTime& dt2) { return !(dt1 == dt2); }

inline BinaryWriter& AFXAPI operator<<(BinaryWriter& wr, const DateTimeBase& dt) {
	dt.Write(wr);
	return wr;
}

inline BinaryReader& AFXAPI operator>>(BinaryReader& rd, DateTimeBase& dt) {
	dt.Read(rd);
	return rd;
}

inline std::ostream& AFXAPI operator<<(std::ostream& os, const Ext::TimeSpan& span) {
	return os << span.ToString();
}

inline std::ostream& AFXAPI operator<<(std::ostream& os, const Ext::DateTime& dt) {
	return os << dt.ToString();
}

class LocalDateTime : public DateTime {
	typedef DateTime base;
public:
	int OffsetSeconds;

	LocalDateTime()
		: OffsetSeconds(0)
	{}

	explicit LocalDateTime(const DateTime& dt, int off = 0)
		: base(dt)
		, OffsetSeconds(off)
	{}

	DateTime ToUniversalTime();
};

class TimeZoneInfo {
public:
	static TimeZoneInfo AFXAPI Local();

	TimeSpan BaseUtcOffset;
};

typedef int64_t (AFXAPI *PFNSimpleUtc)();

class CPreciseTimeBase {
public:
	static CPreciseTimeBase * volatile s_pCurrent;
	int64_t m_mask;
	volatile int32_t m_mul;
	volatile int m_shift;

	virtual ~CPreciseTimeBase() {}
	int64_t GetTime(PFNSimpleUtc pfnSimpleUtc = &DateTime::SimpleUtc);

	void Reset() {
		m_period = 200000;			// 0.02 s
		m_stBase = 0;
		m_tscBase = 0;
		m_stPrev = 0;
		m_shift = 32;
		m_pNext = 0;
		m_mask = -1;

		ResetBounds();
	}
protected:
	atomic_flag m_afCalibrate;

	const int64_t MAX_PERIOD,
				CORRECTION_PRECISION,
				MAX_FREQ_INSTABILITY;

	volatile int64_t m_stBase, m_tscBase, m_stPrev;
	volatile int64_t m_period;
	CPreciseTimeBase *m_pNext;
	int64_t m_minFreq, m_maxFreq;

	CPreciseTimeBase();

	void ResetBounds() {
		m_mul = 0;
		m_minFreq = (std::numeric_limits<int64_t>::max)();
		m_maxFreq = 0;
	}

	static inline int64_t MyAbs(int64_t v) {
		return v < 0 ? -v : v;
	}

	static inline int BitLen(uint64_t n) {
		int r = 0;
		for (; n; ++r)
			n >>= 1;
		return r;
	}

	void AddToList();

	bool Recalibrate(int64_t st, int64_t tsc, int64_t stPeriod, int64_t tscPeriod, bool bResetBase);

	virtual int64_t GetTicks() noexcept =0;
	virtual int64_t GetFrequency(int64_t stPeriod, int64_t tscPeriod);
};

struct Clock {
	static const bool is_steady = false;

	typedef TimeSpan duration;
	typedef	DateTime time_point;

	static time_point AFXAPI now() noexcept;
};

class MeasureTime {
public:
	DateTime Start;
	TimeSpan& Span;

	MeasureTime(TimeSpan& span)
		: Start(Clock::now())
		, Span(span)
	{}

	TimeSpan End() {
		return Span = Clock::now()-Start;
	}

	~MeasureTime() {
		End();
	}
};

int64_t AFXAPI to_time_t(const DateTime& dt);





} // Ext::


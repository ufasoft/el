#pragma once

#ifdef __APPLE__
#	include <OpenCL/opencl.h>
#else
#	include <CL/cl.h>
#	include <CL/cl_ext.h>
#endif

#ifndef CL_VERSION_1_2
#	error OpenCL 1.2 or later version is required
#endif

namespace Ext { namespace cl {

using namespace Ext;

class Device;
class Kernel;

void ClCheck(cl_int rc);

String CommandTypeToString(cl_command_type ct);


class Platform {
	typedef Platform class_type;
public:
	cl_platform_id Id;

	Platform(cl_platform_id id = 0)
		:	Id(id)
	{}

	static vector<Platform> GetAll();

	String get_Vendor() const { return GetStringInfo(CL_PLATFORM_VENDOR); }
	DEFPROP_GET(String, Vendor);

	String get_Version() const { return GetStringInfo(CL_PLATFORM_VERSION); }
	DEFPROP_GET(String, Version);

	vector<String> get_Extensions() const { return GetStringInfo(CL_PLATFORM_EXTENSIONS).Split(); }
	DEFPROP_GET(vector<String>, Extensions);

	bool get_HasRetainDeviceFunction() const;
	DEFPROP_GET(bool, HasRetainDeviceFunction);

	vector<Device> GetDevices(cl_device_type typ = CL_DEVICE_TYPE_ALL) const;
private:
	String GetStringInfo(cl_platform_info param_name) const;
};



} // cl::

#define DEF_CL_OBJECT(uname, lname) template<> struct HandleTraits<cl_##lname> : HandleTraitsBase<cl_##lname> {	\
	static handle_type Retain(const handle_type& h) { cl::ClCheck(::clRetain##uname(h)); return h; }	\
	static void ResourceRelease(const handle_type& h) { cl::ClCheck(::clRelease##uname(h)); }	\
};

DEF_CL_OBJECT(Context, context);
DEF_CL_OBJECT(Program, program);
DEF_CL_OBJECT(CommandQueue, command_queue);
DEF_CL_OBJECT(Kernel, kernel);
DEF_CL_OBJECT(Event, event);
DEF_CL_OBJECT(MemObject, mem);
//DEF_CL_OBJECT(Device, device_id);

namespace cl {

class NDRange {
public:
	NDRange()
		:	m_dims(0)
	{}

	NDRange(size_t x)
		:	m_dims(1)
	{
		m_sizes[0] = x;
	}

	NDRange(size_t x, size_t y)
		:	m_dims(2)
	{
		m_sizes[0] = x;
		m_sizes[1] = y;
	}

	NDRange(size_t x, size_t y, size_t z)
		:	m_dims(3)
	{
		m_sizes[0] = x;
		m_sizes[1] = y;
		m_sizes[3] = z;
	}

	size_t dimensions() const { return m_dims; }
	operator const size_t*() const { return dimensions() ? m_sizes : 0; }
private:
	size_t m_sizes[3];
	int m_dims;
};

static const NDRange NullRange;

ENUM_CLASS(ExecutionStatus) {
	Complete = CL_COMPLETE,
	Running = CL_RUNNING,
	Submitted = CL_SUBMITTED,
	Queued = CL_QUEUED
} END_ENUM_CLASS(ExecutionStatus);

class Event : public ResourceWrapper<cl_event> {
	typedef Event class_type;
public:
	void setCallback(void (CL_CALLBACK *pfn)(cl_event, cl_int, void *), void *userData = 0);
	void wait();

	Event& Retain() {
		traits_type::Retain(m_h);
		return *this;
	}

	ExecutionStatus get_Status();
	DEFPROP_GET(ExecutionStatus, Status);

	int get_RefCount();
	DEFPROP_GET(int, RefCount);

	cl_command_type get_CommandType();
	DEFPROP_GET(cl_command_type, CommandType);

	uint64_t get_ProfilingCommandQueued() { return GetProfilingInfo(CL_PROFILING_COMMAND_QUEUED); }
	DEFPROP_GET(uint64_t, ProfilingCommandQueued);

	uint64_t get_ProfilingCommandSubmit() { return GetProfilingInfo(CL_PROFILING_COMMAND_SUBMIT); }
	DEFPROP_GET(uint64_t, ProfilingCommandSubmit);

	uint64_t get_ProfilingCommandStart() { return GetProfilingInfo(CL_PROFILING_COMMAND_START); }
	DEFPROP_GET(uint64_t, ProfilingCommandStart);

	uint64_t get_ProfilingCommandEnd() { return GetProfilingInfo(CL_PROFILING_COMMAND_END); }
	DEFPROP_GET(uint64_t, ProfilingCommandEnd);
private:
	uint64_t GetProfilingInfo(cl_event_info name);
};

class Device { //!!! special case, OpenCL 1.1 has not retain/release : public ResourceWrapper<cl_device_id> {
	typedef Device class_type;
public:
	typedef cl_device_id handle_type;

	cl_device_id m_h;		//!!! should be private
	CBool m_bHasRetain; 

	Device()
		:	m_h(0)
	{}

	Device(const Device& dev)
		:	m_h(0)
		,	m_bHasRetain(dev.m_bHasRetain)
	{
		operator=(dev);
	}

	Device(cl_device_id id);

	Device(cl_device_id id, bool bHasRetain)
		:	m_h(id)
		,	m_bHasRetain(bHasRetain)
	{
	}

	~Device() {
		if (Valid() && m_bHasRetain) {
			if (!InException)
				ClCheck(::clReleaseDevice(m_h));
			else {
				try {
					ClCheck(::clReleaseDevice(m_h));
				} catch (RCExc&) {
				//	TRC(0, e);
				}
			}
		}
	}

	bool Valid() const {
		return m_h != 0;
	}

	handle_type& OutRef() {
		if (Valid())
			Throw(E_EXT_AlreadyOpened);
		return m_h;
	}

	handle_type Handle() const {
		if (!Valid())
			Throw(E_HANDLE);
		return m_h;
	}

	handle_type operator()() const { return Handle(); }

	handle_type& operator()() {
		if (!Valid())
			Throw(E_HANDLE);
		return m_h;		
	}

	void Close() {
		if (Valid()) {
			cl_device_id id = exchange(m_h, cl_device_id(0));
			if (m_bHasRetain)
				ClCheck(::clReleaseDevice(id));
		}
	}

	Device& operator=(const Device& res) {
		Close();
		m_bHasRetain = res.m_bHasRetain;
		if (res.Valid()) {
			if (m_bHasRetain)
				ClCheck(::clRetainDevice(res.m_h));
			m_h = res.m_h;
		}
		return *this;
	}

	Blob GetInfo(cl_device_info pname) const;

	cl::Platform get_Platform() const {
		return cl::Platform(*(cl_platform_id*)GetInfo(CL_DEVICE_PLATFORM).constData());
	}
	DEFPROP_GET(cl::Platform, Platform);

	String get_Name() const { 
		Blob blob = GetInfo(CL_DEVICE_NAME);
		return String((const char*)blob.constData());
	}
	DEFPROP_GET(String, Name);

	bool get_Available() const { return *(cl_bool*)GetInfo(CL_DEVICE_AVAILABLE).constData(); }
	DEFPROP_GET(bool, Available);

	uint32_t get_MaxComputeUnits() const { return *(cl_uint*)GetInfo(CL_DEVICE_MAX_COMPUTE_UNITS).constData(); }
	DEFPROP_GET(uint32_t, MaxComputeUnits);

	size_t get_MaxWorkGroupSize() const { return *(size_t*)GetInfo(CL_DEVICE_MAX_WORK_GROUP_SIZE).constData(); }
	DEFPROP_GET(size_t, MaxWorkGroupSize);

	uint64_t get_GlobalMemSize() const { return *(cl_ulong*)GetInfo(CL_DEVICE_GLOBAL_MEM_SIZE).constData(); }
	DEFPROP_GET(uint64_t, GlobalMemSize);

	uint64_t get_MaxMemAllocSize() const { return *(cl_ulong*)GetInfo(CL_DEVICE_MAX_MEM_ALLOC_SIZE).constData(); }
	DEFPROP_GET(uint64_t, MaxMemAllocSize);

	vector<String> get_Extensions() const {
		Blob blob = GetInfo(CL_DEVICE_EXTENSIONS);
		return String((const char*)blob.constData(), blob.Size).Split();
	}
	DEFPROP_GET(vector<String>, Extensions);

	bool get_EndianLittle() const { return *(cl_bool*)GetInfo(CL_DEVICE_ENDIAN_LITTLE).constData(); }
	DEFPROP_GET(bool, EndianLittle);

	size_t get_ProfilingTimerResolution() const { return *(size_t*)GetInfo(CL_DEVICE_PROFILING_TIMER_RESOLUTION).constData(); }
	DEFPROP_GET(size_t, ProfilingTimerResolution);
private:
	CInException InException;
};

typedef map<cl_context_properties, cl_context_properties> CContextProps;

class Context : public ResourceWrapper<cl_context> {
	typedef Context class_type;
public:
	Context() {
	}

	Context(const Device& dev, const CContextProps *props = 0);
	~Context();
	void CreateFromType(const CContextProps& props, cl_device_type typ = CL_DEVICE_TYPE_ALL);

	vector<Device> get_Devices() const;
	DEFPROP_GET(vector<Device>, Devices);
};

class Memory : public ResourceWrapper<cl_mem> {
};

class Buffer : public Memory {
	typedef Memory base;
public:
	Buffer() {}
	Buffer(const Buffer& buf) : base(buf) {}

	Buffer(const Context& ctx, cl_mem_flags flags, size_t size, void *host_ptr = 0);
};

class CommandQueue : public ResourceWrapper<cl_command_queue> {
	typedef ResourceWrapper<cl_command_queue> base;
public:
	CommandQueue() {}

	CommandQueue(const CommandQueue& q)
		:	base(q)
	{}

	CommandQueue(const Context& ctx, const Device& dev, cl_command_queue_properties prop = 0);
	Event enqueueTask(const Kernel& kernel, const std::vector<Event> *events = 0);
	Event enqueueNDRangeKernel(const Kernel& kernel, const NDRange& offset, const NDRange& global, const NDRange& local = NullRange, const std::vector<Event> *events = 0);
	Event enqueueNativeKernel(void (CL_CALLBACK *pfn)(void *), std::pair<void*, size_t> args, const std::vector<Memory> *mems = 0, const std::vector<const void*> *memLocs = 0, const std::vector<Event> *events = 0);
	Event enqueueReadBuffer(const Buffer& buffer, bool bBlocking, size_t offset, size_t cb, void *p, const std::vector<Event> *events = 0);
	Event enqueueWriteBuffer(const Buffer& buffer, bool bBlocking, size_t offset, size_t cb, const void *p, const std::vector<Event> *events = 0);
	Event enqueueMarkerWithWaitList(const std::vector<Event> *events = 0);
	Event enqueueBarrierWithWaitList(const std::vector<Event> *events = 0);
	
	void flush() const {
		ClCheck(::clFlush(Handle()));
	}

	void finish() const {
		ClCheck(::clFinish(Handle()));
	}
};

struct ProgramBinary {
	cl::Device Device;
	Blob Binary;
};

class Program : public ResourceWrapper<cl_program> {
	typedef ResourceWrapper<cl_program> base;
	typedef Program class_type;
public:
//!!!R	CPointer<Context> Ctx;
	typedef unordered_map<String, String> COptions;
	COptions Options;

	Program() {}

	Program(const Program& prog)
		:	base(prog)
	{}

	Program(const Context& ctx, RCString s, bool bBuild = false);
	static Program FromBinary(Context& ctx, const Device& dev, const ConstBuf& mb);
	void build();
	void build(const std::vector<Device>& devs);

	vector<Device> get_Devices();
	DEFPROP_GET(vector<Device>, Devices);

	vector<ProgramBinary> get_Binaries();
	DEFPROP_GET(vector<ProgramBinary>, Binaries);
};


class Kernel : public ResourceWrapper<cl_kernel> {
public:
	Kernel() {
	}

	Kernel(const Program& prog, RCString name);

	void setArg(cl_uint idx, const Buffer& buf) {
		ClCheck(::clSetKernelArg(Handle(), idx, sizeof(buf.m_h), &buf.m_h));
	}

	template <typename T>
	void setArg(cl_uint idx, const T& v) {
		ClCheck(::clSetKernelArg(Handle(), idx, sizeof(v), &v));
	}
};

const error_category& opencl_category();


class OpenclException : public Exception {
	typedef Exception base;
public:
	OpenclException(int errval)
		:	base(error_code(errval, opencl_category()))
	{}

};

class BuildException : public OpenclException {
	typedef OpenclException base;
public:
	String Log;

	BuildException(HRESULT hr, RCString log)
		:	base(hr)
		,	Log(log)
	{}
};

}} // Ext::cl::


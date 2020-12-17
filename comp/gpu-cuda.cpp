#include <el/ext.h>

#include "gpu-cuda.h"

#pragma comment(lib, "cuda")

namespace Ext { namespace Gpu {

void CuCheck(CUresult rc) {
	if (rc != CUDA_SUCCESS) {
		throw CuException(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_GPU_CU, rc), nullptr);
	}
}

static bool s_bInited;

void CuEngine::Init() {
	if (!s_bInited) {
		CuCheck(::cuInit(0));
		s_bInited = true;
	}
}

int CuEngine::DeviceCount() {
	int r;
	CuCheck(::cuDeviceGetCount(&r));
	return r;
}

CuDevice CuEngine::GetDevice(int idx) {
	CuDevice r;
	CuCheck(::cuDeviceGet(&r.m_dev, idx));
	return r;
}

int CuEngine::get_Version() {
	int r;
	CuCheck(::cuDriverGetVersion(&r));
	return r;
}

String CuDevice::get_Name() {
	char buf[256];
	CuCheck(::cuDeviceGetName(buf, _countof(buf), m_dev));
	return buf;
}

CuVersion CuDevice::get_ComputeCapability() {
	int mj, mn;
	CuCheck(::cuDeviceComputeCapability(&mj, &mn, m_dev));
	return CuVersion(mj, mn);
}

int CuDevice::GetAttribute(CUdevice_attribute attrib) {
	int r;
	CuCheck(::cuDeviceGetAttribute(&r, attrib, m_dev));
	return r;
}

CuContext CuDevice::CreateContext(UInt32 flags) {
#ifdef X_DEBUG//!!!D
	int i = 5/flags;
#endif

	CUcontext h;
	CuCheck(::cuCtxCreate(&h, flags, m_dev));
	CuContext r;
	r.m_pimpl = new CuContextObj;
	r->m_h = h;
	return r;
}

void CuKernel::ParamSetv(int offset, void *p, size_t size) {
	CuCheck(::cuParamSetv(m_h, offset, p, size));
}

void CuKernel::ParamSetSize(size_t size) {
	CuCheck(::cuParamSetSize(m_h, size));
}

void CuKernel::SetBlockShape(int x, int y, int z) {
	CuCheck(::cuFuncSetBlockShape(m_h, x, y, z));
}

void CuKernel::Launch() {
	CuCheck(::cuLaunch(m_h));
}

void CuKernel::Launch(int width, int height) {
	CuCheck(::cuLaunchGrid(m_h, width, height));
}

CuModule CuModule::FromFile(RCString filename) {
	CUmodule h;
	CuCheck(::cuModuleLoad(&h, filename));
	CuModule r;
	(r.m_pimpl = new CuModuleObj)->m_h = h;
	return r;
}

CuModule CuModule::FromData(const void *image) {
	CUmodule h;
	CuCheck(::cuModuleLoadData(&h, image));
	CuModule r;
	(r.m_pimpl = new CuModuleObj)->m_h = h;
	return r;
}

CuKernel CuModule::GetFunction(RCString name) {
	CuKernel r;
	CuCheck(::cuModuleGetFunction(&r.m_h, _self, name));
	return r;

}

}} // Ext::Gpu::

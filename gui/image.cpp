#include <el/ext.h>

#include <windows.h>
#include <commctrl.h>
#include <commoncontrols.h>
#include <shellapi.h>

#include <initguid.h>

#if UCFG_WCE
#	include <imgguids.h>
#endif

#include "el/gui/image.h"

namespace Ext::Gui {
//!!!R #include "win32/mfcimpl.h"


#if UCFG_WCE

#	include <gdipluspixelformats.h>
#	include <gdiplusimaging.h>

ImageFormat ImageFormat::Bmp(IMGFMT_BMP),
			ImageFormat::Emf(IMGFMT_EMF),
			ImageFormat::Exif(IMGFMT_EXIF),
			ImageFormat::Gif(IMGFMT_GIF),
			ImageFormat::Icon(IMGFMT_ICO),
			ImageFormat::Jpeg(IMGFMT_JPEG),
			ImageFormat::Png(IMGFMT_PNG),
			ImageFormat::Tiff(IMGFMT_TIFF),
			ImageFormat::Wmf(IMGFMT_WMF);

#endif

#ifdef X_UCFG_WCE //!!!confilict

vector<ImageCodecInfo> ImageCodecInfo::GetImageEncoders() {
	vector<ImageCodecInfo> ar;
	CComMem< :: ImageCodecInfo> mem;
	UINT count;
	OleCheck(Image::GetFactory()->GetInstalledEncoders(&count, &mem));
	for (int i=0; i<count; i++) {
		::ImageCodecInfo& ici = mem.m_p[i];
		ImageCodecInfo ci;
		ci.Clsid = ici.Clsid;
		ci.FormatID = ici.FormatID;
		ar.push_back(ci);
	}
	return ar;
}

CComPtr<IImagingFactory> Image::GetFactory() {
	return CreateComObject(CLSID_ImagingFactory);
}

Image Image::FromFile(RCString filename) {
	Image r;
	OleCheck(GetFactory()->CreateImageFromFile(filename, &r));
	return r;
}

Image Image::FromStream(IStream *iStream) {
	Image r;
	OleCheck(GetFactory()->CreateImageFromStream(iStream, &r));
	return r;
}

Image Image::FromBuffer(RCSpan mb, BufferDisposalFlag disposalFlag) {
	Image r;
	OleCheck(GetFactory()->CreateImageFromBuffer(mb.m_p, mb.m_len, disposalFlag, &r));
	return r;
}

Image Image::GetThumbnail(int thumbWidth, int thumbHeight) {
	Image r;
	OleCheck(_self->GetThumbnail(thumbWidth, thumbHeight, &r));
	return r;
}

void Image::Encode(IImageEncoder *iEncoder) {
	CComPtr<IImageSink> iSink;
	OleCheck(iEncoder->GetEncodeSink(&iSink));
	OleCheck(_self->PushIntoSink(iSink));
}

void Image::Save(RCString filename, ImageFormat format) {
	vector<ImageCodecInfo> v = ImageCodecInfo::GetImageEncoders();
	for (int i=v.size(); i--;) {
		ImageCodecInfo& ici = v[i];
		if (ici.FormatID == format.Guid) {
			CComPtr<IImageEncoder> iEnc;
			OleCheck(Image::GetFactory()->CreateImageEncoderToFile(&ici.Clsid, filename, &iEnc));
			Encode(iEnc);
			return;
		}
	}
	throw NoSuchEncoderExc;
}

Bitmap::Bitmap(Image image, PixelFormatID pixelFormat) {
	OleCheck(Image::GetFactory()->CreateBitmapFromImage(image, 0, 0, pixelFormat, InterpolationHintDefault, iface_base::operator&()));
	(CComPtr<IImage>&)_self = (CComPtr<IBitmapImage>&)_self;
}

Size Bitmap::get_Size() {
	SIZE size;
	OleCheck(iface_base::operator->()->GetSize(&size));
	return size;
}

BitmapData Bitmap::LockBits(PixelFormatID pixelFormat) {
	RECT rect = { 0, 0, Size.Width, Size.Height };
	BitmapData bd = { 0 };
	OleCheck(iface_base::operator->()->LockBits(&rect, ImageLockModeRead, pixelFormat, &bd));
	return bd;
}

void Bitmap::UnlockBits(const BitmapData& bd) {
	OleCheck(iface_base::operator->()->UnlockBits(&bd));
}

void Bitmap::Save(Stream& stm) {
	PixelFormat pelFormat = PixelFormat16bppRGB555; //!!!

	BitmapInfoKeeper bik(_self, pelFormat);
	CBitmapLocker bl(_self, pelFormat);
	class Size size = Size;
	size_t len  = bl.BitmapData.Stride*size.Height;
	BITMAPFILEHEADER bmfh = { 'MB', sizeof(bmfh)+bik.m_biSize+len, 0, 0, sizeof(BITMAPFILEHEADER)+bik.m_biSize };
	stm.WriteStruct(bmfh);
	stm.WriteBuffer(bik.m_p, bik.m_biSize);
	stm.WriteBuffer(bl.BitmapData.Scan0, len);
}

CBitmapLocker::CBitmapLocker(class Bitmap& bmp, PixelFormatID pixelFormat)
	: Bitmap(bmp)
{
	BitmapData = bmp.LockBits(pixelFormat);
}

CBitmapLocker::~CBitmapLocker() {
	Bitmap.UnlockBits(BitmapData);
}

BitmapInfoKeeper::BitmapInfoKeeper(Bitmap bmp, PixelFormatID pixelFormat) {
	CComMem<ColorPalette> cp;
	HRESULT hr = bmp->GetPalette(&cp);
	int count = 0;
	if (SUCCEEDED(hr))
		count = cp.m_p->Count;
	else if (hr != IMGERR_NOPALETTE)
		OleCheck(hr);
	m_biSize = sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*count;
	m_p = (BITMAPINFO*)new BYTE[m_biSize];
	memset(m_p, 0, m_biSize);
	BITMAPINFOHEADER& h = m_p->bmiHeader;
	h.biPlanes = 1;
	h.biBitCount = 16;
	h.biSize = sizeof(BITMAPINFOHEADER);
	Size size = bmp.Size;
	h.biWidth = size.Width;
	h.biHeight = -size.Height; // top-down DIB
	h.biClrUsed = count;
	Size physDim = bmp.PhysicalDimension; // in 0.01mm units
	h.biXPelsPerMeter = size.Width*100000/physDim.Width;
	h.biYPelsPerMeter = size.Height*100000/physDim.Height;
	for (int i=0; i<count; i++)
		m_p->bmiColors[i] = *(RGBQUAD*)&cp.m_p->Entries[i];
}
#endif //!!!confilict

Icon::Icon(RCString filename) {
	HANDLE h = ::LoadImage(0, filename, IMAGE_ICON, 0, 0,
#if UCFG_WCE
		0
#else
		LR_LOADFROMFILE
#endif
		);
	Win32Check(h != 0);
	m_pimpl = new IconObj((HICON)h, true);
}

Icon Icon::FromHandle(HICON hicon, bool bNeedDestroy) {
	Icon r;
	r.m_pimpl = new IconObj(hicon, bNeedDestroy);
	return r;
}

Icon Icon::ExtractIcon(RCString filePath) {
	UINT iIcon = 0;
#if UCFG_WCE
	if (HICON hicon = ::ExtractIconEx(filePath, iIcon, 0, 0, 0)) //!!!verify
#else
	if (HICON hicon = ::ExtractIcon(AfxGetInstanceHandle(), filePath, iIcon))
#endif
		return FromHandle(hicon, true);
	else
		throw IconNotFoundExc();
}

#if !UCFG_WCE

Icon Icon::GetShellIcon(RCString path, UINT flags) {
	SHFILEINFO sfi = { 0 };
	Win32Check(::SHGetFileInfo(path, 0, &sfi, sizeof(sfi), flags));
	return FromHandle(sfi.hIcon, true);
}

Icon Icon::GetJumboIcon(RCString path, UINT flags) {
	CComPtr<IImageList> spImageList;
	::SHGetImageList(SHIL_JUMBO, IID_IImageList, (LPVOID*)&spImageList);
	if (spImageList) {
		SHFILEINFO sfi = { 0 };
		Win32Check(::SHGetFileInfo(path, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX));
		if (sfi.iIcon) {
			HICON hIcon;
			OleCheck(spImageList->GetIcon(sfi.iIcon, ILD_TRANSPARENT, &hIcon));
			return FromHandle(hIcon, true);
		}
	}
	return GetShellIcon(path, flags);
}

Icon Icon::ExtractSmallIcon(RCString filePath) {
	HICON hiconLarge, hiconSmall;
	int n = (int)::ExtractIconEx(filePath, 0, &hiconLarge, &hiconSmall, 1);		//!!! Error in Spec:  the ExtractIconEx() can return 0xFFFFFFFF
	if (n <= 0)
		throw IconNotFoundExc();
	HICON hicon;
	if (hiconSmall) {
		hicon = hiconSmall;
		Win32Check(::DestroyIcon(hiconLarge));
	} else
		hicon = hiconLarge;
	return FromHandle(hicon, true);
}

Icon Icon::ExtractAssociatedIcon(RCString filePath) {
	WORD iIcon = 1;
	TCHAR path[_MAX_PATH];
	ZeroStruct(path);
	wcsncpy(path, filePath, size(path)-1);
	if (HICON hicon = ::ExtractAssociatedIcon(AfxGetInstanceHandle(), path, &iIcon))
		return FromHandle(hicon, true);
	else
		throw IconNotFoundExc();
}

Icon Icon::Clone() {
	HICON hicon = ::CopyIcon(_self);
	Win32Check(hicon != 0);
	return FromHandle(hicon, true);
}

void ImageList::EnsureHandle() const {
	if (!m_hImageList) {
		m_hImageList = ::ImageList_Create(m_imageSize.Width, m_imageSize.Height, int(ColorDepth) | ILC_MASK, 16, 16);
		Win32Check(m_hImageList != 0);
	}
}

ImageList::operator HIMAGELIST() const {
	EnsureHandle();
	return m_hImageList;
}

Image ImageList::GetIcon(int i, UINT flags) const {
	EnsureHandle();
	HICON hicon = ::ImageList_GetIcon(m_hImageList, i, flags);
	if (!hicon)
		Throw(E_FAIL);
	return Icon::FromHandle(hicon, true);
}

#endif //UCFG_WCE

Size Icon::get_Size() {
	ICONINFO info;
	Win32Check(::GetIconInfo(_self, &info));
	BITMAP bm;
	Win32Check(::GetObject(info.hbmMask, sizeof bm, &bm));
	return Ext::Size(bm.bmWidth, bm.bmHeight);
}

#if UCFG_THREAD_MANAGEMENT
CHandleMap<ImageList> *afxMapHIMAGELIST() {
	return &AfxGetModuleThreadState()->GetHandleMaps().m_mapHIMAGELIST;
}


ImageList* ImageList::FromHandle(HIMAGELIST h) {
	if (h)
		return afxMapHIMAGELIST()->FromHandle(h);
	else
		return 0;
}

ImageList* ImageList::FromHandlePermanent(HIMAGELIST h) {
	return afxMapHIMAGELIST()->LookupPermanent(h);
}

#endif // UCFG_THREAD_MANAGEMENT

ImageList::ImageList()
	: m_hImageList(0)
	, m_colorDepth(ColorDepth::Depth8Bit)
	, m_imageSize(16, 16)
{
}

ImageList::~ImageList() {
	Delete();
}

void ImageList::Attach(HIMAGELIST hImageList) {
	m_hImageList = hImageList;
#if UCFG_THREAD_MANAGEMENT
	afxMapHIMAGELIST()->SetPermanent(m_hImageList, this);
#endif
}

HIMAGELIST ImageList::Detach() {
	HIMAGELIST h = exchange(m_hImageList, (HIMAGELIST)0);
#if UCFG_THREAD_MANAGEMENT
	if (h)
		afxMapHIMAGELIST()->RemoveHandle(h);
#endif // UCFG_THREAD_MANAGEMENT
	return h;
}

void ImageList::Create(const CResID& resID, int cx, int nGrow, COLORREF crMask) {
	HINSTANCE hInst = AfxFindResourceHandle(resID, RT_BITMAP);
	Attach(ImageList_LoadImage(hInst, resID, cx, nGrow, crMask, IMAGE_BITMAP, LR_DEFAULTCOLOR));
}

void ImageList::Delete() {
	if (HIMAGELIST h = Detach())
		Win32Check(::ImageList_Destroy(h));
}

Size ImageList::get_ImageSize() const {
	int cx, cy;
	Win32Check(::ImageList_GetIconSize(*this, &cx, &cy));
	return Size(cx, cy);
}

void ImageList::put_ImageSize(const Size& size) {
	if (ImageSize != size) {
		if (!::ImageList_SetIconSize(m_hImageList, size.Width, size.Height))
			Throw(E_FAIL);
	}
}

void ImageList::put_ColorDepth(Ext::Gui::ColorDepth v) {
	if (m_colorDepth != v) {
		Delete();
		m_colorDepth = v;
	}
}


Size Icon::GetShellIconSize(UINT uType) {
	ATLASSERT(uType == SHGFI_LARGEICON || uType == SHGFI_SMALLICON);
	SHFILEINFO sfi = { 0 };
	::SHGetFileInfo(_T("temp.txt"), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_USEFILEATTRIBUTES | SHGFI_SHELLICONSIZE | uType);
	if (sfi.hIcon != NULL) {
		ICONINFO ii = { 0 };
		::GetIconInfo(sfi.hIcon, &ii);
		BITMAP bm = { 0 };
		BOOL bRes = ::GetObject(ii.hbmColor, sizeof(BITMAP), &bm);
		::DeleteObject(sfi.hIcon);
		if (bRes) {
			SIZE sz = { bm.bmWidth, abs(bm.bmHeight) };
			return sz;
		}
	}
	int ix = SM_CXSMICON, iy = SM_CYSMICON;
	if (uType == SHGFI_LARGEICON) ix = SM_CXICON, iy = SM_CYICON;
	return Ext::Size(::GetSystemMetrics(ix), ::GetSystemMetrics(iy));
}




} // Ext::Gui

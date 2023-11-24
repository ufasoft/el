#pragma once

#include <el/libext/win32/ext-win.h>

#if UCFG_WCE && !defined(_EXT)
#	include <imaging.h>
#endif

namespace Ext::Gui {

class ImageFormat {
public:
	const Guid Guid;
	static ImageFormat Bmp, Emf, Exif, Gif, Icon, Jpeg, Png, Tiff, Wmf;

	ImageFormat(const GUID& guid)
		:	Guid(guid)
	{}
};

#if !UCFG_WCE
class ImageCodecInfo {
public:
	Guid FormatID,
		Clsid;

	static std::vector<ImageCodecInfo> GetImageEncoders();
};
#endif


class ImagingExc : public Exception {
public:
	ImagingExc(RCString s)
		:	Exception(ExtErr::Imaging, s)
	{
	}
};

class NoSuchEncoderExc : public ImagingExc {
	typedef ImagingExc base;
public:
	NoSuchEncoderExc()
		:	base("No such encoder")
	{}
};

class IconNotFoundExc : public ImagingExc {
	typedef ImagingExc base;
public:
	IconNotFoundExc()
		:	base("Icon not found")
	{}
};


#ifdef X_UCFG_WCE //!!!confilict

class Image : public CComPtr<IImage> {
public:
	static CComPtr<IImagingFactory> GetFactory();

	static Image FromFile(RCString filename);
	static Image FromStream(IStream *iStream);
	static Image FromBuffer(RCSpan mb, BufferDisposalFlag disposalFlag = BufferDisposalFlagNone);

	ImageInfo get_ImageInfo() {
		struct ImageInfo info;
		OleCheck(_self->GetImageInfo(&info));
		return info;
	}
	DEFPROP_GET(ImageInfo, ImageInfo);

	Size get_Size() {
		struct ImageInfo ii = ImageInfo;
		return Ext::Size(ii.Width, ii.Height);
	}
	DEFPROP_GET(Size, Size);

	class Size get_PhysicalDimension() {
		class Size size;
		OleCheck(_self->GetPhysicalDimension(&size));
		return size;
	}
	DEFPROP_GET(class Size, PhysicalDimension);

	ImageFormat get_RawFormat() { return ImageInfo.RawDataFormat; }
	DEFPROP_GET(ImageFormat, RawFormat);

	void Draw(HDC hdc, const RECT& dstRect, const RECT *srcRect = 0) {
		OleCheck(_self->Draw(hdc, &dstRect, srcRect));
	}

	Image GetThumbnail(int thumbWidth, int thumbHeight);
	void Encode(IImageEncoder *iEncoder);
	void Save(RCString filename, ImageFormat format);
	void Save(RCString filename) { Save(filename, RawFormat); }
};

class Bitmap : public CComPtr<IBitmapImage>, public Image {
public:
	typedef CComPtr<IBitmapImage> iface_base;

	Bitmap(Image image, PixelFormatID pixelFormat = PixelFormatDontCare);

	class Size get_Size();
	DEFPROP_GET(class Size, Size);

	BitmapData LockBits(PixelFormatID pixelFormat);
	void UnlockBits(const BitmapData& bd);

	void Save(Stream& stm);
};

class CBitmapLocker {
public:
	class Bitmap& Bitmap;
	class BitmapData BitmapData;

	CBitmapLocker(class Bitmap& bmp, PixelFormatID pixelFormat);
	~CBitmapLocker();
};

class BitmapInfoKeeper {
public:
	BITMAPINFO *m_p;
	size_t m_biSize;

	BitmapInfoKeeper(Bitmap bmp, PixelFormatID pixelFormat);
	~BitmapInfoKeeper() { delete[] (BYTE*)m_p; }
};
#endif // UCFG_WCE

class IconObj : public NonInterlockedObject {
	HICON m_hIcon;
	bool m_bNeedDestroy;

	IconObj(HICON hicon = 0, bool bNeedDestroy = false)
		: m_hIcon(hicon)
		, m_bNeedDestroy(bNeedDestroy)
	{
	}

public:
	~IconObj() {
		if (m_bNeedDestroy)
			Win32Check(::DestroyIcon(m_hIcon));
	}

	HICON Detach() {
		m_bNeedDestroy = false;
		return exchange(m_hIcon, nullptr);
	}

	friend class Icon;
	friend class ImageList;
};

class Icon : public Pimpl<IconObj> {
public:
	Icon()
	{}

	explicit Icon(RCString filename);

	operator HICON() const { return m_pimpl->m_hIcon; }

	HICON Detach() { return m_pimpl->Detach(); }

	HICON get_Handle() { return *this; }
	DEFPROP_GET(HICON, Handle);

	static Icon AFXAPI FromHandle(HICON hicon, bool bNeedDestroy = false);
	static Icon AFXAPI ExtractIcon(RCString filePath);
	static Icon AFXAPI ExtractSmallIcon(RCString filePath);
	static Icon AFXAPI ExtractAssociatedIcon(RCString filePath);
	static Icon AFXAPI GetShellIcon(RCString path, UINT flags);
	static Icon AFXAPI GetJumboIcon(RCString path, UINT flags);
	static Size AFXAPI GetShellIconSize(UINT uType);

	Size get_Size();
	DEFPROP_GET(Size, Size);

	int get_Width() { return Size.Width; }
	DEFPROP_GET(int, Width);

	int get_Height() { return Size.Height; }
	DEFPROP_GET(int, Height);

	Icon Clone();
private:
	friend class ImageList;
};

typedef Icon Image;

ENUM_CLASS(ColorDepth) {
	Depth4Bit = ILC_COLOR4,
	Depth8Bit = ILC_COLOR8,
	Depth16Bit = ILC_COLOR16,
	Depth24Bit = ILC_COLOR24,
	Depth32Bit = ILC_COLOR32
} END_ENUM_CLASS(ColorDepth);

class AFX_CLASS ImageList : public Object {
	Ext::Gui::ColorDepth m_colorDepth;
public:
	mutable HIMAGELIST m_hImageList;
	Size m_imageSize;

	ImageList();
	~ImageList();
	operator HIMAGELIST() const;

	Image GetIcon(int i, UINT flags = ILD_NORMAL) const;
	Image operator[](int i) const { return GetIcon(i); }

	HIMAGELIST GetSafeHandle() const {return this ? m_hImageList : 0;}
	void SetHandle(HANDLE h) {m_hImageList = (HIMAGELIST)h;}
	void Create(const CResID& resID, int cx, int nGrow, COLORREF crMask = CLR_DEFAULT);
	void Delete();
	void Attach(HIMAGELIST hImageList);
	HIMAGELIST Detach();
	static ImageList * AFXAPI FromHandle(HIMAGELIST h);
	static ImageList * AFXAPI FromHandlePermanent(HIMAGELIST h);

	int size() const { return ImageList_GetImageCount(*this); }

	int Add(HBITMAP hbm, COLORREF crMask) {
		return ImageList_AddMasked(*this, hbm, crMask);
	}

	int Add(HICON hicon) {
		return ImageList_AddIcon(*this, hicon);
	}

	Ext::Gui::ColorDepth get_ColorDepth() const { return m_colorDepth; }
	EXT_API void put_ColorDepth(Ext::Gui::ColorDepth v);
	DEFPROP(Ext::Gui::ColorDepth, ColorDepth);

	Size get_ImageSize() const;
	void put_ImageSize(const Size& size);
	DEFPROP(Size, ImageSize);
protected:
	void EnsureHandle() const;
};


} // Ext::Gui

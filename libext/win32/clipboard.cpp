#include <el/ext.h>

#include <ShlObj_core.h>
#include <el/libext/win32/clipboard.h>

#pragma warning(disable: 4073) // initializers put in library initialization area
#pragma init_seg(lib)

namespace Ext {

const String
	DataFormats::FileContents = CFSTR_FILECONTENTS
	, DataFormats::FileGroupDescriptior = CFSTR_FILEDESCRIPTOR
	, DataFormats::FileGroupDescriptiorA = CFSTR_FILEDESCRIPTORA
	, DataFormats::FileName = CFSTR_FILENAME
	, DataFormats::PasteSucceeded = CFSTR_PASTESUCCEEDED
	, DataFormats::LogicalPerformedDropEffect = CFSTR_LOGICALPERFORMEDDROPEFFECT
	, DataFormats::OleClipboardPersistOnFlush = "OleClipboardPersistOnFlush"
	, DataFormats::PerformedDropEffect = CFSTR_PERFORMEDDROPEFFECT
	, DataFormats::PreferredDropEffect = CFSTR_PREFERREDDROPEFFECT
	, DataFormats::ShellIdList = CFSTR_SHELLIDLIST
	, DataFormats::TargetClsid = CFSTR_TARGETCLSID;

DataFormats::Format DataFormats::GetFormat(const String& name) {
	uint16_t id = (uint16_t)Win32Check(::RegisterClipboardFormat(name));
	return Format(name, id);
}

DataFormats::Format DataFormats::GetFormat(CLIPFORMAT id) {
	TCHAR buf[256];
	Win32Check(::GetClipboardFormatName(id, buf, size(buf)));
	return Format(buf, id);
}

CComPtr<IDataObject> Clipboard::GetDataObject() {
	CComPtr<IDataObject> r;
	OleCheck(::OleGetClipboard(&r));
	return r;
}

void Clipboard::SetDataObject(IDataObject *pDataObject) {
	OleCheck(::OleSetClipboard(pDataObject));
}

} // Ext::

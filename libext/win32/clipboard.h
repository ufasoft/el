#pragma once

#include <atlbase.h>
#include <atlcom.h>


namespace Ext {

class DataFormats {
public:
	static const String
		FileContents
		, FileGroupDescriptior
		, FileGroupDescriptiorA
		, FileName
		, PasteSucceeded
		, LogicalPerformedDropEffect
		, OleClipboardPersistOnFlush
		, PerformedDropEffect
		, PreferredDropEffect
		, ShellIdList
		, TargetClsid;

	class Format {
	public:
		String Name;
		uint16_t Id;

		Format(const String& name, uint16_t id) {
			Name = name;
			Id = id;
		}
	};

	static Format GetFormat(const String& name);
	static Format GetFormat(CLIPFORMAT id);
};

class Clipboard {
public:
	static CComPtr<IDataObject> GetDataObject();
	static void SetDataObject(IDataObject *pDataObject);

	static void Clear() { SetDataObject(nullptr); }
	static bool IsCurrent(IDataObject* pDataObject) { return S_OK == OleCheck(::OleIsCurrentClipboard(pDataObject)); }
	static void Flush() { OleCheck(::OleFlushClipboard()); }
};



} // Ext::

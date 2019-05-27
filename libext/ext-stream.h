/*######   Copyright (c) 1997-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

namespace Ext {

ENUM_CLASS(SeekOrigin) {
	Begin,
	Current,
	End
} END_ENUM_CLASS(SeekOrigin);


class EXTCLASS Stream {
	typedef Stream class_type;
public:
	static const size_t DEFAULT_BUF_SIZE = 8192;

	virtual ~Stream() {}
	virtual void WriteBuffer(const void *buf, size_t count) { Throw(E_NOTIMPL); }

	template <typename T>
	void Write(span<T> s) { WriteBuffer(s.data(), s.size_bytes()); }

    void Write(Span s) { WriteBuffer(s.data(), s.size_bytes()); }

	virtual bool Eof() const;
	virtual size_t Read(void *buf, size_t count) const { Throw(E_NOTIMPL); }
	virtual void ReadBuffer(void *buf, size_t count) const;
	virtual int ReadByte() const;
	virtual int64_t Seek(int64_t offset, SeekOrigin origin) const { Throw(E_NOTIMPL); }		// mandatory to implement where put_Position implemented

	virtual void ReadBufferAhead(void *buf, size_t count) const { Throw(E_NOTIMPL); }
	virtual void Close() const {}
	virtual void Flush() {}

	virtual uint64_t get_Length() const {
		uint64_t curPos = Position;
		uint64_t endPos = Seek(0, SeekOrigin::End);
		put_Position(curPos);
		return endPos;
	}
	DEFPROP_VIRTUAL_GET_CONST(uint64_t, Length);

	virtual uint64_t get_Position() const {
		return Seek(0, SeekOrigin::Current);
	}
	virtual void put_Position(uint64_t pos) const {
		Seek(pos, SeekOrigin::Begin);
	}
	DEFPROP_VIRTUAL_CONST_CONST(uint64_t, Position);

	void CopyTo(Stream& dest, size_t bufsize = DEFAULT_BUF_SIZE) const;
protected:
	Stream() {}
};

class BufferedStream : public Stream {
	typedef Stream base;
public:
	Stream& UnderlyingStream;

	BufferedStream(Stream& stm, size_t bufferSize = DEFAULT_BUF_SIZE);

	~BufferedStream() {
		delete[] m_buf;
	}

	void Close() const override { UnderlyingStream.Close(); }
	void Flush() override { UnderlyingStream.Flush(); }
	bool Eof() const override { return m_cur == m_end && UnderlyingStream.Eof(); }
	uint64_t get_Length() const override { return UnderlyingStream.Length; }
	uint64_t get_Position() const override { return UnderlyingStream.Position - (m_end - m_cur); }

	int64_t Seek(int64_t offset, SeekOrigin origin) const override {
		m_cur = m_end = 0;
		return UnderlyingStream.Seek(offset, origin);
	}

	size_t Read(void *buf, size_t count) const override;
private:
	uint8_t *m_buf;
	size_t m_bufSize;
	mutable size_t m_cur, m_end;
};


const std::error_category& AFXAPI zlib_category();

ENUM_CLASS(CompressionMode) {
	Decompress, Compress
} END_ENUM_CLASS(CompressionMode);

class CompressStream : public Stream {
	typedef CompressStream class_type;
public:
	CompressionMode Mode;

	EXT_API CompressStream(Stream& stm, CompressionMode mode);
	~CompressStream();

	size_t Read(void *buf, size_t count) const override;
	void WriteBuffer(const void *buf, size_t count) override;
	bool Eof() const override;
	void SetByByteMode(bool v);
protected:
	mutable CBool m_bInited, m_bByByteMode;
	void *m_pimpl;
	mutable int m_bufpos;
	mutable std::vector<uint8_t> m_sbuf, m_dbuf;
	Stream& m_stmBase;

	virtual void InitImp() const;
private:
	void ReadPortion() const;
	void WritePortion(const void *ibuf, size_t count, int flush);
};

class GZipStream : public CompressStream {
	typedef CompressStream base;
public:
	GZipStream(Stream& stm, CompressionMode mode)
		:	base(stm, mode)
	{
	}
protected:
	void InitImp() const;
};



} // Ext::

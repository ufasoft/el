/*######   Copyright (c) 1997-2023 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>


namespace Ext {
using namespace std;

void Stream::Write(uint64_t pos, Span s) {
	Position = pos;
	Write(s);
}

bool Stream::Eof() const {
	Throw(E_NOTIMPL);
}

int Stream::ReadByte() const {
	uint8_t b;
	return Read(&b, 1) ? int(b) : -1;
}

void Stream::ReadExactly(void *buf, size_t count) const {
	uint8_t *p = (uint8_t *)buf;
	for (size_t cb; count; count -= cb, p += cb) {
		if (!(cb = Read(p, count)))
			Throw(ExtErr::EndOfStream);
	}
}

// Moore law buufer size
const size_t Stream::DEFAULT_BUF_SIZE = clamp(4096 * (Clock::now().Year - 2021), 4096, 1024 * 1024);

void Stream::CopyTo(Stream& dest, size_t bufsize) const {
	vector<uint8_t> buf(bufsize);
	for (size_t cb; cb = Read(buf.data(), buf.size());)
		dest.WriteBuffer(buf.data(), cb);
}

BufferedStream::BufferedStream(Stream& stm, size_t bufferSize)
	: UnderlyingStream(stm)
	, m_buf(new uint8_t[bufferSize])
	, m_bufSize(bufferSize)
	, m_cur(0)
	, m_end(0)
{}

size_t BufferedStream::Read(void *buf, size_t count) const {
	size_t cb = (min)(count, m_end - m_cur);
	memcpy(buf, m_buf + exchange(m_cur, m_cur + cb), cb);
	if (count -= cb) {
		buf = (uint8_t*)buf + cb;
		if (count >= m_bufSize)
			cb += UnderlyingStream.Read(buf, count);
		else {
			if (cb && m_bufSize < DEFAULT_BUF_SIZE)
				delete[] exchange(m_buf, new uint8_t[m_bufSize = DEFAULT_BUF_SIZE]);
			cb += (m_cur = (min)(count, m_end = UnderlyingStream.Read(m_buf, m_bufSize)));
			memcpy(buf, m_buf, m_cur);
		}
	}
	return cb;
}

int64_t MemStreamWithPosition::Seek(int64_t offset, SeekOrigin origin) const {
	switch (origin) {
	case SeekOrigin::Begin:
		put_Position(offset);
		break;
	case SeekOrigin::Current:
		put_Position(m_pos + offset);
		break;
	case SeekOrigin::End:
		put_Position(Length + offset);
		break;
	}
	return m_pos;
}

void CMemWriteStream::WriteBuffer(const void* buf, size_t count) {
	if (count > m_mb.size() - m_pos)
		Throw(ExtErr::EndOfStream);
	memcpy(m_mb.data() + m_pos, buf, count);
	m_pos += count;
}

MemoryStream::MemoryStream(size_t capacity)
	: m_blob(capacity, false)
{
}

size_t MemoryStream::Read(void* buf, size_t count) const {
	size_t r = std::min(count, size() - m_pos);
	if (r)
		memcpy(buf, m_blob.data() + m_pos, r);
	m_pos += r;
	return r;
}

void MemoryStream::WriteBuffer(const void *buf, size_t count) {
	auto nextPos = m_pos + count;
	if (nextPos > m_blob.size())
		m_blob.resize(max3(m_blob.size() * 2, nextPos, DEFAULT_CAPACITY), false);
	m_size = max(m_size, nextPos);
	memcpy(m_blob.data() + exchange(m_pos, nextPos), buf, count);
}

bool MemoryStream::Eof() const {
	return m_pos == size();
}

void MemoryStream::Reset(size_t capacity) {
	m_blob = CBlob(capacity, false);
	m_size = 0;
}

void CMemReadStream::ReadBufferAhead(void *buf, size_t count) const {
	if (count > m_mb.size() - m_pos)
		Throw(ExtErr::EndOfStream);
	if (buf)
		memcpy(buf, m_mb.data() + m_pos, count);
}

size_t CMemReadStream::Read(void *buf, size_t count) const {
	size_t r = std::min(count, m_mb.size() - m_pos);
	if (r)
		memcpy(buf, m_mb.data() + m_pos, r);
	m_pos += r;
	return r;
}

void CMemReadStream::ReadExactly(void *buf, size_t count) const {
	if (count > m_mb.size() - m_pos)
		Throw(ExtErr::EndOfStream);
	if (buf)
		memcpy(buf, m_mb.data() + m_pos, count);
	m_pos += count;
}

int CMemReadStream::ReadByte() const {
	return m_pos < m_mb.size() ? m_mb[m_pos++] : -1;
}

void AFXAPI ReadOneLineFromStream(const Stream& stm, String& beg, Stream *pDupStream) {
	const int MAX_LINE = 4096;
	char vec[MAX_LINE];
	char *p = vec;
	for (int i = 0; i < MAX_LINE; i++) { //!!!
		char ch;
		stm.ReadExactly(&ch, 1);
		if (pDupStream)
			pDupStream->WriteBuffer(&ch, 1);
		switch (ch) {
		case '\n':
			beg += String(vec, p - vec);
			return;
		default:
			*p++ = ch;
		case '\r':
			break;
		}
	}
	Throw(ExtErr::PROXY_VeryLongLine);
}

vector<String> AFXAPI ReadHttpHeader(const Stream& stm, Stream *pDupStream) {
	for (vector<String> vec; ;) {
		String line;
		ReadOneLineFromStream(stm, line, pDupStream);
		if (line.empty())
			return vec;
		vec.push_back(line);
	}
	/*!!!

	int i = beg.Length;
	char buf[256];
	if (i >= sizeof buf)
	Throw(E_PROXY_VeryLongLine);
	strcpy(buf, beg);
	bool b = false;
	for (; i<sizeof buf; i++)
	{
	stm.ReadBuffer(buf+i, 1);
	if (buf[i] == '\n')
	if (b)
	break;
	else
	b = true;
	if (buf[i] != '\n' && buf[i] != '\r')
	b = false;
	}
	beg = String(buf, i+1);*/
}


} // Ext::

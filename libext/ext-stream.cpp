/*######   Copyright (c) 1997-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>


namespace Ext {
using namespace std;

bool Stream::Eof() const {
	Throw(E_NOTIMPL);
}

int Stream::ReadByte() const {
	uint8_t b;
	return Read(&b, 1) ? int(b) : -1;
}

void Stream::ReadBuffer(void *buf, size_t count) const {
	uint8_t *p = (uint8_t *)buf;
	for (size_t cb; count; count-=cb, p+=cb) {
		if (!(cb = Read(p, count)))
			Throw(ExtErr::EndOfStream);
	}
}

void Stream::CopyTo(Stream& dest, size_t bufsize) const {
	vector<uint8_t> buf(bufsize);
	for (size_t cb; cb = Read(&buf[0], buf.size());)
		dest.WriteBuffer(&buf[0], cb);
}

size_t BufferedStream::Read(void *buf, size_t count) const {
	size_t cb = (min)(count, m_end - m_cur);
	memcpy(buf, m_buf.constData() + exchange(m_cur, m_cur + cb), cb);
	if (count -= cb) {
		buf = (uint8_t*)buf + cb;
		if (count >= m_buf.Size)
			cb += Stm.Read(buf, count);
		else {
			cb += (m_cur = (min)(count, m_end = Stm.Read(m_buf.data(), m_buf.Size)));
			memcpy(buf, m_buf.constData(), m_cur);
		}
	}
	return cb;
}

void MemoryStream::WriteBuffer(const void *buf, size_t count) {
	auto nextPos = m_pos + count;
	if (nextPos > m_capacity) {
		m_capacity = max3(m_capacity * 2, nextPos, DEFAULT_CAPACITY);
#if UCFG_HAS_REALLOC
		m_data = (uint8_t*)Ext::Realloc(m_data, m_capacity);
#else
		free(exchange(m_data, (uint8_t*)memcpy(Ext::Malloc(m_capacity), data, m_size)));
#endif
	}
	m_size = max(m_size, nextPos);
	memcpy(&m_data[exchange(m_pos, nextPos)], buf, count);
}

bool MemoryStream::Eof() const {
	Throw(E_NOTIMPL);
}

void MemoryStream::Reset(size_t capacity) {
	free(exchange(m_data, nullptr));
	m_data = (uint8_t *)Ext::Malloc(m_capacity = capacity);
	m_size = 0;
}

void CMemReadStream::ReadBufferAhead(void *buf, size_t count) const {
	if (count > m_mb.Size - m_pos)
		Throw(ExtErr::EndOfStream);
	if (buf)
		memcpy(buf, m_mb.P + m_pos, count);
}

size_t CMemReadStream::Read(void *buf, size_t count) const {
	size_t r = std::min(count, m_mb.Size-m_pos);
	if (r)
		memcpy(buf, m_mb.P+m_pos, r);
	m_pos += r;
	return r;
}

void CMemReadStream::ReadBuffer(void *buf, size_t count) const {
	if (count > m_mb.Size - m_pos)
		Throw(ExtErr::EndOfStream);
	if (buf)
		memcpy(buf, m_mb.P + m_pos, count);
	m_pos += count;
}

int CMemReadStream::ReadByte() const {
	return m_pos < m_mb.Size ? m_mb.P[m_pos++] : -1;
}

void AFXAPI ReadOneLineFromStream(const Stream& stm, String& beg, Stream *pDupStream) {
	const int MAX_LINE = 4096;
	char vec[MAX_LINE];
	char *p = vec;
	for (int i=0; i<MAX_LINE; i++) {		//!!!
		char ch;
		stm.ReadBuffer(&ch, 1);
		if (pDupStream)
			pDupStream->WriteBuffer(&ch, 1);
		switch (ch) {
		case '\n':
			beg += String(vec, p-vec);
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


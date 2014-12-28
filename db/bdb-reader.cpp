/*######     Copyright (c) 1997-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com #########################################################################################################
#                                                                                                                                                                                                                                            #
# This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation;  either version 3, or (at your option) any later version.          #
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.   #
# You should have received a copy of the GNU General Public License along with this program; If not, see <http://www.gnu.org/licenses/>                                                                                                      #
############################################################################################################################################################################################################################################*/

#include <el/ext.h>

#include "bdb-reader.h"

namespace Ext { namespace DB {

#define	DB_BTREEMAGIC	0x053162
#define	P_LBTREE	5

struct BTreeMainPage {
	uint64_t Lsn;
	uint32_t PgNo;
	uint32_t Magic;
	uint32_t Version;
	uint32_t PageSize;
	
	uint64_t m_reserv;
	uint32_t LastPage;
};


BdbReader::BdbReader(RCString path)
	:	m_fs(path, FileMode::Open, FileAccess::Read)
{
	BTreeMainPage mainPage;
	m_fs.ReadBuffer(&mainPage, sizeof mainPage);
	if (mainPage.Magic != DB_BTREEMAGIC)
		Throw(E_FAIL);
	m_lastPage = mainPage.LastPage;
	m_curPage.Size = mainPage.PageSize;

	LoadNextPage(1);
}

bool BdbReader::Read(Blob& key, Blob& value) {
LAB_AGAIN:
	while (true) {
		if (m_idx < 0)
			return false;
		if (m_idx < m_entries)
			break;
		if (!LoadNextPage(m_pgno+1))
			return false;
	}
	int keyIdx = *((uint16_t*)(m_curPage.data()+26)+m_idx*2),
		valIdx = *((uint16_t*)(m_curPage.data()+26)+m_idx*2+1);
	if (keyIdx+3 > m_curPage.Size || valIdx+3 > m_curPage.Size) {
		m_idx = m_entries;
		goto LAB_AGAIN;
	}
	++m_idx;
	int keyLen = *(uint16_t*)(m_curPage.data()+keyIdx),
		valLen = *(uint16_t*)(m_curPage.data()+valIdx);
	if (keyIdx+keyLen > m_curPage.Size || valIdx+valLen > m_curPage.Size) {
		m_idx = m_entries;
		goto LAB_AGAIN;
	}
	if (*(m_curPage.data()+keyIdx+2) != 1 || *(m_curPage.data()+valIdx+2) != 1)
		goto LAB_AGAIN;
	key = Blob(m_curPage.data()+keyIdx+3, keyLen);
	value = Blob(m_curPage.data()+valIdx+3, valLen);
	return true;
}

bool BdbReader::LoadNextPage(int pgno) {
	while (true) {
		if (pgno > m_lastPage) {
			m_idx = -1;
			return false;
		}
		m_pgno = pgno;
		m_fs.Position = m_pgno*m_curPage.Size;
		m_fs.ReadBuffer(m_curPage.data(), m_curPage.Size);
		if (m_curPage.data()[25] == P_LBTREE)
			break;
		++pgno;
	}
	m_idx = 0;
	m_entries = *(uint16_t*)(m_curPage.data()+20);
	return true;
}


}} // Ext::DB::

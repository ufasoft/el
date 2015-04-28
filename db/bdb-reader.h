/*######   Copyright (c) 2013-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

namespace Ext { namespace DB {

class BdbReader {
public:
	FileStream m_fs;

	BdbReader(const path& p);	
	bool Read(Blob& key, Blob& value);
protected:
	Blob m_curPage;
	int m_pgno;
	int m_idx;
	int m_lastPage;
	int m_entries;

	bool LoadNextPage(int pgno);
};



}} // Ext::DB::

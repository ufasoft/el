#include <el/ext.h>

#include "codecvt"

namespace ExtSTL  {
using namespace Ext;

codecvt_base::result codecvt_utf8_utf16<wchar_t>::do_in(codecvt_utf8_utf16<wchar_t>::state_type& s, const extern_type *fb, const extern_type *fe, const extern_type *&fn, intern_type *tb, intern_type *te, intern_type *&tn) const {
	for (fn=fb, tn=tb; fn!=fe && tn!=te;) {
		byte by = (byte)*fn;
		if (0x80<=by && by<0xC0) {
			if (MbstateRef(s) <= 1)
				++fn;
			return error;
		}
		if (MbstateRef(s) > 1) {
			++fn;
			*tn++ = extern_type(exchange(MbstateRef(s), 1) | by & 0x3F);
		} else {
			pair<int, int> chNext = by<0x80 ? make_pair(int(by), 0)
				: by<0xE0 ? make_pair(by & 0x1F, 1)
				: by<0xF0 ? make_pair(by & 0x0F, 2)
				: by<0xF8 ? make_pair(by & 7, 3)
				: by<0xFC ? make_pair(by & 3, 4)
				: make_pair(by & 3, 5);
			int ch = chNext.first;
			bool bTwoWchars = int(chNext.second > 2);
			fb = fn;
			if (0 == chNext.second)
				++fn;
			else if (fe-fn < 1 + chNext.second - int(bTwoWchars))
				break;
			else {
				for (++fn; int(bTwoWchars) - chNext.second--;) {
					if (!(0x80<=(by = *fn++) && by<0xC0))
						return error;
					ch = ch<<6 | by & 0x3F;
				}
			}
			ch <<= 6 * int(bTwoWchars);
			if (ch > Maxcode)
				return error;
			if (ch > 0xFFFF) {
				*tn++ = (intern_type)uint16_t(0xD800 | (ch>>10) - 0x40);
				MbstateRef(s) = 0xDC00 | ch & 0x3FF;
			} else {
				if (bTwoWchars) {
					if (fn == fe)
						break;
					if (!(0x80<=(by = *fn++) && by<0xC0))
						return error;
					ch |= by & 0x3F;
				}
				if (!MbstateRef(s))
					MbstateRef(s) = 1;
				*tn++ = (intern_type)ch;
			}
		}
	}
	return fn!=fb ? ok : partial;
}

codecvt_base::result codecvt_utf8_utf16<wchar_t>::do_out(codecvt_utf8_utf16<wchar_t>::state_type& s, const intern_type *fb, const intern_type *fe, const intern_type *&fn, extern_type *tb, extern_type *te, extern_type *&tn) const {
	for (fn=fb, tn=tb; fn!=fe && tn!=te; ++fn) {
		uint16_t ch1 = *(uint16_t*)fn;
		bool bSave = false;
		int ch;
		if (MbstateRef(s) <= 1)
			ch = (bSave = 0xD800<=ch1 && ch1<0xDC00) ? (ch1 - 0xD800 + 0x40)<<10 : ch1;
		else if (!(0xDC00<=ch1 && ch1<0xE000))
			return error;
		else
			ch = (MbstateRef(s) << 10) | (ch1 - 0xDC00);

		pair<byte, int> pp = ch<0x80 ? make_pair(byte(ch), 0)
			: ch<0x800 ? make_pair(byte(0xC0 | ch>>6), 1)
			: ch<0x10000 ? make_pair(byte(0xE0 | ch>>12), 2)
			: make_pair(byte(0xF0 | ch>>18), 3);

		int n = pp.second<3 ? pp.second+1
			: bSave ? 1 : 3;
		if (te-tn < n)
			break;
		if (bSave || pp.second<3)
			*tn++ = pp.first;
		while (pp.second--)
			*tn++ = extern_type((ch >> 6*pp.second) & 0x3F | 0x80);
		MbstateRef(s) = bSave ? ch>>10 : 1;
	}
	return fn!=fb ? ok : partial;
}

int codecvt_utf8_utf16<wchar_t>::do_length(codecvt_utf8_utf16<wchar_t>::state_type& s, const extern_type *fb, const extern_type *fe, size_t count) const noexcept {
	int r = 0;
	for (state_type state=s; r<count && fb!=fe;) {
		const extern_type *fn;
		intern_type ch, *tn;
		switch (do_in(state, fb, fe, fn, &ch, &ch+1, tn)) {
		case noconv:
			return r + int(fe-fb);
		case ok:
			r += int(tn-&ch);
			fb = fn;
			continue;
		}
		break;
	}
	return r;
}

}  // ExtSTL::



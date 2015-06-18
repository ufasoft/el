/*######   Copyright (c) 2014-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>


#include "mail-address.h"

namespace Ext { namespace Inet {

static regex s_reMailAddress("([A-Z0-9._%+-]+)@([A-Z0-9.-]+\\.[A-Z]{2,4})", regex::icase);

MailAddress::MailAddress(RCString address) {
	cmatch m;
	if (!regex_match(address.c_str(), m, s_reMailAddress))
		Throw(errc::invalid_argument);
	m_user = m[1];
	m_host = m[2];
}

void SendEmail(RCString recipient, RCString subj, RCString msg, RCString from) {
	TRC(2, recipient << " " << subj);

	ostringstream os;
#if UCFG_USE_POSIX
	POpen popen(EXT_STR("ssmtp " << recipient), "w");
	fprintf(popen, "Subject: %s\n", subj.c_str());
	if (!from.empty())
		fprintf(popen, "From: %s\n", from.c_str());
	fprintf(popen, "\n");	
#else
	os << "email -s \"" << subj << "\"";
	if (!from.empty())
		os << " -f" << from;
	os << " " << recipient;	
	
	POpen popen(os.str(), "w");
#endif

	fprintf(popen, "%s", msg.c_str());
	popen.Wait();

	//!!!R	ProcessStartInfo psi("email", EXT_STR("-s \"" << subj << "\" " << recipient));
	//	psi.RedirectStandardInput = true;
	//	StreamWriter(Process::Start(psi).StandardInput()).WriteLine(msg);
}



}} // Ext::Inet::



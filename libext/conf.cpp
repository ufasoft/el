/*######   Copyright (c) 2019      Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include <el/ext.h>

#include "conf.h"

namespace Ext {

void Conf::AssignOption(RCString key, RCString val) {
	auto it = m_nameToBinder.find(key);
	if (it == m_nameToBinder.end())
		throw new Exception(E_FAIL, "Invalid config key: " + key);
	it->second->Parse(val);
}

void Conf::Load(istream& istm) {
	static regex
		reEmptyOrComment("\\s*(#.*)?"),
		reKV("\\s*([_A-Za-z0-9]+)\\s*=\\s*(\\S+)\\s*(#.*)?");
	for (string line; getline(istm, line);) {
		smatch m;
		if (regex_match(line, m, reKV)) {
			AssignOption(m[1], m[2]);
		} else if (!regex_match(line, m, reEmptyOrComment)) {
			Throw(E_FAIL);
		}
	}
}

void Conf::SaveSample(ostream& os) {
	vector<String> keys;
	for (auto& kv : m_nameToBinder)
		keys.push_back(kv.first);
	sort(keys.begin(), keys.end());
	for (auto& key : keys) {
		Binder& binder = *m_nameToBinder[key];
		os << "# " << key << " =  " << binder.ToString();
		if (binder.Help != nullptr)
			os << "     # " << binder.Help;
		os << "\n\n";
	}
}

void Conf::BoolBinder::Parse(RCString s) {
	if (s == "1" || s == "true")
		m_refVal = true;
	else if (s == "0" || s == "false")
		m_refVal = false;
	else
		Throw(E_INVALIDARG);
}

void Conf::Bind(String name, bool& val, bool def, String help) {
	ptr<Binder> binder = new BoolBinder(val, def);
	binder->Help = help;
	m_nameToBinder[name.ToLower()] = binder;
}

void Conf::Bind(String name, int& val, int def, String help) {
	ptr<Binder> binder = new IntBinder(val, def);
	binder->Help = help;
	m_nameToBinder[name.ToLower()] = binder;
}

void Conf::Bind(String name, String& val, RCString def, String help) {
	ptr<Binder> binder = new StringBinder(val, def);
	binder->Help = help;
	m_nameToBinder[name.ToLower()] = binder;
}

void Conf::Bind(String name, vector<String>& val, String help) {
	ptr<Binder> binder = new StringVectorBinder(val);
	binder->Help = help;
	m_nameToBinder[name.ToLower()] = binder;
}



} // Ext::


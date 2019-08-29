/*######   Copyright (c) 2017-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

namespace Ext {

template <typename T>
class MeanCalculator {
public:
	struct Item {
		DateTime Timestamp;
		T Value;
	};

	std::deque<Item> m_deq;
	size_t MaxSize;

	MeanCalculator(size_t maxSize = 10)
		: MaxSize(maxSize)
	{
	}

	void AddValue(const T& v) {
		if (m_deq.size() >= MaxSize)
			m_deq.pop_front();
		Item item = { Clock::now(), v };
		m_deq.push_back(item);
	}

	double FindMeanValue() const {
		if (m_deq.empty())
			return T();
		double sec = duration_cast<milliseconds>(Clock::now() - m_deq.front().Timestamp).count() / 1000.;
		if (sec < 0.1)
			return 0;
		T sum = T();
		for (typename std::deque<Item>::const_iterator it = m_deq.begin(); it != m_deq.end(); ++it)
			sum += it->Value;
		return sum / sec;
	}
};





} // Ext::

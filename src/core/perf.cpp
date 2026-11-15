#include "core/perf.h"

#include <algorithm>

namespace lv::core {

Perf &Perf::instance() {
	static Perf perf;
	return perf;
}

void Perf::record(const std::string &p_name, double p_ms) {
	Stat &s = stats_[p_name];
	s.calls++;
	s.total_ms += p_ms;
	s.max_ms = std::max(s.max_ms, p_ms);
	s.last_ms = p_ms;
}

void Perf::roll_window() {
	for (auto &entry : stats_) {
		Stat &s = entry.second;
		s.window_calls = s.calls;
		s.window_total_ms = s.total_ms;
		s.window_max_ms = s.max_ms;
		s.calls = 0;
		s.total_ms = 0.0;
		s.max_ms = 0.0;
	}
}

void Perf::clear() {
	stats_.clear();
}

} // namespace lv::core

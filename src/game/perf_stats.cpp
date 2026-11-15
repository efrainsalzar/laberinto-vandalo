#include "game/perf_stats.h"

#include "core/perf.h"

#include <godot_cpp/core/class_db.hpp>

void PerfStats::_bind_methods() {
	ClassDB::bind_static_method("PerfStats", D_METHOD("snapshot"), &PerfStats::snapshot);
	ClassDB::bind_static_method("PerfStats", D_METHOD("roll_window"), &PerfStats::roll_window);
	ClassDB::bind_static_method("PerfStats", D_METHOD("clear"), &PerfStats::clear);
	ClassDB::bind_static_method("PerfStats", D_METHOD("take_frame"), &PerfStats::take_frame);
}

Dictionary PerfStats::snapshot() {
	Dictionary result;
	for (const auto &entry : lv::core::Perf::instance().stats()) {
		const lv::core::Perf::Stat &s = entry.second;
		Dictionary d;
		d["calls"] = s.window_calls;
		d["total_ms"] = s.window_total_ms;
		d["max_ms"] = s.window_max_ms;
		d["last_ms"] = s.last_ms;
		result[String(entry.first.c_str())] = d;
	}
	return result;
}

void PerfStats::roll_window() {
	lv::core::Perf::instance().roll_window();
}

void PerfStats::clear() {
	lv::core::Perf::instance().clear();
}

Dictionary PerfStats::take_frame() {
	Dictionary result;
	for (const auto &entry : lv::core::Perf::instance().take_frame()) {
		result[String(entry.first.c_str())] = entry.second;
	}
	return result;
}
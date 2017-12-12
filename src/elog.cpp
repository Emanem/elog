/*
*	elog (C) 2017 E. Oriani, ema <AT> fastwebnet <DOT> it
*
*	This file is part of elog.
*
*	elog is free software: you can redistribute it and/or modify
*	it under the terms of the GNU Lesser General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	elog is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with elog.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "elog.h"
#include <memory>
#include <mutex>
#include <fstream>
#include <ctime>
#include <algorithm>
#include <map>
#include <cstring>
#include <dirent.h>
#include <regex>

namespace {
	std::atomic<bool>		elog_stop(false);
	std::mutex			elog_th_mtx;
	std::shared_ptr<std::thread>	elog_th_stream;
	std::shared_ptr<std::ofstream>	elog_ostr;
	std::string			elog_fname;

	const char* get_level_str(const uint8_t level) {
		switch(level) {
		case elog::level_debug:
			return "DEBUG";
		case elog::level_info:
			return "INFO";
		case elog::level_warning:
			return "WARN";
		case elog::level_error:
			return "ERR";
		case elog::level_fatal:
			return "FATAL";
		default:
			break;
		};
		return "UNKNOWN";
	}

	// writing to stream
	template<typename T>
	void to_stream_el(std::ostream& ostr, const uint8_t*& p) {
		ostr << *reinterpret_cast<const T*>(p);
		p += sizeof(T);
	}

	// roll the log files
	void roll_logs(void) {
		// get directories entries
		const char*			f_sep = std::strrchr(elog_fname.c_str(), '/');
		// if we can find this char, then this is going to be the directory
		const std::string		dir_name = (f_sep) ? std::string(elog_fname.c_str(), f_sep) : ".",
						file_name = (f_sep) ? std::string(f_sep+1) : elog_fname;
		DIR 				*dir_p = 0;
		struct dirent 			*ent_p = 0;
		std::map<long, std::string>	logs_refs;
		if((dir_p = opendir(dir_name.c_str()))) {
			// escape the name string for regex
			const std::regex	esc("[.^$|()\\[\\]{}*+?\\\\]");
			const std::string	rep("\\&"),
						f_regex_expr = std::regex_replace(file_name, esc, rep, std::regex_constants::match_default | std::regex_constants::format_sed) + "\\.([0-9]+)";
			const std::regex	f_regex(f_regex_expr);
			while((ent_p = readdir(dir_p))) {
				// skip directories
				if(ent_p->d_type == DT_DIR) continue;
				std::smatch		m;
				const std::string	cur_f_name(ent_p->d_name);
				if(std::regex_match(cur_f_name, m, f_regex)) {
					// given we're asking for a single match, we need to
					// have 2 entries
					if(m.size() != 2) continue;
					const auto 	index = std::atol(std::string(m[1].first, m[2].second).c_str());
					logs_refs[index] = dir_name + '/' + cur_f_name;
				}
		}
			closedir(dir_p);
		} else {
			throw elog::exception(std::string("Can't open log directory: ") + dir_name);
		}
		// if we got all, perform file rolling then
		for(auto i = logs_refs.rbegin(); i != logs_refs.rend(); ++i) {
			const std::string	tgt_filename = elog_fname + "." + std::to_string(i->first+1);
			if(std::rename(i->second.c_str(), tgt_filename.c_str()))
				throw elog::exception("Can't rename log file '" + i->second + "' to '" + tgt_filename + "'");
		}
		// close and roll current file, then re-open
		elog_ostr->close();
		const std::string	tgt_filename = elog_fname + ".0";
		if(std::rename(elog_fname.c_str(), tgt_filename.c_str()))
			throw elog::exception("Can't rename log file '" + elog_fname + "' to '" + tgt_filename + "'");
		elog_ostr->open(elog_fname);
		if(!elog_ostr->good())
			throw elog::exception("Can't reopen log file '" + elog_fname + "'");
	}
}

std::condition_variable	elog::cv_notify_log;

void elog::entry::to_stream(std::ostream& ostr) const {
	// start printing out the timestamp in ISO format
	// and the thread_id
	using namespace std::chrono;
	const auto	tp_tm = time_point_cast<system_clock::duration>(tp);
	const auto	tm_t = system_clock::to_time_t(tp_tm);
	struct tm	res = {0};
	localtime_r(&tm_t, &res);
	char		tm_fmt[32],
			tm_buf[32];
	std::sprintf(tm_fmt, "%%Y-%%m-%%dT%%H:%%M:%%S.%03i", static_cast<int>(duration_cast<milliseconds>(tp.time_since_epoch()).count()%1000));
	std::strftime(tm_buf, sizeof(tm_buf), tm_fmt, &res);

	const uint8_t	*lcl_type = typelist,
			*lcl_buf = buffer;
	// first preamble
	ostr << tm_buf << " [" << th_id << "](" << get_level_str(*lcl_buf++) << ") ";
	// proceed with each type
	for( ; lcl_type != cur_type; ++lcl_type) {
		switch(*lcl_type) {
		case t_char:
			to_stream_el<char>(ostr, lcl_buf);
			break;
		case t_uchar:
			to_stream_el<unsigned char>(ostr, lcl_buf);
			break;
		case t_short:
			to_stream_el<short>(ostr, lcl_buf);
			break;
		case t_ushort:
			to_stream_el<unsigned short>(ostr, lcl_buf);
			break;
		case t_int:
			to_stream_el<int>(ostr, lcl_buf);
			break;
		case t_uint:
			to_stream_el<unsigned int>(ostr, lcl_buf);
			break;
		case t_float:
			to_stream_el<float>(ostr, lcl_buf);
			break;
		case t_double:
			to_stream_el<double>(ostr, lcl_buf);
			break;
		case t_long:
			to_stream_el<long>(ostr, lcl_buf);
			break;
		case t_ulong:
			to_stream_el<unsigned long>(ostr, lcl_buf);
			break;
		default: // strings
			if((*lcl_type) & t_str) {
				const int	sz = ((*lcl_type)&t_strmask);
				ostr.write(reinterpret_cast<const char*>(lcl_buf), sz);
				lcl_buf += sz;
			}
			break;
		}
	}
#ifdef PERF_TEST
	ostr << "\t(t:" << tries << ')' << std::endl;
#else
	ostr << std::endl;
#endif //PERF_TEST
}

elog::exception::exception(const char* e): std::runtime_error(e) {
}

elog::exception::exception(const std::string& e): std::runtime_error(e) {
}

elog::logger::~logger() {
	cleanup();
}

void elog::logger::init(const char* fname, const bool s_ordering, const size_t e_sz) {
	// check is not being already initialized...
	size_t	cur_status = s_not_init;
	if(!status.compare_exchange_strong(cur_status, s_start_init))
		return;
	// proceed with initialization
	entry_sz = e_sz;
	entries = new entry[entry_sz];
	elog_stop = false;
	elog_ostr = std::shared_ptr<std::ofstream>(new std::ofstream(fname));
	// use locally defined lambdas to manage log printing
	// much more efficient than a std::function<void (void)> which
	// basically implies a virtual call
	if(!s_ordering) {
		elog_th_stream = std::shared_ptr<std::thread>(new std::thread(
			[this]() -> void {
				auto l_print_unordered = [&]() {
					//scan through all avaliable logs and print!
					for(size_t i = 0; i < entry_sz; ++i) {
						if(!entries[i].is_filled()) continue;
						// otherwise print and reset it
						entries[i].to_stream(*elog_ostr);
						entries[i].reset();
						entry_hint = i;
					}
				};
				// main loop
				while(!elog_stop) {
					// wait for a notification
					std::unique_lock<std::mutex> l_(elog_th_mtx);
					cv_notify_log.wait_for(l_, std::chrono::milliseconds(250));
					l_print_unordered();
				}
				l_print_unordered();
			}
		));
	} else {
		elog_th_stream = std::shared_ptr<std::thread>(new std::thread(
			[this]() -> void {
				entry*	p_entries[entry_sz];
				auto l_print_ordered = [&]() {
					// get current timepoint
					const auto	log_tp = std::chrono::high_resolution_clock::now();
					// then scan through all avaliable logs and select them!
					size_t		selected_entries = 0;
					for(size_t i = 0; i < entry_sz; ++i) {
						if(!entries[i].is_filled()) continue;
						// ensure tp <= log_tp...
						if(entries[i].tp > log_tp) continue;
						p_entries[selected_entries++] = &entries[i];
						entry_hint = i;
					}
					// now sort all ptrs to selected entries
					std::sort(p_entries, p_entries+selected_entries, [](entry* &lhs, entry* &rhs) -> bool { return lhs->tp < rhs->tp; });
					// then print and free each one...
					for(size_t i = 0; i < selected_entries; ++i) {
						// otherwise print and reset it
						p_entries[i]->to_stream(*elog_ostr);
						p_entries[i]->reset();
					}
				};
				// main loop
				while(!elog_stop) {
					// wait for a notification
					std::unique_lock<std::mutex> l_(elog_th_mtx);
					cv_notify_log.wait_for(l_, std::chrono::milliseconds(250));
					l_print_ordered();
				}
				l_print_ordered();
			}
		));
	}
	elog_fname = fname;
	status.store(s_init, std::memory_order_release);
}

void elog::logger::cleanup(void) {
	// check status is initialized
	size_t	cur_status = s_init;
	if(!status.compare_exchange_strong(cur_status, s_start_clean))
		return;
	// do the cleanup - note, we don't need to do one last round of log
	// saving because it's done in the body of the thread...
	elog_stop = true;
	elog_th_stream->join();
	delete [] entries;
	elog_ostr.reset();
	status.store(s_not_init, std::memory_order_release);
}


#include "elog.h"
#include <thread>
#include <memory>
#include <mutex>
#include <fstream>
#include <chrono>

namespace {
	std::atomic<bool>		elog_stop(false);
	std::mutex			elog_th_mtx;
	std::shared_ptr<std::thread>	elog_th_stream;
	std::shared_ptr<std::ofstream>	elog_ostr;
}

std::condition_variable	elog::cv_notify_log;

void elog::entry::to_stream(std::ostream& ostr) const {
	const uint8_t	*lcl_type = typelist,
			*lcl_buf = buffer;
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
				// this needs optimization ofc...
				for(int i = 0; i < sz; i++)
					ostr.write(reinterpret_cast<const char*>(lcl_buf++), 1);
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

elog::logger::~logger() {
	if(is_init) cleanup();
}

elog::logger& elog::logger::instance(void) {
	static logger	l;
	return l;
}

void elog::logger::init(const char* fname, const size_t e_sz) {
	if(is_init) throw std::runtime_error("elog already initialized");
	entry_sz = e_sz;
	entries = new entry[entry_sz];
	elog_stop = false;
	elog_ostr = std::shared_ptr<std::ofstream>(new std::ofstream(fname));
	elog_th_stream = std::shared_ptr<std::thread>(new std::thread(
		[this]() -> void {
			while(!elog_stop) {
				// wait for a notification
				std::unique_lock<std::mutex> l_(elog_th_mtx);
				cv_notify_log.wait_for(l_, std::chrono::milliseconds(250));
				// then scan through all avaliable logs and print!
				for(size_t i = 0; i < entry_sz; ++i) {
					if(!entries[i].is_filled()) continue;
					// otherwise print and reset it
					entries[i].to_stream(*elog_ostr);
					entries[i].reset();
					entry_hint = i;
				}
			}
		}
	));
	is_init.store(true, std::memory_order_release);
}

void elog::logger::cleanup(void) {
	if(!is_init) throw std::runtime_error("elog not initialized");
	elog_stop = true;
	elog_th_stream->join();
	// one last scan through all avaliable logs and print!
	for(size_t i = 0; i < entry_sz; ++i) {
		if(!entries[i].is_filled()) continue;
		// otherwise print and reset it
		entries[i].to_stream(*elog_ostr);
		entries[i].reset();
	}
	delete [] entries;
	elog_ostr.reset();
	is_init.store(false, std::memory_order_release);
}

elog::entry* elog::logger::get_entry(void) {
	if(!is_init) throw std::runtime_error("elog not initialized");
#ifdef PERF_TEST
	size_t	lcl_tries = 0;
#endif //PERF_TEST
	while(true) {
		const size_t	cur_hint = entry_hint;
		for(size_t i = 0; i < entry_sz; ++i) {
			const size_t	cur_entry = (i+cur_hint)%entry_sz;
			if(entries[cur_entry].try_assign()) {
				entry_hint = (cur_entry+1)%entry_sz;
#ifdef PERF_TEST
				lcl_tries += i;
				entries[cur_entry].tries = lcl_tries;
#endif //#ifdef PERF_TEST
				return &entries[cur_entry];
			}
		};
#ifdef PERF_TEST
		lcl_tries += entry_sz;
#endif //PERF_TEST
		std::this_thread::yield();
	}
}


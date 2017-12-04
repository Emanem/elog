#ifndef _ELOG_H_
#define _ELOG_H_

#define PERF_TEST

#include <type_traits>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <chrono>
#include <thread>

namespace elog {

	extern std::condition_variable	cv_notify_log;

	class entry {
		std::atomic<size_t>	status;
		std::chrono::time_point<std::chrono::high_resolution_clock>	tp;
		std::thread::id		th_id;
		uint8_t			*cur_type,
					*cur_buf,
					typelist[16],
					buffer[512 - 
						(sizeof(status)+
						sizeof(tp)+
						sizeof(th_id)+
#ifdef PERF_TEST
						sizeof(size_t)+
#endif //PERF_TEST
						sizeof(cur_type)+
						sizeof(cur_buf)+
						sizeof(typelist))]; //make sure whole structure is 512b

		static const size_t	s_free = 0,
					s_assigned = 1,
					s_filled = 2;

		static const uint8_t	t_char = 0x01,
					t_uchar = 0x02,
					t_short = 0x03,
					t_ushort = 0x04,
					t_int = 0x05,
					t_uint = 0x06,
					t_float = 0x07,
					t_double = 0x08,
					t_long = 0x09,
					t_ulong = 0x0A,
					t_str = 0x80,
					t_strmask = 0x7F;
		
		uint8_t get_type(const char& c) {
			return t_char;
		}

		uint8_t get_type(const unsigned char& c) {
			return t_uchar;
		}

		uint8_t get_type(const short& c) {
			return t_short;
		}

		uint8_t get_type(const unsigned short& c) {
			return t_ushort;
		}

		uint8_t get_type(const int& c) {
			return t_int;
		}

		uint8_t get_type(const unsigned int& c) {
			return t_uint;
		}

		uint8_t get_type(const long& c) {
			return t_long;
		}

		uint8_t get_type(const unsigned long& c) {
			return t_ulong;
		}

		uint8_t get_type(const float& c) {
			return t_float;
		}

		uint8_t get_type(const double& c) {
			return t_double;
		}

		void add(const char i[]) {
			if((cur_type - typelist) == sizeof(typelist)) return;
			static const int	MAX_LEN=127;
			const char*		p = i;
			int			cnt = 0;
			while((*p) && (cnt++ < MAX_LEN)) {
				if((cur_buf - buffer) >= static_cast<int>(sizeof(buffer))) break;
				*cur_buf++ = *p++;
			}
			*cur_type++ = t_str | cnt;
		}

		template<typename T>
		void add(const T& t) {
			static_assert(std::is_fundamental<T>::value, "Unsupported type");
			if((cur_type - typelist) == sizeof(typelist)) return;
			if((sizeof(buffer) - (cur_buf - buffer)) < static_cast<int>(sizeof(T))) return;
			*cur_type++ = get_type(t);
			*reinterpret_cast<T*>(cur_buf) = t;
			cur_buf += sizeof(T);
		}

		// writing to stream
		template<typename T>
		void to_stream_el(std::ostream& ostr, const uint8_t*& p) const {
			ostr << *reinterpret_cast<const T*>(p);
			p += sizeof(T);
		}

		entry(entry const&) = delete;
		entry& operator=(entry const&) = delete;

		void write_pvt(void) {
			status.store(s_filled, std::memory_order_release);
			cv_notify_log.notify_one();
		}

		template<typename T, typename... Args>
		void write_pvt(const T& t, Args... args) {
			add(t);
			write_pvt(args...);
		}
	public:
#ifdef PERF_TEST
		size_t			tries;
#endif //PERF_TEST

		entry() : status(s_free), cur_type(typelist), cur_buf(buffer) {
		}

		bool try_assign(void) {
			size_t	f_value = s_free;
			return status.compare_exchange_strong(f_value, s_assigned);
		}

		bool is_filled(void) const {
			return status == s_filled;
		}

		void reset(void) {
			cur_type = typelist;
			cur_buf = buffer;
			status.store(s_free, std::memory_order_release);
		}

		void to_stream(std::ostream& ostr) const;

		template<typename... Args>
		void write(Args... args) {
			assert(status == s_assigned);
			tp = std::chrono::high_resolution_clock::now();
			th_id = std::this_thread::get_id();
			write_pvt(args...);
		}
	};

	class logger {
		std::atomic<bool>	is_init;
		size_t			entry_sz;
		std::atomic<size_t>	entry_hint;
		entry			*entries;

		logger() : is_init(false), entry_sz(0), entry_hint(0), entries(0) {
		}
		~logger();
		logger(logger const&) = delete;
		logger& operator=(logger const&) = delete;
	public:
		static logger& instance(void);

		void init(const char* fname, const size_t e_sz = 16*1024);

		void cleanup(void);

		entry* get_entry(void);
	};
}

#define	LOG_INIT(x)	elog::logger::instance().init(x)
#define	LOG( ... )	elog::logger::instance().get_entry()->write(__VA_ARGS__)
#define LOG_CLEANUP()	elog::logger::instance().cleanup()

#endif //_ELOG_H_


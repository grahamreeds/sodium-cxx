#ifndef _SODIUM_MUTEX_HPP_
#define _SODIUM_MUTEX_HPP_

#if !defined(SODIUM_SINGLE_THREADED)
#include <mutex>

namespace sodium {
	class mutex {
	public:
		constexpr mutex() noexcept : m() { }
		mutex(const mutex&) = delete;
		~mutex() { }

		void lock() { m.lock(); }
		bool try_lock() { return m.try_lock(); }
		void unlock() { m.unlock(); }

	private:
		std::mutex m;
	};

	class recursive_mutex {
	public:
		constexpr recursive_mutex() noexcept : rm() { }
		recursive_mutex(const recursive_mutex&) = delete;
		~recursive_mutex() { }

		void lock() { rm.lock(); }
		bool try_lock() { return rm.try_lock(); }
		void unlock() { rm.unlock(); }

	private:
		std::recursive_mutex rm;
	};

}

#else

// Note: A decent compiler will compile these out if SODIUM_SINGLE_THREADED
// is defined. This is to minimize the proliferation of #if/endif.
namespace sodium {
	class mutex {
	public:
		constexpr mutex() noexcept { }
		mutex(const mutex&) = delete;
		~mutex() { }

		void lock() { }
		bool try_lock() { return true; }
		void unlock() { }
	};
	using recursive_mutex = mutex;
}
#endif

#endif
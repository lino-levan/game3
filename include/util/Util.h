#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <concepts>
#include <csignal>
#include <iostream>
#include <list>
#include <random>
#include <set>
#include <span>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include "util/Math.h"

#define TRY try {
#define CATCH } catch (std::exception &err) { std::cerr << "\e[31m" << __FILE__ << ':' << __LINE__ << ": " << err.what() << "\e[39m\n"; }
#define PRINTMAT4(m) do { std::cerr << "[[[\n"; for (int r = 0; r < 4; ++r) { for (int c = 0; c < 4; ++c) std::cerr << (m)[c][r] << ' '; std::cerr << '\n'; } std::cerr << "]]]\n"; } while (0)

namespace Game3 {
	extern std::default_random_engine utilRNG;

	/** Splits a string by a given delimiter. If condense is true, empty strings won't be included in the output. */
	template <typename T = std::string_view>
	std::vector<T> split(std::string_view str, std::string_view delimiter, bool condense = true);

	template <typename C>
	std::string join(const C &container, std::string_view delimiter = " ") {
		std::stringstream ss;
		bool first = true;
		for (const auto &item: container) {
			if (first)
				first = false;
			else
				ss << delimiter;
			ss << item;
		}
		return ss.str();
	}

	template <typename C>
	std::string hexString(const C &container) {
		std::stringstream ss;
		bool first = true;
		for (const uint8_t byte: container) {
			if (first)
				first = false;
			else
				ss << ' ';
			ss << std::hex << std::setw(2) << std::setfill('0') << std::right << static_cast<uint16_t>(byte);
		}
		return ss.str();
	}

	template <typename T, template <typename...> typename C, typename... Args>
	std::unordered_set<std::shared_ptr<T>> filterWeak(const C<std::weak_ptr<T>, Args...> &container) {
		std::unordered_set<std::shared_ptr<T>> out;
		for (const auto &weak: container)
			if (auto locked = weak.lock())
				out.insert(locked);
		return out;
	}

	long parseLong(const std::string &, int base = 10);
	long parseLong(const char *, int base = 10);
	long parseLong(std::string_view, int base = 10);
	unsigned long parseUlong(const std::string &, int base = 10);
	unsigned long parseUlong(const char *, int base = 10);
	unsigned long parseUlong(std::string_view, int base = 10);

	template <std::integral I>
	I parseNumber(std::string_view view, int base = 10) {
		I out{};
		auto result = std::from_chars(view.begin(), view.end(), out, base);
		if (result.ec == std::errc::invalid_argument)
			throw std::invalid_argument("Not an integer: \"" + std::string(view) + "\"");
		return out;
	}

	template <std::floating_point F>
	F parseNumber(std::string_view view) {
		F out{};
		auto result = std::from_chars(view.begin(), view.end(), out);
		if (result.ec == std::errc::invalid_argument)
			throw std::invalid_argument("Not a floating point: \"" + std::string(view) + "\"");
		return out;
	}

	inline std::chrono::system_clock::time_point getTime() {
		return std::chrono::system_clock::now();
	}

	template <typename T = std::chrono::nanoseconds>
	inline T timeDifference(std::chrono::system_clock::time_point old_time) {
		return std::chrono::duration_cast<T>(getTime() - old_time);
	}

	static inline std::default_random_engine::result_type getRandom(std::default_random_engine::result_type seed = 0) {
		if (seed == 0)
			return utilRNG();
		std::default_random_engine rng;
		rng.seed(seed);
		return rng();
	}

	template <typename T, typename R = std::default_random_engine>
	void shuffle(T &container, typename R::result_type seed = 0) {
		if (seed == 0) {
			std::shuffle(container.begin(), container.end(), utilRNG);
		} else {
			R rng;
			rng.seed(seed);
			std::shuffle(container.begin(), container.end(), rng);
		}
	}

	template <typename T>
	typename T::value_type & choose(T &container, typename std::default_random_engine::result_type seed = 0) {
		if (container.empty())
			throw std::invalid_argument("Container is empty");
		return container.at(getRandom(seed) % container.size());
	}

	template <typename T>
	const typename T::value_type & choose(const T &container, typename std::default_random_engine::result_type seed = 0) {
		if (container.empty())
			throw std::invalid_argument("Container is empty");
		return container.at(getRandom(seed) % container.size());
	}

	template <typename T, typename R>
	typename T::value_type & choose(T &container, R &rng) {
		if (container.empty())
			throw std::invalid_argument("Container is empty");
		return container.at(std::uniform_int_distribution(static_cast<size_t>(0), container.size() - 1)(rng));
	}

	// Here be ugly duplication

	template <typename T, typename R>
	T & choose(std::list<T> &container, R &rng) {
		if (container.empty())
			throw std::invalid_argument("Container is empty");
		return *std::next(container.begin(), std::uniform_int_distribution(static_cast<size_t>(0), container.size() - 1)(rng));
	}

	template <typename T, typename R>
	T & choose(std::set<T> &set, R &rng) {
		if (set.empty())
			throw std::invalid_argument("Set is empty");
		return *std::next(set.begin(), std::uniform_int_distribution(static_cast<size_t>(0), set.size() - 1)(rng));
	}

	template <typename T, typename R>
	T & choose(std::unordered_set<T> &set, R &rng) {
		if (set.empty())
			throw std::invalid_argument("Set is empty");
		return *std::next(set.begin(), std::uniform_int_distribution(static_cast<size_t>(0), set.size() - 1)(rng));
	}

	template <typename T, typename R>
	const typename T::value_type & choose(const T &container, R &rng) {
		if (container.empty())
			throw std::invalid_argument("Container is empty");
		return container.at(std::uniform_int_distribution(static_cast<size_t>(0), container.size() - 1)(rng));
	}

	template <typename T, typename R>
	const T & choose(const std::list<T> &container, R &rng) {
		if (container.empty())
			throw std::invalid_argument("Container is empty");
		return *std::next(container.begin(), std::uniform_int_distribution(static_cast<size_t>(0), container.size() - 1)(rng));
	}

	template <typename T, typename R>
	const T & choose(const std::unordered_set<T> &set, R &rng) {
		if (set.empty())
			throw std::invalid_argument("Set is empty");
		return *std::next(set.begin(), std::uniform_int_distribution(static_cast<size_t>(0), set.size() - 1)(rng));
	}

	template <typename T, typename R>
	const T & choose(const std::set<T> &set, R &rng) {
		if (set.empty())
			throw std::invalid_argument("Set is empty");
		return *std::next(set.begin(), std::uniform_int_distribution(static_cast<size_t>(0), set.size() - 1)(rng));
	}

	template <typename T>
	struct Hash {
		size_t operator()(const T &data) const {
			size_t out = 0xcbf29ce484222325ul;
			const auto *base = reinterpret_cast<const uint8_t *>(&data);
			for (size_t i = 0; i < sizeof(T); ++i)
				out = (out * 0x00000100000001b3) ^ base[i];
			return out;
		}
	};

	template <size_t BL = 128>
	std::string formatTime(const char *format, std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())) {
		tm now_tm;
		localtime_r(&now, &now_tm);
		std::array<char, BL> buffer;
		strftime(buffer.data(), buffer.size() * sizeof(buffer[0]), format, &now_tm);
		return buffer.data();
	}

	// Credit for reverse: https://stackoverflow.com/a/28139075/227663

	template <typename T>
	struct reverse {
		T &iterable;
		reverse() = delete;
		reverse(T &iterable_): iterable(iterable_) {}
	};

	template <typename T>
	auto begin(reverse<T> r) {
		return std::rbegin(r.iterable);
	}

	template <typename T>
	auto end(reverse<T> r) {
		return std::rend(r.iterable);
	}
}

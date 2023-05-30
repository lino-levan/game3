#pragma once

#include <mutex>
#include <shared_mutex>

#include <nlohmann/json.hpp>

#include "net/Buffer.h"

namespace Game3 {
	template <typename T>
	struct Lockable: T {
		using T::T;

		Lockable(const Lockable &other): T(other.getBase()) {}
		Lockable(const T &other): T(other) {}

		Lockable(Lockable &&other): T(other.getBase()) {}
		Lockable(T &&other): T(other) {}

		Lockable<T> operator=(const T &other) {
			auto lock = uniqueLock();
			T::operator=(other);
			return *this;
		}

		Lockable<T> operator=(T &&other) {
			auto lock = uniqueLock();
			T::operator=(other);
			return *this;
		}

		std::shared_mutex mutex;

		inline auto uniqueLock() { return std::unique_lock(mutex); }
		inline auto sharedLock() { return std::shared_lock(mutex); }

		inline T & getBase() { return static_cast<T &>(*this); }
		inline const T & getBase() const { return static_cast<const T &>(*this); }

		inline T copyBase() {
			auto lock = sharedLock();
			return static_cast<T>(*this);
		}
	};

	template <typename T>
	void to_json(nlohmann::json &json, const Lockable<T> &lockable) {
		auto lock = const_cast<Lockable<T> &>(lockable).sharedLock();
		json = lockable.getBase();
	}

	template <typename T>
	void from_json(const nlohmann::json &json, Lockable<T> &lockable) {
		auto lock = lockable.uniqueLock();
		lockable = json.get<T>();
	}

	template <typename T>
	std::ostream & operator<<(std::ostream &os, const Lockable<T> &lockable) {
		auto lock = const_cast<Lockable<T> &>(lockable).sharedLock();
		return os << lockable.getBase();
	}

	template <typename T>
	Buffer & operator+=(Buffer &buffer, const Lockable<T> &lockable) {
		auto lock = const_cast<Lockable<T> &>(lockable).sharedLock();
		return buffer += lockable.getBase();
	}

	template <typename T>
	Buffer & operator<<(Buffer &buffer, const Lockable<T> &lockable) {
		auto lock = const_cast<Lockable<T> &>(lockable).sharedLock();
		return buffer << lockable.getBase();
	}

	template <typename T>
	Buffer & operator>>(Buffer &buffer, Lockable<T> &lockable) {
		auto lock = lockable.uniqueLock();
		return buffer >> lockable.getBase();
	}
}
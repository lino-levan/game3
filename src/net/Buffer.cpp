#include <cassert>
#include <cstring>
#include <iomanip>

#include "net/Buffer.h"

namespace Game3 {
	template <> std::string Buffer::getType<uint8_t> (const uint8_t &)  { return {'\x01'}; }
	template <> std::string Buffer::getType<uint16_t>(const uint16_t &) { return {'\x02'}; }
	template <> std::string Buffer::getType<uint32_t>(const uint32_t &) { return {'\x03'}; }
	template <> std::string Buffer::getType<uint64_t>(const uint64_t &) { return {'\x04'}; }
	template <> std::string Buffer::getType<char>    (const char &)     { return {'\x05'}; }
	template <> std::string Buffer::getType<int8_t>  (const int8_t &)   { return {'\x05'}; }
	template <> std::string Buffer::getType<int16_t> (const int16_t &)  { return {'\x06'}; }
	template <> std::string Buffer::getType<int32_t> (const int32_t &)  { return {'\x07'}; }
	template <> std::string Buffer::getType<int64_t> (const int64_t &)  { return {'\x08'}; }

	template <>
	std::string Buffer::getType<std::string_view>(const std::string_view &string) {
		const auto size = string.size();

		if (size == 0)
			return {'\x10'};

		if (size < 0xf)
			return {static_cast<char>('\x10' + size)};

		assert(size <= UINT32_MAX);
		return {'\x1f', static_cast<char>(size & 0xff), static_cast<char>((size >> 8) & 0xff), static_cast<char>((size >> 16) & 0xff), static_cast<char>((size >> 24) & 0xff)};
	}

	template <>
	std::string Buffer::getType<std::string>(const std::string &string) {
		return getType(std::string_view(string));
	}

	Buffer & Buffer::append(char item) {
		bytes.insert(bytes.end(), static_cast<uint8_t>(item));
		return *this;
	}

	Buffer & Buffer::append(uint8_t item) {
		bytes.insert(bytes.end(), item);
		return *this;
	}

	Buffer & Buffer::append(uint16_t item) {
		if (std::endian::native == std::endian::little) {
			const auto *raw = reinterpret_cast<const uint8_t *>(&item);
			bytes.insert(bytes.end(), raw, raw + sizeof(item));
		} else
			bytes.insert(bytes.end(), {static_cast<uint8_t>(item), static_cast<uint8_t>(item >> 8)});
		return *this;
	}

	Buffer & Buffer::append(uint32_t item) {
		if (std::endian::native == std::endian::little) {
			const auto *raw = reinterpret_cast<const uint8_t *>(&item);
			bytes.insert(bytes.end(), raw, raw + sizeof(item));
		} else
			bytes.insert(bytes.end(), {static_cast<uint8_t>(item), static_cast<uint8_t>(item >> 8), static_cast<uint8_t>(item >> 16), static_cast<uint8_t>(item >> 24)});
		return *this;
	}

	Buffer & Buffer::append(uint64_t item) {
		if (std::endian::native == std::endian::little) {
			const auto *raw = reinterpret_cast<const uint8_t *>(&item);
			bytes.insert(bytes.end(), raw, raw + sizeof(item));
		} else {
			bytes.insert(bytes.end(), {
				static_cast<uint8_t>(item),
				static_cast<uint8_t>(item >> 8),
				static_cast<uint8_t>(item >> 16),
				static_cast<uint8_t>(item >> 24),
				static_cast<uint8_t>(item >> 32),
				static_cast<uint8_t>(item >> 40),
				static_cast<uint8_t>(item >> 48),
				static_cast<uint8_t>(item >> 56),
			});
		}
		return *this;
	}

	Buffer & Buffer::append(std::string_view string) {
		bytes.insert(bytes.end(), string.begin(), string.end());
		return *this;
	}

	template <>
	char Buffer::popRaw<char>() {
		if (bytes.empty())
			throw std::out_of_range("Buffer is empty");
		const auto out = bytes.front();
		bytes.pop_front();
		return out;
	}

	template <>
	uint8_t Buffer::popRaw<uint8_t>() {
		return static_cast<uint8_t>(popRaw<char>());
	}

	template <>
	uint16_t Buffer::popRaw<uint16_t>() {
		if (bytes.size() < sizeof(uint16_t))
			throw std::out_of_range("Buffer is too empty");
		return popRaw<uint8_t>() | (popConv<uint8_t, uint16_t>() << 8);
	}

	template <>
	uint32_t Buffer::popRaw<uint32_t>() {
		if (bytes.size() < sizeof(uint16_t))
			throw std::out_of_range("Buffer is too empty");
		return popRaw<uint8_t>() | (popConv<uint8_t, uint32_t>() << 8) | (popConv<uint8_t, uint32_t>() << 16) | (popConv<uint8_t, uint32_t>() << 24);
	}

	template <>
	uint64_t Buffer::popRaw<uint64_t>() {
		if (bytes.size() < sizeof(uint16_t))
			throw std::out_of_range("Buffer is too empty");
		return popRaw<uint8_t>() | (popConv<uint8_t, uint64_t>() << 8) | (popConv<uint8_t, uint64_t>() << 16) | (popConv<uint8_t, uint64_t>() << 24)
		     | (popConv<uint8_t, uint64_t>() << 32) | (popConv<uint8_t, uint64_t>() << 40) | (popConv<uint8_t, uint64_t>() << 48) | (popConv<uint8_t, uint64_t>() << 56);
	}

	template <>
	int8_t Buffer::popRaw<int8_t>() {
		return static_cast<int8_t>(popRaw<uint8_t>());
	}

	template <>
	int16_t Buffer::popRaw<int16_t>() {
		return static_cast<int16_t>(popRaw<uint16_t>());
	}

	template <>
	int32_t Buffer::popRaw<int32_t>() {
		return static_cast<int32_t>(popRaw<uint32_t>());
	}

	template <>
	int64_t Buffer::popRaw<int64_t>() {
		return static_cast<int64_t>(popRaw<uint64_t>());
	}

	std::string Buffer::popType() {
		const char first = popRaw<char>();
		if ((1 <= first && first <= 8) || ('\x10' <= first && first < '\x1f'))
			return {first};
		if (first == '\x1f') {
			const auto length = popRaw<uint32_t>();
			return {first, static_cast<char>(length & 0xff), static_cast<char>((length >> 8) & 0xff), static_cast<char>((length >> 16) & 0xff), static_cast<char>((length >> 24) & 0xff)};
		}
		if (first == '\x20')
			return first + popType();
		if (first == '\x21')
			return first + popType() + popType();
		throw std::invalid_argument("Invalid type byte: " + std::to_string(first));
	}

	bool Buffer::typesMatch(std::string_view one, std::string_view two) {
		assert(!one.empty());
		assert(!two.empty());
		if (const auto one0 = one[0], two0 = two[0]; ('\x10' <= one0 && one0 <= '\x1f') && ('\x10' <= two0 && two0 <= '\x1f'))
			return true;
		return one == two;
	}

	Buffer & Buffer::operator<<(uint8_t item) {
		return appendType(item).append(item);
	}

	Buffer & Buffer::operator<<(uint16_t item) {
		return appendType(item).append(item);
	}

	Buffer & Buffer::operator<<(uint32_t item) {
		return appendType(item).append(item);
	}

	Buffer & Buffer::operator<<(uint64_t item) {
		return appendType(item).append(item);
	}

	Buffer & Buffer::operator<<(int8_t item) {
		return appendType(item).append(static_cast<uint8_t>(item));
	}

	Buffer & Buffer::operator<<(int16_t item) {
		return appendType(item).append(static_cast<uint16_t>(item));
	}

	Buffer & Buffer::operator<<(int32_t item) {
		return appendType(item).append(static_cast<uint32_t>(item));
	}

	Buffer & Buffer::operator<<(int64_t item) {
		return appendType(item).append(static_cast<uint64_t>(item));
	}

	Buffer & Buffer::operator<<(std::string_view string) {
		const auto type = getType(string);
		std::cout << "type[" << type << "] " << type.size() << '\n';
		bytes.insert(bytes.end(), type.begin(), type.end());
		const auto first = type[0];
		if (first == '\x10')
			return *this;

		if ('\x11' <= first && first < '\x1f') {
			bytes.insert(bytes.end(), string.begin(), string.end());
			return *this;
		}

		assert(string.size() <= UINT32_MAX);
		append(static_cast<uint32_t>(string.size()));
		bytes.insert(bytes.end(), string.begin(), string.end());
		return *this;
	}

	Buffer & Buffer::operator<<(const std::string &string) {
		return *this << std::string_view(string);
	}

	std::ostream & operator<<(std::ostream &os, const Buffer &buffer) {
		os << "Buffer<";

		for (bool first = true; const uint16_t byte: buffer.bytes) {
			if (first)
				first = false;
			else
				os << ' ';
			os << std::hex << std::setw(2) << std::setfill('0') << std::right << byte << std::dec;
		}

		return os << ">[" << buffer.size() << ']';
	}

	template <>
	std::string Buffer::pop<std::string>() {
		const auto type = popType();
		const auto front = type.front();
		uint32_t size;
		if (front == '\x1f')
			size = popRaw<uint32_t>();
		else if ('\x10' <= front && front < '\x1f')
			size = front - '\x10';
		else
			throw std::invalid_argument("Invalid type in buffer");
		std::string out;
		out.reserve(size);
		for (uint32_t i = 0; i < size; ++i)
			out.push_back(popRaw<char>());
		return out;
	}
}
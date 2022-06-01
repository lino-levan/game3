#pragma once

#include <vector>

namespace Game3 {
	template <typename T>
	class SquareVector: public std::vector<T> {
		public:
			size_t width;
			size_t height;

			SquareVector(size_t width_, size_t height_) {
				resize(width_, height_);
			}

			template <typename T>
			void resize(size_t width_, size_t height_, T value = T()) {
				width = width_;
				height = height_;
				resize(width * height, value);
			}

			T & operator()(size_t x, size_t y) {
				return (*this)[x + y * width];
			}

			const T & operator()(size_t x, size_t y) const {
				return (*this)[x + y * width];
			}

			T & at(size_t x, size_t y) {
				return at(x + y * width);
			}

			const T & at(size_t x, size_t y) const {
				return at(x + y * width);
			}
	};
}

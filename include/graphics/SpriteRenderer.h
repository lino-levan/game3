#pragma once

#include "graphics/HasBackbuffer.h"
#include "graphics/RenderOptions.h"
#include "graphics/Shader.h"
#include "types/Types.h"

namespace Game3 {
	class Canvas;
	class Texture;

	class SpriteRenderer: public HasBackbuffer {
		protected:
			SpriteRenderer(Canvas &);

		public:
			Canvas *canvas = nullptr;
			double centerX = 0;
			double centerY = 0;

			virtual ~SpriteRenderer() = default;

			virtual void drawOnMap(const std::shared_ptr<Texture> &, double x, double y, double scale, double angle, double alpha) = 0;
			virtual void drawOnMap(const std::shared_ptr<Texture> &, const RenderOptions &) = 0;
			void drawOnMap(const std::shared_ptr<Texture> &, double x, double y);
			void drawOnMap(const std::shared_ptr<Texture> &);
			virtual void drawOnScreen(GL::Texture &, const RenderOptions &) = 0;

			virtual void renderNow() {}

			virtual void reset() = 0;

			template <typename T>
			void operator()(T &texture, const RenderOptions &options) {
				drawOnMap(texture, options);
			}
	};
}
#pragma once

#include <random>

#include <gdkmm/rectangle.h>
#include "lib/Eigen.h"

#include "resources.h"
#include "Position.h"
#include "graphics/Texture.h"
#include "Types.h"
#include "graphics/GL.h"
#include "graphics/Multiplier.h"
#include "graphics/RectangleRenderer.h"
#include "graphics/SpriteRenderer.h"
#include "graphics/TextRenderer.h"

namespace Game3 {
	class ClientGame;
	class MainWindow;
	class Realm;

	class Canvas {
		public:
			constexpr static float DEFAULT_SCALE = 2.f;
			constexpr static int AUTOFOCUS_DELAY = 1;

			MainWindow &window;
			std::shared_ptr<ClientGame> game;
			Eigen::Vector2f center {0.f, 0.f};
			float scale = DEFAULT_SCALE;
			SpriteRenderer spriteRenderer {*this};
			TextRenderer textRenderer {*this};
			RectangleRenderer rectangleRenderer;
			GL::Texture textureA;
			GL::Texture textureB;
			GL::FBO fbo;
			Multiplier multiplier;
			float magic = 8.f;
			int autofocusCounter = 0;
			Gdk::Rectangle realmBounds;
			const Realm *lastRealm = nullptr;

			Canvas(MainWindow &);

			void drawGL();
			// bool mouseButtonEvent(const Eigen::Vector2i &p, int button, bool down, int modifiers) override;
			int width() const;
			int height() const;

			bool inBounds(const Position &) const;
	};
}

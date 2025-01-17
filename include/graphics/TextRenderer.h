#pragma once

// Credit: https://learnopengl.com/In-Practice/Text-Rendering

#include "graphics/GL.h"
#include "graphics/HasBackbuffer.h"
#include "graphics/Shader.h"
#include "types/Position.h"
#include "types/Types.h"

#include <memory>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <gtkmm.h>

namespace Game3 {
	class Canvas;

	enum class TextAlign {Left, Center, Right};

	struct TextRenderOptions {
		double x      = 0;
		double y      = 0;
		double scaleX = 1;
		double scaleY = 1;
		double angle  = 0;
		Color color{1, 1, 1, 1};
		TextAlign align = TextAlign::Left;
		Color shadow{0, 0, 0, 1};
		Vector2 shadowOffset{.025f, .025f};
	};

	class TextRenderer: public HasBackbuffer {
		public:
			Canvas *canvas = nullptr;
			Shader shader;
			double centerX = 0;
			double centerY = 0;
			uint32_t fontScale = 48;

			struct Character {
				GLuint     textureID; // ID handle of the glyph texture
				glm::ivec2 size;      // Size of glyph
				glm::ivec2 bearing;   // Offset from baseline to left/top of glyph
				FT_Pos     advance;   // Offset to advance to next glyph
			};

			std::unordered_map<gunichar, Character> characters;

			TextRenderer(Canvas &, uint32_t font_scale = 96);
			~TextRenderer();

			void remove();
			void update(int width, int height) override;

			void drawOnMap(Glib::ustring text, float x, float y, TextAlign align, float scale, float angle, float alpha);
			void drawOnMap(Glib::ustring text, TextRenderOptions = {});
			void drawOnScreen(Glib::ustring text, const TextRenderOptions & = {});
			float textWidth(Glib::ustring, float scale = 1.f);
			float textHeight(Glib::ustring, float scale = 1.f);

			template <typename... Args>
			void operator()(Args &&...args) {
				drawOnMap(std::forward<Args>(args)...);
			}

			void reset();

		private:
			struct FreeLibrary {
				void operator()(FT_Library *library) const {
					FT_Done_FreeType(*library);
					delete library;
				}
			};

			struct FreeFace {
				void operator()(FT_Face *face) const {
					FT_Done_Face(*face);
					delete face;
				}
			};

			GLuint vao = 0;
			GLuint vbo = 0;
			bool initialized = false;

			glm::mat4 projection;

			std::unique_ptr<FT_Library, FreeLibrary> freetypeLibrary;
			std::unique_ptr<FT_Face, FreeFace> freetypeFace;

			void initRenderData();
			const Character & getCharacter(gunichar) const;
	};
}

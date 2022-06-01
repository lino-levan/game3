// Contains code from nanogui and from LearnOpenGL (https://github.com/JoeyDeVries/LearnOpenGL)

#include "MarchingSquares.h"
#include "resources.h"
#include "ui/Canvas.h"

namespace Game3 {
// 	constexpr static const char *vertexShader = R"(#version 330
// 		uniform vec2 scaleFactor;
// 		uniform vec2 position;
// 		in vec2 vertex;
// 		out vec2 uv;
// 		void main() {
// 			uv = vertex;
// 			vec2 scaledVertex = (vertex * scaleFactor) + position;
// 			gl_Position = vec4(2.0 * scaledVertex.x - 1.0, 1.0 - 2.0 * scaledVertex.y, 0.0, 1.0);
// 		})";

// 	constexpr static const char *fragmentShader = R"(#version 330
// 		uniform sampler2D image;
// 		out vec4 color;
// 		in vec2 uv;
// 		void main() {
// 			color = texture(image, uv);
// 		})";

	Canvas::Canvas(nanogui::Widget *parent_): GLCanvas(parent_) {
		// setBackgroundColor({255, 255, 255, 255});
		setBackgroundColor({0, 0, 100, 255});

		// textShader.init("text shader", text_vert, text_frag);

		// if (FT_Init_FreeType(&ftLibrary) != 0)
		// 	throw std::runtime_error("Couldn't initialize FreeType");

		// if (FT_New_Face(ftLibrary, "resources/FreeSans.ttf", 0, &face))
		// 	throw std::runtime_error("Couldn't load font");

		// FT_Set_Pixel_Sizes(face, 0, 48);

		grass = Texture("resources/grass.png");
		grass.bind();

		constexpr static int ints[][9] = {
			{1, 1, 1, 1, 1, 1, 1, 1, 1},
			{1, 1, 1, 1, 1, 1, 1, 1, 1},
			{1, 1, 1, 1, 1, 1, 1, 1, 1},
			{1, 1, 1, 0, 0, 0, 1, 1, 1},
			{1, 1, 1, 0, 0, 0, 1, 1, 1},
			{1, 1, 1, 0, 0, 0, 1, 1, 1},
			{1, 1, 1, 1, 1, 1, 1, 1, 1},
			{1, 1, 1, 1, 1, 1, 1, 1, 1},
			{1, 1, 1, 1, 1, 1, 1, 1, 1},
		};

		constexpr static int w = sizeof(ints[0]) / sizeof(ints[0][0]);
		constexpr static int h = sizeof(ints) / sizeof(ints[0]);

		// int dimension = 320 / 32;
		int scale = 32;
		tilemap = std::make_shared<Tilemap>(w, h, scale, grass.width, grass.height, grass.id);

		static int r = 0;
		static int c = 0;

		auto get = [&](int x, int y) -> int {
			x += c;
			y += r;
			if (x < 0 || w <= x || y < 0 || h <= y)
				return 0;
			return ints[y][x];
		};

		constexpr int padding = 0;

		for (r = padding; r < h - padding; ++r) {
			for (c = padding; c < w - padding; ++c) {
				int topleft = get(-1, -1), top = get(0, -1), topright = get(1, -1), left = get(-1, 0), right = get(1, 0), bottomleft = get(-1, 1), bottom = get(0, 1), bottomright = get(1, 1);
				if (!top || !left) topleft = 0;
				if (!top || !right) topright = 0;
				if (!bottom || !left) bottomleft = 0;
				if (!bottom || !right) bottomright = 0;
				// const int sum = get(-1, -1) + (get(0, -1) << 1) + (get(1, -1) << 2) + (get(-1, 0) << 3) +
				// 	(get(1, 0) << 4) + (get(-1, 1) << 5) + (get(0, 1) << 6) + (get(1, 1) << 7);
				int sum = topleft + (top << 1) + (topright << 2) + (left << 3) + (right << 4) + (bottomleft << 5) + (bottom << 6) + (bottomright << 7);

				if (r == 0 && c == 0) {
					for (int i = -1; i <= 1; ++i) {
						for (int j = -1; j <= 1; ++j)
							std::cerr << get(i, j);
						std::cerr << '\n';
					}
				}
				if (get(0, 0) == 0)
					sum = 0;
				int index = 48;
				// const int sum = get(-1,-1) + 2*get(0,-1) + 4*get(1,-1) + 8*get(-1,0) + 16*get(1,0) + 32*get(-1,1) + 64*get(0,1) + 128*get(1,1);
				// const int index = marching[sum];
				// int index = 48;
				// if (sum == 255)
				// 	index += rand() % 3;
				// else
				// 	index = marching_map.at(sum);
				index = marchingMap.at(sum);
				// if (index == 12) {
				// 	constexpr static int full[] {12, 30, 41, 41, 41, 41, 41, 41, 41, 41};
				// 	srand((r << 20) | c);
				// 	index = full[rand() % (sizeof(full) / sizeof(full[0]))];
				// }

				// std::cerr << '(' << r << ", " << c << ") -> " << sum << " -> " << index << " -> (" << x << ", " << y << ")\n";
				// constexpr int scale = 32;
				(*tilemap)(c, r) = index;
				tilemap->sums.at(c + r * tilemap->width) = sum;
				// renderTile(cr, water, scale * (c - padding), scale * (r - padding), 1, 5, scale, scale);
				// renderTile(cr, grass, scale * (c - padding), scale * (r - padding), x, y, scale, scale, std::to_string(sum) + "," + std::to_string(index) + (get(0, 0) == 1? "!" : ""));
			}
		}

		tilemapRenderer.initialize(tilemap);
	}

	Canvas::~Canvas() {
		// shader.free();
	}

	// void Canvas::drawImage(const Texture &texture, const nanogui::Vector2f &screen_pos, const nanogui::Vector2f &image_offset, const nanogui::Vector2f &image_extent) {
		// const nanogui::Vector3f position3(screen_pos.x(), screen_pos.y(), 0.f);
		// auto model = Eigen::Affine3f(Eigen::Translation3f(position3)).matrix();
		// TODO, maybe
	// }

	void Canvas::draw(NVGcontext *context_) {
		nanogui::GLCanvas::draw(context_);
		if (context_ != nullptr && context != context_) {
			context = context_;
			font = nvgCreateFont(context, "FreeSans", "resources/FreeSans.ttf");
		}
	}

	void Canvas::drawGL() {
		tilemapRenderer.onBackBufferResized(width(), height());
		tilemapRenderer.render(context, font);
	}
}

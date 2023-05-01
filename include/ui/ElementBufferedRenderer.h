#pragma once

#include <atomic>
#include <unordered_set>
#include <vector>

#include "Shader.h"
#include "Types.h"
#include "ui/RectangleRenderer.h"
#include "ui/Reshader.h"
#include "util/GL.h"

namespace Game3 {
	class Realm;
	class TileProvider;

	/** Start corresponds to left or top, End corresponds to right or bottom. */
	enum class Alignment {Start, Middle, End};

	class ElementBufferedRenderer {
		public:
			constexpr static float TEXTURE_SCALE = 2.f;
			constexpr static float TILE_TEXTURE_PADDING = 1.f / 2048.f;
			int backbufferWidth = -1;
			int backbufferHeight = -1;
			Alignment horizontal;
			Alignment vertical;
			/** Top left might have {-1, -1}, top right might have {1, -1}, etc. */
			std::pair<int32_t, int32_t> offset {};

			Eigen::Vector2f center {0.f, 0.f};
			std::shared_ptr<Tileset> tileset;
			GL::Texture lightTexture;

			ElementBufferedRenderer();
			ElementBufferedRenderer(Realm &);
			~ElementBufferedRenderer();

			void reset();
			void init(TileProvider &, Layer);
			void render(float divisor, float scale, float center_x, float center_y);
			/** Doesn't bind any texture—the caller must bind a texture before calling this. */
			void render(float divisor);
			bool reupload();
			bool onBackbufferResized(int width, int height);
			void setChunk(Chunk<TileID> &, bool can_reupload = true);
			void setChunkPosition(const ChunkPosition &);
			inline void markDirty() { dirty = true; }
			inline void setRealm(Realm &new_realm) { realm = &new_realm; }

			inline explicit operator bool() const { return initialized; }

		private:
			bool initialized = false;
			/** Whether lighting needs to be recomputed. */
			std::atomic_bool dirty = true;
			Layer layer = -1;
			Shader shader {"terrain"};
			GL::FloatVAO vao;
			GL::VBO vbo;
			GL::EBO ebo;
			GL::FBO fbo;
			GL::Texture blurredLightTexture;
			std::vector<GLint> brightTiles;
			std::unordered_set<TileID> brightSet;
			RectangleRenderer rectangle;
			Reshader reshader;
			Realm *realm = nullptr;
			Chunk<TileID> *chunk = nullptr;
			TileProvider *provider = nullptr;
			std::vector<TileID> tileCache;
			bool vboIsWeird = false;
			bool positionDirty = false;
			ChunkPosition chunkPosition;

			bool generateVertexBufferObject();
			bool generateElementBufferObject();
			bool generateVertexArrayObject();
			bool generateLightingTexture();

			void recomputeLighting();

			static void check(int handle, bool is_link = false);
	};
}

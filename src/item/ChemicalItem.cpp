#include "tools/Flasker.h"
#include "chemistry/MoleculeNames.h"
#include "item/ChemicalItem.h"

#include <random>

namespace Game3 {
	Lockable<std::unordered_map<std::string, Glib::RefPtr<Gdk::Pixbuf>>> ChemicalItem::imageCache{};

	Glib::RefPtr<Gdk::Pixbuf> ChemicalItem::getImage(const Game &game, const ItemStack &stack) const {
		const std::string formula = getFormula(stack);

		{
			auto shared_lock = imageCache.sharedLock();
			if (auto iter = imageCache.find(formula); iter != imageCache.end())
				return iter->second;
		}

		auto unique_lock = imageCache.uniqueLock();
		auto image = makeImage(game, stack);
		imageCache.emplace(formula, image);
		return image;
	}

	Glib::RefPtr<Gdk::Pixbuf> ChemicalItem::makeImage(const Game &, const ItemStack &stack) const {
		const std::string formula = getFormula(stack);

		uint16_t hue = 0;
		float saturation = 1.f;
		float value_difference = -1.f;

		if (!formula.empty()) {
			const size_t hash = std::hash<std::string>{}(formula);
			hue = hash % 360;
			saturation = (hash / double(std::numeric_limits<size_t>::max())) / 2.f + .5f;
			std::default_random_engine rng(hash);
			std::normal_distribution<float> normal(0, 0.1);
			value_difference = normal(rng);
		}

		int width{};
		int height{};

		rawImage = generateFlaskRaw("resources/testtubebase.png", "resources/testtubemask.png", hue, saturation, value_difference, &width, &height);

		return Gdk::Pixbuf::create_from_data(rawImage.get(), Gdk::Colorspace::RGB, true, 8, width, height, 4 * width)->scale_simple(width << 3, height << 3, Gdk::InterpType::NEAREST);
	}

	std::string ChemicalItem::getTooltip(const ItemStack &stack) {
		const std::string formula = getFormula(stack);
		if (formula.empty())
			return "Unknown Chemical";
		if (auto iter = moleculeNames.find(formula); iter != moleculeNames.end())
			return iter->second + " (" + formula + ')';
		return formula;
	}

	std::string ChemicalItem::getFormula(const ItemStack &stack) {
		if (auto iter = stack.data.find("formula"); iter != stack.data.end() && !iter->is_null())
			return *iter;
		return {};
	}
}

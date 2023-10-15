#include "chemistry/MoleculeNames.h"

namespace Game3 {
	std::unordered_map<std::string, std::string> moleculeNames {
		{"C12H20O10", "Cellulose"},
		{"H2O", "Water"},
		{"CO2", "Carbon Dioxide"},
		{"N2", "Dinitrogen"},
		{"H2", "Dihydrogen"},
		{"O3", "Ozone"},
		{"O2", "Dioxygen"},
		{"H", "Hydrogen"},
		{"O", "Oxygen"},
		{"C", "Carbon"},
		{"N", "Nitrogen"},
		{"Si", "Silicon"},
		{"SiO2", "Silica"},
		{"Al2Si2O5(OH)4", "Kaolinite"},
		{"C39H35O10NS", "Lignite"},
		{"NH3", "Ammonia"},
		{"C2H4", "Ethylene"},
		{"S", "Sulfur"},
		{"TiO2", "Titanium oxide"},
		{"Al2Co2O5", "Cobalt aluminate"},
		// {"C12H22O11", "Sucrose"}, // nvm. Lactose has the same formula, so we're just going to leave it nameless
		{"CuCl2", "Copper chloride"},
		{"BaSO4", "Barium sulfate"},
		{"NiCl2", "Nickel chloride"},
		{"Sb2O3", "Antimony trioxide"},
		{"CdS", "Cadmium sulfide"},
		{"Cr2O3", "Chromium(III) oxide"},
		{"K2Cr2O7", "Potassium dichromate"},
		{"As4S3", "Arsenic sulfide"},
		{"KMnO4", "Potassium permanganate"},
		{"HgS", "Mercury sulfide"},
		{"Ca5(PO4)3OH", "Hydroxyapatite"},
		{"PbI2", "Lead iodide"},
		{"NO3", "Nitrate"},
		{"C40H56", "Beta carotene"},
		{"C32H44O8", "Cucurbitacin E"},
		{"C40H56", "Lycopene"},
		{"C4H10", "Butane"},
		{"C5H12", "Pentane"},
	};
}

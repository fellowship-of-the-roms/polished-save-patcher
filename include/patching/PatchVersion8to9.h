#ifndef PATCHVERSION8TO9_H
#define PATCHVERSION8TO9_H

#include "core/SaveBinary.h"

namespace patchVersion8to9Namespace {
	using namespace patchVersion8to9Namespace;
	constexpr int NUM_KEY_ITEMS_V9 = 0x25;
	const std::vector<int> unusedEventIndexesV9 = {
		127, 128, 265, 477, 479, 481, 760, 829, 960, 2200,
		2233, 2234, 2235, 2236, 2237, 2238, 2239, 2240, 2241, 2242,
		2243, 2244, 2245, 2246, 2247, 2248, 2249, 2250, 2251, 2252,
		2253, 2254, 2255, 2256, 2257, 2258, 2259, 2260, 2261, 2262,
		2263, 2264, 2265, 2266, 2267, 2268, 2269, 2270, 2271, 2272,
		2273, 2274, 2275, 2276, 2277, 2278, 2279, 2280, 2281, 2282,
		2283, 2284, 2285, 2286, 2287, 2288, 2289, 2290, 2291, 2292,
		2293, 2294, 2295, 2296, 2297, 2298, 2299, 2300, 2301, 2302
	};

	// converts a version 8 key item to a version 8 key item
	uint8_t mapV8KeyItemToV9(uint8_t v8);

	// bool patchVersion8to9 takes in arguments SaveBinary save7 and SaveBinary save8
	bool patchVersion8to9(SaveBinary& save8, SaveBinary& save9);
}


#endif
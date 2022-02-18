#include "disp.hpp"
#include <cstdio>

int main(void)
{
	try {
		Disp().run();
	} catch (const fr::exception &e) {
		std::printf("FATAL ERROR: %s\n", e.what());
		return 1;
	}
	return 0;
}
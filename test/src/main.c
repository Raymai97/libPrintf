#include "base.h"

int main(void)
{
	test_std();
#ifdef _WIN32
	test_w32();
#endif
	return 0;
}

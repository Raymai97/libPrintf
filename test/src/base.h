#include <assert.h>
#include <stddef.h>

#ifndef ARR_SIZE
#define ARR_SIZE(X)  (sizeof(X) / sizeof(*X))
#endif

void test_std(void);
void test_w32(void);

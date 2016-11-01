#include "xmart.h"
XTEST_SUITE(xstore_uint_test)
{
	using namespace xmart::xstore;
	XUNIT_TEST(to_keys)
	{
		auto keys = to_keys("\\a\\b\\c\\d\\e");
		xassert(keys.size() == 5);

		auto keys2 = to_keys("a\\\\b");
		xassert(keys2.size() == 1);

		auto key3 = to_keys("a\\/b/b");
		xassert(key3.size() == 2);

		auto key4 = to_keys("a\\/b/b/c");
		xassert(key4.size() == 4);
	}
}
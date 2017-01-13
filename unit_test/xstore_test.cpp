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
		xassert(key4.size() == 3);
	}
	XUNIT_TEST(add_watcher)
	{
		xson_store strore;
		strore.bind_watcher_callback([](
			const std::string & id, 
			const std::string & path, 
			const std::string & type, 
			const std::string & value) {

			xassert(id == "id");
			xassert(path == "hello\\a");
			xassert(type == "add");
			xassert(value == "\"b\"");
		});
		strore.add_watch("hello", "id");
		strore.set("hello\\a", "\"b\"");
	}

	XUNIT_TEST(set)
	{
		xson_store strore;
		strore.bind_watcher_callback([](
			const std::string & id,
			const std::string & path,
			const std::string & type,
			const std::string & value) {
			std::cout << "\n	id :" << id << "  path :" << path << "	type :" <<type<< "	value :" << value << std::endl;
		});
		strore.add_watch("hello", "id");
		xassert (strore.set("hello\\a", "\"b\"") == result_t::e_ok);
	}
	XUNIT_TEST(get)
	{

		xson_store strore;
		strore.bind_watcher_callback([](
			const std::string & id,
			const std::string & path,
			const std::string & type,
			const std::string & value) {
			std::cout << "\n	id :" << id << "  path :" << path << "	type :" << type << "	value :" << value << std::endl;
		});
		strore.add_watch("hello", "id");
		xassert(strore.set("hello\\a", "\"b\"") == result_t::e_ok);
		std::string value;
		xassert(strore.get("hello\\a", value) == result_t::e_ok);
		xassert(value == "\"b\"");
		xassert(strore.get("hello", value) == result_t::e_ok);
		xassert(value == "{\"a\":\"b\"}");


		xassert(strore.set("hello\\b\\c\\d", "\"hello\"") == result_t::e_ok);
		xassert(strore.set("hello\\b\\c\\e", "false") == result_t::e_ok);
		xassert(strore.get("hello\\b", value) == result_t::e_ok);
		xassert(value == "{\"c\":{\"d\":\"hello\", \"e\":false}}");

		xassert(strore.del("hello")== result_t::e_ok);
	}

	XUNIT_TEST(del)
	{

		xson_store strore;
		strore.bind_watcher_callback([](
			const std::string & id,
			const std::string & path,
			const std::string & type,
			const std::string & value) {
			std::cout << "\n	id :" << id << "  path :" << path << "	type :" << type << "	value :" << value << std::endl;
		});
		strore.add_watch("hello", "id");
		xassert(strore.set("hello\\a", "\"b\"") == result_t::e_ok);
		xassert(strore.set("hello\\a", "\"b\"") == result_t::e_obj_exist);
		xassert(strore.del("hello\\a") == result_t::e_ok);
		xassert(strore.set("hello\\a", "\"b\"") == result_t::e_ok);
		xassert(strore.set("hello\\a\\b", "\"b\"") == result_t::e_path_err);
		xassert(strore.del("hello\\a\\b") == result_t::e_path_err);
		xassert(strore.del("hello\\a") == result_t::e_ok);
		xassert(strore.del("hello") == result_t::e_ok);
	}

}
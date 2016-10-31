#pragma once
#include "xmart.h"

int main()
{
	using namespace xmart::store::xson;
	obj_t obj;

	obj.should_change_change(true);

	obj["hello"]["world"] = 1;

	std::vector<std::string> path;
	path.push_back("hello");
	path.push_back("worl2");
	path.push_back("obj1");
	path.push_back("obj2");

	obj_t *o = &obj;


	for (auto itr: path)
	{
		o = &(*o)[itr];
	}
	*o = 3;

	std::cout << obj.str().c_str() << std::endl;
	return 0;
}
#pragma once
#include "xmart.h"

int main()
{
	using namespace xmart::store::xson;
	obj_t obj;
	obj.add_watch("a", [](const event_t &e) {
		std::cout << (e.type_==event_t::e_add?"add":
			(e.type_ == event_t::e_delete?"delete":
					  (e.type_ == event_t::e_update? "update":"error_type")))
			<< e.path_ << e.obj->str().c_str() << std::endl;
	});

	obj.open_callback(true);

	obj["hello"]["world"] = 1;
	obj["hello"]["world2"].add(false);
	obj["hello"]["world2"].add(true);
	obj["hello"]["world2"].add(1);
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

	obj["hello"]["world2"].get(1).del();
	return 0;
}
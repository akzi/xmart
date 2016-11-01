#pragma once
#include "xmart.h"
using namespace xmart::xstore;

XTEST_SUITE(xon_test)
{
	XUNIT_TEST(insert)
	{
		obj_t obj;
		obj.watch("a", [](const event_t &e) {
			xassert(e.path_ == "hello");
			xassert(e.type_ == event_t::e_add);
			xassert(e.obj->str() == "false");
		});
		auto o = new obj_t;
		*o = false;
		obj.insert("hello",o);
	}
	XUNIT_TEST(build_obj)
	{
		obj_t *o;
		o= build_obj("false");
		xassert(o->type_ == obj_t::e_bool);
		del_obj(o);

		o = build_obj("true");
		xassert(o->type_ == obj_t::e_bool);
		del_obj(o);

		o = build_obj("12345");
		xassert(o->type_ == obj_t::e_num);
		del_obj(o);

		o = build_obj("0.12345");
		xassert(o->type_ == obj_t::e_float);
		del_obj(o);

		o = build_obj("[]");
		xassert(o->type_ == obj_t::e_vec);

		o = build_obj("{}");
		xassert(o->type_ = obj_t::e_obj);

	}
}
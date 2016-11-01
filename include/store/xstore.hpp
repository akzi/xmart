#pragma once
#include "xmart.h"
#include "xson_t.hpp"
namespace xmart
{
namespace xstore
{
	static inline 
		std::vector<std::string> to_keys(const std::string &path)
	{
		std::vector<std::string> keys;
		std::string key;
		auto len = path.size();
		for (int i = 0; i < len; )
		{
			if (path[i] == '\\')
			{
				if (i < len + 1)
				{
					if (path[i + 1] == '/' ||
						path[i + 1] == '\\')
					{
						key.push_back(path[i + 1]);
						i += 2;
						continue;
					}
				}
				if (key.size())
					keys.emplace_back(key);
				key.clear();
			}
			key.push_back(path[i]);
			++i;
		}
	}
	enum class result_t
	{
		e_ok,
		e_obj_exist,
		e_obj_err,
		e_path_err,
	};

	class xson_store
	{
	public:
			
		typedef std::function<
			void(const std::string &,//id
					const std::string &,//path
					const std::string &,//type
					const std::string &//value
				 )> watcher_callback_t;

		result_t get(const std::string &path, std::string &value)
		{
			auto keys = to_keys(path);
			obj_t *o = &obj_;
			for (auto &itr : keys)
			{
				o = (*o)[itr];
				if(!o)
					return result_t::e_path_err;
			}
			value = o->str();
			return result_t::e_ok;
		}
		result_t set(const std::string &path, const std::string &value)
		{
			auto keys = to_keys(path);
			obj_t *v = build_obj(value);
			if(!v)
				return result_t::e_obj_err;
			obj_t *o = &obj_;
			for(auto &itr : keys)
			{
				o = (*o)[itr];
				if(!o)
					return result_t::e_path_err;
			}
			if(o->type_ != obj_t::e_null)
				return result_t::e_obj_exist;
			*o = std::move(*v);
			del_obj(o);
			return result_t::e_ok;
		}
		result_t add_watch(const std::string & path,const std::string & id)
		{
			auto keys = to_keys(path);
			obj_t *o = &obj_;
			for (auto &itr: keys)
			{
				o = (*o)[itr];
				if(!o)
					return result_t::e_path_err;
			}
			o->watch(id, [this](const event_t &e) { on_watch(e); });
			return result_t::e_ok;
		}
		void bind_watcher_callback(watcher_callback_t cb)
		{
			assert(cb);
			watcher_callback_ = cb;
		}
	private:
		void on_watch(const event_t &e)
		{
			std::string type = 
				(e.type_ == event_t::e_add    ? "add": 
				(e.type_ == event_t::e_delete ? "delete":
				(e.type_ == event_t::e_update ? "update":
								(assert(false),"err_type"))));
			assert(watcher_callback_);
			watcher_callback_(e.id_,e.path_,type,e.obj->str());
		}
		watcher_callback_t watcher_callback_;
		obj_t obj_;
	};
}
}
#pragma once
#include "xmart.h"
#include "xson_t.hpp"
namespace xmart
{
	namespace xstore
	{
		static inline std::vector<std::string> to_keys(const std::string &path)
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

		class xson_store
		{
		public:
			typedef std::function<void(const std::string &, const std::string &, const std::string &)> watcher_t;
			std::string get(const std::string &path)
			{

			}
			void set(const std::string &path, const std::string &value)
			{
				auto keys = to_keys(path);

				obj_t *obj = build_obj(value);

				obj_t *o = &obj_;
				for (auto &itr : keys)
					o = &(*o)[itr];

			}
			void watch(const std::string & path, const std::string & id, watcher_t)
			{

			}
		private:
			
			obj_t obj_;
		};
	}
}
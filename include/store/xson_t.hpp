#pragma once
namespace xmart
{
namespace xstore
{
	struct obj_t;
	struct event_t
	{
		std::string path_;
		std::string id_;
		enum type_t
		{
			e_null,
			e_add,
			e_update,
			e_delete,
			e_lost_watcher,
		}type_ = e_null;

		obj_t *obj = NULL;
	};
	struct obj_t
	{
		typedef std::function<void(const event_t &)> watcher_t;
		enum type_t
		{
			e_null,
			e_num,
			e_str,
			e_bool,
			e_float,
			e_obj,
			e_vec,
		};

		type_t type_;
		union value_t
		{
			std::string *str_;
			int64_t num_;
			double double_;
			bool bool_;
			std::map<std::string, obj_t*> *obj_;
			std::vector<obj_t*> *vec_;
		} val_;
		obj_t *parent_ = NULL;
		obj_t *root = this;
		std::string key_;
		std::map<std::string, watcher_t> watchers_;

		obj_t()
			:type_(e_null)
		{
			memset(&val_, 0, sizeof(val_));
		}
		virtual ~obj_t()
		{
			reset_val();
		}
		void reset_val()
		{
			switch (type_)
			{
			case e_null:
			case e_num:
			case e_bool:
			case e_float:
				break;
			case e_str:
				delete val_.str_;
				break;
			case e_obj:
				for (auto&itr : *val_.obj_)
					delete itr.second;
				delete val_.obj_;
				break;
			case e_vec:
				for (auto&itr : *val_.vec_)
					delete itr;
				delete val_.vec_;
				break;
			default:
				break;
			}
			type_ = e_null;
		}

		template<typename T>
		typename std::enable_if<
			std::is_floating_point<T>::value, obj_t &>::type
			operator=(T val)
		{
			event_t e;
			if (type_ == e_null)
				e.type_ = event_t::e_add;
			else
			{
				e.type_ = event_t::e_update;
			}
			type_ = e_float;
			val_.double_ = val;
			get_path(e.path_);
			e.obj = this;
			on_change(e);
			return *this;
		}
		obj_t &operator=(const char *val)
		{
			event_t e;
			if (type_ == e_null)
				e.type_ = event_t::e_add;
			else
			{
				e.type_ = event_t::e_update;

			}
			type_ = e_str;
			val_.str_ = new std::string(val);
			get_path(e.path_);
			e.obj = this;
			on_change(e);
			return *this;
		}

		obj_t& operator=(const std::string &val)
		{
			return operator=(val.c_str());
		}

		template<typename T>
		typename std::enable_if<
			std::is_integral<T>::value, obj_t&>::type
			operator=(T val)
		{
			event_t e;
			if (type_ == e_null)
				e.type_ = event_t::e_add;
			else
			{
				e.type_ = event_t::e_update;
			}
			type_ = e_num;
			val_.num_ = val;
			get_path(e.path_);
			e.obj = this;
			on_change(e);
			return *this;
		}
		obj_t &operator =(bool val)
		{
			event_t e;
			if (type_ == e_null)
				e.type_ = event_t::e_add;
			else
			{
				e.type_ = event_t::e_update;
			}
			type_ = e_bool;
			val_.bool_ = val;
			get_path(e.path_);
			e.obj = this;
			on_change(e);
			return *this;

		}

		obj_t &operator=(obj_t &&obj)
		{
			assert(type_ == e_null);
			event_t e;
			e.type_ = event_t::e_add;
			type_ = obj.type_;
			val_ = obj.val_;
			obj.type_ = e_null;
			get_path(e.path_);
			e.obj = this;
			on_change(e);
			return *this;
		}
		obj_t *operator[](const std::string & key)
		{
			return operator[](key.c_str());
		}

		obj_t *operator[](const char *key)
		{
			if(type_ != e_obj && type_ != e_null)
				return nullptr;

			if(type_ == e_null)
			{
				type_ = e_obj;
				val_.obj_ = new std::map<std::string, obj_t*>;
			}
			auto itr = val_.obj_->find(key);
			if (itr == val_.obj_->end())
			{
				auto obj = new obj_t;
				obj->parent_ = this;
				obj->root = root;
				obj->key_ = key;
				val_.obj_->emplace(key, obj);
				return obj;
			}
			return itr->second;
		}
		bool open_callback(bool b = false)
		{
			static bool val = false;
			if (b == false)
				return val;
			else
				val = b;
			return val;
		}
		bool insert(const std::string &key, obj_t *o)
		{
			if(type_ != e_obj && type_ != e_null)
				return false;
			if(type_ == e_null)
			{
				type_ = e_obj;
				val_.obj_ = new std::map<std::string, obj_t*>;
			}

			o->root = root;
			o->parent_ = this;
			o->key_ = key;
			val_.obj_->emplace(key, o);
			event_t e;
			e.type_ = event_t::e_add;
			e.obj = o;
			o->get_path(e.path_);
			o->on_change(e);
			return true;
		}
		void del()
		{
			if (type_ == e_null)
				return;
			if (type_ == e_obj)
				for (auto &itr : *val_.obj_)
					itr.second->del();
			else if (type_ == e_vec)
				for (auto &itr : *val_.vec_)
					itr->del();

			event_t e;
			get_path(e.path_);
			e.type_ = event_t::e_delete;
			e.obj = this;
			reset_val();
			on_change(e);
			reset_watcher();
			if (parent_ &&parent_->type_ == e_vec)
			{
				int i = 0;
				for (auto &itr = parent_->val_.vec_->begin();
					itr != parent_->val_.vec_->end()
					;)
				{
					if (*itr == this)
					{
						itr = parent_->val_.vec_->erase(itr);
					}
					else
					{
						(*itr)->key_ = "[" + std::to_string(i)+"]";
						++itr;
					}

				}
				delete this;
			}
		}
		void on_change(event_t &e)
		{
			if (!open_callback())
				return;

			if(e.path_.size() && e.path_.back() == '\\')
				e.path_.pop_back();

			for (auto &itr : watchers_)
			{
				e.id_ = itr.first;
				itr.second(e);
			}
			if (parent_ && e.obj != parent_)
			{
				parent_->on_change(e);
			}
		}
		void reset_watcher()
		{
			event_t e;
			e.type_ = event_t::e_lost_watcher;
			e.obj = this;
			get_path(e.path_);
			if (e.path_.size() && e.path_.back() == '\\')
				e.path_.pop_back();
			for (auto &itr : watchers_)
			{
				e.id_ = itr.first;
				itr.second(e);
			}
			watchers_.clear();
		}
		void del_watch(const std::string &id)
		{
			auto itr = watchers_.find(id);
			if(itr != watchers_.end())
				watchers_.erase(itr);
		}
		std::string dump_watcher()
		{
			std::string data;
			if (type_ == e_vec)
				for (auto &itr : *val_.vec_)
					data += itr->dump_watcher();
			else if(type_ == e_obj)
				for (auto &itr : *val_.obj_)
					data += itr.second->dump_watcher();
			std::string path;
			get_path(path);
			for (auto &itr: watchers_)
			{
				data += path + itr.first;
				data += "\n";
			}
			return data;
		}
		bool watch(const std::string &id, watcher_t watcher)
		{
			return watchers_.
				insert(std::make_pair(id, watcher)).second;
		}
		void get_path(std::string &path)
		{
			path = key_ + "\\" + path;
			if (parent_ && parent_->key_.size())
			{
				parent_->get_path(path);
			}
		}
		template<typename T>
		obj_t& add(const T &val)
		{
			assert(type_ == e_vec || type_ == e_null);
			type_ = e_vec;
			if (val_.vec_ == NULL)
				val_.vec_ = new std::vector<obj_t *>;
			obj_t *obj = new obj_t;
			obj->parent_ = this;
			obj->root = root;
			obj->key_ = "[" + std::to_string(val_.vec_->size())+"]";
			*obj = val;
			val_.vec_->push_back(obj);
			event_t e;
			e.type_ = event_t::e_add;
			e.obj = obj;
			obj->get_path(e.path_);
			on_change(e);
			return *this;
		}

		bool add(obj_t *obj)
		{
			if (type_ != e_vec && type_ != e_null)
				return false;
			if(type_ == e_null)
			{
				type_ = e_vec;
				val_.vec_ = new std::vector<obj_t *>;
			}
			obj->parent_ = this;
			obj->root = root;
			obj->key_ = "[" + std::to_string(val_.vec_->size()) + "]";
			val_.vec_->push_back(obj);
			event_t e;
			e.type_ = event_t::e_add;
			e.obj = obj;
			obj->get_path(e.path_);
			on_change(e);
			return true;
		}
		template<class T>
		typename std::enable_if<
			std::is_integral<T>::value &&!
			std::is_same<T, bool>::value, T>::type
			get()
		{
			assert(type_ == e_num);
			return static_cast<T>(val_.num_);
		}
		template<class T>
		typename std::enable_if<
			std::is_same<T, bool>::value, T>::type
			get()
		{
			assert(type_ == e_bool);
			return val_.bool_;
		}
		template<class T>
		typename std::enable_if<
			std::is_floating_point<T>::value, T>::type
			get()
		{
			assert(type_ == e_float);
			return static_cast<T>(val_.double_);
		}
		template<class T>
		typename std::enable_if<
			std::is_same<T, std::string>::value,
			std::string &>::type
			get()
		{
			assert(type_ == e_str);
			return *val_.str_;
		}
		template<typename T>
		T get(std::size_t idx)
		{
			assert(type_ == e_vec);
			assert(val_.vec_);
			assert(idx < val_.vec_->size());
			return ((*val_.vec_)[idx])->get<T>();
		}
		obj_t &get(std::size_t idx)
		{
			assert(type_ == e_vec);
			assert(val_.vec_);
			assert(idx < val_.vec_->size());
			return *((*val_.vec_)[idx]);
		}
		bool is_null()
		{
			return type_ == e_null;
		}
		std::size_t len()
		{
			assert(type_ == e_vec);
			return val_.vec_->size();
		}
		type_t type()
		{
			return type_;
		}
		std::string str()
		{
			switch (type_)
			{
			case e_bool:
				return val_.bool_ ? "true" : "false";
			case e_num:
				return std::to_string(val_.num_);
			case e_str:
				return "\"" + *val_.str_ + "\"";
			case e_obj:
				return map2str();
			case e_float:
				return std::to_string(val_.double_);
			case e_vec:
				return vec2str();
			}
			return "null";
		}
		std::string vec2str()
		{
			std::string str("[");
			assert(type_ == e_vec);
			assert(val_.vec_);

			for (auto &itr : *val_.vec_)
			{
				str += itr->str();
				str += ", ";
			}
			str.pop_back();
			str.pop_back();
			str += "]";
			return str;
		}
		std::string map2str()
		{
			std::string str("{");
			for (auto &itr : *val_.obj_)
			{
				str += "\"";
				str += itr.first;
				str += "\":";
				str += itr.second->str();
				str += ", ";
			}
			str.pop_back();
			str.pop_back();
			str += "}";
			return str;
		}
	};
	static const obj_t null;

	namespace parser
	{
		static inline bool
			skip_space(int &pos, int len, const char * str)
		{
			assert(len > 0);
			assert(pos >= 0);
			assert(pos < len);

			while (pos < len &&
				(str[pos] == ' ' ||
					str[pos] == '\r' ||
					str[pos] == '\n' ||
					str[pos] == '\t'))
				++pos;

			return pos < len;
		}
		static inline std::pair<bool, std::string> 
			get_str(int &pos, int len, const char *str)
		{
			assert(len > 0);
			assert(pos >= 0);
			assert(pos < len);
			assert(str[pos] == '"');

			std::string text;
			++pos;
			while (pos < len)
			{
				if (str[pos] != '"')
					text.push_back(str[pos]);
				else if (text.size() && text.back() == '\\')
					text.push_back(str[pos]);
				else
				{
					++pos;
					return{ true, text };
				}
				++pos;
			}
			return{ false, ""};
		}
		static obj_t *get_bool(int &pos, int len, const char *str)
		{
			assert(len > 0);
			assert(pos >= 0);
			assert(pos < len);
			assert(str[pos] == 't' || str[pos] == 'f');
			std::string key;

			while (pos < len)
			{
				if (isalpha(str[pos]))
					key.push_back(str[pos]);
				else
					break;
				++pos;
			}
			if(key == "true")
			{
				auto *o = new obj_t;
				*o = true;
				return o;
			}
			else if(key == "false")
			{
				auto *o = new obj_t;
				*o = false;
				return o;
			}
			return nullptr;

		}
		static inline obj_t *
			get_null(int &pos, int len, const char *str)
		{
			assert(len > 0);
			assert(pos >= 0);
			assert(pos < len);
			assert(str[pos] == 'n');
			std::string null;

			while (pos < len)
			{
				if (isalpha(str[pos]))
					null.push_back(str[pos]);
				else
					break;
				++pos;
			}
			if(null == "null")
			{
				return new obj_t;
			}
			return nullptr;
		}
		static inline obj_t *
			get_num(int &pos, int len, const char *str)
		{
			bool sym = false;
			std::string tmp;
			while (pos < len)
			{
				if (str[pos] >= '0' &&str[pos] <= '9')
				{
					tmp.push_back(str[pos]);
					++pos;
				}
				else if (str[pos] == '.')
				{
					tmp.push_back(str[pos]);
					++pos;
					if (sym == false)
						sym = true;
					else
						return nullptr;
				}
				else
					break;
			}
			errno = 0;
			if (sym)
			{
				double d = std::strtod(tmp.c_str(), 0);
				if (errno == ERANGE)
					return nullptr;
				obj_t *o = new obj_t;
				*o = d;
				return o;
			}
			int64_t i = std::strtoll(tmp.c_str(), 0, 10);
			if (errno == ERANGE)
				return nullptr;
			obj_t *o = new obj_t;
			*o = i;
			return o;
		}

		static inline obj_t *get_obj(int &pos, int len, const char * str);
		static inline obj_t* get_vec(int &pos, int len, const char *str)
		{
			obj_t *vec = new obj_t;
			vec->type_ = obj_t::e_vec;

			if (str[pos] == '[')
				pos++;

			while (pos < len)
			{
				switch (str[pos])
				{
				case ']':
					++pos;
					return vec;
				case '[':
				{
					obj_t * obj = get_vec(pos, len, str);
					if (!!!obj)
						goto fail;
					vec->add(obj);
					break;
				}
				case '"':
				{
					auto o = get_str(pos, len, str);
					if (o.first == false)
						goto fail;
					vec->add(o.second);
					break;
				}
				case 'n':
					if (get_null(pos, len, str))
					{
						vec->add(new obj_t);
						break;
					}
				case '{':
				{
					obj_t *tmp = get_obj(pos, len, str);
					if (!!!tmp)
						goto fail;
					vec->add(tmp);
					break;
				}
				case 'f':
				case 't':
				{
					auto b = get_bool(pos, len, str);
					if(b)
						vec->add(true);
					else
						goto fail;
					break;
				}
				case ',':
				case ' ':
				case '\r':
				case '\n':
				case '\t':
					++pos;
					break;
				default:
					if (str[pos] == '-' ||
						str[pos] == '+' ||
						(str[pos] >= '0' && 
						str[pos] <= '9'))
					{
						obj_t *o = get_num(pos, len, str);
						if (o == nullptr)
							goto fail;
						vec->add(o);
					}
				}
			}
		fail:
			delete vec;
			return NULL;
		}

		#define check_ahead(ch)\
		do{\
			if(!skip_space(pos, len, str))\
					goto fail; \
			if(str[pos] != ch)\
				goto fail;\
		} while(0)

		static inline obj_t *
			get_obj(int &pos, int len, const char * str)
		{
			obj_t *obj_ptr = new obj_t;
			obj_t &obj = *obj_ptr;
			std::string key;

			skip_space(pos, len, str);

			if (pos >= len)
				goto fail;
			if (str[pos] == '{')
				++pos;
			else
				goto fail;
			while (pos < len)
			{
				switch (str[pos])
				{
				case '"':
				{
					auto res = get_str(pos, len, str);
					if (res.first == false)
						goto fail;
					if (key.empty())
					{
						key = res.second;
						check_ahead(':');
					}
					else
					{
						*obj[key] = res.second;
						key.clear();
					}
					break;
				}
				case 'f':
				case 't':
				{
					if (key.empty())
						goto fail;
					auto b = get_bool(pos, len, str);
					if (b == nullptr)
						obj.insert(key, b);
					else
						goto fail;
					key.clear();
					break;
				}
				case 'n':
				{
					if (key.empty() ||
						get_null(pos, len, str) == false)
						goto fail;
					obj[key];
				}
				case '{':
				{
					if (key.empty())
						goto fail;
					obj_t *o = get_obj(pos, len, str);
					if (o == NULL)
						goto fail;
					obj.insert(key, o);
					key.clear();
					break;
				}
				case '}':
					if (key.size())
						goto fail;
					++pos;
					return obj_ptr;
				case ':':
				{
					if (key.empty())
						goto fail;
					++pos;
					break;
				}
				case ',':
				{
					++pos;
					check_ahead('"');
					break;
				}
				case ' ':
				case '\r':
				case '\n':
				case '\t':
					++pos;
					break;
				case '[':
				{
					obj_t *v = get_vec(pos, len, str);
					if (!v || key.empty())
						goto fail;
					obj.insert(key,v);
					key.clear();
					break;
				}
				default:
					if (str[pos] == '-' ||
						str[pos] >= '0' && 
						str[pos] <= '9')
					{
						obj_t *o = get_num(pos, len, str);
						if (!o)
							goto fail;
						obj.insert(key, o);
					}
				}
			}
		fail:
			delete obj_ptr;
			return NULL;
		}
	}
	
	static inline obj_t *build_obj(const char *str, int len)
	{
		int pos = 0;
		if (parser::skip_space(pos, len, str) == false)
			return nullptr;
		char ch = str[pos];
		if (ch == '{')
		{
			return parser::get_obj(pos, len, str);
		}
		else if (ch == '[')
		{
			return parser::get_vec(pos, len, str);
		}
		else if (ch == '"')
		{
			auto res = parser::get_str(pos, len, str);
			if (res.first) 
			{
				obj_t *o = new obj_t;
				*o = res.second;
				return o;
			}
		}
		else if (ch == 'n')
		{
			return parser::get_null(pos, len, str);
		}
		else if (('0' <= ch && '9' >= ch)||
				 '-' == ch ||
				 '+' == ch)
		{
			return  parser::get_num(pos, len, str);
		}
		else if (ch == 't' || ch == 'f')
		{
			return parser::get_bool(pos, len, str);
		}
		return nullptr;
	}
	static inline obj_t *build_obj(const std::string &str)
	{
		return build_obj(str.c_str(), (int)str.size());
	}
	static inline void del_obj(obj_t *json)
	{
		delete json;
	}
}
}
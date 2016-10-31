#pragma once
namespace xmart
{
namespace store
{
	namespace xson
	{
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
		struct obj_t;
		struct event_t
		{
			std::string path_;
			enum type_t
			{
				e_null,
				e_add,
				e_update,
				e_delete,
			}type_ = e_null;

			obj_t *obj = NULL;
		};
		struct obj_t
		{
			type_t type_;
			union value_t
			{
				std::string *str_;
				int64_t num_;
				double double_;
				bool bool_;
				std::map<std::string,obj_t*> *obj_;
				std::vector<obj_t*> *vec_;
			} val_;
			obj_t *parent_ = NULL;
			obj_t *root = this;
			uint32_t ver_= 0;
			std::string key_;
			std::map<std::string, std::function<void(event_t)>> watchers_;

			obj_t()
				:type_(e_null)
			{
				memset(&val_, 0, sizeof(val_));
			}
			virtual ~obj_t()
			{
				reset();
			}
			void reset()
			{
				switch (type_)
				{
				case xson::e_null:
				case xson::e_num:
				case xson::e_bool:
				case xson::e_float:
					break;
				case xson::e_str:
					delete val_.str_;
					break;
				case xson::e_obj:
					for (auto&itr : *val_.obj_)
						delete itr.second;
					delete val_.obj_;
					break;
				case xson::e_vec:
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
			typename std::enable_if<std::is_floating_point<T>::value, void>::type
				operator=(T val)
			{
				assert(type_ == e_null);
				type_ = e_float;
				val_.double_ = val;
			}
			void operator=(const char *val)
			{
				assert(type_ == e_null);
				type_ = e_str;
				val_.str_ = new std::string(val);
			}
			
			void operator=(const std::string &val)
			{
				type_ = e_str;
				val_.str_ = new std::string(val);
			}
			template<typename T>
			typename std::enable_if<std::is_integral<T>::value, void>::type
				operator=(T val)
			{
				event_t e;
				if(type_ == e_null)
					e.type_ = event_t::e_add;
				else
					e.type_ = event_t::e_update;
				type_ = e_num;
				val_.num_ = val;
				make_path(e.path_);
				e.obj = this;
				on_change(e);
			}
			void operator=(bool val)
			{
				assert(type_ == e_null);
				type_ = e_bool;
				val_.bool_ = val;
			}
			obj_t &operator=(obj_t &&obj)
			{
				type_ = obj.type_;
				val_ = obj.val_;
				obj.type_ = e_null;
				memset(&obj.val_, 0, sizeof(val_));
				return *this;
			}
			obj_t &operator[](const std::string & key)
			{
				return operator[](key.c_str());
			}

			obj_t &operator[](const char *key)
			{
				assert(type_ == e_null || type_ == e_obj);
				type_ = e_obj;
				if (!!!val_.obj_)
					val_.obj_ = new std::map<std::string, obj_t*>;
				auto itr = val_.obj_->find(key);
				if (itr == val_.obj_->end())
				{
					auto obj = new obj_t;
					obj->parent_ = this;
					obj->root = root;
					obj->key_ = key;
					val_.obj_->emplace(key, obj);
					return *obj;
				}
				return *itr->second;
			}
			bool open_callback(bool b)
			{
				static bool val = false;
				if (b == false)
					return val;
				else
					val = b;
				return val;
			}
			void del()
			{
				if(type_ == e_obj)
				{
					for(auto &itr : *val_.obj_)
						itr.second->del();
				}else if (type_ == e_vec)
				{
					for(auto &itr : *val_.vec_)
						itr->del();
				}
				event_t e;
				make_path(e.path_);
				if(e.path_.size())
					e.path_.pop_back();
				e.type_ = event_t::e_delete;
				e.obj = this;
				reset();
				if (!open_callback(false))
					return;
				on_change(e);
			}
			void on_change(const  event_t &e)
			{
				for (auto &itr: watchers_)
				{
					itr.second(e);
				}
				if(parent_ && e.obj != parent_)
				{
					parent_->on_change(e);
				}
			}
			bool add_watch(const std::string &id,
						   std::function<void(const event_t &)> watcher)
			{
				return watchers_.insert(std::make_pair(id, watcher)).second;
			}
			void make_path(std::string &path)
			{
				path = key_ +"\\" + path;
				if (parent_ && parent_->key_.size())
				{
					parent_->make_path(path);
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
				obj->key_ = "$"+std::to_string(val_.vec_->size());
				*obj = val;
				val_.vec_->push_back(obj);
				return *this;
			}
			
			obj_t& add(obj_t *obj)
			{
				assert(type_ == e_vec || type_ == e_null);
				type_ = e_vec;
				if (val_.vec_ == NULL)
					val_.vec_ = new std::vector<obj_t *>;
				obj->parent_ = this;
				obj->root = root;
				val_.vec_->push_back(obj);
				return *this;
			}
			template<class T>
			typename std::enable_if<std::is_integral<T>::value &&
				!std::is_same<T, bool>::value, T>::type
				get()
			{
				assert(type_ == e_num);
				return static_cast<T>(val_.num_);
			}
			template<class T>
			typename std::enable_if<std::is_same<T, bool>::value, T>::type
				get()
			{
				assert(type_ == e_bool);
				return val_.bool_;
			}
			template<class T>
			typename std::enable_if<std::is_floating_point<T>::value, T>::type
				get()
			{
				assert(type_ == e_float);
				return static_cast<T>(val_.double_);
			}
			template<class T>
			typename std::enable_if<std::is_same<T, std::string>::value, std::string &>::type
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
				for (auto &itr: *val_.obj_)
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
				get_text(int &pos, int len, const char *str)
			{
				assert(len > 0);
				assert(pos >= 0);
				assert(pos < len);
				assert(str[pos] == '"');

				std::string key;
				++pos;
				while (pos < len)
				{
					if (str[pos] != '"')
						key.push_back(str[pos]);
					else if (key.size() && key.back() == '\\')
						key.push_back(str[pos]);
					else
					{
						++pos;
						return{ true, key };
					}
					++pos;
				}
				return{ false,"" };
			}
			static int get_bool(int &pos, int len, const char *str)
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
				if (key == "true")
					return 1;
				else if (key == "false")
					return 0;
				return -1;
			}
			static inline bool get_null(int &pos, int len, const char *str)
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
				if (null == "null")
					return true;
				return false;
			}
			static inline std::string get_num(
				bool &sym, int &pos, int len, const char *str)
			{
				sym = false;
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
							return false;
					}
					else
						break;
				}
				return tmp;
			}

			static inline obj_t *get_obj(int &pos, int len, const char * str);
			static inline obj_t* get_vec(int &pos, int len, const char *str)
			{
				obj_t *vec = new obj_t;
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
						std::pair<bool, std::string > res = get_text(pos, len, str);
						if (res.first == false)
							goto fail;
						vec->add(res.second);
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
						int b = get_bool(pos, len, str);
						if (b == 0)
							vec->add(false);
						else if (b == 1)
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
						if (str[pos] == '-' || str[pos] >= '0' && str[pos] <= '9')
						{
							bool is_float = false;
							std::string tmp = get_num(is_float, pos, len, str);
							errno = 0;
							if (is_float)
							{
								double d = std::strtod(tmp.c_str(), 0);
								if (errno == ERANGE)
									goto fail;
								vec->add(d);
							}
							else
							{
								int64_t val = std::strtoll(tmp.c_str(), 0, 10);
								if (errno == ERANGE)
									goto fail;
								vec->add(val);
							}
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

			static inline obj_t *get_obj(int &pos, int len, const char * str)
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
						std::pair<bool, std::string > res = get_text(pos, len, str);
						if (res.first == false)
							goto fail;
						if (key.empty())
						{
							key = res.second;
							check_ahead(':');
						}
						else
						{
							obj[key] = res.second;
							key.clear();
						}
						break;
					}
					case 'f':
					case 't':
					{
						if (key.empty())
							goto fail;
						int b = get_bool(pos, len, str);
						if (b == 0)
							obj[key] = false;
						else if (b == 1)
							obj[key] = true;
						else
							goto fail;
						key.clear();
						break;
					}
					case 'n':
					{
						if (key.empty() || get_null(pos, len, str) == false)
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
						obj[key] = std::move(*o);
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
						obj_t *vec = get_vec(pos, len, str);
						if (!!!vec || key.empty())
							goto fail;
						obj[key] =  std::move(*vec);
						key.clear();
						break;
					}
					default:
						if (str[pos] == '-' || str[pos] >= '0' && str[pos] <= '9')
						{
							bool is_float = false;
							std::string tmp = get_num(is_float, pos, len, str);
							errno = 0;
							if (is_float)
							{
								double d = std::strtod(tmp.c_str(), 0);
								if (errno == ERANGE)
									return false;
								obj[key] = d;
								key.clear();
							}
							else
							{
								int64_t val = std::strtoll(tmp.c_str(), 0, 10);
								if (errno == ERANGE)
									return false;
								obj[key] = val;
								key.clear();
							}
						}
					}
				}

			fail:
				delete obj_ptr;
				return NULL;
			}
		}
		static inline obj_t *build_json(const std::string &str)
		{
			int pos = 0;
			return parser::get_obj(pos, (int)str.size(), str.c_str());
		}
		static inline obj_t *build_json(const char *str)
		{
			int pos = 0;
			return parser::get_obj(pos, (int)strlen(str), str);
		}
		static inline void destory_json(obj_t *json)
		{
			delete json;
		}
	}
}
}
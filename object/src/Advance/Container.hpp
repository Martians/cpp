
#pragma once

#include <cassert>

#include "Common/Container.hpp"

namespace common {

	/**
	 * register for two-way map
	 * example: RegistMap<int, string> s_errmap = { {DT_time, "timer"}, {DT_down, "down"} };
	 **/
	template<class Key, class Type,	class Comp = std::less<Key>>
	class RegistMap
	{
	public:
		/**
		 * regist pair
		 **/
		struct Pair {
			Key  key;
			Type type;
		};

    public:
		RegistMap(Key key = Key(), Type type = Type())
			: m_key(key), m_type(type) {}

		RegistMap(std::initializer_list<Pair> array) {
			regist(array);
		}

	public:
		/**
		 * regist with
		 **/
		void	regist(const std::initializer_list<Pair> array) {
			for (const auto& pair : array) {
				regist(pair.key, pair.type);
			}
		}

		/**
		 * regist pair
		 **/
		void	regist(const Key& key, const Type& type) {
			assert(m_key != key);
			assert(m_type != type);
			assert(m_key_map.add(key, type));
			assert(m_type_map.add(type, key));
		}

	public:
		/**
		 * get type by key
		 **/
		const Type&	get(const Key& key) {
			Type* type = m_key_map.get(key);
			return type ? *type : m_type;
		}

		/**
		 * get key by type
		 **/
		const Key& rget(const Type& type) {
			Key* key = m_key_map.get(key);
			return key ? *key : m_key;
		}

	protected:
		TypeMap<Key, Type> m_key_map;
		TypeMap<Type, Key> m_type_map;
		/** default key */
		Key  m_key = {};
		/** defualt type */
        Type m_type = {};
	};

	/**
	 * two-level map, map key to struct
	 * example: typedef std::tuple<UnitIndex, int> UnitStatus;
	 *			MapContain<int, UnitStatus> mIndexMap
	 **/
	template<class Key, class Type>
	class MapContain
	{
	public:
		/**
		 * regist pair
		 **/
		struct Pair {
			Key  	key;
			std::set<Type> set;
		};
	public:
		MapContain(Key key = Key(), Type type = Type())
			: m_key(key), m_type(type) {}

		MapContain(std::initializer_list<Pair> array) {
			regist(array);
		}

	public:
		/**
		 * regist by array
		 **/
		void	regist(std::initializer_list<Pair> array) {
			for (const auto& pair : array) {
				for (const auto& type : pair.set) {
					assert(add(pair.key, type));
				}
			}
		}

		/**
		 * get inner size
		 **/
		size_t	size(const Key& key) {
			if (key == m_key) {
				return m_map.size();

			} else {
				SetType* vec = m_map.get(key);
				return vec ? vec->size() : 0;
			}
		}

		/**
		 * add new entry
		 **/
		Type*	add(const Key& key, const Type& type) {
			SetType* set = NULL;
			if (!(set = m_map.get(key))) {
				if (!(set = m_map.add(key))) {
					return NULL;
				}
			}
			return set->add(type);
		}

		/**
		 * add new entry
		 **/
		bool	del(const Key& key, const Type& type) {
			SetType* set = NULL;
			if ((set = m_map.get(key))) {
				if (set->del(type)) {
					if (set->size() == 0) {
						m_map.del(set);
					}
					return true;
				}
			}
			return false;
		}

		/**
		 * loop init
		 **/
		bool	init(const Key& key) {
			/** default key, loop all */
			if (key == m_key) {
				m_single = false;
				if (m_map.init()) {
					while ((m_last = m_map.next())) {
						if (m_last->init()) {
							return true;
						}
					}
				}
			} else {
				m_single = true;
				if ((m_last = m_map.get(key))) {
					return m_last->init();
				}
			}
			m_last = NULL;
			return false;
		}

		/**
		 * loop next
		 **/
		Type*	next() {
			if (m_last) {
				Type* type = m_last->next();
				if (type) {
					return type;

				} else if (!m_single) {
					while ((m_last = m_map.next())) {
						if (m_last->init()) {
							return m_last->next();
						}
					}
				}
				m_last = NULL;
			}
			return NULL;
		}

		/**
		 * traverse next value
		 **/
		bool	next(Type& type) {
			Type* value = next();
			if (value) {
				type = *value;
				return true;
			}
			return false;
		}

	protected:
		typedef TypeSet<Type> SetType;
		TypeMap<Key, SetType> m_map;
		bool  m_single  = {false};
		/** loop single */
		SetType* m_last = {NULL};
		/** default key */
		Key  m_key = {};
		/** defualt type */
		Type m_type = {};
	};
}
#if COMMON_SPACE
	using common::RegistMap;
	using common::MapContain;
#endif


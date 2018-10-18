
#pragma once

#include <map>
#include <set>

namespace common {

	/** loop container */
	#define loop_for_each(Type, type, container)	\
		Type type;  Type* ptr = NULL;				\
		for (container.init(), ptr = container.next(); (ptr != NULL && (type = *ptr)); ptr = container.next())

	#define	loop_for_each_ptr(Type, type, container)\
		loop_for_each(Type*, type, container)

	#define	loop_for_each_key(key, type, container)\
		for (container.init(), type = container.next(&key); type != NULL; type = container.next(&key))

	/**
	 * @brief type set
	 */
	template<class Type, class Comp = std::less<Type> >
	class TypeSet
	{
	public:
		TypeSet() {}
		TypeSet(std::initializer_list<Type> array) {
			for (const auto &type : array) {
				success(add(type));
			}
		}
		virtual ~TypeSet() {}

	public:
		typedef typename std::set<Type, Comp> curr_type;
		typedef typename curr_type::iterator  curr_iter;

		/**
		 * get set size
		 **/
		size_t	size() { return m_local.size(); }

		/**
		 * clear map
		 **/
		void	clear() { m_local.clear(); }

		/**
		 * resize map
		 **/
		void	resize(unsigned int size = 0) { m_local.resize(size); }

		/**
		 * add new type
		 **/
		Type*	add(Type* v) {
			return m_local.find(*v) != m_local.end() ? NULL :
				const_cast<Type*>(&(*m_local.insert(*v).first));
		}

		/**
		 * add new type
		 **/
		Type*	add(const Type& v) {
			return m_local.find(v) != m_local.end() ? NULL :
				const_cast<Type*>(&(*m_local.insert(v).first));
		}

		/**
		 * del type
		 **/
		bool	del(Type* v) { return m_local.erase(*v) != 0; }

		/**
		 * del type
		 **/
		bool	del(const Type& v) { return m_local.erase(v) != 0; }

		/**
		 * get type
		 **/
		Type*	get(Type* v) {
			curr_iter it = m_local.find(*v);
			return it == m_local.end() ? NULL : const_cast<Type*>(&(*it));
		}

		/**
		 * get type
		 **/
		Type*	get(const Type& v) {
			curr_iter it = m_local.find(v);
			return it == m_local.end() ? NULL : const_cast<Type*>(&(*it));
		}

		/**
		 * try to traverse container
		 *
		 * @return false if no element
		 **/
		bool	init() {
			if (m_local.empty()) {
				m_curr = m_local.end();
				return false;
			}
			m_curr = m_local.begin();
			return true;
		}

		/**
		 * traverse next type
		 *
		 * @return type ptr
		 **/
		Type*	next() {
			return m_curr == m_local.end() ? NULL :
				const_cast<Type*>(&(*m_curr++));
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

		/**
		 * min value
		 **/
		Type*	minv() {
			if (m_local.empty()) {
				return NULL;
			}
			return const_cast<Type*>(&(*m_local.begin()));
		}

		/**
		 * max value
		 **/
		Type*	maxv() {
			if (m_local.empty()) {
				return NULL;
			}
			return const_cast<Type*>(&(*m_local.rbegin()));
		}

		/**
		 * begin reference
		 **/
		curr_iter begin() { return m_local.begin(); }

		/**
		 * end reference
		 **/
		curr_iter end() { return m_local.end(); }

	protected:
		/** set container */
		curr_type	m_local;
		/** set iterator */
		curr_iter	m_curr;
	};

	/**
	 * type map
	 **/
	template<class Key, class Type,	class Comp = std::less<Key> >
	class TypeMap
	{
	public:
		TypeMap() {}

		TypeMap(Type type) : m_type(type) {}

		struct Pair {
			Key	 key;
			Type type;
		};

		TypeMap(std::initializer_list<Pair> array) {
			for (const auto &pair : array) {
				success(add(pair.key, pair.array));
			}
		}
	public:
		typedef typename std::map<Key,Type, Comp> curr_type;
		typedef typename curr_type::iterator	curr_iter;
		typedef typename curr_type::value_type 	curr_value;

		/**
		 * get map size
		 **/
		size_t	size() { return m_local.size(); }

		/**
		 * @brief clear map
		 */
		void	clear() { m_local.clear(); }

		/**
		 * add new pair
		 *
		 * @param key key value
		 * @param type type ptr
		 * @return new add type
		 **/
		Type*	add(const Key& key, Type* type = NULL) {
			std::pair<curr_iter,bool> ret = m_local.insert(curr_value(key, type == NULL ? m_type : *type));
			return ret.second ? &(*ret.first).second : NULL;
		}

		/**
		 * add new pair
		 *
		 * @param key key value
		 * @param type type ptr
		 * @return new add type
		 **/
		Type*	add(const Key& key, const Type& type) {
			std::pair<curr_iter,bool> ret = m_local.insert(curr_value(key, type));
			return ret.second ? &(*ret.first).second : NULL;
		}

		/**
		 * del key entry
		 *
		 * @param key key value
		 **/
		bool	del(const Key& key) { return m_local.erase(key) != 0; }

		/** get type
		 *
		 * @param key type key
		 **/
		Type*	get(const Key& key) {
			curr_iter it = m_local.find(key);
			return it == m_local.end() ? NULL : &(*it).second;
		}

		/**
		 * try to traverse container
		 *
		 * @return false if no element
		 **/
		bool	init() {
			if (m_local.empty()) {
				m_curr = m_local.end();
				return false;
			}
			m_curr = m_local.begin();
			return true;
		}

		/**
		 * traverse next type
		 *
		 * @param key [out] the key correspond to the type
		 * @return type ptr
		 **/
		Type*	next(Key* key = NULL) {
			if (m_curr == m_local.end()) {
				return NULL;
			}
			if (key) {
				*key = (*m_curr).first;
			}
			return &(*m_curr++).second;
		}

		/**
		 * traverse next value
		 **/
		bool	next(Key& key, Type& type) {
			Type* value = next(&key);
			if (value) {
				type = *value;
				return true;
			}
			return false;
		}

		/**
		 * min value
		 **/
		Type*	minv(Key* key = NULL) {
			if (m_local.empty()) return NULL;
			if (key) *key = (*m_local.begin()).first;
			return &(*m_local.begin()).second;
		}

		/**
		 * max value
		 **/
		Type*	maxv(Key* key = NULL) {
			if (m_local.empty()) return NULL;
			if (key) *key = (*m_local.rbegin()).first;
			return &(*m_local.rbegin()).second;
		}

		/**
		 * begin reference
		 **/
		curr_iter begin() { return m_local.begin(); }

		/**
		 * end reference
		 **/
		curr_iter end() { return m_local.end(); }

	protected:
		/** default type */
		Type	m_type = {};
		/** map container */
		curr_type	m_local;
		/**  traverse iterator */
		curr_iter	m_curr;
	};
}

#if COMMON_SPACE
	using common::TypeSet;
	using common::TypeMap;
#endif

/* vim: set ts=4 sw=4 expandtab */

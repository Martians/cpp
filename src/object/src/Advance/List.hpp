
#pragma once

#include <cassert>

#include "Common/Const.hpp"

namespace common
{
	/** check if link is good */
	#define LIST_CHECK_NODE_LINK		1
	/** use high link point to list */
	#define LIST_USE_HIGH_LINK			1

	/*
	 * @brief doubly linked list
	 * @note work as list head or link
	 */
	struct ListLink
	{
	public:
		ListLink() : next((ListLink*)c_invalid_ptr),
			prev((ListLink*)c_invalid_ptr) {}

		/** unset link */
		explicit ListLink(bool unset) {}

		/** note, should not use virtual destruct, 
			or you can't use link_t as a place holder */
		//virtual ~ListLink() { /*unlink(); */}

	public:
		/**
		 * @brief delete entry from list
		 * @note should never del list head
		 */
		void	unlink() {
			/** should make sure about this by user */
			//if (!linked()) return;
			__del(prev, next);
			__init();
		}

		/**
		 * @brief check if node is linked
		 * @note if c_invalid_ptr is not null, we should check
		 * if ptr is null, for null is the unused state
		 */
		bool	linked() { return next != (ListLink*)c_invalid_ptr &&
							prev != (ListLink*)c_invalid_ptr; }

		/**
		 * @brief clear link state
		 */
		void	clear(bool force = false) {
			if (force) {
				__init();

			} else if (linked()) {
				unlink();
			}
		}

		/**
		 * @brief replace us with new link
		 */
		void	replace(ListLink* link, bool init = false) {
			link->next = next;
			link->prev = prev;
			
			link->prev->next = link;
			link->next->prev = link;

			if (init) {
				__init();
			}
		}

		/**
		 * @brief replace us with new node
		 */
		void	replace_by(ListLink* link, bool init = false) { link->replace(this, init); }

		/**
		 * @brief add new link after curr
		 */
		void	add_after(ListLink* link) { __add(link, this, next); }

		/**
		 * @brief add new link before curr
		 */
		void	add_before(ListLink* link) { __add(link, prev, this); }

		/**
		 * @brief unlink list link
		 */
		static void unlink(void* link) { ((ListLink*)link)->unlink(); }

	protected:
		/**
		 * @brief init and set invalid position
		 */
		void	__init() {
			next = (ListLink*)c_invalid_ptr;
			prev = (ListLink*)c_invalid_ptr;
		}

		/**
		 * @brief add link between two consecutive entries
		 */
		static void	__add(ListLink* _link, ListLink* _prev, ListLink* _next) {
			_next->prev = _link;
			_link->next = _next;
			_link->prev = _prev;			
			_prev->next = _link;
		}

		/**
		 * @brief del link by making two consecutive entries
		 */
		static void	__del(ListLink* _prev, ListLink* _next) {
			_prev->next = _next;
			_next->prev = _prev;
		}

	public:
		/** next node */
		ListLink*	next;
		/** prev node */
		ListLink*	prev;
	};

	class BaseAlloter;

	/**
	 * @brief list head
	 */
	class ListHead : public ListLink
	{
	public:
		ListHead() { init_head(); }

		/** operator = */
		const ListLink& operator = (const ListLink& v) {
			next = v.next;
			prev = v.prev;
			return *this;
		}

	public:
		/**
		 * @brief add link to list head
		 */
		void	add_head(ListLink* link) { __add(link, this, next); }

		/**
		 * @brief add link to list tail
		 */
		void	add_tail(ListLink* link) { __add(link, prev, this); }

		/**
		 * @brief move list from head
		 */
		void	move_head(ListLink* link) {
			__del(link->prev, link->next);
			__add(link, this, next);
		}

		/**
		 * @brief move list from head
		 */
		void	move_tail(ListLink* link) {
			__del(link->prev, link->next);
			__add(link, prev, this);
		}

		/**
		 * @brief pop list head
		 */
		ListLink* del_head() { 
			if (empty()) {
				return NULL;
			}
			ListLink* head = next;
			head->unlink();
			return head;
		}

		/**
		 * @brief pop list tail
		 */
		ListLink* del_tail() {
			if (empty()) {
				return NULL;
			}
			ListLink* tail = prev;
			tail->unlink();
			return tail;
		}
		
		/**
		 * @brief del node from list
		 */
		void	del(ListLink* link) { link->unlink(); }

		/**
		 * @brief tests whether the link is the first entry
		 */
		bool	is_head(ListLink* link) { return link->prev == this; }

		/**
		 * @brief tests whether the link is the last entry
		 */
		bool	is_tail(ListLink* link) { return link->next == this; }

		/**
		 * @brief check if list is empty
		 */
		bool	empty() { return next == this; }

		/**
		 * @brief reverse the list
		 */
		void	reverse() {
			ListHead tmp;
			ListLink* link;
			while ((link = del_tail())) {
				tmp.add_tail(link);
			}
			tmp.add_head(this);
			tmp.unlink();
		}

		/**
		 * @brief init list head
		 */
		void	init_head() { 
			next = this;
			prev = this;
		}
	};

	class List;

	/**
	 * @brief high level link, which can delete from list by itself
	 */
	class HighLink : public ListHead
	{
	public:
		HighLink() {}

	public:
		/**
	 	 * @brief delete entry from list
		 * @note should never del list head
		 */
		void	del();

	public:
		/** list pointer */
		List*	list = {NULL};
	};

	/**
	 * list_entry get list entry
	 * ptr:		struct pointer
	 * type:	struct name
	 * member:	link member name
	 */
	#define	list_entry(ptr, type, member)	\
		container_of(ptr, type, member)

	/**
	 * list_for_each - iterate over a list
	 * @pos:	the &struct list_head to use as a loop cursor.
	 * @head:	the head for your list.
	 * @type	struct type name
	 * @member	the name of the list member within the struct
	 */
	#define	list_for_each(pos, head, type, member)			\
		for (pos = list_entry((head)->next, type, member);	\
			&pos->member != (head);							\
			pos = list_entry(pos->member.next, type, member))

	/**
	 * list_for_each - iterate over a list, given type safe against removal of list entry
	 * @pos:	the &struct list_head to use as a loop cursor.
	 * @head:	the head for your list.
	 * @type	struct type name
	 * @member	the name of the list member within the struct
	 */
	#define	list_for_each_safe(pos, head, type, member)		\
		ListLink* save = (head)->next;						\
		for (pos = list_entry(save, type, member),save = save->next;\
			&pos->member != (head);							\
			pos = list_entry(save, type, member), save = save->next)

	typedef void (*item_destory_t)(void* item);

	/**
	 * @brief list wrapper for convenient type list
	 */
	class List
	{
	public:
		/**
		 * @brief list base construct
		 * @param pos link part offset in type
		 * @note the type have link part itself
		 */
		List(uint32_t off = 0, bool high = false)
			: m_pos(off), m_bits(high ? ll_high : 0) {}

		/**
		 * @brief list construct
		 * @param at data link alloter for type, if null, set as default
		 * @note type do not have link part, alloc for it when need
		 */
		List(BaseAlloter* at) { set(at); }

		/**
		 * @brief hold type in list
		 * @note used for type have no link part
		 */
		struct DataLink : public ListLink {
			DataLink(void* d) : data(d) {}
			void* data;
		};

	public:
		/**
		 * @brief list node
		 */
		enum _TAG
		{
			ll_null = 0,
			/** use high link */
			ll_high,
			/** reverse traverse */
			ll_back,
		};

		/** 
		 * @brief set list offset
		 */
		void	set(int off, bool high = false) {
			m_pos = off;
			set_bit(ll_high, high); }

		/** 
		 * @brief set link alloter
		 */
		void	set(BaseAlloter* at = NULL) { m_alloter = at; }

		/**
		 * @brief clear list entry
		 * @param destroy handle
		 */
		void	clear(item_destory_t destroy = NULL);

		/**
		 * @brief get list size
		 */
		size_t	size() { return m_size; }

		/** 
		 * @brief check if list is empty
		 */
		bool	empty() { return m_head.empty(); }

		/**
		 * @brief del entry from list
		 * @note have not check if link is in such list
		 */
		bool	del(void* ptr) {
			/** if set alloter, should not use this */
			assert(!m_alloter);

			ListLink* node = link(ptr);
			if (!node->linked()) {
				return false;
			}

			#if LIST_USE_HIGH_LINK
			if (get_bit(ll_high)) {
				HighLink* hnode = (HighLink*)node;
				if (hnode->list != this) return false;
				hnode->list = NULL;
			}
			#endif

			node->unlink();
			m_size--;
			return true;
		}

		/**
		 * @brief add entry to list tail
		 */
		void	enque(void* ptr) {
			ListLink* node = new_link(ptr);
			
			#if LIST_CHECK_NODE_LINK
			/** check if not unlink */
			if (node->linked()) assert(0);
			#endif

			#if LIST_USE_HIGH_LINK
			if (get_bit(ll_high)) {
				HighLink* hnode = (HighLink*)node;
				if (hnode->list != NULL) assert(0);
				hnode->list = this;
			}
			#endif

			m_head.add_tail(node);
			m_size++;
		}

		/**
		 * @brief pop head entry
		 */
		void*	deque() {
			ListLink* node = m_head.del_head();
			if (!node) return NULL;

			#if LIST_USE_HIGH_LINK
			if (get_bit(ll_high)) {
				HighLink* hnode = (HighLink*)node;
				hnode->list = NULL;
			}
			#endif

			m_size--;
			return del_link(node);
		}

		/**
		 * @brief add entry to list head
		 */
		void	enque_head(void* ptr) {
			ListLink* node = new_link(ptr);
	
			#if LIST_CHECK_NODE_LINK
			/** check if not unlink */
			if (node->linked()) assert(0);
			#endif

			#if LIST_USE_HIGH_LINK
			if (get_bit(ll_high)) {
				HighLink* hnode = (HighLink*)node;
				if (hnode->list != NULL) assert(0);
				hnode->list = this;
			}
			#endif

			m_head.add_head(node);
			m_size++;
		}

		/**
		 * @brief pop tail entry
		 */
		void*	deque_tail() {
			ListLink* node = m_head.del_tail();
			if (!node) return NULL;

			#if LIST_USE_HIGH_LINK
			if (get_bit(ll_high)) {
				HighLink* hnode = (HighLink*)node;
				hnode->list = NULL;
			}
			#endif

			m_size--;
			return del_link(node);
		}

		/**
		 * @brief add entry to list tail
		 */
		void	enque_tail(void* ptr) { return enque(ptr); }

		/**
		 * @brief pop head entry
		 */
		void*	deque_head() { return deque(); }

		/**
		 * @brief move entry to head
		 */
		void	move_head(void* ptr) {
			/** if set alloter, should not use this */
			assert(!m_alloter);

			ListLink* _link = link(ptr);
			if (m_head.is_head(_link)) return;
			m_head.move_head(_link);
		}

		/**
		 * @brief move entry to tail
		 */
		void	move_tail(void* ptr) {
			/** if set alloter, should not use this */
			assert(!m_alloter);

			ListLink* _link = link(ptr);
			if (m_head.is_tail(_link)) return;
			m_head.move_tail(_link);
		}

		/**
		 * @brief replace entry with new one
		 */
		void	replace(void* dst, void* src) {
			/** if set alloter, should not use this */
			assert(!m_alloter);

			ListLink* sl = link(src);
			ListLink* dl = link(dst);

			#if LIST_USE_HIGH_LINK
			if (get_bit(ll_high)) {
				HighLink* hnodes = (HighLink*)sl;
				HighLink* hnoded = (HighLink*)dl;
				hnodes->list = this;
				hnoded->list = NULL;
			}
			#endif

			dl->replace(sl);
		}

		/**
		 * @breif prepare to traverse for each
		 */
		bool	init(bool back = false) { 
			m_curr = (!back ? m_head.next : m_head.prev);
			set_bit(ll_back, back);
			return !m_head.empty(); 
		}

		/**
		 * @breif get next type
		 */
		void*	next() {
			if (m_curr == &m_head) {
				return NULL;
			}
			void* ptr = get_item(m_curr);
			m_curr = (!get_bit(ll_back) ? m_curr->next : m_curr->prev);
			return ptr;

		}

		/**
		 * @breif get next link
		 */
		ListLink* next_link() {
			if (m_curr == &m_head) {
				return NULL;
			}
			ListLink* link = m_curr;
			m_curr = (!get_bit(ll_back) ? m_curr->next : m_curr->prev);
			return link;
		}

		/**
		 * @brief get head struct
		 */
		void*	headv() { return m_head.empty() ? NULL : get_item(m_head.next); }

		/**
		 * @brief get tail struct
		 */
		void*	tailv() { return m_head.empty() ? NULL : get_item(m_head.prev); }

		/**
		 * @brief append element of another list to the end
		 */
		void	append(List* v);

		/**
		 * @brief append element to another list end
		 */
		void	reader(List* v) { v->append(this); }

		/**
		 * @brief swap two list
		 */
		void	swap(List* v);

		/** compatible with old version */
		//#define	add_tail enque
		//#define	pop_head deque
	public:
		/**
		 * inner iterator
		 **/
		class iterator {
		public:
			iterator(List* _list, bool end = false) : list(_list),
				curr(!end ? _list->m_head.next : &_list->m_head) {}

		public:
			bool operator != (const iterator& v) {
				assert(v.list == list);
				return curr != v.curr;
			}

			iterator& operator ++ () {
				curr = curr->next;
				return *this;
			}
			void* operator* () {
				return list->get_item(curr);
			}
		protected:
			List* list;
			ListLink* curr;
		};

		/**
		 * get list begin
		 **/
		iterator	begin() { return iterator(this, false); }

		/**
		 * get list end
		 **/
		iterator	end() { return iterator(this, true); }

	protected:
		/** 
		 * @brief get link according to entry ptr 
		 */
		ListLink* link(void* ptr) { return (ListLink*)((char*)ptr + m_pos); }

		/**
		 * @brief get entry according to link ptr
		 */
		void*	entry(ListLink* link) { return (char*)link - m_pos; }

		/**
		 * @brief alloc new link for type
		 * @note if type set
		 */
		ListLink* new_link(void* ptr);

		/**
		 * @brief del link and return item
		 */
		void*	del_link(ListLink* link);

		/**
		 * set inner bit
		 **/
		void	set_bit(int flag, bool set);

		/**
		 * get inner bit
		 **/
		bool	get_bit(int flag);

		/**
		 * @brief get type by link
		 */
		void*	get_item(ListLink* link) {
			if (m_alloter) {
				void* data = ((DataLink*)link)->data;
				return data;
			} else {
				return entry(link);
			}
		}
		friend class iterator;

    protected:
		/** define list head */
		ListHead	m_head;
		/** list node offset in entry */
		uint32_t	m_pos  = {0};
		/** list tag */
		int			m_bits = {0};
		/** list count */
		size_t		m_size = {0};
		/** alloter for data link */
		BaseAlloter* m_alloter = {NULL};
		/** type save ptr for traverse */
		ListLink*	m_curr = {NULL};
	};
}


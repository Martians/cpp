
#pragma once

namespace common
{
	/** check if link is good */
	#define SINGLE_LIST_CHECK_NODE_LINK		1

	/**
	 * single linked list
	 **/
	struct SingleLink
	{
	public:
		/**
		 * set link
		 **/
		void	set(void* _link) { next = (SingleLink*)_link; }

		/**
		 * clear next
		 **/
		void	clear() { next = NULL; }

		/**
		 * check if is linked
		 **/
		bool	linked() { return next != NULL; }

	public:
		SingleLink* next = {NULL};
	};

	class SingleHead : public SingleLink
	{
	public:
		/**
		 * check if list is empty
		 **/
		bool		empty() { return next == NULL; }

		/**
		 * push link after head
		 **/
		void		enque(SingleLink* link) {
			link->next = next;
			next = link;
		}

		/**
		 * pop link head
		 **/
		SingleLink*	deque() {
			if (next) {
				SingleLink* link = next;
				next = next->next;
				return link;
			}
			return NULL;
		}
	};

	/**
	 * @brief list wrapper for convenient type list
	 */
	class SingleList
	{
	public:
		/**
		 * @brief list base construct
		 * @param pos link part offset in type
		 * @note the type have link part itself
		 */
		SingleList(uint32_t off = 0)
			: m_pos(off), m_size(0), m_curr(NULL) {}

	public:
		/**
		 * @brief set list offset
		 */
		void	set(int off) { m_pos = off; }

		/**
		 * @brief clear list entry
		 * @param destroy handle
		 */
		void	clear(item_destory_t destroy = NULL) {
			if (destroy) {
				SingleLink* ptr;
				init();
				while ((ptr = next_link())) {
					void* item = entry(ptr);
					if (destroy) {
						destroy(item);
					}
				}
			}
			m_head.clear();
			m_size = 0;
		}

		/**
		 * @brief get list size
		 */
		size_t	size() { return m_size; }

		/**
		 * @brief check if list is empty
		 */
		bool	empty() { return m_head.empty(); }

		/**
		 * @brief get head struct
		 */
		void*	headv() { return m_head.empty() ? NULL : entry(m_head.next); }

		/**
		 * @brief add entry to list tail
		 */
		void	enque(void* ptr) {
			SingleLink* node = link(ptr);

			#if SINGLE_LIST_CHECK_NODE_LINK
			/** check if not unlink */
			if (node->linked()) assert(0);
			#endif

			m_head.enque(node);
			m_size++;
		}

		/**
		 * @brief pop head entry
		 */
		void*	deque() {
			SingleLink* node = m_head.deque();
			if (!node) {
				return NULL;
			}
			node->clear();

			m_size--;
			return entry(node);
		}

		/**
		 * @breif prepare to traverse for each
		 */
		bool	init(bool back = false) {
			m_curr = m_head.next;
			return !m_head.empty();
		}

		/**
		 * @breif get next type
		 */
		void*	next() {
			if (m_curr == NULL) {
				return NULL;
			}
			void* ptr = entry(m_curr);
			m_curr = m_curr->next;
			return ptr;
		}

		/**
		 * @breif get next link
		 */
		SingleLink* next_link() {
			if (m_curr == NULL) {
				return NULL;
			}
			SingleLink* link = m_curr;
			m_curr = m_curr->next;
			return link;
		}

	public:
		/**
		 * inner iterator
		 **/
		class iterator {
		public:
			iterator(SingleList* _list, bool end = false) : list(_list),
				curr(!end ? _list->m_head.next : NULL) {}

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
				return list->entry(curr);
			}
		protected:
			SingleList* list;
			SingleLink* curr;
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
		SingleLink* link(void* ptr) { return (SingleLink*)((char*)ptr + m_pos); }

		/**
		 * @brief get entry according to link ptr
		 */
		void*		entry(SingleLink* link) { return (char*)link - m_pos; }

        friend class iterator;

    protected:
		/** define list head */
		SingleHead	m_head;
		/** list node offset in entry */
		uint32_t	m_pos;
		/** list count */
		size_t		m_size;
		/** type save ptr for traverse */
		SingleLink*	m_curr;
	};
}


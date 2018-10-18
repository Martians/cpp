
#pragma once

#include <vector>
#include <memory>
#include <functional>

namespace common {

	typedef std::function<void()> task_t;

	/**
	 * bind with pointer member function
	 *
	 * @note must use pointer or shared ptr
	 **/
	#define BIND(pointer, member, arg...)	\
		std::bind(&std::remove_reference<decltype(*pointer)>::type::member, pointer, ##arg)

	/**
	 * bind in current class
	 **/
	#define BIND_THIS(member, arg...)		\
		BIND(this, member, ##arg)

	/**
	 * bind with member function
	 *
	 * @note must use a reference type
	 **/
	#define BIND_VAL(refer, member, arg...)	\
		std::bind(&decltype(refer)::member, &refer, ##arg)


	#if 0 // do nothing
	/**
	 * bind to void handle
	 **/
	template<class Handle, class... Args>
	auto binds(Handle&& handle, Args&&...args) -> std::function<decltype(*handle(args...))()> {
		return std::bind(std::forward<Handle>(handle), std::forward<Args>(args)...);
	}
	#endif

	/**
	 * handle list
	 **/
	class HandleList
	{
	public:
		/**
		 * regist display handle
		 **/
		template<class Handle, class ... Types>
		void 	add(Handle&& handle, Types&&...args) {
			m_handle.push_back(std::bind(std::forward<Handle>(handle), std::forward<Types>(args)...));
		}

		/**
		 * expand handle
		 **/
		void	work() {
			for (auto& handle : m_handle) {
				handle();
			}
		}

	public:
		/**
		 * check if local is empty
		 **/
		bool	empty() { return m_handle.empty(); }

		/**
		 * set handle with index
		 **/
		template<class Handle, class ... Types>
		void 	insert(int index, Handle&& handle, Types&&...args) {
			task_t curr = std::bind(std::forward<Handle>(handle), std::forward<Types>(args)...);
			if (index == -1) {
				index = m_handle.size();
			}
			if (index >= m_handle.size()) {
				task_t task;
				m_handle.resize(index + 1, task);
			}
			m_handle[index] = curr;
		}

	protected:
		 std::vector<task_t> m_handle;
	};

}


#pragma once

#include "Common/Const.hpp"
#include "Advance/BaseBuffer.hpp"

namespace common {

	class BaseAlloter;

	/**
	 * set dyn chunk length and align
	 **/
	void	dyn_chunk_pool(length_t len, length_t align);

	/**
	 * @brief dynamic buffer
	 */
	class DynBuffer : public BaseBuffer
	{
	public:
		/** 
		 * @brief constructor
		 */
		DynBuffer() {}

		/** 
		 * @brief constructor
		 * @param chunk alloter
		 */
		DynBuffer(BaseAlloter* alloter) : m_alloter(alloter) {}

		/** 
		 * @brief destructor, free chunks
		 */
		virtual ~DynBuffer() { clear(); }
		
		/** 
		 * @brief copy constructor
		 */
		DynBuffer(const DynBuffer& v) {
			operator = (v);
		}

		/** 
		 * @brief = operator, copy existing data
		 */
		const DynBuffer& operator = (const DynBuffer& v) {
			if (this != &v) {
				clear();
				copy(const_cast<DynBuffer*>(&v));
				m_alloter = v.m_alloter;
			}
			return *this;
		}

		struct Chunk;

	public:
		/** 
		 * @brief write data from buffer to dyn
		 * @param data src buffer
		 * @param len write len
		 */
		virtual length_t write(const void* data, length_t len);

		/**
		 * @brief handle data
		 * @param handle function
		 * @param ptr handle param
		 * @param len handle data length
		 * @param off handle data offset
		 **/
		virtual length_t dispatch(data_handle_t handle, void* ptr,
				length_t len = BaseBuffer::c_invalid_length, length_t off = 0) const;

		/** 
		 * @brief remove data from dyn buffer
		 * @param len remove len
		 * @note free chunk not in use
		 */
		virtual length_t remove(length_t len);

	public:
		/** 
		 * @brief retrieve last space in buffer, from end to start
		 * @param len retrieve len
		 * @return actually remove length
		 */
		length_t	retrive(length_t len);

		/** 
		 * @brief set chunk alloter
		 */
		void		alloter(BaseAlloter* alloter) { m_alloter = alloter; }

		/**
		 * @brief get inner alloter
		 **/
		BaseAlloter* alloter();

		/** 
		 * @brief get new chunk
		 * @note if set chunk alloter, use it 
		 */
		Chunk*		new_chunk();

		/** 
		 * @brief del chunk
		 * @note if set chunk alloter, use it
		 */
		void		del_chunk(Chunk* chunk);

		/** 
		 * @brief copy data from another dyn
		 * @param dyn src dyn buffer
		 * @param len read len
		 * @note data position maybe not be same as the source dyn, copy
		 * data and append to the origin dyn data end, then data is continuous
		 */
		//length_t	copy(DynBuffer* dyn, length_t len = BaseBuffer::c_invalid_length);

		/** 
		 * @brief grab data from another dyn
		 * @param dyn src dyn buffer
		 * @param len read len
		 * @note will not change data position in chunk if no needed, maybe do
		 * more work at start chunk and end chunk 
		 */
		length_t	append(DynBuffer* dyn, length_t len = BaseBuffer::c_invalid_length);

		/** 
		 * @brief close old data and attach to another dyn
		 * @param dyn the src dyn
		 * @note grab data from dyn
		 */
		void		attach(DynBuffer* dyn) {
			clear();

			if (m_alloter != dyn->m_alloter) {
				copy(dyn);
			} else {
				append(dyn);
			}
		}

		/** 
		 * @brief free all chunks
		 */
		void		clear();

		/** 
		 * @brief adjust data position
		 * @note not complete, we could copy to another dyn first, then
		 *append back
		 */
		void		adjust();

		/** 
		 * @brief operator []
		 * @return -1 if exceed data length
		 */
		char 		operator[] (length_t pos) const;

		/**
		 * get head chunk
		 **/
		Chunk*		head() { return m_head; }

		/**
		 * get tail chunk
		 **/
		Chunk*		tail() { return m_tail; }

		/** 
		 * @brief check buffer length
		 */
		void		check();

		/**
		 * @brief show debug info
		 */
		void		dump();

		/** default chunk length */
		static const int c_length = c_length_64K;

	protected:
		/** 
		 * @brief append chunk to dyn buffer
		 * @param chunk if null, will new it
		 * @return the chunk just add in
		 */
		Chunk*		append_chunk(Chunk* chunk = NULL);

	public:
		/** start chunk */
		Chunk*	m_head = {NULL};
		/** end chunk */
		Chunk*	m_tail = {NULL};
		/** use allocter */
		BaseAlloter* m_alloter = {NULL};
	};
}

#if COMMON_SPACE
	using common::DynBuffer;
#endif


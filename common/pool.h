#ifndef POOL_H
#define POOL_H

#include <condition_variable>
#include <mutex>
#include <list>
#include <memory>
#include <functional>




template <typename T>
class Pool
{
public:
	class PoolResource
	{
	public:
		PoolResource(Pool<T>* pool, std::shared_ptr<T> t) :
			m_pool(pool),
			m_t(t)
		{

		}

		PoolResource(const PoolResource& other)
		{
			m_pool = other.m_pool;
			m_t = other.m_t;
			other.m_moved_obj = true;
		}
		PoolResource& operator = (const PoolResource& ori) = delete;

		T* operator -> ()
		{
			return &(*m_t);
		}
		~PoolResource()
		{
			if (!m_moved_obj)
			{
				m_pool->Push(m_t);
			}
		}
	private:
		std::shared_ptr<T> m_t;
		Pool<T> *m_pool;
		mutable bool m_moved_obj{ false };
	};

	Pool(std::shared_ptr<T> t) :
		m_resource(t)
	{
	}

	PoolResource Pop()
	{
		std::lock_guard<std::mutex> lck(m_mutex);
		if (m_list_resources.empty())
		{
			m_list_resources.push_back(std::shared_ptr<T>(m_resource->Clone()));
		}
		auto t = m_list_resources.front();
		m_list_resources.pop_front();
		return PoolResource(this, t);
	}

	int GetListCnt()
	{
		std::lock_guard<std::mutex> lck(m_mutex);
		return m_list_resources.size();
	}

private:
	void Push(std::shared_ptr<T> t)
	{
		std::lock_guard<std::mutex> lck(m_mutex);
		m_list_resources.push_back(t);
	}

	std::shared_ptr<T> m_resource;
	std::list<std::shared_ptr<T>> m_list_resources;
	std::mutex m_mutex;
};






#endif

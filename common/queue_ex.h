#ifndef QUEUE_EX_H
#define QUEUE_EX_H

#include <list>
#include <mutex>
#include <condition_variable>
#include <iostream>

template <typename T>
class QueueEx
{
public:
	void PushData(T data)
	{
		std::lock_guard<std::mutex> lck(m_mutex_data);
		m_list_data.push_back(data);
		m_cv_data.notify_one();
	}

	T PopData()
	{
		std::unique_lock<std::mutex> lck(m_mutex_data);

		m_cv_data.wait(lck, 
			[this]()
		{
			return !m_list_data.empty();
		});


		T str = m_list_data.front();
		m_list_data.pop_front();
		return str;
	}

	size_t Size() const
	{
		return m_list_data.size();
	}

private:

	std::list<T> m_list_data;

	std::mutex m_mutex_data;
	std::condition_variable m_cv_data;
};




#endif
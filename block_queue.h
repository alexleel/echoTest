#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "locker.h"
using namespace std;

template <class T>
class block_queue
{
public:
	block_queue(int max_size = 1000)
	{
		if (max_size <= 0){
			exit(-1);
		}

		m_max_size = max_size;
		m_array = new T[max_size];
		m_size = 0;
		m_front = -1;
		m_back = -1;
	}
	
	~block_queue()
	{
		m_mutex.lock();
		if (m_array != NULL){
			delete [] m_array;
		}
		m_mutex.unlock();
	}
	
	bool full()
	{
		m_mutex.lock();
		if(m_size > m_max_size) {
			m_mutex.unlock();
			return true;
		}
		m_mutex.unlock();
		return false;
	}
	
	bool empty()
	{
        m_mutex.lock();
        if (0 == m_size)
        {
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }
	// return the head
    bool front(T &value)
    {
		m_mutex.lock();
		if(m_size == 0) {
			m_mutex.unlock();
			return false;
		}
		value = m_array[m_front];
		m_mutex.unlock();
		return true;
    }

	// return the head
	bool back(T &value) 
	{
		m_mutex.lock();
		if (0 == m_size) {
            m_mutex.unlock();
            return false;
        }
		value = m_array[m_back];
		m_mutex.unlock();
		
		return true;
	}
	
	//current size
	int size()
	{
		int tmp = 0;
		m_mutex.lock();
		tmp = m_size;
		m_mutex.unlock();

		return tmp;
	}

	int max_size()
	{
        int tmp = 0;

        m_mutex.lock();
        tmp = m_max_size;

        m_mutex.unlock();
        return tmp;
    }
	// productor add item to queue, need wakeup consumer's thread.
	// if no thread wait for condition, broadcast is no means
	//<--m_front -- m_back -- if push m_back ++

	bool push(const T &item)
	{
		m_mutex.lock();
		if (m_size >= m_max_size){
			m_cond.broadcast();
			m_mutex.unlock();
			return false;
		}
		m_back = (m_back+1) % m_max_size;
		m_array[m_back] = item;

		m_size ++;
		m_cond.broadcast();
		m_mutex.unlock();

		return true;

	}
	
	//<--m_front -- m_back -- if pop m_front ++
	bool pop(T &item)
	{
		m_mutex.lock();
		// no element, wait for condition
		//many consumer use while not if
		while (m_size <= 0)
		{
			if(!m_cond.wait(m_mutex.get())){
				printf("aaaaaa\n");
				m_mutex.unlock();
				return false;
			}
		}
		
		m_front = (m_front+1) % m_max_size;
		item = m_array[m_front];
		m_size --;
		
		m_mutex.unlock();
		return true;

	}
	// timeout scenario, use pthread_cond_wait parameter time, need to get mutex  in the time
	bool pop(T &item, int ms_timeout)
	{
		struct timespec ts = {0, 0};
		struct timeval now = {0, 0};

		gettimeofday(&now, NULL);
		m_mutex.lock();
		if (m_size <= 0) {
			ts.tv_sec = now.tv_sec + ms_timeout/1000;
			ts.tv_nsec = (ms_timeout % 1000) * 1000;
			if (!m_cond.timewait(m_mutex.get(), ts)) {
				m_mutex.unlock();
				return false;
			}
		}
		// if still <= 0 return false
		if (m_size <= 0){
			m_mutex.unlock();
			return false;
		}
		m_front = (m_front+1) % m_max_size;
		item = m_array[m_front];
		m_size --;
		m_mutex.unlock();
		return true;

	}
private:
	locker m_mutex;
	cond m_cond;

	T *m_array;
	int m_size;
	int m_max_size;
	int m_front;
	int m_back;
};

#endif

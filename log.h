#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "block_queue.h"

using namespace std;
class Log
{
public:
	static const int LEN = 128;
	static Log *get_instance()
	{
		static Log instance;
		return &instance;
	}

	static void *flush_log_thread(void *args)
	{
		Log::get_instance()->async_write_log();
	}
	
	//split_lines means max line.
	bool init(const char *filename, int close_log, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);
	void write_log(int level, const char* format, ...);
	void flush(void);
	
private:
	Log();
	virtual ~Log();
	void *async_write_log()
	{
		string single_log;

		while(m_log_queue->pop(single_log)){
			mutex.lock();
			fputs(single_log.c_str(), m_fp);
			fflush(m_fp);

			mutex.unlock();
		}
	}
	
private:
	char dirname[LEN];
	char log_name[LEN];
	int max_lines; // max lines of log
	int max_buffer_size;
	long long m_count; // line count
	int today; // log as day
	FILE *m_fp;
	char *buf;
	block_queue<string>*m_log_queue;
	bool is_async;
	locker mutex;
	int m_close_log; //close log
};

#define LOG_DEBUG(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(0, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_INFO(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(1, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_WARN(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(2, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_ERROR(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(3, format, ##__VA_ARGS__); Log::get_instance()->flush();}


#endif

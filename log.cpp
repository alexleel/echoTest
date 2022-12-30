#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include "log.h"
#include <pthread.h>

using namespace std;

Log::Log()
{
	m_count = 0;
	is_async = false;
}

Log::~Log()
{
	if (m_fp != NULL) {
		fclose(m_fp);
	}
}

// max_queue_size 0 means synchronization
bool Log::init(const char*filename, int close_log, int log_buf_size, int split_lines, int max_queue_size)
{
	m_close_log = close_log;
	max_buffer_size = log_buf_size;
	buf = new char[max_buffer_size];
	memset(buf, 0, max_buffer_size);
	max_lines = split_lines;

	time_t t = time(NULL);
	struct tm *sys_tm = localtime(&t);
	struct tm my_tm = *sys_tm;

	// check '/' backwards
	const char *p = strrchr(filename, '/');
	char fullname[LEN] = {0};

	if (p == NULL) { // not contain '/'
		snprintf(fullname, 255, "%d_%02d_%02d_%s", my_tm.tm_year+1900, my_tm.tm_mon+1, my_tm.tm_mday, filename);
	} else {
		strncpy(log_name, p+1, LEN);
		//filename: xxx/xxx/xxx.log  p:/fillename
		strncpy(dirname, filename, p-filename+1);
		snprintf(fullname, LEN, "%s%d_%02d_%02d_%s", dirname, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
	}

	today = my_tm.tm_mday;
	m_fp = fopen(fullname, "a+");
	if (m_fp == NULL) {
    	return false;
	}
	printf("fopen fp success %s\n", fullname);

	if (max_queue_size >= 1) {

		is_async = true;
		m_log_queue = new block_queue<string>(max_queue_size);
		
		//flush_log_thread callback to write log asynchronous
		pthread_t tid;
		pthread_create(&tid, NULL, flush_log_thread, NULL);
	}

	return true;
		
}

void Log::write_log(int level, const char * format,...)
{
	struct timeval now = {0,0};
	gettimeofday(&now, NULL);

	time_t t = now.tv_sec;
	struct tm *sys_tm = localtime(&t);
	struct tm my_tm = *sys_tm;

	char str[16] = {0};
	switch (level)
	{
	case 0:
	    strcpy(str, "[debug]:");
	    break;
	case 1:
	    strcpy(str, "[info]:");
	    break;
	case 2:
	    strcpy(str, "[warn]:");
	    break;
	case 3:
	    strcpy(str, "[erro]:");
	    break;
	default:
	    strcpy(str, "[info]:");
	    break;
	}
	
	mutex.lock();
	m_count ++;
	//Not today or max_lines multiple
	if (today != my_tm.tm_mday || m_count % max_lines == 0) {//everyday log
		char new_log[LEN] = {0};
        fflush(m_fp);
        fclose(m_fp);
		char tail[16] = {0};

		snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);
		if (today != my_tm.tm_mday){
            snprintf(new_log, 255, "%s%s%s", dirname, tail, log_name);
           	today = my_tm.tm_mday;
            m_count = 0;
		} else {
			snprintf(new_log, LEN, "%s%s%s.%lld", dirname, tail, log_name, m_count /max_lines);
		}

		m_fp = fopen(new_log, "a");
	}
	mutex.unlock();

	va_list valst;
	va_start(valst, format);

	string log_str;
    mutex.lock();

	int n = snprintf(buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, str);

	int m = vsnprintf(buf + n, max_buffer_size - 1, format, valst);
	
	buf[n + m] = '\n';
    buf[n + m + 1] = '\0';
    log_str = buf;

	mutex.unlock();

	// async --> to blockqueue
	if (is_async && !m_log_queue->full()){
        m_log_queue->push(log_str);
    }
    else {
        mutex.lock();
        fputs(log_str.c_str(), m_fp);
        mutex.unlock();
    }
    va_end(valst);

}

void Log::flush(void)
{
    mutex.lock();
	printf("flush!\n");
    fflush(m_fp);
    mutex.unlock();
}



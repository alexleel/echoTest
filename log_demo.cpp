#include "log.h"


void log_init()
{
	Log::get_instance()->init("./DemoLog", 0, 2000, 800000, 800);
}

int main(int argc, char *argv[])
{
	log_init();
	int m_close_log = 0;
	LOG_ERROR("%s", "Test for log...");
	return 0;
}

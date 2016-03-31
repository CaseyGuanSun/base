#include "Sleep.h"

void SleepMs(int msecs)
{
	struct timespec short_wait;
	struct timespec remainder;

	short_wait.tv_sec = msecs/1000;
	short_wait.tv_nsec = (msecs%1000)*1000*1000;

	nanosleep(&short_wait, &remainder);
}

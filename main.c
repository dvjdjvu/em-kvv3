#include "em_kvv.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

//#include <linux/art_task.h>

int fd = 0;

int _index = 0;

#define MAX_NANO_SEC 999999999

//const unsigned long IRQ_NANO_SEC = 32768000;
//#define IRQ_NANO_SEC 32768000
// Предельная задержка в 50 мкс.
#define IRQ_NANO_SEC 32818000 

void stop_daemon()
{
	unsigned long time;
	static flag = 0;
	static struct timespec now__ts;
	static struct timespec prev_ts;
	static struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &now__ts);

	if(flag == 0)
	{
		goto end;
	}

	if((unsigned long)now__ts.tv_sec == (unsigned long)prev_ts.tv_sec)
	{
		time = (unsigned long)now__ts.tv_nsec - (unsigned long)prev_ts.tv_nsec;

		if(time > IRQ_NANO_SEC)
		{
			printf("Time = %09ld mksec limited!\n", time - IRQ_NANO_SEC);
		}else{
			printf("Time = %09ld\n", time);
		}
	}
	else if((unsigned long)now__ts.tv_sec - (unsigned long)prev_ts.tv_sec > 0)
	{
		time = (unsigned long)now__ts.tv_nsec - (unsigned long)prev_ts.tv_nsec + MAX_NANO_SEC;

		if(time > IRQ_NANO_SEC)
		{
			printf("Time = %09ld mksec limited!\n", time - IRQ_NANO_SEC );
		}else{
			printf("Time = %09ld\n", time);
		}
	}else{
		puts("Time = limited!");
	}

	// Точность
	//clock_getres(CLOCK_REALTIME, &ts);
	//printf("Res  = %ld,%09ld\n", (long)ts.tv_sec, (long)ts.tv_nsec);	

	//time = ioctl(fd, IOCTL_GET_JIFF);
	//printf("while = %u\t%u\n", time, _index);

	_index++;
	if(_index == 20)
	{
		//EM_close(&fd);
		EM_stop_daemon(SIGUSR1);
		//exit(0);
	}
	prev_ts = now__ts;
	return;

end:
	flag = 1;
	prev_ts = now__ts;

	return;
}

#define SIZE 8
int main(int argc, char *argv[])
{
	int i;
	int time = 0;	

	printf("EM_find_devices() = %d\n", EM_find_devices());

	//art_enter(ART_PRIO_MAX, ART_TASK_RR, 0);

	//art_exit();

	printf("PID = %d\n", getpid());


	if((EM_find_devices()&1) != 1)
	{
		printf("Plate %s dont find!\n", FILE0);
		return 1;
	}

	if(EM_open(FILE0, &fd)!=0)
	{
		puts("EM_open ERROR");
		return 1;
	}
	
	if(EM_config(&fd))
	{
		puts("EM_config ERROR/плата не обнаруженна в системе");
		return 1;
	}

	EM_setpid(&fd);

	// Повышение приоритета до мягкого реального времени.
	if(EM_set_priority_max())
	{
		puts("should be run as root");
	}

	if(EM_start_daemon(stop_daemon, SIGUSR1) == FUNCTION_NONE)
	{
		puts("Error: Pointer on arg function handler is NULL!");
	}

	for(;;) sleep(5);

	puts("END");

	return 0;
}

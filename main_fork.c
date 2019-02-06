#include "em_kvv.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <linux/art_task.h>

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
	static flag;
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
			printf("Time = %08ld mksec limited!\n", time - IRQ_NANO_SEC);
		}else{
			printf("Time = %08ld\n", time);
		}
	}
	else if((unsigned long)now__ts.tv_sec - (unsigned long)prev_ts.tv_sec > 0)
	{
		time = (unsigned long)now__ts.tv_nsec - (unsigned long)prev_ts.tv_nsec + MAX_NANO_SEC;

		if(time > IRQ_NANO_SEC)
		{
			printf("Time = %08ld mksec limited!\n", time - IRQ_NANO_SEC );
		}else{
			printf("Time = %08ld\n", time);
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
		EM_close(&fd);
		exit(0);
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

	/**
		Обработчик прерываний, запускается в отдельном процессе.
	*/

	pid_t pid = fork();

	if (pid < 0) {
		perror("fork");
		return -1;
	}

	if (pid == 0) {

		// Отдельный дочерний процесс,

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

		// Ребенок
		printf("pid = %d\n", getpid());

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

		exit(0);
	}

	// Исходный родительский процесс.

	pid_t pid2;
	int status;

	// Ожидаем завершения обработчика прерывания. pid дочернего процесса
	if((pid2 = waitpid(pid, &status, 0)) && WIFEXITED(status)) {
		printf("Дочерний процесс с PID = %d завершил выполнение\n", pid2);
		printf("Код статуса завершения равен %d\n", WEXITSTATUS(status));
	}

	return 0;
	/*
	char *arr = (char*) malloc(SIZE * sizeof(char));
	*/
	/*
	arr[0]=0x01;//0xaa;
	arr[1]=0x23;//0x55;
	arr[2]=0x45;//0xaa;
	arr[3]=0x67;//0x55;

	arr[4]=0x01;//0xaa;
	arr[5]=0x23;//0x55;
	arr[6]=0x45;//0xaa;
	arr[7]=0x67;//0x55;
	*/
	//for(i=0; i<SIZE; i++)
	//	arr[i] = 0xaa;

	//EM_write(arr, SIZE);
	//ioctl(fd, IOCTL_MODE_RW, IOCTL_MODE_WRITE);


	/*
	int count;
	if(!(count = write(fd, arr, sizeof(char)*SIZE)))
		perror("write");

	printf("count %d\n", count);
	*/
	//ioctl(fd, IOCTL_MODE_RW, IOCTL_MODE_WRITE);
	//ioctl(fd, IOCTL_MODE_DELAY, 0);


	//int _index=-1111;	
	//ioctl(fd, IOCTL_GET_RSTAT, _index);
	//printf("_index -- %d\n", _index);

	//for(i=0; i<SIZE; i++)
	//	arr[i] = 0x55;
	//EM_write(arr, SIZE);
	//printf("%d\n", (249>>1));

	close(&fd);

	return 0;
}

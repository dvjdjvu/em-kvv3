#include "em_kvv.h"

const char one  = 1;
const char noll = 1;

int IOCTL_MODE_READ  = 0;
int IOCTL_MODE_WRITE = 1;

// Структура настройки режима реального времени.
struct sched_param param;

int EM_open(char *file, int *fd)
{

	*fd = open(file, O_RDWR);

	if(*fd == -1)
	{
		printf("EM_open: open failed %s\n", file);
		return OPEN_FAILED;
	}

	return 0;
}

int EM_close(int *fd)
{
	if(!(*fd))
	{
		puts("EM_close: Start up 'EM_open' !");
		return OPEN_FAILED;
	}

	if(close(*fd) != 0)
	{
		return CLOSE_FAILED;
	}

	*fd = 0;

	return 0;
}

// Настройка платы для работы, 0 если все хорошо
int EM_config(int *fd)
{
	int scan=0;
	pid_t pid;

	if(!(*fd))
	{
		puts("EM_config: Start up 'EM_open' !");
		return OPEN_FAILED;
	}

	ioctl(*fd, IOCTL_CHANGE_PORT331);
	write(*fd, &one, sizeof(char));

	ioctl(*fd, IOCTL_CHANGE_PORT330);
	if(read(*fd, &scan, 1))
	{
		//test if(scan%2!=1)
		if((scan&1) != 1)
			return CONFIG_FAILED;
	}else{
		return CONFIG_FAILED;
	}

	ioctl(*fd, IOCTL_SET_PID, getpid());

	return 0;
}

// Инициализация платы на чтение
int EM_init_read(int *fd)
{
	if(!(*fd))
	{
		puts("EM_init_read: Start up 'EM_open' !");
		return OPEN_FAILED;
	}

	ioctl(*fd, IOCTL_CHANGE_PORT331);
	write(*fd, &noll, sizeof(char));

	ioctl(*fd, IOCTL_CHANGE_PORT330);

	return 0;
}

// Побайтовое чтение из платы
char EM_read(int *fd)
{
	char scan=0;
	int i, count=0;
	int error = 0;
	char byte=0;

	//EM_init_read(fd);

	for(;; count=read(*fd, &scan, sizeof(char)))
		if(count)
		{
			//test if((scan>>1)%2==1)
			if((scan&2) == 1)
			{
				read(*fd, &byte, sizeof(char));
				break;
			}
			else
			{
				error++;
				if(error>10)
					return READ_TIME_LIMITED;
				//usleep(1);
			}
			printf("%d \n", scan);
		}
	
	return byte;
}

// Инициализация платы на запись
int EM_init_write(int *fd)
{
	if(!(*fd))
	{
		puts("EM_init_write: Start up 'EM_open' !");
		return OPEN_FAILED;
	}

	ioctl(*fd, IOCTL_CHANGE_PORT331);
	write(*fd, &one, sizeof(char));

	ioctl(*fd, IOCTL_CHANGE_PORT330);

	return 0;
}

// Запись в плату fd массива данных arr, размером size
int EM_write(int *fd, char *arr, int size)
{
	int scan=0, i, count=0;
	int error=0;

	//EM_init_write(fd);

	for(i=0; i<size; i++)
	{
		printf("i = %d\n", i);
		for(;; count=read(*fd, &scan, 1))
		{
			if(count)
			{	
				//printf("scan = %d\n", scan);
				//test if(scan%2==1)
				if((scan&1) == 1)
				{
					if(!write(*fd, &arr[i], sizeof(char)))
						puts("EM_write: Write Failed");
					//scan=0;
					printf("%x\n", arr[i]);
					break;
				}
				else
				{
					error++;
					if(error>10)
					{	
						puts("EM_write: WRITE_TIME_LIMITED");
						return WRITE_TIME_LIMITED;
					}
				}
				printf("%d\n", scan);
			}
		}
	}
	return 0;
}

// Сообщение драйверу номера процесса который работает с платой fd
int EM_setpid(int *fd)
{
	if(!(*fd))
	{
		puts("EM_init_write: Start up 'EM_open' !");
		return OPEN_FAILED;
	}

	pid_t pid = getpid();
	printf("pid = %d\n", pid);
	ioctl(*fd, IOCTL_SET_PID, pid);

	return 0;
}

/**
	Демон обрабатывающий прерывания, в качестве аргумента передается функция которая будет выполняться всякий раз когда происходит прерывание.
	Программист создает её сам на свое усмотрение. Аргументов у этой функции нет.
	Для 1-ой платы signum = SIGUSR1, 2-ой SIGUSR2, 3-ей SIGURG
	Порядок плат определяется здесь:

	sudo insmod ./em_kvv3_mod.ko ports0=0x330 count0=2 irq0=5 name0="/dev/em_kvv0" minor0=0 ports1=0x378 count1=3 irq1=6 name1="/dev/em_kvv1" minor1=1
	=> плата /dev/em_kvv0 генерирует SIGUSR1, плата /dev/em_kvv1 генерирует SIGUSR2
*/

sigset_t mask;
struct sigaction act_start;
struct sigaction act_stop;

void EM_stop_daemon(int signum)
{
	sigfillset(&mask);
	sigprocmask(SIG_SETMASK, &mask, NULL);

	memset(&act_start, 0, sizeof(&act_start));

	act_start.sa_handler = SIG_IGN;
	sigaction(signum, &act_start, NULL);

	// Отключаем обработчик сигналов.

	/*
	memset(&act_stop, 0, sizeof(&act_stop));

	act_stop.sa_handler = SIG_IGN;

	// Отключаем обработчик сигналов.
	sigaction(signum, &act_stop, &act_start);
	*/
}

// Возможен запуск сразу 3-х обработчиков сигналов здесь!
int EM_start_daemon(void (*function)(), int signum/*, void (*function1)(), int signum1, void (*function2)(), int signum2*/)
{
	if(function == NULL)
	{
		return FUNCTION_NONE;		
	}

	// Инициализируем полный набор POSIX сигналов
	sigfillset(&mask);
	// Удаляем из нашего набора сигнал signum
	sigdelset(&mask, signum);

	// Запрещаем перехватывать все сигналы кроме, signum. Т.к. мы удалили его из этого набора.
	sigprocmask(SIG_SETMASK, &mask, NULL);

	//act.sa_handler = stop_daemon;
	// Для сигналов реального времени SA_SIGINFO
	act_start.sa_flags = SA_SIGINFO;

	// Функция которая будет обрабатывать сигналы.
	act_start.sa_handler = (*(function));

	// Запускаем обработчик сигналов.
	sigaction(signum, &act_start, NULL);

	/*
	sigset_t mask1;
	static struct sigaction act1;

	// Инициализируем полный набор POSIX сигналов
	sigfillset(&mask1);
	// Удаляем из нашего набора сигнал signum
	sigdelset(&mask1, signum1);

	// Запрещаем перехватывать все сигналы кроме, signum. Т.к. мы удалили его из этого набора.
	sigprocmask(SIG_SETMASK, &mask1, NULL);

	//act.sa_handler = stop_daemon;
	// Для сигналов реального времени SA_SIGINFO
	act.sa_flags = SA_SIGINFO;

	// Функция которая будет обрабатывать сигналы.
	act.sa_handler = (*(function1));

	// Запускаем обработчик сигналов.
	sigaction(signum1, &act, NULL);


	sigset_t mask2;
	static struct sigaction act2;

	// Инициализируем полный набор POSIX сигналов
	sigfillset(&mask2);
	// Удаляем из нашего набора сигнал signum
	sigdelset(&mask2, signum2);

	// Запрещаем перехватывать все сигналы кроме, signum. Т.к. мы удалили его из этого набора.
	sigprocmask(SIG_SETMASK, &mask2, NULL);

	//act.sa_handler = stop_daemon;
	// Для сигналов реального времени SA_SIGINFO
	act.sa_flags = SA_SIGINFO;

	// Функция которая будет обрабатывать сигналы.
	act.sa_handler = (*(function2));

	// Запускаем обработчик сигналов.
	sigaction(signum2, &act, NULL);
	*/

	//for(;;) sleep(5);
}

/**
	Повышение приоритета приложения до реального времени
	2 варианта работы приложения:
	SCHED_FIFO приложение начавшее работать не передаст управление другим процессам пока не закончит работать само
	SCHED_RR   все приложения с одинаковым приоретитеом выполнются равное колличество времени, потом пердают управление следующему

*/
int EM_set_priority_max()
{
	// Только root может повысить приоритет приложения
	if(getuid() != 0)
	{
		return 1;
	}

	// Повышение приоритета до мягкого реального времени.
	param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	if( sched_setscheduler( getpid(), SCHED_RR, &param ) == -1 )
	{
		perror("sched_setscheduler");
	}else{
		printf("param.sched_priority %d\n", param.sched_priority);
	}

	return 0;
}

// Поиск плат в системе, всего предполагаю что может быть 3 платы.
int EM_find_devices()
{
	int fd;
	int plate = 0;
	if(EM_open(FILE0, &fd) == 0)
	{
		if(EM_config(&fd) == 0)
		{
			plate |= 1;
		}
		EM_close(&fd);
	}
	if(EM_open(FILE1, &fd) == 0)
	{
		if(EM_config(&fd) == 0)
		{
			plate |= 2;
		}
		EM_close(&fd);
	}
	if(EM_open(FILE2, &fd) == 0)
	{
		if(EM_config(&fd) == 0)
		{
			plate |= 4;
		}
		EM_close(&fd);
	}

	// 001 Есть плата для FILE0, 010 Есть плата для FILE1, 100 Есть плата для FILE2
	// 111 Есть плата для FILE0, FILE1, FILE2
	// На выходе проверить увстановленные биты

	return plate;
}


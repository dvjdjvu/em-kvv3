#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>

#include <time.h>

#include <signal.h>
#include <errno.h>


// Файлы устройств
#define FILE0 "/dev/em_kvv0"
#define FILE1 "/dev/em_kvv1"
#define FILE2 "/dev/em_kvv2"

// Флаги IOCTL
#define IOCTL_GET_RSTAT    1
#define IOCTL_GET_WSTAT    2
#define IOCTL_RESET_RSTAT  3
#define IOCTL_RESET_WSTAT  4

#define IOCTL_CHANGE_PORT330  5
#define IOCTL_CHANGE_PORT331  6
#define IOCTL_CHANGE_RPORT    7
#define IOCTL_CHANGE_WPORT    8

#define IOCTL_MODE_RW	      9
#define IOCTL_MODE_DELAY      10

#define IOCTL_SET_PID	      11
#define IOCTL_GET_JIFF	      12

// Ошибки
#define OPEN_FAILED		1
#define CLOSE_FAILED		2
#define CONFIG_FAILED		3
#define WRITE_TIME_LIMITED	4
#define READ_TIME_LIMITED	5
#define FUNCTION_NONE		6


extern int IOCTL_MODE_READ;
extern int IOCTL_MODE_WRITE;

extern sigset_t mask;
extern struct sigaction act_start;
extern struct sigaction act_stop;

#include <sched.h>

extern struct sched_param param;

extern int EM_open(char *file, int *fd);
extern int EM_close(int *fd);
extern int EM_config(int *fd);
extern int EM_init_read(int *fd);
extern char EM_read(int *fd);
extern int EM_init_write(int *fd);
extern int EM_write(int *fd, char *arr, int size);
extern int EM_setpid(int *fd);
extern int EM_start_daemon(void (*function)(), int signum);
extern void EM_stop_daemon(int signum);
extern int EM_set_priority_max();
extern int EM_find_devices();



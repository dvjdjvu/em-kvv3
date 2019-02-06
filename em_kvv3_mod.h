#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <asm/io.h>

#include <linux/fs.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include <linux/delay.h>

#include <linux/rwsem.h>
#include <linux/pagemap.h>

#include <asm/uaccess.h>
#include <linux/version.h>

#include <linux/list.h>
#include <linux/timer.h>

//#include <signal.h>
//#include <sys/types.h>

#include <asm/signal.h>

#include <asm/bitops.h>

//#include <linux/devfs_fs_kernel.h>

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) ((a)*65536+(b)*256+(c))
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#define LINUX26
#endif

#ifndef SET_MODULE_OWNER
#define SET_MODULE_OWNER(some_struct) do { } while (0)
#endif

// Имя устройства
#define DEV_NAME "/dev/em_kvv0"

// Порты устройства
#define PORT_START 0x330
#define PORT_COUNT 0x2

// Память устройства
#define MEM_START 0x10000000
#define MEM_COUNT 0x20

// Номер прерывания 
#define IRQ_NUM 5

int index=0;

// Структура процессов
struct task_struct *task;

// Структура для хранения состояния устройства
struct dev_state
{
	struct list_head list;

	int minor; // младший номер устройства

	int port_work; // порт для работы

	int port;  // начальный порт
	int count; // кол-во выделяемых портов

	int port_read;  // текущий порт для чтения
	int port_write; // текущий порт для записи

	int dev_open; // Открыто ли устройство
	ssize_t byte_read;  // Сколько байт прочитано из устройства
	ssize_t byte_write; // Сколько байт записано в устройство

	int mode; // Режим работы с платой(чтение/запись)

	char *buf;  // Буфер с данными
	int  bsize; // Размер посылки
	int  bdone; // Готов ли буфер для работы? 1 да, 0 нет.

	int time_now; // Время сейчас
	struct timer_list timer_delay; // Таймер задержки
	int delay; // Задержка для приёма/передачи информации относительно прерывания

	pid_t pid; // pid процесса работающего с устройством, по умолчанию -1(не один процесс не работает с устройством)
	int sig;

	struct task_struct *task; // Указатель на структура процесса работающего с данным устройством
};

struct list_head *pos;

// Информация о состоянии устройств
static struct dev_state state;


// Старший номер файла устройств
static int major = 0x00;

// Порты ввода.вывода устройств
static int ports0 = 0x00;
static int count0 = 0x00;

static int ports1 = 0x00;
static int count1 = 0x00;

static int ports2 = 0x00;
static int count2 = 0x00;

// Номера прерываний
static int irq0 = 0x00;
static int irq1 = 0x00;
static int irq2 = 0x00;

// Имена наших устройств
static char *name0 = 0x00;
static char *name1 = 0x00;
static char *name2 = 0x00;

// Младшие номера устройств
static int minor0 = 0x00;
static int minor1 = 0x00;
static int minor2 = 0x00;

int error;


#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,3)
module_param(major,  int, 0);
//
module_param(ports0,    int, 0);
module_param(count0,    int, 0);

module_param(ports1,    int, 0);
module_param(count1,    int, 0);

module_param(ports2,    int, 0);
module_param(count2,    int, 0);
//
module_param(irq0,   int, 0);
module_param(irq1,   int, 0);
module_param(irq2,   int, 0);
//
module_param(name0, charp, 0);
module_param(name1, charp, 0);
module_param(name2, charp, 0);
//
module_param(minor0, int, 0);
module_param(minor1, int, 0);
module_param(minor2, int, 0);
//
//
MODULE_PARM_DESC(major, "Major number");
//
MODULE_PARM_DESC(ports0,   "ISA0 port 0");
MODULE_PARM_DESC(count0,   "ISA0 port 1");
//
MODULE_PARM_DESC(ports1,   "ISA1 port 0");
MODULE_PARM_DESC(count1,   "ISA1 port 1");
//
MODULE_PARM_DESC(ports2,   "ISA2 port 0");
MODULE_PARM_DESC(count2,   "ISA2 port 1");
//
MODULE_PARM_DESC(irq0,  "IRQ ISA0");
MODULE_PARM_DESC(irq1,  "IRQ ISA1");
MODULE_PARM_DESC(irq2,  "IRQ ISA2");
//
MODULE_PARM_DESC(name0, "Define device name the driver registers, /dev/em_kvv0");
MODULE_PARM_DESC(name1, "Define device name the driver registers, /dev/em_kvv1");
MODULE_PARM_DESC(name2, "Define device name the driver registers, /dev/em_kvv2");
//
MODULE_PARM_DESC(minor0, "Minor number, /dev/em_kvv0");
MODULE_PARM_DESC(minor1, "Minor number, /dev/em_kvv1");
MODULE_PARM_DESC(minor2, "Minor number, /dev/em_kvv2");
//
#else
MODULE_PARM(major, "i");
//
MODULE_PARM(ports0,   "i");
MODULE_PARM(count0,   "i");
//
MODULE_PARM(ports1,   "i");
MODULE_PARM(count1,   "i");
//
MODULE_PARM(ports2,   "i");
MODULE_PARM(count2,   "i");
//
MODULE_PARM(irq0,  "i");
MODULE_PARM(irq1,  "i");
MODULE_PARM(irq2,  "i");
//
MODULE_PARM(name0, "s");
MODULE_PARM(name1, "s");
MODULE_PARM(name2, "s");
//
MODULE_PARM(minor0,  "i");
MODULE_PARM(monor1,  "i");
MODULE_PARM(minor2,  "i");
#endif

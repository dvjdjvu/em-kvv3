/*
#if (LINUX_VERSION_CODE < Djvu_VERSION(2,6,18))
#include <linux/config.h>
#endif
*/

#include "em_kvv3_mod.h"

int IOCTL_MODE_READ  =  0; // Режим чтения
int IOCTL_MODE_WRITE =  1; // Режим записи


// Функция поиска открытых устройств
struct dev_state *find_device_open(struct dev_state *dev_state, int minor)
{
	list_for_each(pos, &state.list){
		dev_state = list_entry (pos, struct dev_state, list);

		if(dev_state->minor == minor)
		{
			return (struct dev_state *)dev_state;
		}
	}

	return (struct dev_state *)NULL;
}


//------------------------------------------------------------------------------------------//

// Буферы приёма информации от плат
#define BUF_SIZE 8192

char *buffer0;
char *buffer1;
char *buffer2;

// Таймер
int time_now;

// Статические ссылки на соответствующие структуры
static struct dev_state *irq_dev_state0;
static struct dev_state *irq_dev_state1;
static struct dev_state *irq_dev_state2;

struct timer_list my_timer;

void f_timer_delay0(unsigned long data)
{
	//printk("ftimer  time: %ld\n", jiffies);

	if(index==200)
		return;
		//return IRQ_NONE;

	if(irq_dev_state0 == NULL || irq_dev_state0->task == NULL)
	{
		//return IRQ_NONE;
		return;
	}
	irq_dev_state0->time_now = jiffies;	

	send_sig(SIGUSR1, irq_dev_state0->task, 0);


	printk("f_timer_delay0 time\t%lu\t%d\n", jiffies, index);
	index++;

	//add_timer(&my_timer);
	mod_timer(&my_timer, jiffies + 8);

}

void f_timer_delay1(unsigned long data)
{
	printk("ftimer  time: %ld\n", jiffies);
}

void f_timer_delay2(unsigned long data)
{
	printk("ftimer  time: %ld\n", jiffies);
}

/*
// Обработчик прерывания  нижней половины
char tasklet_data0[] = "tasklet_function0";

int index=0;
void tasklet_function0(unsigned long data)
{
	if(index<6)
	{
		time_now = jiffies;
	
		printk("tasklet0: %d\n", time_now);
		index++;
	}
	return;
}

DECLARE_TASKLET(tasklet0, tasklet_function0, (unsigned long)&tasklet_data0);
*/

// Обработчик прерывания верхней половины
// Для 1-ой платы
static irqreturn_t irq_handler0(int irq, void *dev_id, struct pt_regs *regs)
{
	
	/* отладка
	index++;
	if(index==20)
		return IRQ_NONE;
	*/
	if(irq_dev_state0 == NULL || irq_dev_state0->task == NULL || irq_dev_state0->pid == -1)
	{
		return IRQ_NONE;
	}
	
	send_sig(SIGUSR1, irq_dev_state0->task, 0);

	irq_dev_state0->time_now = jiffies;

	//printk("irq_handler0 time\t%d\t%d\n", jiffies, index);
	
	return IRQ_HANDLED;
}

// Обработчик прерывания
// Для 2-ой платы
static irqreturn_t irq_handler1(int irq, void *dev_id, struct pt_regs *regs)
{
	if(irq_dev_state1 == NULL || irq_dev_state1->task == NULL || irq_dev_state1->pid == -1)
	{
		return IRQ_NONE;
	}
	
	send_sig(SIGUSR2, irq_dev_state1->task, 0);

	return IRQ_HANDLED;
}

// Обработчик прерывания
// Для 3-ей платы
static irqreturn_t irq_handler2(int irq, void *dev_id, struct pt_regs *regs)
{
	if(irq_dev_state2 == NULL || irq_dev_state2->task == NULL || irq_dev_state2->pid == -1)
	{
		return IRQ_NONE;
	}
	
	send_sig(SIGURG, irq_dev_state2->task, 0);
	return IRQ_HANDLED;
}

// Функция проверки корректности переданных аргументов
static int arg_correct(int ports, int count, int irq, char *name)
{
	// Проверка начала диапозона
	if(ports == 0x00)
		return 1;
	// Проверка колличества запрашиваемых портов
	if(count == 0x00)
		return 1;
	// Проверка номера прерывания
	if(irq   == 0x00)
		return 1;
	// Проверка имени устройства
	if(name  == 0x00)
		return 1;

	return 0;
}


// Функция регистрации новых устройств
struct dev_state *reg_dev_state_device(struct dev_state *dev_state, int minor)
{
	dev_state = (struct dev_state *)kmalloc(sizeof (struct dev_state), GFP_KERNEL);

	dev_state->dev_open   	= 1;
	dev_state->byte_read  	= 0;
	dev_state->byte_write 	= 0;
	dev_state->minor 	= minor;
	dev_state->mode         = -1;
	dev_state->delay	= 0;
	dev_state->bdone	= 0;

	dev_state->pid		= -1;

	list_add(&(dev_state->list), &(state.list));

	return (struct dev_state *)dev_state;
}

// Функция захвата ресурсов
static int capture_of_resources(int ports, int count, int irq, char *name, int minor, struct file_operations Fops_EM_KBB3)
{
	struct dev_state *dev_state;
	/*
	#ifdef CONFIG_DEVFS_FS
		if (!devfs_mk_dir(NULL, name, NULL))
			return -EBUSY;
	#else
	*/
	// Захват портов ввода-вывода

	//printk("Djvu Init: Try allocating io ports\n");
	if (check_region(ports, count)) 
	{
		printk("Djvu Init: Allocation io ports failed\n");
		return -EBUSY;
	}else{
		request_region(ports, count, name);
		printk ("Djvu Init: IO ports allocated\n");
	}
	
	/*
	// Захват памяти 
	if (check_mem_region(MEM_START, MEM_COUNT)) 
	{
		printk("Djvu: memio ports allocation failed\n");
		release_region(PORT_START, PORT_COUNT);
		return -EBUSY;
	}else{
		request_mem_region(MEM_START, MEM_COUNT, DEV_NAME);
		printk ("Djvu: Memio ports allocated\n");
	}
	*/	

	// Захват прерывания

	if(minor == 0){
		if (request_irq(irq, irq_handler0, 0, name, NULL))
		{
			printk("Djvu Init: IRQ%d allocation failed\n", irq);
			//release_mem_region(MEM_START, MEM_COUNT);
			release_region(ports, count);
			return -EBUSY;
		}else{
			printk ("Djvu Init: IRQ%d allocated\n", irq);
		}
	}else if(minor == 1){
		if (request_irq(irq, irq_handler1, 0, name, NULL))
		{
			printk("Djvu Init: IRQ%d allocation failed\n", irq);
			//release_mem_region(MEM_START, MEM_COUNT);
			release_region(ports, count);
			return -EBUSY;
		}else{
			printk ("Djvu Init: IRQ%d allocated\n", irq);
		}
	}else if(minor == 2){
		if (request_irq(irq, irq_handler2, 0, name, NULL))
		{
			printk("Djvu Init: IRQ%d allocation failed\n", irq);
			//release_mem_region(MEM_START, MEM_COUNT);
			release_region(ports, count);
			return -EBUSY;
		}else{
			printk ("Djvu Init: IRQ%d allocated\n", irq);
		}
	}

	// Регистрируем устройство в системе, при загрузке модуля.
	dev_state = reg_dev_state_device(dev_state, minor);
	// Принудительно делаем его открытым на открывание, т.к. функция reg_dev_state_device блокирует его на открывание.
	dev_state->dev_open   	= 0;
	dev_state->port   	= ports;
	dev_state->count   	= count;
	dev_state->delay	= 0;
	dev_state->bdone	= 0;

	dev_state->pid		= -1;
	dev_state->task		= NULL;

	if(minor == 0)
	{
		irq_dev_state0 = (struct dev_state *) dev_state;
		// Захватываем память для приёма информации
		if(!(buffer0 = dev_state->buf = (char *)kmalloc(BUF_SIZE, GFP_KERNEL)))
		{
			printk("Djvu Init: Error kmalloc buffer0\n");
			return 1;
		}
	}else if(minor == 1)
	{
		irq_dev_state1 = (struct dev_state *) dev_state;
		// Захватываем память для приёма информации
		if(!(buffer1 = dev_state->buf = (char *)kmalloc(BUF_SIZE, GFP_KERNEL)))
		{
			printk("Djvu Init: Error kmalloc buffer1\n");
			return 1;
		}
	}else if(minor == 2)
	{
		irq_dev_state2 = (struct dev_state *) dev_state;
		// Захватываем память для приёма информации
		if(!(buffer2 = dev_state->buf = (char *)kmalloc(BUF_SIZE, GFP_KERNEL)))
		{
			printk("Djvu Init: Error kmalloc buffer2\n");
			return 1;
		}
	}

	return 0;
}

// Функция освобождения ресурсов
static int uncapture_of_resources(int ports, int count, int irq, char *name)
{

	// Снимаем захват портов ввода-вывода
	release_region(ports, count);
	//printk("Djvu Exit: release io ports\n");

	/*
	// Снимаем захват памяти
	release_mem_region(MEM_START, MEM_COUNT);
	printk("Djvu: release memio ports\n");
	*/

	// Освобождаем прерывание
	free_irq(irq, NULL);
	//printk("Djvu Exit: release irq\n");
	
	// Снимаем регистрацию устройства
	//unregister_chrdev(major, name);
	//printk("Djvu Exit: device unregistered, major number is %d\n", major);

	return 0;
}

// Функция открытия устройства
static int device_open(struct inode *inode, struct file *filp)
{
	struct dev_state *dev_state;
	int minor = MINOR(inode->i_rdev);
	index = 0;

	if(dev_state = find_device_open(dev_state, minor))
	{
		if(dev_state->dev_open)
		{
			//printk(KERN_EMERG "Djvu Open: Devise busy, w/mirror number %d\n", minor);
			return -EBUSY;
		}else{
			dev_state->dev_open++;
			dev_state->byte_read  =  0;
			dev_state->byte_write =  0;
			dev_state->mode       = -1;
			dev_state->delay      =  0;

			dev_state->pid		= -1;
			dev_state->task		= NULL;
		}
	}else{
		dev_state = reg_dev_state_device(dev_state, minor);
	}

	//printk(KERN_EMERG "Djvu Open: try opening device w/mirror number %d\n", minor);

	////////////////////////////////

	/*
	init_timer(&my_timer);
	my_timer.expires = jiffies + 8;
	my_timer.data = 0;
	my_timer.function = f_timer_delay0;
	add_timer(&my_timer);
	*/

	////////////////////////////////

	// Инкременируем счетчик использования данного модуля. Пока он используется, его нельзя будет выгрузить.
	#ifdef LINUX26
		if (!try_module_get(THIS_MODULE))
			return -ENODEV;
	#else
		MOD_INC_USE_COUNT;
	#endif

	return 0;
}

// Функция закрытия устройства
static int device_close(struct inode *inode, struct file *filp)
{
	struct dev_state *dev_state;
	int minor = MINOR(inode->i_rdev);

	if(dev_state = find_device_open(dev_state, minor))
	{
		if(dev_state->dev_open)
		{
			dev_state->dev_open--;
			dev_state->task = NULL;
			dev_state->pid  = -1;
		}else{
			//printk(KERN_DEBUG "Djvu Close: Devise not open, w/mirror number %d\n", minor);
			return 0;
		}
	}else{
		//return 0;
	}

	//printk(KERN_EMERG "Djvu Close: try to close device w/mirror number %d\n", minor);

	#ifdef LINUX26
		module_put(THIS_MODULE);
	#else
		MOD_DEC_USE_COUNT;
	#endif

	return 0;
}


// Функция чтения из устройства
static ssize_t device_read(struct file *filp, char *buf, size_t buf_len, loff_t *offset)
{
	struct inode *inode;
	int count = buf_len;
	unsigned char byte;
	struct dev_state *dev_state;
	int minor;

	inode = filp->f_dentry->d_inode;

	minor = MINOR(inode->i_rdev);

	//printk(KERN_DEBUG "Djvu Read: Read data, minor number %d\n", minor);

	//dev_state = &state;

	if(dev_state = find_device_open(dev_state, minor))
	{
		if(dev_state->dev_open)
		{
			while(count--)
			{
				//put_user(minor, buf++);
				byte = inb(dev_state->port);
				put_user(byte, buf++);
				//printk("PORT START %x\n", dev_state->port);
			}
		}else{
			//printk(KERN_DEBUG "Djvu Read: Devise not open, don't read, w/mirror number %d\n", minor);
			return 0;
		}
	}else{
		return 0;
	}

	dev_state->byte_read += buf_len;

	return buf_len;
}

// Функция записи в устройство
static ssize_t device_write(struct file *filp, char *buf, size_t buf_len, loff_t *offset)
{
	
	struct inode *inode;
	int count = buf_len;
	unsigned char byte;
	struct dev_state *dev_state;	
	int minor, i=0;

	inode = filp->f_dentry->d_inode;

	minor = MINOR(inode->i_rdev);

	if(dev_state = find_device_open(dev_state, minor))
	{
		//printk("START dev_state->dev_open %d\tdev_state->dev_bdone %d\n", dev_state->dev_open, dev_state->bdone);

		if(dev_state->dev_open)
		{
			dev_state->bsize = buf_len;
			while(count--)
			{
				get_user(byte, buf++);
				outb_p(byte, dev_state->port_work);

			}
 		}else{
			//printk(KERN_DEBUG "Djvu Write: Devise not open, don't write, w/mirror number %d\n", minor);
			return 0;
		}
	}else{
		//printk(KERN_DEBUG "Djvu Write: Devise not open, don't write, w/mirror number %d\n", minor);
		return 0;
	}

	dev_state->byte_write += buf_len;

	return buf_len;
}

// Флаги IOCTL
const int IOCTL_GET_RSTAT   = 1;
const int IOCTL_GET_WSTAT   = 2;
const int IOCTL_RESET_RSTAT = 3;
const int IOCTL_RESET_WSTAT = 4;

const int IOCTL_CHANGE_PORT330 = 5;
const int IOCTL_CHANGE_PORT331 = 6;
const int IOCTL_CHANGE_RPORT   = 7;
const int IOCTL_CHANGE_WPORT   = 8;

const int IOCTL_MODE_RW   	 = 9;
const int IOCTL_MODE_DELAY   	 = 10;

const int IOCTL_SET_PID	 	 = 11;
const int IOCTL_GET_JIFF	 = 12;


static int device_ioctl(struct inode *inode, struct file *filp, unsigned int ioctl, unsigned long param)
{
	ssize_t *size = (ssize_t *) param;
	struct dev_state *dev_state;
	int minor;
	int port;

	inode = filp->f_dentry->d_inode;

	minor = MINOR(inode->i_rdev);

	if(dev_state = find_device_open(dev_state, minor))
	{
		if(dev_state->dev_open)
		{

		}else{
			return 1;
		}
	}else{
		return 1;
	}	

	//printk("Djvu IOCTL: %d\tparam: %d\n", ioctl, param);
	//printk("IOCTL_GET_JIFF %d %d\n", IOCTL_GET_JIFF, _IOC_TYPE(ioctl));

	if(ioctl == IOCTL_GET_RSTAT)
	{
		*size = dev_state->byte_read;
	}
	else if(ioctl == IOCTL_GET_WSTAT)
	{
		*size = dev_state->byte_write;
	}
	else if(ioctl == IOCTL_RESET_RSTAT)
	{
		dev_state->byte_read = 0;
	}
	else if(ioctl == IOCTL_RESET_WSTAT)
	{
		dev_state->byte_write = 0;
	}
	else if(ioctl == IOCTL_CHANGE_RPORT)
	{
		dev_state->port_read = dev_state->port;
	}
	else if(ioctl == IOCTL_CHANGE_WPORT)
	{
		dev_state->port_write = dev_state->port + 1;
	}
	else if(ioctl == IOCTL_CHANGE_PORT330)
	{
		dev_state->port_work = dev_state->port;
	}
	else if(ioctl == IOCTL_CHANGE_PORT331)
	{
		dev_state->port_work = dev_state->port + 1;
	}
	else if(ioctl == IOCTL_MODE_RW)
	{
		if(minor == 0)
			dev_state->mode = param;
		else if (minor == 1)
			dev_state->mode = param;
		else if (minor == 2)
			dev_state->mode = param;
	}
	else if(ioctl == IOCTL_MODE_DELAY)
	{
		if(minor == 0)
			dev_state->delay = param;
		else if (minor == 1)
			dev_state->delay = param;
		else if (minor == 2)
			dev_state->delay = param;
	}
	else if(ioctl == IOCTL_SET_PID)
	{
		dev_state->pid = param;

		//printk("dev_state->pid %d\n", dev_state->pid);

		for_each_process(task)
		{
			if(task->pid == dev_state->pid)
			{
				//printk("task->pid %d\n", task->pid);

				dev_state->task = task;

				//send_sig(SIGUSR2, task, 0);
			}
		}
	}else if(ioctl == IOCTL_GET_JIFF)
	{
		//printk("dev_state->time_now %d\n", dev_state->time_now);
		//copy_to_user((void *)param, aaa, _IOC_SIZE(ioctl));
		//param = aaa;
		//printk("aaa %d\n", aaa);
		//put_user(aaa, &param);
		//return dev_state->time_now;
		//mdelay(5000);
		return jiffies;
	}

	return 0;
}

// Структура с указателями на функции драйвера
//struct file_operations Fops_3M_KBB_3;

// Указываем ядру на наши функции
struct file_operations Fops_EM_KBB3 =
{
	open:		device_open,
	release:	device_close,
	read:		device_read,
	write:		device_write,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
	unlocked_ioctl:	device_ioctl,
#else
	ioctl:			device_ioctl,
#endif
};

int resurs0 = 0;
int resurs1 = 0;
int resurs2 = 0;

static /*__init*/ int EM_KBB3_init(void)
{

	//LIST_HEAD(state);
	INIT_LIST_HEAD(&state.list);

	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
		SET_MODULE_OWNER(&Fops_EM_KBB3);
	#endif

	printk("Djvu Init: #----------------------------#\n");
	printk("Djvu Init: This is a driver EM-KVV %d\n", major);
	printk("Djvu Init: HZ %d\n", HZ);

	if(major == 0)
		major = register_chrdev(major, DEV_NAME, &Fops_EM_KBB3);
	else
		if((error = register_chrdev(major, DEV_NAME, &Fops_EM_KBB3)))
		{
			printk("Djvu Init: Register failed!\n");
			return error;
		}

	// Проверка успешной регистрации
	if(major < 0)
	{
		printk("Djvu Init: Register failed!\n");
		return major;
	}else{
		printk("Djvu Init: Device registered. major number is - %d\n", major);
	}
	/*
	IOCTL_GET_RSTAT   = _IOWR(major, 1, ssize_t *);
	IOCTL_GET_WSTAT   = _IOWR(major, 2, ssize_t *);
	IOCTL_RESET_RSTAT = _IOWR(major, 3, ssize_t *);
	IOCTL_RESET_WSTAT = _IOWR(major, 4, ssize_t *);

	IOCTL_CHANGE_PORT330 = _IOWR(major, 5, ssize_t *);
	IOCTL_CHANGE_PORT331 = _IOWR(major, 6, ssize_t *);
	IOCTL_CHANGE_RPORT   = _IOWR(major, 7, ssize_t *);
	IOCTL_CHANGE_WPORT   = _IOWR(major, 8, ssize_t *);

	IOCTL_MODE_RW        = _IOWR(major, 9, ssize_t *);
	*/

	//IOCTL_GET_JIFF	 = _IOR(major, 12, unsigned long);

	if(arg_correct(ports0, count0, irq0, name0))
	{

	}else{
		// Функция захвата ресурсов
		if(capture_of_resources(ports0, count0, irq0, name0, minor0, Fops_EM_KBB3))
		{
			printk("Djvu Init: Failed capture of resources device %s\n", name0);
		}else{
			resurs0++;
			printk("Djvu Init: Capture of resources device %s\n", name0);
		}
	}

	if(arg_correct(ports1, count1, irq1, name1))
	{

	}else{
		// Функция захвата ресурсов
		if(capture_of_resources(ports1, count1, irq1, name1, minor1,Fops_EM_KBB3))
		{
			printk("Djvu Init: Failed capture of resources device %s\n", name1);
		}else{
			resurs1++;
			printk("Djvu Init: Capture of resources device %s\n", name1);
		}
		
	}

	if(arg_correct(ports2, count2, irq2, name2))
	{

	}else{
		// Функция захвата ресурсов
		if(capture_of_resources(ports2, count2, irq2, name2, minor2, Fops_EM_KBB3))
		{
			printk("Djvu Init: Failed capture of resources device %s\n", name2);
		}else{
			resurs2++;
			printk("Djvu Init: Capture of resources device %s\n", name2);
		}
	}

	printk("Djvu Init: Please, create a dev file with 'mknod /dev/em_kvv* c %d [0-..]'.\n", major);

	return 0;
}

static void /*__exit*/ EM_KBB3_exit(void)
{
	printk("Djvu Exit: Driver stoped.\n");

	if(resurs0)
	{
		// Функция освобождения ресурсов
		if(uncapture_of_resources(ports0, count0, irq0, name0))
		{
		}else{
			printk("Djvu Exit: Uncapture of resources device %s\n", name0);
		}
	}

	if(resurs1)
	{
		// Функция освобождения ресурсов
		if(uncapture_of_resources(ports1, count1, irq1, name1))
		{
		}else{
			printk("Djvu Exit: Uncapture of resources device %s\n", name1);
		}
	}

	if(resurs2)
	{
		// Функция освобождения ресурсов
		if(uncapture_of_resources(ports2, count2, irq2, name2))
		{
		}else{
			printk("Djvu Exit: Uncapture of resources device %s\n", name1);
		}
	}

	// Снимаем регистрацию устройства
	unregister_chrdev(major, DEV_NAME);
	//printk("Djvu Exit: device unregistered, major number is %d\n", major);

	// Освобождаем память выделенную для приёма информации
	kfree(buffer0);
	kfree(buffer1);
	kfree(buffer2);

	printk("Djvu Exit: module EM-KVV-3 is dead \n");
	return;
}

module_init(EM_KBB3_init);
module_exit(EM_KBB3_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("djvu djvu@inbox.ru");
MODULE_DESCRIPTION("Driver for ЕМ-КВВ-3");

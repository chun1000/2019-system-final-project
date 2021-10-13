#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define BTN_MAJOR_NUMBER 504
#define BTN_DEV_NAME   "btn_dev2"

#define GPIO_BASE_ADDR 0x3F200000
#define GPFSEL1 0X08
#define GPLEV0 0x34

#define IOCTL_MAGIC_NUMBER 'W'
#define IOCTL_CMD_SET_DIRECTION_BTN _IOWR(IOCTL_MAGIC_NUMBER, 0, int)
#define IOCTL_CMD_IS_BUTTON_ON _IOWR(IOCTL_MAGIC_NUMBER, 1, int)

static void __iomem *gpio_base;
volatile unsigned int *gpsel1;
volatile unsigned int *gplev0;

int btn_open(struct inode *inode, struct file *filp){
   printk(KERN_ALERT "BTN driver open!!\n");
   
   gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
   gpsel1=(volatile unsigned int *)(gpio_base+GPFSEL1);
   gplev0=(volatile unsigned int *)(gpio_base+GPLEV0);
   
   return 0;
}

int btn_release(struct inode *inode, struct file *filp){
   printk(KERN_ALERT "BTN driver close!!\n");
   iounmap((void*)gpio_base);
   return 0;
}
long btn_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int kbuf = -1;
	int i = 0;
	int dbg = -8;
	
	switch(cmd) {
		case IOCTL_CMD_SET_DIRECTION_BTN:
			copy_from_user(&kbuf, (const void*)arg, 4);
			if(kbuf==0) {
				printk(KERN_ALERT "BTN set direction in!!\n");
				*gpsel1 |= (0<<24);
			} else if(kbuf==1){
				printk(KERN_ALERT "BTN set direction out!!\n");
				*gpsel1 |= (1<<24);
			} else {
				printk(KERN_ALERT "ERROR direction parameter error\n");
				return -1;
			}
			return 1;
		case IOCTL_CMD_IS_BUTTON_ON:
			dbg = 1;
			if(((*gplev0) &( 1 << 23)) == 0) kbuf = 1;
			else kbuf = 0;
			
			copy_to_user((const void*)arg, &kbuf, 4);
			return 2;
	}
	return dbg;
}

static struct file_operations btn_fops = {
	.owner = THIS_MODULE,
	.open = btn_open,
	.release = btn_release,
	.unlocked_ioctl = btn_ioctl
};
	
int __init btn_init(void){
	if(register_chrdev(BTN_MAJOR_NUMBER, BTN_DEV_NAME, &btn_fops) < 0)
		printk(KERN_ALERT "BTN driver initialization fail\n");
	else
		printk(KERN_ALERT "BTN driver initialization success\n");
	
	return 0;
}

void __exit btn_exit(void){
	unregister_chrdev(BTN_MAJOR_NUMBER, BTN_DEV_NAME);
	printk(KERN_ALERT "BTN driver exit done\n");
}

module_init(btn_init);
module_exit(btn_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jiwoong");
MODULE_DESCRIPTION("des");

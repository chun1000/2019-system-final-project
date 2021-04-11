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

#define SEND_MAJOR_NUMBER 508
#define SEND_DEV_NAME "send_dev"

#define GPIO_BASE_ADDR 0x3F200000
#define GPFSEL1 0X04 //000 input 001 output
#define GPSET0 0x1c
#define GPLEV0 0x34
#define GPCLR0 0x28

#define IOCTL_MAGIC_NUMBER_S 'w'
#define IOCTL_SEND _IOWR(IOCTL_MAGIC_NUMBER_S, 0, int)

static void __iomem *gpio_base;
volatile unsigned int *gpsel1;
volatile unsigned int *gpset1;
volatile unsigned int *gpclr1;
volatile unsigned int *gplev1;

int send_open(struct inode *inode, struct file *filp){
   printk(KERN_ALERT "send driver open!!\n");
   
   gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
   gpsel1=(volatile unsigned int *)(gpio_base+GPFSEL1);
   gpset1=(volatile unsigned int *)(gpio_base+GPSET0);
   gpclr1=(volatile unsigned int *)(gpio_base+GPCLR0);
   gplev1=(volatile unsigned int *)(gpio_base+GPLEV0);
   
   
   return 0;
}

int send_release(struct inode *inode, struct file *filp){
   printk(KERN_ALERT "send driver close!!\n");
   iounmap((void*)gpio_base);
   return 0;
}

long send_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{   
   int cbuf = 0x00;
   
   switch(cmd) {
      case IOCTL_SEND:
      *gpsel1 |= (1<<9); //14 pin(output)
      *gpsel1 &= ~(1<<10);
      *gpsel1 &= ~(1<<11);
      copy_from_user(&cbuf,(const void*)arg,4);
      
      if(cbuf==0)
      {
         *gpset1 |= (1<<13);
         cbuf = 2;
      }
      else
      { 
         *gpclr1 |= (1<<13);  
          cbuf = 3;
      }
        return cbuf;
   }
   return 0;
}

static struct file_operations send_fops = {
   .owner = THIS_MODULE,
   .open = send_open,
   .release = send_release,
   .unlocked_ioctl = send_ioctl
};
   
int __init send_init(void){
   if(register_chrdev(SEND_MAJOR_NUMBER, SEND_DEV_NAME, &send_fops) < 0)
      printk(KERN_ALERT "chat driver initialization fail\n");
   else
      printk(KERN_ALERT "chat driver initialization success\n");
   
   return 0;
}

void __exit send_exit(void){
   unregister_chrdev(SEND_MAJOR_NUMBER, SEND_DEV_NAME);
   printk(KERN_ALERT "send driver exit done\n");
}

module_init(send_init);
module_exit(send_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jiwoong");
MODULE_DESCRIPTION("des");

   

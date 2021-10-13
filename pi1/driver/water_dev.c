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

#define WATER_MAJOR_NUMBER 501
#define WATER_DEV_NAME   "water_dev"

#define GPIO_BASE_ADDR 0x3F200000
#define GPFSEL0 0x00
#define GPFSEL1 0X04
#define GPSET0 0x1c
#define GPCLR0 0x28

#define SPI_BASE_ADDR 0x3F204000
#define SPICS 0x00
#define SPIFIFO 0x04
#define SPICLK 0x08
#define SPIDLEN 0x0c
#define SPILTOH 0x10
#define SPIDC 0x14


#define IOCTL_MAGIC_NUMBER 'y'
#define WATER_SET _IOWR(IOCTL_MAGIC_NUMBER, 0, int)
#define WATER_GET _IOWR(IOCTL_MAGIC_NUMBER, 1, int)

static void __iomem *gpio_base;

volatile unsigned int *gpsel0;
volatile unsigned int *gpsel1;
volatile unsigned int *gpset1;
volatile unsigned int *gpclr1;

static void __iomem *spi_base;

volatile unsigned int *spics;
volatile unsigned int *spififo;
volatile unsigned int *spiclk;
volatile unsigned int *spidlen;
volatile unsigned int *spiltoh;
volatile unsigned int *spidc;


static void __iomem *aux_base;

int water_open(struct inode *inode, struct file *filp){
   printk(KERN_ALERT "water driver open!!\n");
   
   gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
   spi_base = ioremap(SPI_BASE_ADDR, 0x0C);
   
   gpsel1=(volatile unsigned int *)(gpio_base+GPFSEL1);
   gpsel0=(volatile unsigned int *)(gpio_base+GPFSEL0);
   gpset1=(volatile unsigned int *)(gpio_base+GPSET0);
   gpclr1=(volatile unsigned int *)(gpio_base+GPCLR0);
   
   spics = (volatile unsigned int*)(spi_base+SPICS);
   spififo = (volatile unsigned int*)(spi_base +SPIFIFO);
   spiclk = (volatile unsigned int*)(spi_base + SPICLK);
   spidlen = (volatile unsigned int*)(spi_base + SPIDLEN);
   spiltoh = (volatile unsigned int*)(spi_base + SPILTOH);
   spidc = (volatile unsigned int*)(spi_base + SPIDC);
   
   return 0;
}

int water_release(struct inode *inode, struct file *filp){
   printk(KERN_ALERT "water driver close!!\n");
   iounmap((void*)gpio_base);
   return 0;
}


int get_value(void)
{
    int kbuf = 0;
   char buf1 = 0x01;
   char buf2 = 0x80;
   *spics = 0x00B0; //Initialization
   *spiclk = 64;
         
         printk("spics:%x\n", *spics);
         //first cycle
         while(1) if(((*spics) &(1<<18)) != 0) break; //wait for tx buffer has space.
         *spififo = buf1; //send mcp3008 start bit(1/2)
         while(1) if(((*spics) & (1<<16)) != 0) break; //wait for done.
         
         
         
         //second cycle
         while(1) if(((*spics) &(1<<18)) != 0) break;
         *spififo = buf2; // send mcp3008 start bit(2/2)
         while(1) if(((*spics) & (1<<16)) != 0) break;


         //third cycle
          while(1) if(((*spics) &(1<<18)) != 0) break;
         *spififo = 0x00; // don't care value
         while(1) 
         {
            if(((*spics)&(1<<17)) == 0) break; //check read fifo empty
            buf1 = *spififo; //read bit from mcp(1/2)
         }
         while(1) if(((*spics) & (1<<16)) != 0) break;
   
   
         //fourth cycle
         while(1) if(((*spics) &(1<<18)) != 0) break;
         *spififo = 0x00; // don't care value
         while(1) 
         {
            if(((*spics)&(1<<17)) == 0) break; //check read fifo empty
            buf2 = *spififo; //read bit from mcp(2/2)
         }
         while(1) if(((*spics) & (1<<16)) != 0) break;

         buf1 = buf1 & 0x0F; //mask
         kbuf = (buf1 <<8) | buf2;
         printk("buf1:%u\n", kbuf); 
      
        
         *spics = 0x0000; //reset
         return kbuf;
}

long water_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
   int kbuf = 0;
   int i = 0;
   int flag = 0;
   switch(cmd) {
      case WATER_SET:
         //gpfsel setting.
         printk("spics:%x\n", *spics);
         *gpsel0 &= ~(1<<21);
         *gpsel0 &= ~(1<<22);
         *gpsel0 |= (1<<23); //GPIO 7 = ALT0
         
         *gpsel0 &= ~(1<<24);
         *gpsel0 &= ~(1<<25);
         *gpsel0 |= (1<<26); //GPIO 8 = ALT0
         
         *gpsel0 &= ~(1<<27);
         *gpsel0 &= ~(1<<28);
         *gpsel0 |= (1<<29); //GPIO 9 = ALT0
         
         *gpsel1 &= ~(1<<0);
         *gpsel1 &= ~(1<<1);
         *gpsel1 |= (1<<2); //GPIO 10 = ALT0
         
         *gpsel1 &= ~(1<<3);
         *gpsel1 &= ~(1<<4);
         *gpsel1 |= (1<<5); // GPIO 11 = ALT0
         return 1;

      case WATER_GET:
         for(i = 0; i < 10; i++)
         {
            kbuf += get_value();  
         }
         kbuf /= 10;
         copy_to_user((const void*)arg, &kbuf, 1);
      default:
         return 3;
   }
   return 1500; //no error.
}


static struct file_operations water_fops = {
   .owner = THIS_MODULE,
   .open = water_open,
   .release = water_release,
   .unlocked_ioctl = water_ioctl
};
   

int __init water_init(void){
   if(register_chrdev(WATER_MAJOR_NUMBER, WATER_DEV_NAME, &water_fops) < 0)
      printk(KERN_ALERT "water driver initialization fail\n");
   else
      printk(KERN_ALERT "water driver initialization success\n");
   
   
   return 0;
}


void __exit water_exit(void){
   unregister_chrdev(WATER_MAJOR_NUMBER, WATER_DEV_NAME);
   printk(KERN_ALERT "water driver exit done\n");
}

module_init(water_init);
module_exit(water_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jiwoong");
MODULE_DESCRIPTION("des");

   

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

#define DUST_MAJOR_NUMBER 502
#define DUST_DEV_NAME   "dust_dev"

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

#define IOCTL_MAGIC_NUMBER 'z'
#define DUST_SET _IOWR(IOCTL_MAGIC_NUMBER, 0, int)
#define DUST_GET _IOWR(IOCTL_MAGIC_NUMBER, 1, int)

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

int dust_open(struct inode *inode, struct file *filp){
   printk(KERN_ALERT "dust driver open!!\n");
   
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

int dust_release(struct inode *inode, struct file *filp){
   printk(KERN_ALERT "dust driver close!!\n");
   iounmap((void*)gpio_base);
   return 0;
}

int get_ad_value(void)
{
   
    int kbuf = 0;
   char buf1 = 0x01;
   char buf2 = 0x90;
    *spics = 0x00B0; //Initialization
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
               
        return kbuf;
         
}


long dust_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
   int kbuf = 0;
   int i = 0;
   int k =0;
   int flag = 0;
   int buf;
   switch(cmd) {
      case DUST_SET:
          //gpfsel setting.
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
         
         *gpsel1 |= (1<<24);
         *gpsel1 &= ~(1<<25);
         *gpsel1 &= ~(1<<26); //GPIO 18 = Out
         *spiclk = 64;
         return 1;
      case DUST_GET:
         for(i=0; i<100; i++)
         {
            *gpclr1 |= (0<<18);
            udelay(280);
            
            buf = get_ad_value();
            
            udelay(30);
            *gpset1 |= (1<<18);
            mdelay(9);
            udelay(680);

            kbuf += buf;

         }
      
         kbuf /= 100;
         
         *spics = 0x0000; //reset
         copy_to_user((const void*)arg, &kbuf, 1);
         return 2;
         break;
      default:
         return 3;
   }
   return 1600; //no error.
}


static struct file_operations dust_fops = {
   .owner = THIS_MODULE,
   .open = dust_open,
   .release = dust_release,
   .unlocked_ioctl = dust_ioctl
};
   

int __init dust_init(void){
   if(register_chrdev(DUST_MAJOR_NUMBER, DUST_DEV_NAME, &dust_fops) < 0)
      printk(KERN_ALERT "dust driver initialization fail\n");
   else
      printk(KERN_ALERT "dust driver initialization success\n");
   
   
   return 0;
}


void __exit dust_exit(void){
   unregister_chrdev(DUST_MAJOR_NUMBER, DUST_DEV_NAME);
   printk(KERN_ALERT "dust driver exit done\n");
}

module_init(dust_init);
module_exit(dust_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jiwoong");
MODULE_DESCRIPTION("des");

   

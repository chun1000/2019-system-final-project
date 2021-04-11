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

#define ACT_MAJOR_NUMBER 505
#define ACT_DEV_NAME   "act_dev"

#define GPIO_BASE_ADDR 0x3F200000
#define GPFSEL1 0X04
#define GPLEV0 0x34
//PWM
#define PWM_BASE_ADDR 0x3F20C000
//
#define PWM_CTL 0x00
#define PWM_RNG1 0x10
#define PWM_DAT1 0x14

// clock control
#define CLK_BASE_ADDR 0x3F101000
//offset
#define CLK_PWM_CTL 0xa0
#define CLK_PWM_DIV 0xa4

//PASSWORD
#define BCM_PASSWORD 0x5A000000

static void __iomem *gpio_base;
volatile unsigned int *gpsel1;
volatile unsigned int *gplev1;

static void __iomem *pwm;
volatile unsigned int *pwmctl;
volatile unsigned int *pwmrng1;
volatile unsigned int *pwmdat1;

static void __iomem *clk;
volatile unsigned int *clkdiv;
volatile unsigned int *clkctl;

#define IOCTL_MAGIC_NUMBER_P 's'
#define IOCTL_CMD_SET_DIRECTION _IOWR(IOCTL_MAGIC_NUMBER_P, 0, int)
#define IOCTL_CMD_CLEAR_DIRECTION _IOWR(IOCTL_MAGIC_NUMBER_P,1,int)
#define IOCTL_FLAG_CHECK _IOWR(IOCTL_MAGIC_NUMBER_P,2,int)
int init_act(void) {
   int pwm_ctrl = *pwmctl;
   *pwmctl = 0;                // store PWM control and stop PWM
   msleep(10);                  // sleep
   *clkctl = BCM_PASSWORD | (0x01 << 5); // stop PWM Clock
   msleep(10);                  //sleep
   
   int idiv = (int)(19200000.0f / 16000.0f); // Oscilloscope to 16kHz
   *clkdiv = BCM_PASSWORD | (idiv << 12); // integer part of divisior register
   *clkctl = BCM_PASSWORD | (0x11); //set source to oscilloscope & enable PWM CLK

   *pwmctl = pwm_ctrl;             // restore PWM control and enable PWM
}

int act_open(struct inode *inode, struct file *filp){
   printk(KERN_ALERT "act driver open!!\n");
   
   gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
   gpsel1=(volatile unsigned int *)(gpio_base+GPFSEL1);
   gplev1 = (volatile unsigned int *)(gpio_base+GPLEV0);
   clk = ioremap(CLK_BASE_ADDR, 0xFF);   //size
   clkctl=(volatile unsigned int*)(clk+CLK_PWM_CTL);
   clkdiv=(volatile unsigned int*)(clk+CLK_PWM_DIV);
   
   pwm = ioremap(PWM_BASE_ADDR, 0xFF);
   pwmctl = (volatile unsigned int*)(pwm+PWM_CTL);
   pwmrng1 = (volatile unsigned int*)(pwm+PWM_RNG1);
   pwmdat1 = (volatile unsigned int*)(pwm+PWM_DAT1);
   *gpsel1 &= ~(1<<26);   //Alternate function5
   *gpsel1 |= (1<<25);
   *gpsel1 &= ~(1<<24);
   *pwmctl |= (1);            //PWEN       1
   *pwmctl &= ~(1<<1);          //MODE1      0
   *pwmctl |= (1<<7);          //MSEN1      1
   *pwmrng1 = 320;            //RANGE      320
   *gpsel1 |= (0<<15); // 15 pin(input)
   
   return 0;
}

int act_release(struct inode *inode, struct file *filp){
   printk(KERN_ALERT "act driver close!!\n");
   iounmap((void*)gpio_base);
   iounmap((void*)clk);
   iounmap((void*)pwm);
   return 0;
}

long act_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
   int kbuf = 0;
   int i = 0;
   init_pwm();
   switch(cmd) {
      
      case IOCTL_CMD_SET_DIRECTION:
         printk(KERN_ALERT "motor set\n");
         *pwmdat1|= (1<<5);          //DAT         32   (1/10) 90degrees
         printk(KERN_ALERT,"%x",*pwmdat1);
         break;
       
      case IOCTL_CMD_CLEAR_DIRECTION: 
         printk(KERN_ALERT "motor clear\n");
         *pwmdat1&= ~(1<<5);
         *pwmdat1|= (1<<4);          
         *pwmdat1|= (1<<3);         //DAT         24   (1.5/20) 0degrees
         printk(KERN_ALERT,"%x",*pwmdat1);
         break;
      case IOCTL_FLAG_CHECK:
         if(((*gplev1)&( 1 << 15)) == 0) return 10;
         else return 11;
   }

   return 0;
}

static struct file_operations act_fops = {
   .owner = THIS_MODULE,
   .open = act_open,
   .release = act_release,
   .unlocked_ioctl = act_ioctl,
};
   
int __init act_init(void){
   if(register_chrdev(ACT_MAJOR_NUMBER, ACT_DEV_NAME, &act_fops) < 0)
      printk(KERN_ALERT "act driver initialization fail\n");
   else
      printk(KERN_ALERT "act driver initialization success\n");
   
   return 0;
}

void __exit act_exit(void){
   unregister_chrdev(ACT_MAJOR_NUMBER, ACT_DEV_NAME);
   printk(KERN_ALERT "act driver exit done\n");
}

module_init(act_init);
module_exit(act_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wooseok");
MODULE_DESCRIPTION("des");
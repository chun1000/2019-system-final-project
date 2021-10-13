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

#define LCD_DEV_PATH_NAME "/dev/lcd_dev1"
#define LCD_MAJOR_NUMBER 511
#define LCD_MINOR_NUMBER 100
#define LCD_DEV_NAME "lcd_dev1"

#define GPIO_BASE_ADDR 0x3F200000
#define GPFSEL0 0x00
#define GPFSEL1 0X04 //000 input 001 output
#define GPFSEL2 0x08
#define GPSET0 0x1c
#define GPLEV0 0x34
#define GPCLR0 0x28
#define IOCTL_MAGIC_NUMBER 'r'
#define IOCTL_LCD_SETTING _IOWR(IOCTL_MAGIC_NUMBER, 0, char)
#define IOCTL_LCD_START_PRINT _IOWR(IOCTL_MAGIC_NUMBER, 1, char)
#define IOCTL_LCD_PRINT_CHAR _IOWR(IOCTL_MAGIC_NUMBER, 2, char)

#define BSC_BASE_ADDR 0x3F804000
#define I2C_C 0x0
#define I2C_A 0xc
#define I2C_S 0x4
#define I2C_FIFO 0x10
#define I2C_DIV 0x14
#define I2C_DLEN 0x8

static void __iomem *gpio_base;
static void __iomem *i2c_base;
volatile unsigned int *gpsel1;
volatile unsigned int *gpsel0;
volatile unsigned int *gpsel2;
volatile unsigned int *gpset1;
volatile unsigned int *gpclr1;
volatile unsigned int *gplev1;
volatile unsigned int *i2cc;
volatile unsigned int *i2ca;
volatile unsigned int *i2cdiv;
volatile unsigned int *i2cs;
volatile unsigned int *i2cfifo;
volatile unsigned int *i2cdlen;


int lcd_open(struct inode *inode, struct file *filp){
   printk(KERN_ALERT "lcd driver open!!\n");
 
   i2c_base = ioremap(BSC_BASE_ADDR, 0xFF);
   gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
 
   gpsel0=(volatile unsigned int *)(gpio_base+GPFSEL0);
   gpsel1=(volatile unsigned int *)(gpio_base+GPFSEL1);
   gpsel2=(volatile unsigned int *)(gpio_base+GPFSEL2);
   
   i2cc=(volatile unsigned int *)(i2c_base+I2C_C);
   i2ca=(volatile unsigned int *)(i2c_base+I2C_A);
   i2cs=(volatile unsigned int *)(i2c_base+I2C_S);
   i2cdiv=(volatile unsigned int *)(i2c_base+I2C_DIV);
   i2cfifo=(volatile unsigned int *)(i2c_base+I2C_FIFO);
   i2cdlen=(volatile unsigned int *)(i2c_base+I2C_DLEN);
   return 0;
}

int lcd_release(struct inode *inode, struct file *filp){
   printk(KERN_ALERT "lcd driver close!!\n");
   iounmap((void*)gpio_base);
   iounmap((void*)i2c_base);
   return 0;
}

void wait_done(void)
{
   int i = 0;
   while(1) 
   {
      if((((*i2cs)&(1<<1))!= 0) && (((*i2cs)&(1<<0)) == 0)) break;
      if(i==1000) 
      {
         printk("error done register!\n");
         break;
      }
      i++;
   }
}

void clear_state(void)
{
   *i2cs &= ~(1<<8); //clear err
   *i2cs &= ~(1<<9); //clear clkt
   *i2cs &= ~(1<<1); //clear done
}

void print_all_state(int num)
{
   printk("%d: i2cc: %x, i2cs: %x, dlen: %x",num, *i2cc, *i2cs, *i2cdlen);
}

void write_i2c(char value)
{
   int i = 0;
   *i2cc |= (1<<7); //start new transfer
   while(1)
   {
      if(((*i2cs)&(1<<4)) != 0) break;
      if(i>1000)
      {
         printk(KERN_ALERT "Time out check TXD!\n");
         break;
      }
      i++;
   }
   
   *i2cfifo = value;
   
   wait_done();
   print_all_state(111);
   clear_state();
}

void lcd_write(char input)
{
   //backlight;
   char buffer;
   buffer = input | 0x04 | 0x08 ;
   printk("buffer: %x\n", buffer);
   write_i2c(buffer); //enable pulse
   mdelay(1);
   buffer = input & (~0x04) | 0x08 ;
   write_i2c(buffer); //disable pulse
   printk("buffer: %x\n", buffer);
   mdelay(60);
   
}
//only handling first 4 bit, don't care last 4 bit

void write_letter(char input)
{
   char buffer[2];
   char cbuf;
   
   if(('0' <= input) && ( '9' >= input))
   {
      cbuf = input - '0';
      cbuf = (cbuf << 4);
      printk("cbuf:%x\n", cbuf);
      buffer[0] = 0x30 | 0x01;
      buffer[1] = cbuf | 0x01;
      lcd_write(buffer[0]);
      msleep(1);
      lcd_write(buffer[1]); 
   }
   else if(('A' <= input) && ( 'O' >= input))
   {
       cbuf = input - 'A' + 1;
      cbuf = (cbuf << 4);
      printk("cbuf:%x\n", cbuf);
      buffer[0] = 0x40 | 0x01;
      buffer[1] = cbuf | 0x01;
      lcd_write(buffer[0]);
      msleep(1);
      lcd_write(buffer[1]);
   }
   else if(('P' <= input) && ( 'Z' >= input))
   {
       cbuf = input - 'P';
      cbuf = (cbuf << 4);
      printk("cbuf:%x\n", cbuf);
      buffer[0] = 0x50 | 0x01;
      buffer[1] = cbuf | 0x01;
      lcd_write(buffer[0]);
      msleep(1);
      lcd_write(buffer[1]);
   }
   else
   {
      buffer[0] = 0x80 | 0x01;
      buffer[1] = 0x00 | 0x01;
      lcd_write(buffer[0]);
      lcd_write(buffer[1]);
   }
}


long lcd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
   //int kbuf = -1;
  // int x=0;
   //char addr = 0x3f;
   //char data = 0x28;
   //char arraybuff[4];
   //char a;
   int i=0;
   char input;
   switch(cmd) {
      case IOCTL_LCD_SETTING:
         *gpsel0 &= ~(1<<6);
         *gpsel0 &= ~(1<<7);
         *gpsel0 |= (1<<8); //ALT 0
         
         *gpsel0 &= ~(1<<9);
         *gpsel0 &= ~(1<<10);
         *gpsel0 |= (1<<11); //ALT 0
         
         clear_state(); 
         *i2cdlen = 1; //1-byte transfer
         *i2ca = 0x3f;//slave address
         *i2cc = 0x00; //clear c register
         *i2cc |= (1<<15); //i2cenable
         *i2cc |= (1<<4); //clear
         *i2cc |= (1<<5); //clear
            
         wait_done();
         print_all_state(0);
         return 100;
      case IOCTL_LCD_START_PRINT:
         
         lcd_write(0x30);
         mdelay(5);
         lcd_write(0x30);
         mdelay(1);
         lcd_write(0x30);
         mdelay(1);
         lcd_write(0x20); //begin lcd(don't remove!!!)
         
         
         lcd_write(0x20);
         lcd_write(0x80); //NF
         lcd_write(0x00);
         lcd_write(0xC0);
         lcd_write(0x00);
         lcd_write(0x60);
         lcd_write(0x00);
         lcd_write(0x10); //Display Clear
         //initialization finish
         return 200;
      case IOCTL_LCD_PRINT_CHAR:
         copy_from_user( &input, (const void*)arg, 1);
         write_letter(input);
         return 300;
   }
   return -100; 
}


static struct file_operations lcd_fops = {
   .owner = THIS_MODULE,
   .open = lcd_open,
   .release = lcd_release,
   .unlocked_ioctl = lcd_ioctl
};
   

int __init lcd_init(void){
   if(register_chrdev(LCD_MAJOR_NUMBER, LCD_DEV_NAME, &lcd_fops) < 0)
      printk(KERN_ALERT "lcd driver initialization fail\n");
   else
      printk(KERN_ALERT "lcd driver initialization success\n");
   
   return 0;
}


void __exit lcd_exit(void){
   unregister_chrdev(LCD_MAJOR_NUMBER, LCD_DEV_NAME);
   printk(KERN_ALERT "lcd driver exit done\n");
}

module_init(lcd_init);
module_exit(lcd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jiwoong");
MODULE_DESCRIPTION("des");

   

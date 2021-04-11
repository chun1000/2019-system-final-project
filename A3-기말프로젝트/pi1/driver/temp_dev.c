#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define TEMP_MAJOR_NUMBER 503
#define TEMP_DEV_NAME   "temp_dev"
#define MAXTIMINGS    85

#define GPIO_BASE_ADDR 0x3F200000
#define GPFSEL1 0X04
#define GPSET0 0x1c
#define GPCLR0 0x28
#define GPLEV 0x34

#define IOCTL_MAGIC_NUMBER 't'
#define TEMP_SET _IOWR(IOCTL_MAGIC_NUMBER, 0, int)
int dht11_dat[5] = { 0, 0, 0, 0, 0 };

static void __iomem *gpio_base;
volatile unsigned int *gpsel1;
volatile unsigned int *gpset1;
volatile unsigned int *gpclr1;
volatile unsigned int *gplev0;



int TEMP_open(struct inode *inode, struct file *filp){
   printk(KERN_ALERT "TEMPA driver open!!\n");
   
   gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
   gpsel1=(volatile unsigned int *)(gpio_base+GPFSEL1);
   gpset1=(volatile unsigned int *)(gpio_base+GPSET0);
   gpclr1=(volatile unsigned int *)(gpio_base+GPCLR0);
   gplev0 = (volatile unsigned int *)(gpio_base + GPLEV);
   return 0;
}

int TEMP_release(struct inode *inode, struct file *filp){
   printk(KERN_ALERT "TEMPA driver close!!\n");
   iounmap((void*)gpio_base);
   return 0;
}

long TEMP_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int laststate =1;
    int counter        = 0;
    int j        = 0,i;
    int temp=0;
	int flag=0;
	dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;
	int kbuf[2];
	int x=0;
	int t=0;
	switch(cmd) {
		case TEMP_SET:
			
			//*gpsel1 &= ~(1<<26);	//pinMode(DHpin, OUTPUT);
			//*gpsel1 &= ~(1<<25);
			*gpsel1 |= (1<<21);	
			*gpclr1 |= (1<<17);	//digitalWrite(DHpin, LOW); // bus down, send start signal
			mdelay(18); 		// delay greater than 18ms, so DHT11 start signal can be detected
			*gpset1 |= (1<<17);	//digitalWrite(DHpin, HIGH);
			udelay(40); 		// Wait for DHT11 response
			
			//*gpsel1 &= ~(1<<26);	//pinMode(DHpin, INPUT);
			//*gpsel1 &= ~(1<<25);	
			*gpsel1 &= ~(1<<21);
			while(counter<255){
				if(((*gplev0) &( 1 << 17)))
					break;
				udelay(1);
				counter++;
			}
			if(counter==255){
				flag=1;
				return;
			}
			counter=0;
			udelay(110);
			for ( i= 0; i < 40; i++ )
			{
				counter = 0;
				while ( counter<255 )
				{
					if(((*gplev0) &( 1 << 17)))
						break;
					udelay(1);
					counter++;
				}
				if(counter==255){
					flag=1;
					return;
				}
				counter=0;
				
				while(((*gplev0) &( 1 << 17))){
					udelay(1);
					counter++;
					
					if(counter==255){
						flag=1;
						return;
					}
				}
				j=i/8;
				
				//printk(KERN_ALERT "na o gin ha ni : ");
				dht11_dat[j]=dht11_dat[j] << 1;
				if ( counter > 25 /*16*/ )	//"1"
					dht11_dat[j] =dht11_dat[j]+ 1;
					
			}
			//sprintf("%s%s",kbuf,dht11_dat[0],dht11_dat[2]);
			t=(dht11_dat[0]<<8)|(dht11_dat[2]);
			printk(KERN_ALERT "t=%d\n",t);
			copy_to_user((const void*)arg, &t, 4); 
			printk(KERN_ALERT "Humidity = %d.%d %% Temperature = %d.%d \n",
					dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3]);
			if ((dht11_dat[4] == ( (dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 256) ) )
			{
				
				printk(KERN_ALERT "Humidity = %d.%d %% Temperature = %d.%d \n",
					dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3]);
			}else  {
				printk(KERN_ALERT "Data not good, skip\n" );
			}
			
			
			
			break;
	}
	return 1700; //no error.
}

static struct file_operations TEMP_fops = {
	.owner = THIS_MODULE,
	.open = TEMP_open,
	.release = TEMP_release,
	.unlocked_ioctl = TEMP_ioctl
};
	

int __init TEMP_init(void){
	if(register_chrdev(TEMP_MAJOR_NUMBER, TEMP_DEV_NAME, &TEMP_fops) < 0)
		printk(KERN_ALERT "TEMPA driver initialization fail\n");
	else
		printk(KERN_ALERT "TEMPA driver initialization success\n");
	
	return 0;
}


void __exit TEMP_exit(void){
	unregister_chrdev(TEMP_MAJOR_NUMBER, TEMP_DEV_NAME);
	printk(KERN_ALERT "TEMPA driver exit done\n");
}

module_init(TEMP_init);
module_exit(TEMP_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jiwoong");
MODULE_DESCRIPTION("des");

	

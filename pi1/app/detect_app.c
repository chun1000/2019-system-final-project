#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define WATER_DEV_PATH_NAME "/dev/water_dev"
#define WATER_MAJOR_NUMBER 501
#define WATER_MINOR_NUMBER 100
#define WATER_DEV_NAME   "water_dev"

#define DUST_DEV_PATH_NAME "/dev/dust_dev"
#define DUST_MAJOR_NUMBER 502
#define DUST_MINOR_NUMBER 100
#define DUST_DEV_NAME   "dust_dev"

#define TEMP_DEV_PATH_NAME "/dev/temp_dev"
#define TEMP_MAJOR_NUMBER 503
#define TEMP_MINOR_NUMBER 100
#define TEMP_DEV_NAME   "temp_dev"

#define TEMP1_DEV_PATH_NAME "/dev/temp_dev1"
#define TEMP1_MAJOR_NUMBER 510
#define TEMP1_MINOR_NUMBER 100
#define TEMP1_DEV_NAME   "temp_dev1"

#define TT_DEV_PATH_NAME "/dev/tt_dev"
#define TT_MAJOR_NUMBER 505
#define TT_MINOR_NUMBER 100
#define TT_DEV_NAME   "led_pwm1"

#define LCD_DEV_PATH_NAME "/dev/lcd_dev1"
#define LCD_MAJOR_NUMBER 511
#define LCD_MINOR_NUMBER 100
#define LCD_DEV_NAME "lcd_dev1"

#define SEND_DEV_PATH_NAME "/dev/send_dev"
#define SEND_MAJOR_NUMBER 508
#define SEND_DEV_NAME "send_dev"
#define SEND_MINOR_NUMBER 100

#define GPIO_BASE_ADDR 0x3F200000
#define GPFSEL1 0X04 //000 input 001 output
#define GPSET0 0x1c
#define GPLEV0 0x34
#define GPCLR0 0x28

#define IOCTL_MAGIC_NUMBER_W 'y'
#define WATER_SET _IOWR(IOCTL_MAGIC_NUMBER_W, 0, int)
#define WATER_GET _IOWR(IOCTL_MAGIC_NUMBER_W, 1, int)


#define IOCTL_MAGIC_NUMBER_D 'z'
#define DUST_SET _IOWR(IOCTL_MAGIC_NUMBER_D, 0, int)
#define DUST_GET _IOWR(IOCTL_MAGIC_NUMBER_D,1,int)

#define IOCTL_MAGIC_NUMBER_T 't'
#define TEMP_SET _IOWR(IOCTL_MAGIC_NUMBER_T, 0, int)

#define IOCTL_MAGIC_NUMBER_U 'k'
#define TEMP_SET1 _IOWR(IOCTL_MAGIC_NUMBER_U, 0, int)

#define IOCTL_MAGIC_NUMBER_L 'r'
#define IOCTL_LCD_SETTING _IOWR(IOCTL_MAGIC_NUMBER_L, 0, char)
#define IOCTL_LCD_START_PRINT _IOWR(IOCTL_MAGIC_NUMBER_L, 1, char)
#define IOCTL_LCD_PRINT_CHAR _IOWR(IOCTL_MAGIC_NUMBER_L, 2, char)

#define IOCTL_MAGIC_NUMBER_P 'p'
#define IOCTL_CMD_SET_DIRECTION_90_INVERSE _IOWR(IOCTL_MAGIC_NUMBER_P, 0, int)
#define IOCTL_CMD_SET_DIRECTION_90 _IOWR(IOCTL_MAGIC_NUMBER_P, 1, int)

#define IOCTL_MAGIC_NUMBER_S 'w'
#define IOCTL_SEND _IOWR(IOCTL_MAGIC_NUMBER_S, 0, int)

#define BTN_MAJOR_NUMBER 504
#define BTN_MINOR_NUMBER 100
#define BTN_DEV_NAME   "btn_dev2"
#define BTN_DEV_PATH_NAME "/dev/btn_dev2"

#define IOCTL_MAGIC_NUMBER_B 'W'
#define IOCTL_CMD_SET_DIRECTION_BTN _IOWR(IOCTL_MAGIC_NUMBER_B, 0, int)
#define IOCTL_CMD_IS_BUTTON_ON _IOWR(IOCTL_MAGIC_NUMBER_B, 1, int)

#define DUST_THRESHOLD 40
#define WATER_THRESHOLD 70
#define TEMP_THRESHOLD 10
#define MOIST_THRESHOLD 20

int fd_c, fd_w, fd_d, fd_t,fd_t1, fd_l, fd_p, fd_b, fd_s;
char trash = 0xff;
char name[256];
int water =0, dust = 0;
int comp_hu = 0; 
int comp_temp = 0; 
int cur_hu;
int cur_temp;
int tmp_i[4];
int button_state = 1;
int is_window_open = 0;

void *detecter(void *data)
{
   int d = 0;
   int q = 0;
   int tmp1[2];
   int tmp2[2];

   while(1)
   {
      ioctl(fd_w, WATER_GET, &water);
      ioctl(fd_d, DUST_GET, &dust);
      sleep(1);
      
    
   
      ioctl(fd_t, TEMP_SET, &q);
      tmp1[0] = ((0xff00&q)>>8);
      tmp1[1] = (0xff&q);
      tmp_i[0] = (tmp1[0] - (tmp1[0]%10))/10; //huminity 10
      tmp_i[1] = tmp1[0]%10;  //huminity 1    
      tmp_i[2] = (tmp1[1] - (tmp1[1]%10))/10;    //T 10
      tmp_i[3] = tmp1[1]%10; 
      
      
      q=0;
      ioctl(fd_t1, TEMP_SET1, &q);
      tmp2[0] = ((0xff00&q)>>8);
      tmp2[1] = (0xff&q);
   
      cur_hu = tmp1[0];
      cur_temp = tmp1[1];
      comp_hu = tmp1[0]-tmp2[0];
      comp_temp = tmp1[1]-tmp2[1]; 
      sleep(1);
   }
   
}

void fill_lcd_buffer()
{
   char input;
   char start = 0;
   char lcd_buf[51];
   int i = 0;
         for(i=0;i<51;i++)
         {
            if(i==0) lcd_buf[i] = 'W';
            else if(i==2)
            {
               if(water < WATER_THRESHOLD) lcd_buf[i] = 'O'; //AAAA = Water value
               else lcd_buf[i] = 'X'; 
            }
            else if(i==6) lcd_buf[i] = 'D';
            else if(i==8)
            {
               if(dust < DUST_THRESHOLD) lcd_buf[i] = 'O'; //BBBB = Dust value
               else lcd_buf[i] = 'X'; 
            }
            else if(i == 10)
            {
               lcd_buf[i] = 'O';
            }
            else if(i == 11)
            {
                if(button_state == 0) lcd_buf[i] = 'F';
                else lcd_buf[i] = 'N';
            }
            else if(i == 12)
            {
                if(button_state == 0) lcd_buf[i] = 'F';
                else lcd_buf[i] = '@';
            }
            else if(i==40) lcd_buf[i] = 'H';
            else if(i==42) lcd_buf[i] = (char)tmp_i[0]+0x30; //CCCC = 10 Huminity value inside
            else if(i==43) lcd_buf[i] = (char)tmp_i[1]+0x30; //DDDD - 1 Huminity value inside
            else if(i==47) lcd_buf[i] = 'T';
            else if(i==49) lcd_buf[i] = (char)tmp_i[2]+0x30; //EEEE = 10 Temparature value inside
            else if(i==50) lcd_buf[i] = (char)tmp_i[3]+0x30;//FFFF = 1 Temparature value inside
            else lcd_buf[i] = '@';
         }
         i=0;
 
         ioctl(fd_l, IOCTL_LCD_START_PRINT,&start);
         
         for(i=0; i<51;i++)
         {
            input = lcd_buf[i];
            ioctl(fd_l, IOCTL_LCD_PRINT_CHAR ,&input);
         }

}

void *detect_button(void *data)
{
   int p = 0;
   int u =32;
   u = ioctl(fd_b, IOCTL_CMD_IS_BUTTON_ON, &p);
   while(1)
   {
      sleep(1);
      u = ioctl(fd_b, IOCTL_CMD_IS_BUTTON_ON, &p);
      if(p==1) 
      {
         if(button_state == 0) button_state = 1;
         else button_state = 0;
         printf("button toggle: %d\n", button_state);
      }
   }
   
}



int main(void)
{
   dev_t chat_dev;
   dev_t water_dev;
   dev_t dust_dev;
   dev_t temp_dev;
   dev_t temp1_dev;
   dev_t lcd_dev;
   dev_t pwm_dev;
   dev_t btn_dev;
   dev_t send_dev;
   char input = 0;
   char p = '0';
   int x=0;
   int d = 0;
   int t_result;
   
   water_dev=makedev(WATER_MAJOR_NUMBER, WATER_MINOR_NUMBER);
   mknod(WATER_DEV_PATH_NAME, S_IFCHR|0666, water_dev);
   
   dust_dev=makedev(DUST_MAJOR_NUMBER, DUST_MINOR_NUMBER);
   mknod(DUST_DEV_PATH_NAME, S_IFCHR|0666, dust_dev);
   
   temp_dev=makedev(TEMP_MAJOR_NUMBER, TEMP_MINOR_NUMBER);
   mknod(TEMP_DEV_PATH_NAME, S_IFCHR|0666, temp_dev);
   
   temp1_dev =makedev(TEMP1_MAJOR_NUMBER, TEMP1_MINOR_NUMBER);
   mknod(TEMP1_DEV_PATH_NAME, S_IFCHR|0666, temp1_dev);

   pwm_dev =makedev(TT_MAJOR_NUMBER, TT_MINOR_NUMBER);
   mknod(TT_DEV_PATH_NAME, S_IFCHR|0666, pwm_dev);
   
   lcd_dev =makedev(LCD_MAJOR_NUMBER, LCD_MINOR_NUMBER);
   mknod(LCD_DEV_PATH_NAME, S_IFCHR|0666, lcd_dev);
   
   btn_dev =makedev(BTN_MAJOR_NUMBER, BTN_MINOR_NUMBER);
   mknod(BTN_DEV_PATH_NAME, S_IFCHR|0666, btn_dev);
   
   send_dev =makedev(SEND_MAJOR_NUMBER, SEND_MINOR_NUMBER);
   mknod(SEND_DEV_PATH_NAME, S_IFCHR|0666, send_dev);

   fd_w = open(WATER_DEV_PATH_NAME, O_RDWR);
   fd_t = open(TEMP_DEV_PATH_NAME, O_RDWR);
   fd_t1 = open(TEMP1_DEV_PATH_NAME, O_RDWR);
   fd_d = open(DUST_DEV_PATH_NAME, O_RDWR);
   fd_l = open(LCD_DEV_PATH_NAME, O_RDWR);
   fd_p = open(TT_DEV_PATH_NAME, O_RDWR);
   fd_b = open(BTN_DEV_PATH_NAME, O_RDWR);
   fd_s = open(SEND_DEV_PATH_NAME, O_RDWR);
   
   if(fd_l < 0) {
      printf("ERROR: LCD FAIL!\n");
   }
   
   if(fd_b < 0) {
      printf("ERROR: BUTTON FAIL!\n");
   }
   
   ioctl(fd_w,WATER_SET,&d);
   ioctl(fd_d,DUST_SET,&d);
   x = ioctl(fd_l,IOCTL_LCD_SETTING, &p);
   pthread_t p_thread[2];
   int th_id;
   
   
   //char input;
   char start = 0;
   char lcd_buf[51];
   int i = 0;
   
   th_id = pthread_create(&p_thread[0], NULL, detecter, NULL);
   
   if(th_id < 0) {
      printf("thread error\n");
      exit(0);
   }
   th_id = pthread_create(&p_thread[1], NULL, detect_button, NULL);
   
   while(1)
   {
      sleep(3);
      printf("water: %d Dust: %d Humi: %d Temp: %d btn State : %d\n", water, dust, comp_hu, comp_temp, button_state);
      
     if(button_state == 1)
     {
        
         if(water > WATER_THRESHOLD)
        {
           is_window_open = 0;
        }
        else if(dust > DUST_THRESHOLD)
        {
           is_window_open = 0;
        }
        else if(comp_temp > TEMP_THRESHOLD)
        {
           is_window_open = 1;
        }
        else if(comp_hu > MOIST_THRESHOLD)
        {
           is_window_open = 1;
        }
       
        else is_window_open = 1;
     }
     else is_window_open = 0;
      
     if(is_window_open == 0) printf("Window Close!\n");
     else printf("Window Open\n");
     x = ioctl(fd_s,IOCTL_SEND,&is_window_open);
     //printf("send: %d\n", x);
     fill_lcd_buffer();
   }
   
   
   pthread_join(p_thread[0], (void*)&t_result);
   pthread_join(p_thread[1], (void*)&t_result);
   
   
   

   
   close(fd_w);
   close(fd_t);
   close(fd_d);
   close(fd_t1);
   close(fd_l);
   close(fd_b);
   close(fd_s);
   
   return 0;
}


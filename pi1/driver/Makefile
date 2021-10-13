KERNEL_DIR = /lib/modules/4.19.75-v7+/build

obj-m := water_dev.o temp_dev.o dust_dev.o temp2_dev.o lcd_dev.o pwm_dev.o button_dev.o send_dev.o

PWD := $(shell pwd)

all:
	make -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean

#CC = aarch64-linux-gnu-gcc
CC = /opt/gcc-linaro-6.3.1-2017.02-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-gcc
CFLAGS = -static
TARGET = exp_trigger_demo
OBJ = exp_trigger_demo.o access_flag_fault.o translation_fault.o address_size_fault.o permission_fault.o

all:$(OBJ)
	$(CC) $(CFLAGS) $^ -o $(TARGET)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf *.o
	rm -rf $(TARGET)

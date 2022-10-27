#CC = arm-none-linux-gnueabi-gcc
CC = gcc

#CFLAGS += -Wno-unused-result
#CFLAGS += -Wunused-result
CFLAGS += -Wall -O2
CFLAGS += -lpthread
CFLAGS += -lm

.SUFFIXES : .c .o


TARGET := container

all: $(TARGET)


MAIN_OBJS := spidev.o
OBJS := gpio_api.o udp_tx.o en100.o


TARGET_OBJS := $(OBJS) \
	       $(MAIN_OBJS)

$(TARGET): $(TARGET_OBJS)
	$(CC) -o $@ $(TARGET_OBJS) $(CFLAGS) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGET) $(TARGET_OBJS)

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include <math.h>
#include <sched.h>
#include <sys/syscall.h>

#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>

#include "gpio_api.h"
#include "udp_tx.h"
#include "en100.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

struct sched_attr {
   uint32_t size;              /* Size of this structure */
   uint32_t sched_policy;      /* Policy (SCHED_*) */
   uint64_t sched_flags;       /* Flags */
   int32_t  sched_nice;        /* Nice value (SCHED_OTHER,SCHED_BATCH) */
   uint32_t sched_priority;    /* Static priority (SCHED_FIFO, SCHED_RR) */

   /* Remaining fields are for SCHED_DEADLINE */
   uint64_t sched_runtime;
   uint64_t sched_deadline;
   uint64_t sched_period;
};

///////////////////////////////////////////////////////////////
// 실제로 스케줄링 속성을 변경하는 sched_setattr 함수
int sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags)
{
    return syscall(SYS_sched_setattr, pid, attr, flags);
}

int rt_process() 
{
    int result;
 
    struct sched_attr attr;
    // 초기화
    memset(&attr, 0, sizeof(attr));
    attr.size = sizeof(struct sched_attr);
 
    attr.sched_priority = 95; // 우선순위 : 95 -> 135였나.. 를 의미
    attr.sched_policy = SCHED_FIFO; // SCHED_FIFO는 상수로서 정의되어 있다.
 
    // 스케줄링 속성 
    result = sched_setattr(getpid(), &attr, 0);
    if (result == -1) {
        perror("Error calling sched_setattr.");
    }
    return result;
}


///////////////////////////////////////////////////////////////
static void pabort(const char *s)
{
    perror(s);
    abort();
}

static const char *device = "/dev/spidev3.0";
static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 27000000;
static uint16_t delay = 0;          // 7usec

#define DAT_SIZE 4     // 4 byte 보다 작을 경우 1byte 씩 데이터를 끊어서 가져온다


//////////////////////////////////////////
struct epoll_event evlist[10];
struct epoll_event ev;

int epoll_fd = -1;

unsigned int gpio_drdy = GPIO_PIN_ADC_DRDY;
unsigned int gpio_sync = GPIO_PIN_ADC_SYNC;

#define GPIO_CHECK_ENABLE 0

#if ( GPIO_CHECK_ENABLE == 1 )
    unsigned int gpio_chk  = 9;
    int gpio_chk_fd = -1;
#endif

int gpio_fd = -1;
int gpio_sync_fd = -1;



int gpio_sync_init(void)
{
    gpio_export(gpio_sync);
    gpio_set_direction(gpio_sync, GPIO_OUTPUT, 1);
    gpio_sync_fd = gpio_open(gpio_sync);

#if ( GPIO_CHECK_ENABLE == 1 )
    // debug gpio
    gpio_export(gpio_chk);
    gpio_set_direction(gpio_chk, GPIO_OUTPUT, 1);
    gpio_chk_fd = gpio_write_open(gpio_chk);
    gpio_write(gpio_chk_fd, GPIO_LOW);
#endif 

    return gpio_sync_fd;
}

int gpio_drdy_init(void)
{
    epoll_fd = epoll_create(2);
    if(epoll_fd == -1)
    {
        perror("epoll create error, epoll");
        return -1;
    }

    gpio_export(gpio_drdy);
    gpio_set_direction(gpio_drdy, GPIO_INPUT, 0);

#if 0
    gpio_set_edge(gpio_drdy, GPIO_FALLING); // 수정 전 ADC
#else
    //gpio_set_edge(gpio_drdy, GPIO_RISING);  // 수정 후 ADC
#endif
    gpio_fd = gpio_open(gpio_drdy);

    ev.events = POLLPRI;
    ev.data.fd = gpio_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, gpio_fd, &ev) == -1) {
        /* error */
        close(gpio_fd);
        close(epoll_fd);

        return -2;
    }

    return 0;
}

int gpio_drdy_release(void)
{
    gpio_close(gpio_fd);
    gpio_unexport(gpio_drdy);

    return 0;
}

int gpio_sync_release(void)
{
    gpio_close(gpio_sync_fd);
    gpio_unexport(gpio_sync);

    return 0;
}


int gpio_drdy_get(int timeout)
{
    int ret = GPIO_DRDY_ERROR;

    // BLOCKING : time == -1
    // NON_BLOCKING: time == 0
    // TIMEOUT : time  > 0
    //int nfds = epoll_wait(epoll_fd, evlist, 1, timeout /* -1 or 1000 */);
    printf("epoll_fd %d \n", epoll_fd);
    int nfds = epoll_wait(epoll_fd, evlist, 1, -1);
    //printf ("gpio_drdy_get(), nfds = %d \n", nfds);
    printf("nfds %d \n", nfds);
    if ( nfds == -1)
    {
        /* error */
        ret = GPIO_DRDY_ERROR;
    }
    else if (nfds == 0 )
    {
        /* timeout */
        ret = GPIO_DRDY_TIEMOUT;
    }
    else
    {
        int i = 0;
        for (i = 0; i < nfds; i++) {
            if (evlist[i].events & EPOLLPRI) {
                if (evlist[i].data.fd == gpio_fd ) {

                    char buf[MAX_BUF] = { 0x00, };
                    ret = read(gpio_fd, buf, 1);

                    if (*buf != '0')
                        ret = GPIO_DRDY_OK;
                    else
                        ret = GPIO_DRDY_NONE;
                }
            }
        }
    }

    ret = GPIO_DRDY_OK;
    printf ("gpio_drdy_get(), ret = %d \n", ret);
    return ret;
}


//////////////////////////////////////////

//#define COUNT 65536
//#define COUNT 16384         // 약 2초 정도
//#define COUNT 81920          // 약 10초
#define COUNT 162580
//#define COUNT 491520
//#define COUNT 2048
#define ADC_TIMEOUT 3000
uint8_t buffer[ 3200000 ] = { 0x00, };           // DAT_SIZE * COUNT
int     g_dat [ COUNT  ] = { 0x00, };

static void transfer(int fd)
{
    printf("1111111111111\n");
    int ret;
    uint8_t tx[ DAT_SIZE * 3 ] = { 0x00, };
    uint8_t rx[ DAT_SIZE * 3 ] = { 0x00, };
                    
    struct spi_ioc_transfer tr;

    memset(&tr, 0, sizeof(tr));
    tr.tx_buf = (unsigned long)tx;
    tr.rx_buf = (unsigned long)rx;
    //tr.len = DAT_SIZE;
    tr.len = 3;
    tr.delay_usecs = delay;
    tr.speed_hz = speed;
    tr.bits_per_word = bits;
    tr.cs_change = 0;

    usleep(1000);

    if ( gpio_drdy_get(ADC_TIMEOUT) == GPIO_DRDY_OK )
    {
        printf("222222222222\n");
        volatile int offset = 0;
        //volatile int cnt = COUNT * 2;
        volatile int cnt = COUNT;

        gpio_set_value(gpio_sync, GPIO_HIGH);
        usleep(1800);

        while ( cnt > 0 ) 
        {
#if ( GPIO_CHECK_ENABLE == 1 )
            gpio_write(gpio_chk_fd, GPIO_HIGH);
#endif
            ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);

#if ( GPIO_CHECK_ENABLE == 1 )
            gpio_write(gpio_chk_fd, GPIO_LOW);
#endif

            if (ret < 1)
            {
                pabort("can't send spi message");
            }

            if ( cnt <= COUNT ) {
                buffer[offset + 0] = rx[0];
                buffer[offset + 1] = rx[1];
                buffer[offset + 2] = rx[2];

                offset += 3;
            }
            cnt -= 1;
        }
        gpio_set_value(gpio_sync, GPIO_LOW);    
    }
    
}

void filter_output(void) 
{
    int cnt = 0;

    FILE *fp = fopen("/home/keti/projects/adc_output/contaner2.csv",  "wa+");
    

    //fprintf(fp, "count, data\n");

    for ( cnt = 0; cnt < COUNT; cnt++)
    {
        if ( cnt >= 2 
           &&   g_dat[cnt-2] != 0 )
        {
            if (   abs( g_dat[cnt-2] - g_dat[cnt  ] ) < 200000
               && abs( g_dat[cnt-2] - g_dat[cnt-1] ) > 500000 )
            {
                g_dat[cnt-1] = g_dat[cnt-2] + (( g_dat[cnt-2] - g_dat[cnt] ) / 2);
            }
            else if ( g_dat[cnt-1] > 4300000 ) 
            {
                g_dat[cnt-1] = g_dat[cnt-2] + (( g_dat[cnt-2] - g_dat[cnt] ) / 2);
            }

            if (g_dat[cnt] < 3400000){
                g_dat[cnt] = ((g_dat[cnt-1] + g_dat[cnt-2]) /2);
            }
            else if(g_dat[cnt] > 4300000){
                g_dat[cnt] = ((g_dat[cnt-1] + g_dat[cnt-2]) /2);
            }

        }
    }

    for ( cnt = 0; cnt < COUNT; cnt++)
    {
        fprintf(fp, "%d, %d\n", cnt, (g_dat[cnt]*-1));
    }

    if (fp != NULL)
    {
        fclose(fp);
        fp = NULL;
    }
}

void data_output(void)
{
    int cnt = 0;
    int offset = 0;
    
    FILE *fp = fopen("/home/keti/projects/adc_output/contaner.csv",  "wa+");

    //fprintf(fp, "count, data\n");

    for ( cnt = 0; cnt < COUNT; cnt++)
    {
        int value = 0;
        unsigned char *ptr_value = (unsigned char *)&value;

        //ptr_value[3] = 0x00;
        ptr_value[2] = buffer[offset+0];
        ptr_value[1] = buffer[offset+1];
        ptr_value[0] = buffer[offset+2];

        if ( buffer[offset+0] > 0x7F )
        {
            // two's complement = 0x007FFFFF - (signed int)*(unsigned int *)&value[0] -1; 
            printf("value %d, cnt %d, ptr %d \n", value, cnt, ptr_value[2]);
            value = 1 + ~value;
            printf(">0x7F, value %d, cnt %d, ptr %d \n", value, cnt, buffer[offset+0]);
            
        }
        g_dat[cnt] = value;
        if(cnt <10 ){
            //printf("value %d, cnt %d, ptr %d \n", value, cnt, ptr_value[2]);
	    printf("value %d, cnt %d, buffer %d \n", value, cnt, buffer[offset+0]);
        }  
        fprintf(fp, "%d, %d\n", cnt, value);

#if 0   // debug code
        unsigned char *ptr = (unsigned char *)&twos_complement; 

        printf("[ %02X %02X %02X ], [ %02X %02X %02X %02X ], [ %02X %02X %02X %02X ] value = %d \n"
                , buffer[offset], buffer[offset+1], buffer[offset+2]
                , value[0], value[1], value[2], value[3]
                , ptr[0]
                , ptr[1]
                , ptr[2]
                , ptr[3]
                , twos_complement);

        printf("[ %02X %02X %02X ], [ %02X %02X %02X %02X ] value = %d \n"
                , buffer[offset], buffer[offset+1], buffer[offset+2]
                , value[0], value[1], value[2], value[3]
                , twos_complement);
#endif
        offset += (DAT_SIZE - 1);
    }

    if (fp != NULL)
    {
        fclose(fp);
        fp = NULL;
    }
}

void data_send(void)
{
    int cnt = 0;
    int offset = 0;

    unsigned int payload_data[COUNT] = { 0x00, };

    for ( cnt = 0; cnt < COUNT; cnt++)
    {
        int value = 0;
        unsigned char *ptr_value = (unsigned char *)&value;

        ptr_value[3] = buffer[offset+0];
        ptr_value[2] = buffer[offset+1];
        ptr_value[1] = buffer[offset+2];
        ptr_value[0] = 0x00;

        if ( buffer[offset+0] > 0x7F )
        {
            // two's complement = 0x007FFFFF - (signed int)*(unsigned int *)&value[0] -1; 
            value = 1 + ~value;     
        }
        payload_data[cnt] = value;

        offset += (DAT_SIZE - 1);
    }

#if 1
    int sock_udp_fd = udp_tx_init(NULL, 0);
    printf("sock_udp_fd = %d\n", sock_udp_fd );

    int cc = 0;
    int loop = 0;
    for (loop = 0; loop < COUNT; loop += PACKET_ADC_DATA_LEN )
    {
        packet_adc_t packet;
        make_packet(&packet, &payload_data[loop], PACKET_ADC_DATA_LEN, cc);

        udp_tx(sock_udp_fd, (unsigned char *)&packet, sizeof(packet) );
        cc++;
    }
    printf("TRACE, %s, LINE %d\n", __FUNCTION__, __LINE__ );
    udp_tx_release();

    printf("TRACE, %s, LINE %d\n", __FUNCTION__, __LINE__ );
#endif
}



int main(int argc, char *argv[])
{
    int ret = 0;
    int fd;

    // 프로그램 우선순위 높임
    rt_process();

    // GPIO 설정, SPI 초기화
    gpio_sync_init();
    gpio_drdy_init();

    fd = open(device, O_RDWR);
    if (fd < 0)
    {
        pabort("can't open device");
    }

    //Mode setting
    mode |= SPI_CPHA;

    // spi mode
    ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
    if (ret == -1)
        pabort("can't set spi mode");

    ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
    if (ret == -1)
        pabort("can't get spi mode");

    // bits per word
    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1)
        pabort("can't set bits per word");

    ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
    if (ret == -1)
        pabort("can't get bits per word");

    // max speed hz
    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    if (ret == -1)
        pabort("can't set max speed hz");

    ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
    if (ret == -1)
        pabort("can't get max speed hz");

#if 0   // debug code
    printf("spi mode: %d\n", mode);
    printf("bits per word: %d\n", bits);
    printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);
#endif

    //usleep(1000);

    transfer(fd);
    close(fd);      // spi close

    data_output();
    filter_output();
    //data_send();

    gpio_close(gpio_fd);
    gpio_close(gpio_sync_fd);

#if ( GPIO_CHECK_ENABLE == 1 )
    gpio_close(gpio_chk_fd);
#endif
    return ret;
}


#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define DAT_SIZE 4     // 4 byte 보다 작을 경우 1byte 씩 데이터를 끊어서 가져온다

#define GPIO_CHECK_ENABLE 0

//#define COUNT 65536
//#define COUNT 16384         // 약 2초 정도
//#define COUNT 81920          // 약 10초
#define COUNT 162580
//#define COUNT 491520
//#define COUNT 2048
#define ADC_TIMEOUT 3000


static const char *device = "/dev/spidev3.0";
static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 27000000;
static uint16_t delay = 0;          // 7usec


int sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags);
int rt_process();
void pabort(const char *s);
int gpio_sync_init(void);
int gpio_drdy_init(void);
int gpio_drdy_release(void);
int gpio_sync_release(void);
int gpio_drdy_get(int timeout);
void transfer(int fd);
QString filter_output(QString date, QString Time);
void data_output(QString date, QString Time);
void data_send(void);



#ifndef __GPIO_API_H__
#define __GPIO_API_H__


#if 0
    // 1차 보드 테스트
    #define GPIO_PIN_ADC_DRDY	7
    #define GPIO_PIN_ADC_SYNC   8
#else
    // 2차 보드
    //#define GPIO_PIN_ADC_DRDY	8
    //#define GPIO_PIN_ADC_SYNC   6
    //tx2
    #define GPIO_PIN_ADC_DRDY	255
    #define GPIO_PIN_ADC_SYNC   254
#endif


#define GPIO_DRDY_ERROR     -1
#define GPIO_DRDY_TIEMOUT   0
#define GPIO_DRDY_OK        1
#define GPIO_DRDY_NONE      2

#define GPIO_OUTPUT 0
#define GPIO_INPUT  1

#define GPIO_LOW    0
#define GPIO_HIGH   1

#define GPIO_NONE    "none"
#define GPIO_FALLING "falling"
#define GPIO_RISING  "rising"
#define GPIO_BOTH    "both"

#define SYSFS_GPIO_DIR "/sys/class/gpio"

#define MAX_BUF     64

int gpio_export(unsigned int gpio);
int gpio_unexport(unsigned int gpio);
int gpio_get_direction(unsigned int gpio, unsigned int *direction);
int gpio_set_direction(unsigned int gpio, unsigned int direction, unsigned int out_value);
int gpio_get_value(unsigned int gpio, unsigned int *value);
int gpio_set_value(unsigned int gpio, unsigned int value);
int gpio_set_edge(unsigned int gpio, char *edge);

int gpio_write_open(unsigned int gpio);
void gpio_write(int gpio_fd, unsigned char value);

int gpio_open(unsigned int gpio);
int gpio_close(int fd);

#endif // __GPIO_API_H__

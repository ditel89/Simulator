#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>

#include "gpio_api.h"

int gpio_export(unsigned int gpio)
{
    int ret = -1;
    int fd, len;
    char buf[MAX_BUF] = { 0x00, };

    fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);

    if (fd < 0) {
        fprintf(stderr, "Can't export GPIO %d pin: %s\n", gpio, strerror(errno));
        return fd;
    }

    len = snprintf(buf, sizeof(buf), "%d", gpio);
    ret = write(fd, buf, len);
    close(fd);

    return ret;
}

int gpio_unexport(unsigned int gpio)
{
    int ret = -1;
    int fd, len;
    char buf[MAX_BUF] = { 0x00, };

    fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);

    if (fd < 0) {
        fprintf(stderr, "Can't unexport GPIO %d pin: %s\n", gpio, strerror(errno));
        return fd;
    }

    len = snprintf(buf, sizeof(buf), "%d", gpio);
    ret = write(fd, buf, len);
    close(fd);

    return ret;
}

int gpio_get_direction(unsigned int gpio, unsigned int *direction)
{
    int ret = -1;
    int fd, len;
    char buf[MAX_BUF] = { 0x00, };

    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/direction", gpio);

    fd = open(buf, O_RDONLY);

    if (fd < 0) {
        fprintf(stderr, "Can't get GPIO %d pin direction: %s\n", gpio, strerror(errno));
        return fd;
    }

    ret = read(fd, &buf, MAX_BUF);
    close(fd);

    if (strcmp(buf, "in") == 0)
        *direction = GPIO_INPUT;
    else
        *direction = GPIO_OUTPUT;
        
    printf ("gpio_get_direction(), ret = %d \n", ret);
    return ret;
}

int gpio_set_direction(unsigned int gpio, unsigned int direction, unsigned int out_value)
{
    int ret = -1;
    int fd, len;
    char buf[MAX_BUF] = { 0x00, };

    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", gpio);

    fd = open(buf, O_WRONLY);

    if (fd < 0) {
        fprintf(stderr, "Can't set GPIO %d pin direction: %s\n", gpio, strerror(errno));
        return fd;
    }

    if (direction == GPIO_OUTPUT) {
        if (out_value == GPIO_HIGH)
            ret = write(fd, "high", 5);
        else
            ret = write(fd, "out", 4);
    } else {
        ret = write(fd, "in", 3);
    }

    close(fd);
    printf ("gpio_set_direction(), ret = %d \n", ret);
    return ret;
}

int gpio_get_value(unsigned int gpio, unsigned int *value)
{
    int ret = -1;
    int fd, len;
    char buf[MAX_BUF] = { 0x00, };

    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

    fd = open(buf, O_RDONLY);

    if (fd < 0) {
        fprintf(stderr, "Can't get GPIO %d pin value: %s\n", gpio, strerror(errno));
        return fd;
    }

    ret = read(fd, buf, 1);
    close(fd);

    if (*buf != '0')
        *value = GPIO_HIGH;
    else
        *value = GPIO_LOW;

    printf ("gpio_get_value(), ret = %d \n", ret);
    return ret;
}

int gpio_set_value(unsigned int gpio, unsigned int value)
{
    int ret = -1;
    int fd, len;
    char buf[MAX_BUF] = { 0x00, };

    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

    fd = open(buf, O_WRONLY);

    if (fd < 0) {
        fprintf(stderr, "Can't set GPIO %d pin value: %s\n", gpio, strerror(errno));
        return fd;
    }

    if (value == GPIO_HIGH){
        ret = write(fd, "1", 2);
        printf ("gpio_high\n");
    }
    else{
        ret = write(fd, "0", 2);
        printf ("gpio_high\n");
    }

    close(fd);
    
    printf ("gpio_set_value(), ret = %d \n", ret);
    return ret;
}

int gpio_set_edge(unsigned int gpio, char *edge)
{
    int ret = -1;
    int fd, len;
    char buf[MAX_BUF] = { 0x00, };

    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/edge", gpio);

    fd = open(buf, O_WRONLY);

    if (fd < 0) {
        fprintf(stderr, "Can't set GPIO %d pin edge: %s\n", gpio, strerror(errno));
        return fd;
    }

    ret = write(fd, edge, strlen(edge)+1);
    close(fd);

    printf ("gpio_set_edge(), ret = %d \n", ret);
    return ret;
}

int gpio_open(unsigned int gpio)
{
    int fd, len;
    char buf[MAX_BUF] = { 0x00, };

    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

    fd = open(buf, O_RDONLY | O_NONBLOCK);

    if (fd < 0)
    {
        fprintf(stderr, "Can't open GPIO %d pin: %s\n", gpio, strerror(errno));
    }
    return fd;
}

int gpio_write_open(unsigned int gpio)
{
    int fd, len;
    char buf[MAX_BUF] = { 0x00, };

    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

    fd = open(buf, O_WRONLY);
    if (fd < 0)
    {
        fprintf(stderr, "Can't open GPIO %d pin: %s\n", gpio, strerror(errno));
    }
    return fd;
}

int gpio_close(int fd)
{
    int ret = 0;
    if ( fd > 0 )
    {
        ret = close(fd);
    }
    return ret;
}

void gpio_write(int gpio_fd, unsigned char value)
{
    if (gpio_fd >= 0)
    {
        if (value == GPIO_HIGH)
            write(gpio_fd, "1", 2);
        else
            write(gpio_fd, "0", 2);
    }
}

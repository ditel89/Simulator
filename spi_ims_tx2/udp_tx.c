#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <termios.h>
#include <netdb.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <sys/select.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include "udp_tx.h"


#define UDP_PORT 12121

char target_ip[100] = "192.168.0.43";
int  target_port = UDP_PORT;

static pthread_mutex_t mutex_lock;

static int sock_udp_fd = -1;


static int lock_init()
{
    pthread_mutex_init(&mutex_lock, NULL);
    return 0;
}

static int lock()
{
    pthread_mutex_lock(&mutex_lock);
    return 0;
}

static int unlock()
{
    pthread_mutex_unlock(&mutex_lock);
    return 0;
}

int udp_tx_init(char *ip, int port)
{
    lock_init();

    //int fd;
    //int rc;

    // Open UDP socket
    sock_udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sock_udp_fd == -1 )
    {
        printf ("socket failed errno (%d)", errno);
        return -1;
    }

    //strcpy(target_ip, "127.0.0.1");
    if ( ip != NULL ) {
        strcpy(target_ip, ip);
        printf("update target ip, %s\n", target_ip);
    }

    if ( port > 0 ) {
        target_port = port;
        printf("update target port, %d\n", target_port);
    }

    int optval = 1;
    setsockopt(sock_udp_fd, SOL_SOCKET, SO_REUSEADDR,(const void *)&optval , sizeof(int));

#if 0
    rc = bind(sock_udp_fd, (struct sockaddr *)&sockAddr, sizeof(struct sockaddr));
    if ( rc == -1 )
    {
        printf ("bind failed errno (%d)", errno);
        return -1;
    }
#endif

    printf("%s, %s, %d, sock_udp_fd = %d\r\n", __FILE__, __FUNCTION__, __LINE__, sock_udp_fd);

    return sock_udp_fd;
}


void udp_tx_release(void)
{
    if (sock_udp_fd != -1){
        close(sock_udp_fd);
        sock_udp_fd = -1;
    }
}


int udp_tx(int sock_udp_fd, unsigned char *buf, int blen)
{
    int bytes_sent = 0;

    struct sockaddr_in sockAddr;

    /* Bind to server port */
    memset(&(sockAddr), 0, sizeof(sockAddr));
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons( target_port );
	sockAddr.sin_addr.s_addr = inet_addr(target_ip);

    lock();
    bytes_sent = sendto(sock_udp_fd, buf, blen, 0, (struct sockaddr*)&sockAddr, sizeof(struct sockaddr_in));
    unlock();

    return bytes_sent;
}


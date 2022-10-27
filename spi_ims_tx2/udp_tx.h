#ifndef __UDP_TX_H__
#define __UDP_TX_H__

int  udp_tx_init(char *ip, int port);
void udp_tx_release(void);
int  udp_tx(int sock_ipc_fd, unsigned char *buf, int blen);

#endif // __UDP_TX_H__
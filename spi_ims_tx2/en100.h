#ifndef __EN100_H__
#define __EN100_H__

#define PACKET_ADC_DATA_LEN     200 

typedef struct __attribute__((aligned(1), packed)) packet_adc_s 
{
    unsigned char  stx[2];        // 0xaabb
    unsigned char  device_id;     // device id ( 0x00 ~ 0x0F )
    unsigned char  data_type;     // 0x01 : adc 1ch data
    unsigned short cc;            // continuous_count
    unsigned short data_len;      // playload data 의 크기
    unsigned int   payload_data[PACKET_ADC_DATA_LEN]; // 24bit , 채널달 4 byte 전송
    unsigned short crc16;         // stx부터 packet data 까지 crc
    unsigned char  etx[2];        // 0xccdd
} packet_adc_t;

void make_packet( packet_adc_t *packet, unsigned int *data, unsigned short data_len, unsigned short cc );

#endif // __EN100_H__



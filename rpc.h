#ifndef _RPC_H_
#define _RPC_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct rpc_message_t
{
    uint8_t cmd;
    uint8_t id;
    uint8_t len;
    uint8_t data[UINT8_MAX];
    uint16_t crc;
} rpc_message_t;


void rpc_init(void);
bool rpc_process_message(void);
void rpc_send_message(const rpc_message_t *msg);
uint16_t rpc_calculate_crc(const rpc_message_t *msg);


#endif /* _RPC_H_ */

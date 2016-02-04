#include <stdlib.h>
#include <util/crc16.h>
#include "rpc.h"
#include "uart.h"
#include "temp_control.h"

#define RPC_SYNC_BYTE 0x7E
#define RPC_ESCAPE_BYTE 0x7D
#define RPC_ESCAPE_XOR 0x20

#define RPC_COMMAND_LIST_DEVICES 0x01
#define RPC_COMMAND_SET_TARGET_TEMP 0x02

typedef enum rpc_state_t
{
    WAITING_FOR_SYNC,
    COMMAND,
    ID,
    LENGTH,
    CRC1,
    CRC2,
    DATA,
} rpc_state_t;

static rpc_message_t recv_msg;
static rpc_message_t send_msg;


static void rpc_send_devices(void)
{
    uint8_t i, data_pos;
    struct temp_sensor *sensor;

    send_msg.cmd = 0x00;
    send_msg.id = recv_msg.id;
    send_msg.len = 0;

    i = 0;
    data_pos = 0;

    while ((sensor = temp_control_get_sensor_data(i++)) != NULL)
    {
        uint8_t j;

        for (j = 0; j < OW_ROMCODE_SIZE; j++)
            send_msg.data[data_pos++] = sensor->id[j];

        for (j = 0; j < 10; j++)
            send_msg.data[data_pos++] = sensor->name[j];

        send_msg.data[data_pos++] = (sensor->temp >> 8) & 0xFF;
        send_msg.data[data_pos++] = sensor->temp & 0xFF;
    }

    send_msg.len = data_pos;
    send_msg.crc = rpc_calculate_crc(&send_msg);

    rpc_send_message(&send_msg);
}

static bool rpc_set_target_temp(void)
{
    int16_t temp;

    if (recv_msg.len != sizeof(int16_t))
        return false;

    temp = ((int16_t) recv_msg.data[0] << 8) | recv_msg.data[1];
    temp_control_set_target_temp(temp);

    send_msg.cmd = 0x00;
    send_msg.id = recv_msg.id;
    send_msg.len = 0;
    send_msg.crc = rpc_calculate_crc(&send_msg);
    rpc_send_message(&send_msg);

    return true;
}

static bool rpc_parse_message(void)
{
    switch (recv_msg.cmd)
    {
        case RPC_COMMAND_LIST_DEVICES:
            rpc_send_devices();
            return true;
        case RPC_COMMAND_SET_TARGET_TEMP:
            return rpc_set_target_temp();
        default:
            break;
    }

    return false;
}

void rpc_init(void)
{
    uart_init();
}

bool rpc_process_message(void)
{
    static rpc_state_t state = WAITING_FOR_SYNC;
    static uint8_t data_pos = 0;
    static bool escape = false;

    while (uart_bytes_available() > 0)
    {
        uint8_t byte = uart_getc();

        if (byte == RPC_SYNC_BYTE)
        {
            state = COMMAND;
            data_pos = 0;
            continue;
        }

        if (byte == RPC_ESCAPE_BYTE)
        {
            escape = true;
            continue;
        }

        if (escape)
        {
            byte ^= RPC_ESCAPE_XOR;
            escape = false;
        }

        switch (state)
        {
            case WAITING_FOR_SYNC:
                break;
            case COMMAND:
                recv_msg.cmd = byte;
                state = ID;
                break;
            case ID:
                recv_msg.id = byte;
                state = LENGTH;
                break;
            case LENGTH:
                recv_msg.len = byte;
                if (recv_msg.len > 0)
                    state = DATA;
                else
                    state = CRC1;
                break;
            case DATA:
                recv_msg.data[data_pos++] = byte;
                if (data_pos == recv_msg.len)
                    state = CRC1;
                break;
            case CRC1:
                recv_msg.crc = byte << 8;
                state = CRC2;
                break;
            case CRC2:
                recv_msg.crc |= byte;
                state = WAITING_FOR_SYNC;
                if (recv_msg.crc != rpc_calculate_crc(&recv_msg))
                    return false;
                return rpc_parse_message();
        }
    }

    return false;
}

static void rpc_send_escaped(uint8_t byte)
{
    if (byte == RPC_SYNC_BYTE || byte == RPC_ESCAPE_BYTE)
    {
        uart_putc(RPC_ESCAPE_BYTE);
        uart_putc(byte ^ RPC_ESCAPE_XOR);
    }
}

void rpc_send_message(const rpc_message_t *msg)
{
    uint8_t i;

    if (msg == NULL)
        return;

    uart_putc(RPC_SYNC_BYTE);
    rpc_send_escaped(msg->cmd);
    rpc_send_escaped(msg->id);
    rpc_send_escaped(msg->len);
    for (i = 0; i < msg->len; i++)
        rpc_send_escaped(msg->data[i]);
    rpc_send_escaped((msg->crc >> 8) & 0xFF);
    rpc_send_escaped(msg->crc & 0xFF);
    uart_putc(RPC_SYNC_BYTE);   // probably not necessary
}

uint16_t rpc_calculate_crc(const rpc_message_t *msg)
{
    uint16_t crc = 0xFFFF;
    uint8_t i;

    if (msg == NULL)
        return crc;

    crc = _crc_ccitt_update(crc, msg->cmd);
    crc = _crc_ccitt_update(crc, msg->id);
    crc = _crc_ccitt_update(crc, msg->len);
    for (i = 0; i < msg->len; i++)
        crc = _crc_ccitt_update(crc, msg->data[i]);

    return crc;
}

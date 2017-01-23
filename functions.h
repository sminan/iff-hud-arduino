//Own functions

#include <TFT_ILI93XX.h>
#include <mcp_can.h>
#include <mcp_can_dfs.h>


#define BUFSIZE 2

void receive_data(uint8_t * buf_rx, MCP_CAN CAN)
{
  unsigned char len = 0;
  uint8_t buf[BUFSIZE];

  CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf

  unsigned char canId = CAN.getCanId();

  if ( canId == 0x0A )
  {
    buf_rx[0] = buf[0];
    buf_rx[1] = buf[1];
  }
}



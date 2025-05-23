#ifndef INC_FX_RECEIVE_H_
#define INC_FX_RECEIVE_H_

//****************************************************************************
// Include(s)
//****************************************************************************

#include "main.h"

//****************************************************************************
// Definition(s):
//****************************************************************************

//****************************************************************************
// Public Function Prototype(s):
//****************************************************************************

uint8_t fx_register_rx_cmd_handlers(void);
uint8_t fx_receive(uint8_t *send_reply, uint8_t *reply_cmd);

uint8_t fx_rx_cmd_demo(uint8_t cmd_6bits, ReadWrite rw, uint8_t *buf,
		uint8_t len);
uint8_t fx_rx_cmd_stress_test(uint8_t cmd_6bits, ReadWrite rw, uint8_t *buf,
		uint8_t len);

//****************************************************************************
// Shared variable(s)
//****************************************************************************

#endif //INC_FX_RECEIVE_H_

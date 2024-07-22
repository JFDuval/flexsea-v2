/****************************************************************************
 [Project] FlexSEA: Flexible & Scalable Electronics Architecture v2
 Copyright (C) 2024 JFDuval Engineering LLC

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************
 [Lead developer] Jean-Francois (JF) Duval, jfduval at jfduvaleng dot com.
 [Origin] Based on Jean-Francois Duval's work at the MIT Media Lab
 Biomechatronics research group <http://biomech.media.mit.edu/> (2013-2015)
 [Contributors to v1] Work maintained and expended by Dephy, Inc. (2015-20xx)
 [v2.0] Complete re-write based on the original idea. (2024)
 *****************************************************************************
 [This file] flexsea: top-level FlexSEA protocol v2.0
 ****************************************************************************/

#ifndef INC_FX_FLEXSEA_H
#define INC_FX_FLEXSEA_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************************************
// Include(s)
//****************************************************************************

#include "circ_buf.h"
#include <flexsea_codec.h>
#include <flexsea_command.h>

//****************************************************************************
// Definition(s):
//****************************************************************************

//****************************************************************************
// Public Function Prototype(s):
//****************************************************************************

uint8_t fx_create_bytestream_from_cmd(uint8_t cmd_6bits, ReadWrite rw,
		uint8_t *buf_in, uint8_t buf_in_len, uint8_t* bytestream,
		uint8_t *bytestream_len);
uint8_t fx_get_cmd_handler_from_bytestream(circ_buf_t *cb,
		uint8_t *cmd_6bits, ReadWrite *rw, uint8_t *buf,
		uint8_t *buf_len);

//****************************************************************************
// Structure(s):
//****************************************************************************

//****************************************************************************
// Shared variable(s)
//****************************************************************************

#ifdef __cplusplus
}
#endif

#endif	//INC_FX_FLEXSEA_H

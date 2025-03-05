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

#ifndef INC_FX_FLEXSEA_TOOLS_H
#define INC_FX_FLEXSEA_TOOLS_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************************************
// Include(s)
//****************************************************************************

#include <stdint.h>

//****************************************************************************
// Definition(s):
//****************************************************************************

//****************************************************************************
// Public Function Prototype(s):
//****************************************************************************

void SPLIT_16(uint16_t var, uint8_t *buf, uint16_t *index);
uint16_t REBUILD_UINT16(uint8_t *buf, uint16_t *index);
void SPLIT_32(uint32_t var, uint8_t *buf, uint16_t *index);
uint32_t REBUILD_UINT32(uint8_t *buf, uint16_t *index);
uint32_t REBUILD_UINT32_LE(uint8_t *buf, uint16_t *index);

//****************************************************************************
// Structure(s):
//****************************************************************************

//****************************************************************************
// Shared variable(s)
//****************************************************************************

#ifdef __cplusplus
}
#endif

#endif	//INC_FX_FLEXSEA_TOOLS_H

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

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************************************
// Include(s)
//****************************************************************************

#include <flexsea_tools.h>

//****************************************************************************
// Variable(s)
//****************************************************************************

//****************************************************************************
// Private Function Prototype(s):
//****************************************************************************

//****************************************************************************
// Public Function(s)
//****************************************************************************

//Important note: these functions are not compatible with FlexSEA v1's
//We are now using Little endian to simplify the data exchange with our scripts

//Splits 1 uint16 in 2 bytes, stores them in buf[index] and increments index
inline void SPLIT_16(uint16_t var, uint8_t *buf, uint16_t *index)
{
	buf[*index] = (uint8_t) (var & 0xFF);
	buf[(*index)+1] = (uint8_t) ((var >> 8) & 0xFF);
	(*index) += 2;
}

//Inverse of SPLIT_16()
uint16_t REBUILD_UINT16(uint8_t *buf, uint16_t *index)
{
	uint16_t tmp = 0;
	tmp = (((uint16_t)buf[(*index)+1] << 8) + ((uint16_t)buf[(*index)+0] ));
	(*index) += 2;
	return tmp;
}

//Splits 1 uint32 in 4 bytes, stores them in buf[index] and increments index
inline void SPLIT_32(uint32_t var, uint8_t *buf, uint16_t *index)
{
	buf[(*index)] = (uint8_t) (var & 0xFF);
	buf[(*index)+1] = (uint8_t) ((var >> 8) & 0xFF);
	buf[(*index)+2] = (uint8_t) ((var >> 16) & 0xFF);
	buf[(*index)+3] = (uint8_t) ((var >> 24) & 0xFF);
	(*index) += 4;
}

//Inverse of SPLIT_32()
uint32_t REBUILD_UINT32(uint8_t *buf, uint16_t *index)
{
	uint32_t tmp = 0;
	tmp = (((uint32_t)buf[(*index)+3] << 24) + ((uint32_t)buf[(*index)+2] << 16) \
			+ ((uint32_t)buf[(*index)+1] << 8) + ((uint32_t)buf[(*index)+0]));
	(*index) += 4;
	return tmp;
}

//Splits 1 float in 4 bytes, stores them in buf[index] and increments index
inline void SPLIT_FLOAT(float var, uint8_t *buf, uint16_t *index)
{
	uint32_t tmp = *((uint32_t *) &var);
	buf[(*index)] = (uint8_t) (tmp & 0xFF);
	buf[(*index)+1] = (uint8_t) ((tmp >> 8) & 0xFF);
	buf[(*index)+2] = (uint8_t) ((tmp >> 16) & 0xFF);
	buf[(*index)+3] = (uint8_t) ((tmp >> 24) & 0xFF);
	(*index) += 4;
}

//Reconstruct float
float REBUILD_FLOAT(uint8_t *buf, uint16_t *index)
{
	uint32_t tmp = 0;
	tmp = (((uint32_t)buf[(*index)+3] << 24) + ((uint32_t)buf[(*index)+2] << 16) \
			+ ((uint32_t)buf[(*index)+1] << 8) + ((uint32_t)buf[(*index)+0]));
	(*index) += 4;
	//This uses the 4 bytes to reconstruct. A simple (float) cast doesn't.
	return *((float *) &tmp);
}

#ifdef __cplusplus
}
#endif

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
 [Lead developer] Jean-Francois (JF) Duval, jf at jfduvaleng dot com.
 [Origin] Based on Jean-Francois Duval's work at the MIT Media Lab
 Biomechatronics research group <http://biomech.media.mit.edu/> (2013-2015)
 [Contributors to v1] Work maintained and expended by Dephy, Inc. (2015-20xx)
 [v2.0] Complete re-write based on the original idea. (2024)
 *****************************************************************************
 [This file] flexsea_codec: Data-Link layer of the FlexSEA protocol v2.0
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************************************
// Include(s)
//****************************************************************************

#include "flexsea.h"
#include <flexsea_comm.h>

//****************************************************************************
// Variable(s)
//****************************************************************************

//****************************************************************************
// Private Function Prototype(s):
//****************************************************************************

//****************************************************************************
// Public Function(s)
//****************************************************************************

//We write to the Circular Buffer here, when new data has been received.
void fx_comm_process_ping_pong_buffers(CommPort *cp)
{
	//We read from the buffer that's not selected for a Write
	if(cp->dbuf_selected == 1)
	{
		//Pong is lined up for the next write => read Ping
		if(cp->dbuf_ping_len > 0 && !cp->dbuf_lock_ping)
		{
			for(int i = 0; i < cp->dbuf_ping_len; i++)
			{
				circ_buf_write_byte(cp->cb, cp->dbuf_ping[i]);
			}
			cp->dbuf_ping_len = 0;
		}
	}
	else
	{
		//Ping is lined up for the next write => read Pong
		if(cp->dbuf_pong_len > 0 && !cp->dbuf_lock_pong)
		{
			for(int i = 0; i < cp->dbuf_pong_len; i++)
			{
				circ_buf_write_byte(cp->cb, cp->dbuf_pong[i]);
			}
			cp->dbuf_pong_len = 0;
		}
	}
}

#ifdef __cplusplus
}
#endif

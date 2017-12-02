/*
    ---------------------------------------------------------------------
    sjpcm_rpc.c - SjPCM EE-side code. (c) Nick Van Veen (aka Sjeep), 2002
	---------------------------------------------------------------------

    This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "sjpcm.h"

static unsigned sbuff[64] __attribute__((aligned (64)));
static struct t_SifRpcClientData cd0;

int sjpcm_inited = 0;
int pcmbufl, pcmbufr;
int bufpos;

void SjPCM_Puts(char *format, ...)
{
	static char buff[4096];
    va_list args;
    int rv;

	if(!sjpcm_inited) return;

    va_start(args, format);
    rv = vsnprintf(buff, 4096, format, args);

	memcpy((char*)(&sbuff[0]),buff,252);
	SifCallRpc(&cd0,SJPCM_PUTS,0,(void*)(&sbuff[0]),252,(void*)(&sbuff[0]),252,0,0);
}

void SjPCM_Play()
{
	if(!sjpcm_inited) return;

	SifCallRpc(&cd0,SJPCM_PLAY,0,(void*)(&sbuff[0]),0,(void*)(&sbuff[0]),0,0,0);
}

void SjPCM_Pause()
{
	if(!sjpcm_inited) return;

	SifCallRpc(&cd0,SJPCM_PAUSE,0,(void*)(&sbuff[0]),0,(void*)(&sbuff[0]),0,0,0);
}

void SjPCM_Setvol(unsigned int volume)
{
	if(!sjpcm_inited) return;

	sbuff[5] = volume&0x3fff;
	SifCallRpc(&cd0,SJPCM_SETVOL,0,(void*)(&sbuff[0]),64,(void*)(&sbuff[0]),64,0,0);
}

void SjPCM_Clearbuff()
{
	if(!sjpcm_inited) return;

	SifCallRpc(&cd0,SJPCM_CLEARBUFF,0,(void*)(&sbuff[0]),0,(void*)(&sbuff[0]),0,0,0);
}

int SjPCM_Init(int sync)
{
	int i;
/*
	do {
        if (sif_bind_rpc(&cd0, SJPCM_IRX, 0) < 0) {
            return -1;
        }
        nopdelay();
    } while(!cd0.server);
*/
	while(1){
		if (SifBindRpc( &cd0, SJPCM_IRX, 0) < 0) return -1; // bind error
 		if (cd0.server != 0) break;
    	i = 0x10000;
    	while(i--);
	}

	sbuff[0] = sync;

	SifCallRpc(&cd0,SJPCM_INIT,0,(void*)(&sbuff[0]),64,(void*)(&sbuff[0]),64,0,0);

	FlushCache(0);

	pcmbufl = sbuff[1];
	pcmbufr = sbuff[2];
	bufpos = sbuff[3];

	sjpcm_inited = 1;

	return 0;
}

// size should either be either 800 (NTSC) or 960 (PAL)
void SjPCM_Enqueue(short *left, short *right, int size, int wait)
{
    int i;
    struct t_SifDmaTransfer sdt;

    if (!sjpcm_inited) return;

    sdt.src = (void *)left;
    sdt.dest = (void *)(pcmbufl + bufpos);
    sdt.size = size*2;
    sdt.attr = 0;

	FlushCache(0);

    i = SifSetDma(&sdt, 1); // start dma transfer
    while ((wait != 0) && (SifDmaStat(i) >= 0)); // wait for completion of dma transfer

    sdt.src = (void *)right;
    sdt.dest = (void *)(pcmbufr + bufpos);
    sdt.size = size*2;
    sdt.attr = 0;

	FlushCache(0);

    i = SifSetDma(&sdt, 1);
    while ((wait != 0) && (SifDmaStat(i) >= 0));

	sbuff[0] = size;
	SifCallRpc(&cd0,SJPCM_ENQUEUE,0,(void*)(&sbuff[0]),64,(void*)(&sbuff[0]),64,0,0);
	bufpos = sbuff[3];

}

int SjPCM_Available()
{
  if (!sjpcm_inited) return -1;
  SifCallRpc(&cd0,SJPCM_GETAVAIL,0,(void*)(&sbuff[0]),64,(void*)(&sbuff[0]),64,0,0);
  return sbuff[3];
}

int SjPCM_Buffered()
{
  if (!sjpcm_inited) return -1;
  SifCallRpc(&cd0,SJPCM_GETBUFFD,0,(void*)(&sbuff[0]),64,(void*)(&sbuff[0]),64,0,0);
  return sbuff[3];
}

void SjPCM_Quit()
{
	if(!sjpcm_inited) return;

	SifCallRpc(&cd0,SJPCM_QUIT,0,(void*)(&sbuff[0]),0,(void*)(&sbuff[0]),0,0,0);
	sjpcm_inited = 0;
}

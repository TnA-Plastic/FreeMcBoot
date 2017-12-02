/*      
  _____     ___ ____ 
   ____|   |    ____|      PS2 Open Source Project
  |     ___|   |____       
  
---------------------------------------------------------------------------

    Copyright (C) 2008 - Neme & jimmikaelkael (www.psx-scene.com) 

    This program is free software; you can redistribute it and/or modify
    it under the terms of the Free McBoot License.
    
	This program and any related documentation is provided "as is"
	WITHOUT ANY WARRANTIES, either express or implied, including, but not
 	limited to, implied warranties of fitness for a particular purpose. The
 	entire risk arising out of use or performance of the software remains
 	with you.
   	In no event shall the author be liable for any damages whatsoever
 	(including, without limitation, damages to your hardware or equipment,
 	environmental damage, loss of health, or any kind of pecuniary loss)
 	arising out of the use of or inability to use this software or
 	documentation, even if the author has been advised of the possibility of
 	such damages.

    You should have received a copy of the Free McBoot License along with
    this program; if not, please report at psx-scene :
    http://psx-scene.com/forums/freevast/

---------------------------------------------------------------------------

mcid.c

Handles file decryption and file signing 

Decrypt_Card_File & Sign_Encrypted_Disk_File uses mcsio2 module, and
XSIO2MAN, XMCMAN, XMCSERV that must be loaded.

main funcs :
int   mcsio2_rpc_Init(void); 						: Inits mcsio2 module rpc (prior to call functions below)
int   Card_Auth(int port);							: Authentificate a Memory Card by Mechacon
void *Decrypt_Card_File(int port, void *buf);  		: decrypt an encrypted file for memory card.
void *Decrypt_Disk_File(void *buf);					: decrypt an encrypted file for disk or rom.	
void *Sign_Encrypted_Disk_File(int port, void *buf);: sign an encrypted (that's the sony trick) file from disk or rom to memory card.
*/
 
#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <stdio.h>
#include <libcdvd.h>
#include <string.h>
//#include <sysclib.h>
#include <malloc.h>

int   mcsio2_rpc_Init(void);
int   Card_Auth(int port);
void *Decrypt_Card_File(int port, void *buf);
void *Decrypt_Disk_File(void *buf);
void *Sign_Encrypted_Disk_File(int port, void *buf);


// Encrypted file Data Block struct
typedef struct {
	u32 size;               // Size of data block 
	u32 flags;				// Flags : 3 -> block to decrypt, 0 -> block to take in input file, 2 -> block to prepare.
    u8  checksum[8];
} Block_Data_t;

// Encrypted file bit table struct 
typedef struct {
	u32 headersize;          // header size (same as mg_ELF_Header_t->mg_header_size)
	u8 block_count;          // Number of encrypted blocks
	u8 pad1;
	u8 pad2;
	u8 pad3;
	Block_Data_t blocks[63]; // Block description, see Block_Data_t
} Bit_Data_t;

// Encrypted file header struct 
typedef struct {
	u32 unknown1;             
    u32 unknown2;
    u16 unknown3_half;
    u16 version;
    u32 unknown4;
	u32 ELF_size;			 // Size of data blocks = Decrypted elf size
	u16 mg_header_size;      // header size
	u16 unknown5;
	u16 flags;               // ? for header purpose
	u16 BIT_count;           // ?  
	u32 mg_zones;
} mg_ELF_Header_t;



// sio2 packet structs
 
struct _sio2_dma_arg { // size 12
	void	*addr;
	int	size;
	int	count; 
};

typedef struct {
	u32	stat6c;
	u32	port_ctrl1[4];
	u32	port_ctrl2[4]; 
	u32	stat70;
	u32	regdata[16]; 
	u32	stat74; 
	u32	in_size; 
	u32	out_size; 
	u8	*in;
	u8	*out; 
	struct _sio2_dma_arg in_dma; 
	struct _sio2_dma_arg out_dma;
} Sio2Packet;
 

// rpc packets structs
 
 typedef struct {
	u32    ret; 
	u16	   port;	
 	u32    stat6c;
 	u32    port_ctrl1[4];
 	u32    port_ctrl2[4];
 	u32    stat70;   
 	u32    regdata[16];
 	u32    stat74;   
 	u32    in_size; 
 	u32    out_size; 
 	u8     in[14]; 
 	u8     out[14]; 
 } Rpc_Packet_Send_SIO2;

typedef struct {
	int ret;
    u16 port;
    u16 slot;
    u16 a2;    
} Rpc_Packet_Send_CARDAUTH;	


int mcman_getcnum (int port, int slot);

 
#define MCSIO2_IRX 0x0E0E0E0

static SifRpcClientData_t mcsio2    __attribute__((aligned(64)));
static int Rpc_Buffer[1024] __attribute__((aligned(64)));

int MCSIO2_Inited  = 0;

//--------------------------------------------------------------
int mcsio2BindRpc(void) {
	int ret;
	int retryCount = 0x1000;

	while(retryCount--) {
	        ret = SifBindRpc( &mcsio2, MCSIO2_IRX, 0);
        	if ( ret  < 0)  {
			  	printf("mcsio2: EE Bind RPC Error.\n");
	          	return -1;
	        }
	        if (mcsio2.server != 0){
	        	printf("mcsio2: EE Bind RPC Set.\n");
	        	break;
	        }

	        // short delay 
	      	ret = 0x10000;
	    	while(ret--) asm("nop\nnop\nnop\nnop");
	}

	MCSIO2_Inited = 1;
	return retryCount;
}
//--------------------------------------------------------------
int mcsio2_rpc_Init(void)
{
 	mcsio2BindRpc();

 	if(!MCSIO2_Inited)
 	{
 		printf("\tmcsio2 RPC Init failed\n");
 		return -1;
	}
	
 	return 1;
}
//--------------------------------------------------------------
int mcsio2_transfer(int port, Sio2Packet *sio2data)
{
	// Perform sio2data transfer
	
	int i;
	Rpc_Packet_Send_SIO2 *Packet = (Rpc_Packet_Send_SIO2 *)Rpc_Buffer;

 	if(!MCSIO2_Inited) {
	 	if (!mcsio2_rpc_Init())
	 		return -1;
 	}
		
 	Packet->port = port;
 	
	Packet->stat6c = sio2data->stat6c;
	for(i = 0; i < 4; i++) {
 		Packet->port_ctrl1[i] = sio2data->port_ctrl1[i];
 		Packet->port_ctrl2[i] = sio2data->port_ctrl2[i];
	}
 	Packet->stat70 = sio2data->stat70;   
	for(i = 0; i < 16; i++) 
		Packet->regdata[i] = sio2data->regdata[i];
 	Packet->stat74 = sio2data->stat74;   
 	Packet->in_size = sio2data->in_size; 
 	Packet->out_size = sio2data->out_size; 
 	memcpy(Packet->in, sio2data->in, sio2data->in_size);
 	memcpy(Packet->out, sio2data->out, sio2data->out_size); 	
 	
 
   	if ((SifCallRpc(&mcsio2, 1, 0, (void*)Rpc_Buffer, sizeof(Rpc_Packet_Send_SIO2), (void*)Rpc_Buffer, sizeof(Rpc_Packet_Send_SIO2),0,0)) < 0) {
		printf("\tmcsio2_transfer: RPC error\n");
	    return -1;
    }

	sio2data->stat6c = Packet->stat6c;
	for(i = 0; i < 4; i++) {
 		sio2data->port_ctrl1[i] = Packet->port_ctrl1[i];
 		sio2data->port_ctrl2[i] = Packet->port_ctrl2[i];
	}
 	sio2data->stat70 = Packet->stat70;   
	for(i = 0; i < 16; i++) 
		sio2data->regdata[i] = Packet->regdata[i];
 	sio2data->stat74 = Packet->stat74;   
 	sio2data->in_size = Packet->in_size; 
 	sio2data->out_size = Packet->out_size; 
 	memcpy(sio2data->in, Packet->in, Packet->in_size);
 	memcpy(sio2data->out, Packet->out, Packet->out_size); 	
 	
	
 	return Packet->ret;
}

//--------------------------------------------------------------

int Card_Auth(int port)
{
	// Authentificate a Memory Card by Mechacon
	
 	Rpc_Packet_Send_CARDAUTH *Packet = (Rpc_Packet_Send_CARDAUTH *)Rpc_Buffer;

 	if(!MCSIO2_Inited) {
	 	if (!mcsio2_rpc_Init())
	 		return -1;
 	}
 	
  	Packet->port = port;
 	Packet->slot = 0; 
	Packet->a2 = mcman_getcnum (Packet->port, Packet->slot);  	
 
   	if ((SifCallRpc(&mcsio2, 2, 0, (void*)Rpc_Buffer, sizeof(Rpc_Packet_Send_CARDAUTH), (void*)Rpc_Buffer, sizeof(int),0,0)) < 0) {
		printf("\tCard_Auth: RPC error\n");
	    return -1;
    }

 	return Packet->ret;
}

//_______________________________________________________________________________

 int mcman_getcnum (int port, int slot)
 {
	 // Get Memory Card num like MCMAN does
	 
	 return (port & 1) << 3;
 }
  
 //---------------------------------------------------------------------------------------------------------------

int mcsio2_buf_checksum(u8 *buf, int size)
 {
	 // Calculate checksum of a sio2data transfer buf
	 
	 int i;
	 int checksum = 0;
	 
	 if (size <= 0) return checksum & 0xFF; 
  	 for (i = 0; i < size; i++) checksum ^= buf[i];
	 return checksum & 0xFF; 
 }

//_______________________________________________________________________________
 
int mcsio2_MagicGate_Cmd_2(int port, int slot, u8 *buf, int a3, int cmd)
 {
	 // Send some MagicGate commands, used to generate mcid
	 
	 int i;
	 u8 sio2buf[148];
	 u8 wrbuf[14];
	 u8 rdbuf[14];
	 
     Sio2Packet *sio2packet = (Sio2Packet *) sio2buf;	
	 sio2packet->in = (u8 *)&wrbuf;
	 sio2packet->out = (u8 *)&rdbuf;	 	 
 	 for (i=0; i < 4; i++) {
	 	 sio2packet->port_ctrl1[i] = 0;
	 	 sio2packet->port_ctrl2[i] = 0;	 	 
 	 }
 	 sio2packet->port_ctrl1[port] = 0xFF060505;
	 sio2packet->port_ctrl2[port] = 0x0003FFFF;
 	 sio2packet->regdata[0] = (port & 3) | 0x00380E40;
 	 sio2packet->regdata[1] = 0;
	 sio2packet->out_size = 14;
	 sio2packet->in_size = 14;
	 memset(wrbuf, 0, 14);
	 wrbuf[0] = 0x81;
	 wrbuf[1] = (u8)a3;
	 wrbuf[2] = (u8)cmd;
  	 for (i = 0; i < 8; i++) wrbuf[3 + i] = buf[7 - i];
	 wrbuf[11] = (u8)mcsio2_buf_checksum(buf, 8);
	 if (!(mcsio2_transfer(port, (Sio2Packet *)sio2packet))) {
		 printf("mcsio2 transfer error %d\n", cmd & 0xFF);
		 return 0;
	 }
	 if (((sio2packet->stat6c >> 13) & 1) || ((sio2packet->stat6c >> 14) & 3)) {
		 printf("sio2 command error %d\n", cmd & 0xFF);
		 return 0;
	 }
	 if (rdbuf[12] != 0x2B) {
		 printf("ID error %d\n", cmd & 0xFF);
		 return 0;
	 }
	 if (rdbuf[13] == 0x66) {
		 printf("result failed %d\n", cmd & 0xFF);
		 return 0;
	 }
	 return 1;
 }
 
//---------------------------------------------------------------------------------------------------------------
 
int mcsio2_MagicGate_Cmd_1(int port, int slot, u8 *buf, int a3, int cmd)
 {
	 // Send some MagicGate commands, used to generate mcid
	 	 
	 int i;
	 u8 sio2buf[148];
	 u8 wrbuf[14];
	 u8 rdbuf[14];
	 	 
     Sio2Packet *sio2packet = (Sio2Packet *) sio2buf;	
	 sio2packet->in = (u8 *)&wrbuf;
	 sio2packet->out = (u8 *)&rdbuf;	 	 
 	 for (i = 0; i < 4; i++) {
	 	 sio2packet->port_ctrl1[i] = 0;
	 	 sio2packet->port_ctrl2[i] = 0;	 	 
 	 }
 	 sio2packet->port_ctrl1[port] = 0xFF060505;
	 sio2packet->port_ctrl2[port] = 0x0003FFFF;
 	 sio2packet->regdata[0] = (port & 3) | 0x00380E40;
 	 sio2packet->regdata[1] = 0;
	 sio2packet->out_size = 14;
	 sio2packet->in_size = 14;
	 memset(wrbuf, 0, 14);
	 wrbuf[0] = 0x81;
	 wrbuf[1] = (u8)a3;
	 wrbuf[2] = (u8)cmd;
	 if (!(mcsio2_transfer(port, (Sio2Packet *)sio2packet))) {
		 printf("mcsio2 transfer error %d\n", cmd & 0xFF);
		 return 0;
	 }
	 if (((sio2packet->stat6c >> 13) & 1) || ((sio2packet->stat6c >> 14) & 3)) {
		 printf("sio2 command error %d\n", cmd & 0xFF);
		 return 0;
	 }
	 if (rdbuf[3] != 0x2B) {
		 printf("ID error %d\n", cmd & 0xFF);
		 return 0;
	 }
	 if (rdbuf[13] == 0x66) {
		 printf("result failed %d\n", cmd & 0xFF);
		 return 0;
	 }
	 if (((u8)mcsio2_buf_checksum(rdbuf + 4, 8) & 0xFF) != rdbuf[12]) {
		 printf("mcsio2 checksum error %d\n", cmd & 0xFF);
		 return 0;
	 }
  	 for (i = 0; i < 8; i++) 
  	 	buf[7 - i] = rdbuf[4 + i];
	 return 1; 
 }

 
//---------------------------------------------------------------------------------------------------------------
 
int mcsio2_MagicGate_Cmd_0(int port, int slot, int a2, int cmd)
 {
	 // Send some MagicGate commands, used to generate mcid
	 	 
	 int i;
	 u8 sio2buf[148];
	 u8 wrbuf[5];
	 u8 rdbuf[5];
	 	 
     Sio2Packet *sio2packet = (Sio2Packet *) sio2buf;	
	 sio2packet->in = (u8 *)&wrbuf;
	 sio2packet->out = (u8 *)&rdbuf;	 	 
 	 for (i=0; i < 4; i++) {
	 	 sio2packet->port_ctrl1[i] = 0;
	 	 sio2packet->port_ctrl2[i] = 0;	 	 
 	 }
 	 sio2packet->port_ctrl1[port] = 0xFF060505;
	 sio2packet->port_ctrl2[port] = 0x0003FFFF;
 	 sio2packet->regdata[0] = (port & 3) | 0x00140540;
 	 sio2packet->regdata[1] = 0;
	 sio2packet->out_size = 5;
	 sio2packet->in_size = 5;
	 memset(wrbuf, 0, 5);
	 wrbuf[0] = 0x81;
	 wrbuf[1] = (u8)a2;
	 wrbuf[2] = (u8)cmd;
	 if (!(mcsio2_transfer(port, (Sio2Packet *)sio2packet))) {
		 printf("mcsio2 transfer error %d\n", cmd & 0xFF);
		 return 0;
	 }
	 if (((sio2packet->stat6c >> 13) & 1) || ((sio2packet->stat6c >> 14) & 3)) {
		 printf("sio2 command error %d\n", cmd & 0xFF);
		 return 0;
	 }
	 if (rdbuf[3] != 0x2B) {
		 printf("ID error %d\n", cmd & 0xFF);
		 return 0;
	 }
	 if (rdbuf[4] == 0x66) {
		 printf("result failed %d\n", cmd & 0xFF);
		 return 0;
	 }
	 return 1; 
 }
//_______________________________________________________________________________

 int mecha_command(int cmd, u8 *argp, int arglen, u8 *buf)
 {
	 // Send S-command to the Mechacon
	 
	 return cdApplySCmd(cmd, argp, arglen, buf, 16);	
 }
 
//---------------------------------------------------------------------------------------------------------------  

 int Mecha_write_data(void *data, int size)
 {
	 // Send some datas to the Mechacon
	 
	 u8 ret[1];
	 u8 mecha_param[16];	 
	 
	 if (size > 16) 
	 	return 0;
	 	
	 if (size > 0) 
	 	memcpy(mecha_param, data, size);
	 	
	 if (!(mecha_command(0x8D, mecha_param, size, ret)))
	 	return 0;
	 	
	 if (!((int)ret[0]))
	 	return 1;
	 	
	 return 0;
 }
   
 //---------------------------------------------------------------------------------------------------------------
 
 int Mecha_read_data(void *data, int size)
 {
	 // Get back datas from Mechacon
	 
	 u8 ret[16];
	 
	 if (size > 16) 
	 	return 0;
	 	
	 if (!(mecha_command(0x8E, NULL, 0, ret)))
	 	return 0;
	 	
	 if (size <= 0)
	 	return 1;
	 	
	 memcpy(data, ret, size);
	 
	 return 1;
 }
 
 //---------------------------------------------------------------------------------------------------------------
 
 int Mecha_pol_cal_cmplt(void)
 {
	 // Send the command that tells to Mechacon to decrypt datas received
	 
	 u8 ret[1];
	 
	 do {
		 if (!(mecha_command(0x8F, NULL, 0, ret)))
		 	return 0;
	 }
	 while ((int)ret[0]);
	 
	 if (!((int)ret[0]))
	 	return 1;
	 	
	 return 0;
 }
 
 //--------------------------------------------------------------------------------------------------------------- 
 
 int  Mecha_ReadKbit1(void* kbit_part1)
 {
	 // Send the Mechacon command that generate first half (8 bytes) of kbit
	 
	 u8 ret[9];	 
	 
	 if (!(mecha_command(0x94, NULL, 0, ret)))
	 	return 0;
	 	
 	 if ((int)ret[0]!= 0)
 	 	return 0;
 	 	
 	 memcpy(kbit_part1, ret + 1, 8);
 	 
	 return 1; 	 
 }

 //--------------------------------------------------------------------------------------------------------------- 
 
 int  Mecha_ReadKbit2(void* kbit_part2)
 {
	 // Send the Mechacon command that generate second half (8 bytes) of kbit
	 	 
	 u8 ret[9];	 
	 
	 if (!(mecha_command(0x95, NULL, 0, ret)))
	 	return 0;
	 	
 	 if ((int)ret[0]!= 0)
 	 	return 0;
 	 	
 	 memcpy(kbit_part2, ret + 1, 8);
 	 
	 return 1; 	 
 }

 //--------------------------------------------------------------------------------------------------------------- 
 
 int  Mecha_ReadKc1(void* kc_part1)
 {
	 // Send the Mechacon command that generate first half (8 bytes) of kc
	 	 
	 u8 ret[9];	 
	 
	 if (!(mecha_command(0x96, NULL, 0, ret)))
	 	return 0;
	 	
 	 if (((int)ret[0])!= 0)
 	 	return 0;
 	 	
 	 memcpy(kc_part1, ret + 1, 8);
 	 
	 return 1; 	 
 }
 //--------------------------------------------------------------------------------------------------------------- 
 
 int  Mecha_ReadKc2(void* kc_part2)
 {
	 // Send the Mechacon command that generate second half (8 bytes) of kc
	 	 
	 u8 ret[9];	 
	 
	 if (!(mecha_command(0x97, NULL, 0, ret)))
	 	return 0;
	 	
 	 if (((int)ret[0])!= 0)
 	 	return 0;
 	 	
 	 memcpy(kc_part2, ret + 1, 8);
 	 
	 return 1; 	 
 }
 
 //--------------------------------------------------------------------------------------------------------------- 
 
 int  Mecha_ReadICVPS2(void* icvps2)
 {
	 // Send the Mechacon command that generate ICVPS2 (8 bytes), this is never used in encrypted we knows...
	 
	 u8 ret[9];	 
	 
	 if (!(mecha_command(0x98, NULL, 0, ret)))
	 	return 0;
	 	
 	 if (((int)ret[0])!= 0)
 	 	return 0;
 	 	
 	 memcpy(icvps2, ret + 1, 8);
 	 
	 return 1; 	 
 }

 //---------------------------------------------------------------------------------------------------------------

 int Mecha_write_HD_start(int hcode, int cnum, int a2, int hlength)
 {
	 // Send Mechacon command that inits it to receive encrypted file header 
	 
	 u8 ret[1];	 
	 u8 param_buf[5];
   	 
	 param_buf[0] = (u8)hcode;
	 param_buf[1] = (u8)hlength;
	 param_buf[2] = (u8)(hlength>>8);
	 param_buf[3] = (u8)cnum;	 	 	 
	 param_buf[4] = (u8)a2;
	 
	 if (!(mecha_command(0x90, param_buf, 5, ret)))
	 	return 0;
	 	
	 if (!((int)ret[0]))
	 	return 1;
	 	
	 return 0;
 }
 
 //---------------------------------------------------------------------------------------------------------------

 int Mecha_ReadBITLength(int *bit_size)
 {
	 // Send Mechacon command that inits it to get back decrypted BIT table
	 	 
	 u8 ret[3];
	  
	 if (!(mecha_command(0x91, NULL, 0, ret)))
	 	return 0;
	 	
	 if (((int)ret[0])!= 0)
	 	return 0;
	 	
	 *bit_size = (int)ret[1] + (int)(ret[2]<<8);
	 
	 return 1;
 }
 
 //---------------------------------------------------------------------------------------------------------------

 int Mecha_WriteDataInLength(int size)
 {
 	 // Send Mechacon command that inits it to receive some datas
 	 
	 u8 ret[1];	 
	 u8 param_buf[2];
	 
 	 param_buf[0] = (u8)size;
	 param_buf[1] = (u8)(size>>8);
	 
	 if (!(mecha_command(0x92, param_buf, 2, ret)))
	 	return 0;
	 	
	 if (!((int)ret[0]))
	 	return 1;
	 	
	 return 0;
 }

 //---------------------------------------------------------------------------------------------------------------

 int Mecha_WriteDataOutLength(int size)
 {
 	 // Send Mechacon command that inits it to get back some datas
 	 	 
	 u8 ret[1];	 
	 u8 param_buf[2];
	 
 	 param_buf[0] = (u8)size;
	 param_buf[1] = (u8)(size>>8);
	 
	 if (!(mecha_command(0x93, param_buf, 2, ret)))
	 	return 0;
	 	
	 if (!((int)ret[0]))
	 	return 1;
	 	
	 return 0;	 
 }
  
//---------------------------------------------------------------------------------------------------------------

 int Mecha_ReadBIT(void* bit)
 {
	// Get back full decrypted BIT table from Mechacon
	
	int offset;
	int bit_length, bitlength;
	int *p_bitlength = &bit_length;
	
	if (!(Mecha_ReadBITLength(p_bitlength))) // send mecha command 0x91
		return 0;
		
	bitlength = bit_length;
	
	if ((bit_length) > 0) {
		
 		offset = 0;
 		do {	 	 
 	 		if ((bit_length & 0xFFFF) < 16)	{
	 			if (!(Mecha_read_data(bit + offset, bit_length & 0xFFFF))) // send mecha command 0x8E
	 				return 0;
	 			bit_length = 0;		 	 
 	 		}
 	 		else {
	 	 		if (!(Mecha_read_data(bit + offset, 16))) // send mecha command 0x8E
	 	 			return 0; 
	 	 		offset += 16;
	 	 		bit_length -= 16;
 	 		}
 		} while ((bit_length & 0xFFFF) > 0);
	}
	
	return bitlength;
 } 
 
 //---------------------------------------------------------------------------------------------------------------
 
 int Mecha_ReadBLOCK(void* buf, int size)
 {
	// Get back full decrypted data block from Mechacon
		 
	int offset;
	 
	if (!(Mecha_WriteDataOutLength(size & 0xFFFF))) // send mecha command 0x93
		return 0;
		
	if (size <= 0)
		return 1;
		
 	offset = 0;
 	do {	 	 
 	 	if (size < 16) {
	 		if (!(Mecha_read_data(buf + offset, size))) // send mecha command 0x8E
	 			return 0; 
	 		size = 0;		 	 
 	 	}
 	 	else {
	 	 	if (!(Mecha_read_data(buf + offset, 16))) // send mecha command 0x8E
	 	 		return 0;
	 	 	offset += 16;
	 	 	size -= 16;
 	 	}
 	} while (size > 0);
 	
	return 1;
 } 
 
//---------------------------------------------------------------------------------------------------------------

 int Mecha_Set_Header(int hcode, int cnum, int a2, void* buf)
 {
	 // Send encrypted file header to Mechacon, and send command to decrypt it
		 
	 int hlength, offset;
	 int r;
	 
	 hlength = ((mg_ELF_Header_t *) buf)->mg_header_size;
	 printf("header length %d\n", hlength & 0xFFFF);
	 
	 if (!Mecha_write_HD_start(hcode & 0x00FF, cnum, a2, hlength & 0xFFFF)) {  // send mecha command 0x90
		 printf("Mecha_set_header: fail Mecha_write_HD_start\n");
		 return 0;
 	 }
 	 
 	 if ((hlength & 0xFFFF) > 0) { // assume 0 < hlength < 65536
	 	offset = 0;
	 	do {	 	 
	 	 	if ((hlength & 0xFFFF) < 16) {
		 		r = Mecha_write_data(buf + offset, hlength & 0xFFFF);  // send mecha command 0x8D
		 		hlength = 0;		 	 
	 	 	}
	 	 	else {
		 	 	r = Mecha_write_data(buf + offset, 16); // send mecha command 0x8D
		 	 	offset += 16;
		 	 	hlength -= 16;

	 	 	}
	 	 	if (r == 0) {
		 		printf("Mecha_set_header: fail write_data\n");
		 		return 0;
	 	 	}
 	 	} while ((hlength & 0xFFFF) > 0);
 	 }
 	 
	 if (!(Mecha_pol_cal_cmplt())) { // send mecha command 0x8F
		printf("Mecha_set_header: fail pol_cal_cmplt\n");
		return 0;
	 }
	 
	 printf("Mecha_set_header: OK\n");
	 
	 return 1;
	 
 } 

//---------------------------------------------------------------------------------------------------------------
 
 int Mecha_Set_Block(void *buf, int size) 
 {
	 // Send encrypted file data block to Mechacon, and send command to decrypt it
	 	 
	 int offset, r;
	 
	 if (!(Mecha_WriteDataInLength(size & 0xFFFF))) // send mecha command 0x92
	 	return 0;
	 	
 	 if (size > 0) {
	 	offset = 0;
	 	do {	 	 
	  		if (size < 16) {
	 			r = Mecha_write_data(buf + offset, size);  // send mecha command 0x8D
	 			size = 0;		 	 
 	 		}
 	 		else {
	 	 		r = Mecha_write_data(buf + offset, 16); // send mecha command 0x8D
	 	 		offset += 16;
	 	 		size -= 16;
 	 		}
 	 		if (r == 0)
 	 			return 0;
 	 	} while (size > 0);
	 }
	 
	 if (!(Mecha_pol_cal_cmplt())) // send mecha command 0x8F
	 	return 0; 
	 	
	 return 1;
	 	  
 }
 
//---------------------------------------------------------------------------------------------------------------

 int GetKbit(void *kbit) 
 {
	// Get the 16 bytes kbit from Mechacon 
	
	if (!(Mecha_ReadKbit1(kbit)))
		return 0;
		
	if (!(Mecha_ReadKbit2(kbit + 8)))
		return 0;
		
    return 1;	
 }

//---------------------------------------------------------------------------------------------------------------

 int GetKc(void *kc) 
 {
	// Get the 16 bytes kc from Mechacon 
		 
	if (!(Mecha_ReadKc1(kc)))
		return 0;
		
	if (!(Mecha_ReadKc2(kc + 8)))
		return 0;
		
    return 1;	
 }
 
//---------------------------------------------------------------------------------------------------------------

 int GetICVPS2(void *icvps2) 
 {
	// Get the 8 bytes icvps2 from Mechacon 
	 
	if (!(Mecha_ReadICVPS2(icvps2)))
		return 0;
		
    return 1;	
 }
 
//---------------------------------------------------------------------------------------------------------------
 
 int Card_Encrypt(int port, int slot, void *buf) 
 {
	// Perform the MagicGate encryption of a 8 bytes data buffer
	 
	if (!(mcsio2_MagicGate_Cmd_0(port, slot, 0xF2, 0x50)))
		return 0;
		
	if (!(mcsio2_MagicGate_Cmd_2(port, slot, buf, 0xF2, 0x51)))
		return 0;	
		
	if (!(mcsio2_MagicGate_Cmd_0(port, slot, 0xF2, 0x52)))
		return 0;
		
	if (!(mcsio2_MagicGate_Cmd_1(port, slot, buf, 0xF2, 0x53)))
		return 0;	
		
	return 1;
 }
  
//---------------------------------------------------------------------------------------------------------------

 int Card_Decrypt(int port, int slot, void *buf) 
 {
	// Perform the MagicGate decryption of a 8 bytes data buffer
	 
	if (!(mcsio2_MagicGate_Cmd_0(port, slot, 0xF1, 0x40)))
		return 0;
		
	if (!(mcsio2_MagicGate_Cmd_2(port, slot, buf, 0xF1, 0x41)))
		return 0;	
		
	if (!(mcsio2_MagicGate_Cmd_0(port, slot, 0xF1, 0x42)))
		return 0;
		
	if (!(mcsio2_MagicGate_Cmd_1(port, slot, buf, 0xF1, 0x43)))
		return 0;	
		
	return 1;
 }
 
 //---------------------------------------------------------------------------------------------------------------
 
void store_kbit(u8 *inbuf, u8 *kbit)
{	
	// Stores bit Key at right offset in a encrypted Header
	
  	mg_ELF_Header_t *header = (mg_ELF_Header_t *) inbuf;
  	int offset = 0x20;
  	
  	if (header->BIT_count > 0)
  		offset += header->BIT_count * 16;
  		
  	if ((*(u32*)&header->flags) & 1)
  		offset += inbuf[offset] + 1;
  		
  	if (((*(u32*)&header->flags) & 0xf000) == 0)
  		offset += 8;
  		
 	memcpy(inbuf + offset, kbit, 16);
}

//--------------------------------------------------------------

void store_kc(u8 *inbuf, u8 *kc)
{	
	// Stores content Key at right offset in a encrypted Header
				
  	mg_ELF_Header_t *header = (mg_ELF_Header_t *) inbuf;
  	int offset = 0x20;

  	if (header->BIT_count > 0)
  		offset += header->BIT_count * 16;
  		
  	if ((*(u32*)&header->flags) & 1)
  		offset += inbuf[offset] + 1;
  		
  	if (((*(u32*)&header->flags) & 0xf000) == 0)
  		offset += 8;
  		
 	offset += 16;
	memcpy(inbuf + offset, kc, 16);
}

//--------------------------------------------------------------

void store_icvps2(u8 *inbuf, u8 *icvps2)
{
	// Stores ICVPS2 at right offset in a encrypted Header
	
 	mg_ELF_Header_t *header = (mg_ELF_Header_t *) inbuf;
 	int offset = header->mg_header_size - 8;

 	memcpy(inbuf + offset, icvps2, 8);
}

//--------------------------------------------------------------

void read_kbit(u8 *inbuf, u8 *kbit)
{
	// Get kbit from right offset in a encrypted Header
	
  	int offset = 0x20;
  	mg_ELF_Header_t *header = (mg_ELF_Header_t *) inbuf;
  	
  	if (header->BIT_count > 0)
  		offset += header->BIT_count * 16;
  		
  	if ((*(u32*)&header->flags) & 1)
  		offset += inbuf[offset] + 1;
  		
  	if (((*(u32*)&header->flags) & 0xf000) == 0)
  		offset += 8;
  		
	memcpy (kbit, inbuf + offset, 16);
}

//---------------------------------------------------------------------------------------------------------------

void read_kc(u8 *inbuf, u8 *kc)
{
	// Get kc from right offset in a encrypted Header
	
	int offset = 0x20; 
  	mg_ELF_Header_t *header = (mg_ELF_Header_t *) inbuf;
  	
  	if (header->BIT_count > 0)
  		offset += header->BIT_count * 16;
  		
  	if ((*(u32*)&header->flags) & 1)
  		offset += inbuf[offset] + 1;
  		
  	if (((*(u32*)&header->flags) & 0xf000) == 0)
  		offset += 8;
  		
	offset += 16;
	memcpy (kc, inbuf + offset, 16);
}

//---------------------------------------------------------------------------------------------------------------
 
 int Decrypt_Disk_Header (void *buf, void *bit, int *psize)
 {
	// Perform decryption of an encrypted file header from disk or rom
	
    int size;
 	psize= &size; 	    
 	int hcode = 0; // 0 = DiskBoot 1 = CardBoot 2 = Download 
 	int cnum = 0; 
 	
 	if (!Mecha_Set_Header(hcode, cnum, 0, buf)) // send mecha commands 0x90, 0x8D, 0x8F
 		return 0;
 		
 	size = Mecha_ReadBIT(bit); // send mecha commands 0x91, 0x8E	
 	
 	if (!size)
 		return 0; 
 		
 	return 1; 	
 }  
 
//---------------------------------------------------------------------------------------------------------------
 
 int Download_Header (int port, int slot, void *buf, void *bit, int *psize)
 {
	// Perform decryption of an encrypted file header from disk or rom
	// But takes care to prepare some init stuff to get back kbit and kc
		 
	int size;
	psize = &size;
 	int hcode = 2; // 0 = DiskBoot 1 = CardBoot 2 = Download 
 	
 	int cnum = mcman_getcnum(port, slot); 
 	
 	if (cnum < 0)
 		return 0;
 		
	if (!(Mecha_Set_Header(hcode, cnum, 0, buf)))	
		return 0; // send mecha commands 0x90, 0x8D, 0x8F

 	size = Mecha_ReadBIT(bit);
 	if (!size)
	 	return 0; // send mecha commands 0x91, 0x8E	
	 	
 	return 1; 	
 } 
 
//---------------------------------------------------------------------------------------------------------------
 
 int Decrypt_Card_Header (int port, int slot, void *buf, void *bit, int *psize)
 {
	// Perform decryption of an encrypted file header from memory card
		 
 	u8 kbit[16];
 	u8 kc[16]; 	
 	int size; 
 	psize = &size; 	
 	  	
 	read_kbit(buf, kbit);
 	read_kc(buf, kc); 	
 	
 	if (!Card_Decrypt(port, slot, kbit))
 		return 0;
 		
 	if (!Card_Decrypt(port, slot, kbit + 8))
 		return 0;
 		
 	if (!Card_Decrypt(port, slot, kc))
 		return 0;
 		
 	if (!Card_Decrypt(port, slot, kc + 8))
 		return 0;
 	
 	store_kbit(buf, kbit);
 	store_kc(buf, kc); 	
 	
 	int hcode = 1; // 0 = DiskBoot 1 = CardBoot 2 = Download 
 	
 	int cnum = mcman_getcnum(port, slot); 
 	
 	if (cnum < 0)
 		return 0;
	
 	if (!Mecha_Set_Header(hcode, cnum, 0, buf)) // send mecha commands 0x90, 0x8D, 0x8F
 		return 0;
 		
 	size = Mecha_ReadBIT(bit); // send mecha commands 0x91, 0x8E	
 	
 	if (!size)
 		return 0; 
	    
 	return 1; 	
 }
 
//---------------------------------------------------------------------------------------------------------------
 int Decrypt_Block (void *src, void *dst, int size) 
 {
	// Decrypt an encrypted block of datas
	 
 	if (!Mecha_Set_Block(src, size)) // send mecha commands 0x92, 0x8D, 0x8F	 
 		return 0;
 		
 	if (!(Mecha_ReadBLOCK(dst, size))) // send mecha commands 0x93, 0x8E 	
 		return 0;
 		
 	return 1;
 }
//---------------------------------------------------------------------------------------------------------------
 int Download_Block (void *src, int size) 
 {
	// Send encrypted block of datas, but doesn't decrypt it
	// Certainly necessary for the kc generation
	 
 	if (!Mecha_Set_Block(src, size)) // send mecha commands 0x92, 0x8D, 0x8F	 
 		return 0;
 		
 	return 1;
 }
//---------------------------------------------------------------------------------------------------------------

int bit_offset(u8 *inbuf)
{	
	// returns the BIT table offset
	
  	mg_ELF_Header_t *header = (mg_ELF_Header_t *) inbuf;
  	int offset = 0x20;

  	if (header->BIT_count > 0)
  		offset += header->BIT_count * 16;
  		
  	if ((*(u32*)&header->flags) & 1)
  		offset += inbuf[offset] + 1;
  		
  	if (((*(u32*)&header->flags) & 0xf000) == 0)
  		offset += 8;
  		
 	return offset + 0x20; 
}

//---------------------------------------------------------------------------------------------------------------
 
 int Download_GetKbit (int port, int slot, void *kbit)
 { 
	// Perform the MagicGate encryption of kbit
	 
	if (!GetKbit(kbit)) // send mecha commands 0x94, 0x95
		return 0;

	if (!Card_Encrypt(port, slot, kbit))
		return 0;
		
	if (!Card_Encrypt(port, slot, kbit + 8))
		return 0;
		
 	return 1;
 }

//--------------------------------------------------------------------------------------------------------------- 

 int Download_GetKc (int port, int slot, void *kc)
 { 
	// Perform the MagicGate encryption of kc
		 
	if (!GetKc(kc)) // send mecha commands 0x96, 0x97
		return 0;
		
  	if (!Card_Encrypt(port, slot, kc))
  		return 0;
  		
 	if (!Card_Encrypt(port, slot, kc + 8))
 		return 0;
 		
 	return 1;
 }
 
//------------------------------------------------------------------------------------------------------------------------ 

 int Download_GetICVPS2 (void *icvps2)
 { 
	if (!GetICVPS2(icvps2)) // send mecha command 0x98
		return 0;
		
 	return 1;
 }

//--------------------------------------------------------------------------------------------------------------- 

void *Sign_Encrypted_Disk_File(int port, void *buf)
{
	// Perform whole preparation of an encrypted file for mc
	
  	u8 bit[0x400];
  	u8 kbit[16];
  	u8 kcontent[16];
  	u8 icvps2[8];
  	int size, bit_ofs;
  	int *psize = &size;

  	int i;

  	printf("Sign_Encrypted_Disk_File ");
  	  	
	bit_ofs = bit_offset(buf);
	
    Bit_Data_t *b = (Bit_Data_t *) bit;  	
	memcpy (bit, buf + bit_ofs, 1024);

  	if (!Download_Header(port, 0, buf, bit, psize)) {
    	printf("Failed to decrypt header\n");
    	return NULL;
  	}
  	
  	if (b->block_count > 0) {
    	int offset = b->headersize;
    	for (i = 0; i < b->block_count; i++) {
      		if (b->blocks[i].flags & 2) {
        		if (!Download_Block(buf + offset, b->blocks[i].size)) {
          			printf("Failed\n");
          			return NULL;
        		}
      		}
      		offset += b->blocks[i].size;
    	}
  	}

  	if (!Download_GetKbit(port, 0, kbit)) {
    	printf("Failed to get kbit\n");
    	return NULL;
  	}
  	
  	if (!Download_GetKc(port, 0, kcontent)) {
    	printf("Failed to get kc\n");
    	return NULL;
  	}
  	
  	store_kbit(buf, kbit);
  	store_kc(buf, kcontent);

  	if (((mg_ELF_Header_t *) buf)->flags & 2) {
    	if (!Download_GetICVPS2(icvps2)) {
      		printf("Failed to get icvps2\n");
      		return NULL;
    	}
    	store_icvps2(buf, icvps2);
  	}
  	
  	printf("Done\n");
  	
  	return (void *) buf;
}
//------------------------------------------------------------------------------------------------------------------------ 

void *Decrypt_Disk_File(void *buf)
{
	// Perform whole decryption of file from Disk or rom
		
	int i, r, bit_ofs;
	u8 bit[0x400];
  	int size;
  	int *psize = &size;
	
  	printf("\n\tDecrypting file ");        
  	
	bit_ofs = bit_offset(buf);
	
	r = Decrypt_Disk_Header(buf, buf + bit_ofs, psize);
	if (r == 0) {
		printf ("Failed to decrypt header\n");
		return NULL;
    }
    Bit_Data_t *b = (Bit_Data_t *) bit;	
	memcpy (bit, buf + bit_ofs, 1024);
	        
	if (b->block_count > 0) {
    	int offset = b->headersize;
    	for (i = 0; i < b->block_count; i++) {
	    	if (b->blocks[i].flags & 3) {
				r = Decrypt_Block(buf + offset, buf + offset, b->blocks[i].size);
				if (r == 0) {
					printf ("Failed\n");
					return NULL;
				}	
	    	}
	    	offset += b->blocks[i].size;	
    	}

	}
	
  	printf("Done\n");	

	return (void*) (buf + (((mg_ELF_Header_t *) buf)->mg_header_size & 0xFFFF));	

}

//_______________________________________________________________________________

void *Decrypt_Card_File(int port, void *buf)
{
	// Perform whole decryption of file from mc
		
	int r, bit_ofs, i;
	u8 bit[0x400];
	int size;
	int *psize = &size;
	
	printf("\n\tDecrypting file ");
        
	bit_ofs = bit_offset(buf);
	
	r = Decrypt_Card_Header(port, 0, buf, buf + bit_ofs, psize);
	if (r==0) {
		printf ("Failed to decrypt header\n");
		return NULL;
    }
    
    Bit_Data_t *b = (Bit_Data_t *) bit;	    
	memcpy (bit, buf + bit_ofs, sizeof(bit));    
	
	if (b->block_count > 0) {
    	int offset = b->headersize;
    	for (i = 0; i < b->block_count; i++) {
	    	if (b->blocks[i].flags & 3) {
				r = Decrypt_Block(buf + offset, buf + offset, b->blocks[i].size);
				if (r==0) {
					printf ("Failed\n");
					return NULL;
				}	
	    	}
	    	offset += b->blocks[i].size;	
    	}

	}	

  	printf("Done\n");	
  		
	return (void*) (buf + (((mg_ELF_Header_t *) buf)->mg_header_size & 0xFFFF));	
}

//_______________________________________________________________________________


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <hidapi/hidapi.h>
#include "hidprog.h"


static uint32_t get_le32(uint8_t buf[4])
{
	return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}

static void put_le32(uint8_t buf[4], uint32_t val)
{
    buf[0] = (val >> 0) & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
    buf[2] = (val >> 16) & 0xFF;
    buf[3] = (val >> 24) & 0xFF;
}


static int send_command(hid_device* handle, hidprog_command_t* cmd)
{
	uint8_t buf[sizeof(*cmd)+1];
	buf[0] = 0; // Report ID

	memcpy(&(buf[1]), cmd->bytes, sizeof(*cmd) );

	return hid_write(handle, buf, sizeof(buf)) != sizeof(buf);
}

static int get_response(hid_device* handle, hidprog_response_t* rsp, int timeout)
{
	uint8_t buf[sizeof(*rsp)+1];

	int len = hid_read(handle, buf, sizeof(buf));

	if(len != sizeof(*rsp)) // Only full responses need apply
		return -1;
	//if(buf[0] != 0) // Report ID must be 0
	//	return -1;
	
	memcpy(rsp->bytes, &(buf[0]), sizeof(*rsp) );

	return 0;
}

static int run_command(hid_device* handle, hidprog_command_t* cmd, hidprog_response_t* rsp)
{	
	int res = send_command(handle, cmd);
	if(res)
		return res;
	return get_response(handle, rsp, -1);
}

static int program_flash(hid_device* handle, uint32_t base_address, uint8_t* data, size_t len)
{
	hidprog_command_t cmd = {0};
	hidprog_response_t rsp = {0};
	int res;

	cmd.setaddr.id = HIDPROG_ID_SETADDR;
	put_le32(cmd.setaddr.addr, base_address); 
	res = run_command(handle, &cmd, &rsp);
	if(res)
		return res;

	for(int i = 0; i<len;)
	{
		cmd.id  = HIDPROG_ID_WRITEDATA;
		cmd.writedata.len = 4 | HIDPROG_WRITEDATA_LEN_MAY_ERASE;

		int remain = len - i;
		if(remain > 16)
			remain = 16;

		memcpy(cmd.writedata.data, data+i, remain);
		res = run_command(handle, &cmd, &rsp);
		if(res){
			return res;
		}
		i += 16 ;
	}
}



static char * get_id_name(uint8_t id){
	switch (id)
	{
		case HIDPROG_ID_PING: return "PING";
		case HIDPROG_ID_SETADDR: return "SETADDR";
		case HIDPROG_ID_GETADDR: return "GETADDR";
		case HIDPROG_ID_WRITEDATA: return "WRITEDATA";
		case HIDPROG_ID_READDATA: return "READDATA";
		case HIDPROG_ID_FINISH: return "FINISH";
		case HIDPROG_ID_GETINFO: return "GETINFO";
		default: return "UNKNOWN";
	}
}


static void dump_response(hidprog_response_t* rsp)
{	
	printf("Response ID %08X (%s)\n", rsp->id, get_id_name(rsp->id));
	switch(rsp->id)
	{
		case HIDPROG_ID_PING:
			printf("\tping.echo: 0x%02X, ping.count: %d\n",rsp->ping.echo, rsp->ping.count);
			break;

		case HIDPROG_ID_GETINFO:
			printf("\tversion: %d, magic: 0x%08X\n", rsp->getinfo.version, get_le32(rsp->getinfo.magic));
			printf("\tflash_size: %d bytes (0x%08X)\n", get_le32(rsp->getinfo.flash_size), get_le32(rsp->getinfo.flash_size));
			printf("\tFlash Blocks:\n");
			for(int i=0; i < sizeof(rsp->getinfo.block_count); i++)
			{
				if(rsp->getinfo.block_count[i] == 0)
					break;
				printf("\t\t %d bytes x %d %s\n", 1 << (rsp->getinfo.block_size[i] & 0x7F), rsp->getinfo.block_count[i], rsp->getinfo.block_size[i] & 0x80 ? "READ_ONLY" : "" );
			};
			break;

		case HIDPROG_ID_READDATA:
			printf("\tlength: %d (%d bytes)\n", rsp->readdata.len, 4*rsp->readdata.len);
			for(int i=0; i < rsp->readdata.len; i++ )
			{	
				printf("\t0x%08X\n",get_le32(rsp->readdata.data+4*i));
			}
			break;
		case HIDPROG_ID_GETADDR:
			printf("\taddr: 0x%08X\n", get_le32(rsp->getaddr.addr));

			break;
	}
}



int main(int argc, char* argv[])
{
	int res;
	hid_device *handle;
	int i;

	// Initialize the hidapi library
	res = hid_init();

	// Open the device using the VID, PID,
	// and optionally the Serial number.
	handle = hid_open(0x1d50, 0x613b, NULL);
    printf("hid_open() = %d\n", handle);
    if(!handle)
        return -1;

	// Toggle LED (cmd 0x80). The first byte is the report number (0x0).
	hidprog_command_t cmd;
	hidprog_response_t rsp;
	cmd.ping.id = HIDPROG_ID_PING;
	cmd.ping.echo = 0x5A;

	res = run_command(handle, &cmd, &rsp);
    printf("run_command = %d\n", res);
	if(!res)
		dump_response(&rsp);


	cmd.getinfo.id = HIDPROG_ID_GETINFO;
	res = run_command(handle, &cmd, &rsp);
    printf("run_command = %d\n", res);
	if(!res)
		dump_response(&rsp);

	cmd.setaddr.id = HIDPROG_ID_SETADDR;
	put_le32(cmd.setaddr.addr, 0x8004000); 
	res = run_command(handle, &cmd, &rsp);
    printf("run_command = %d\n", res);
	if(!res)
		dump_response(&rsp);


	if(argc > 1)
	{
		FILE* f = fopen(argv[1], "r");
		if(!f)
		{
			printf("Failed to open '%s'\n", argv[1]);
			exit(-1);
		}
		res = fseek(f, 0, SEEK_END);
		if(res)
		{
			printf("fseek error : %d\n", res);
			exit(-1);
		}
		long len = ftell(f);
		if(len <= 0)
		{
			printf("ftell error : %d\n", len);
			exit(-1);
		}		
		res = fseek(f, 0, SEEK_SET);
		if(res)
		{
			printf("fseek error : %d\n", res);
			exit(-1);
		}

		uint8_t * buf = malloc(len);
		if(!buf)
		{
			printf("malloc failure\n");
			exit(-1);
		}

		ssize_t rlen = fread(buf, 1, len, f);
		if(rlen != len)
		{
			printf("Read error: expected %ld bytes, got %ld\n", len, rlen);
			exit(-1);
		}

		res = program_flash(handle, 0x8004000, buf, len);
		if(res)
		{
			printf("flash_program() = %d\n", res);
		}

		free(buf);
		fclose(f);
	}


	cmd.setaddr.id = HIDPROG_ID_GETADDR;
	res = run_command(handle, &cmd, &rsp);
	printf("run_command = %d\n", res);
	if(!res)
		dump_response(&rsp);


	cmd.setaddr.id = HIDPROG_ID_SETADDR;
	put_le32(cmd.setaddr.addr, 0x8004000); 
	res = run_command(handle, &cmd, &rsp);
    printf("run_command = %d\n", res);
	if(!res)
		dump_response(&rsp);
	
	i=0;
	while(i < 256)
	{	
			cmd.id  = HIDPROG_ID_READDATA;
			cmd.readdata.len = 4;
			res = run_command(handle, &cmd, &rsp);
			if(res) {
				printf("run_command = %d\n", res);
				break;
			} else {
				dump_response(&rsp);
			}
			i += 16 ;
	}



	// Finalize the hidapi library
	hid_close(handle);
	res = hid_exit();

	return 0;
}



#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <hidapi/hidapi.h>
#include "hidprog_cmds.h"
#include "hidprog.h"

#define DEFAULT_VID  (0x1d50)
#define DEFAULT_PID  (0x613b)
#define DEFAULT_MAGIC (0x57535446u)

#define ERROR(s, ...) do{fprintf(stderr, "%s: " s, cmdname, __VA_ARGS__); exit(-1);} while(0)

static char* cmdname = NULL;
static int   verbose = 0;

static void program(hid_device* handle, char * path, uint32_t magic, int do_reset)
{
	hidprog_info_t info;
	int res = hidprog_getinfo(handle, &info);
	if(res) ERROR("Error retreiving device info: %d", res);

	if(verbose)
	{
		printf("Bootloader Info:\n");	
		printf("\tversion: %d, magic: %08X\n", info.version, info.magic);
		printf("\tflash_base: 0x%08X, flash_size: 0x%08X (%u bytes)\n", info.flash_base, info.flash_size, info.flash_size);
	}

	if(info.magic != magic) ERROR("Device magic ID 0x%08X does not match expected ID 0x%08X\n", info.magic, magic);
	
	FILE *f = fopen(path, "rb");
	if (!f) ERROR("failed to open file '%s': %s\n", path, strerror(errno));
	
	res = fseek(f, 0, SEEK_END);
	if (res) ERROR("Seek failed: %s\n", strerror(errno));

	long len = ftell(f);
	if (len < 0) ERROR("Tell failed: %s\n", strerror(errno));
	if(len > info.flash_size)
	{
		ERROR("Input size %ld bytes exceeds flash size %u bytes\n", len, info.flash_size);
	}

	res = fseek(f, 0, SEEK_SET);
	if (res) ERROR("Seek failed: %s\n", strerror(errno));

	uint8_t *buf = malloc(len);
	if (res) ERROR("Unable to allocate buffer: %s\n", strerror(errno));

	size_t rlen = fread(buf, 1, len, f);
	if (rlen != (size_t)len) ERROR("Read error: expected %ld bytes, got %ld: %s\n", len, rlen, strerror(errno));

	if(verbose)
	{
		fprintf(stderr, "Programming...");
	}
	res = hidprog_program(handle, info.flash_base, buf, len);
	if (res) {
		printf("Programming Failed!\n");
		exit(-1);
	} else if(verbose) {
		fprintf(stderr, " Done!\n");
	}
	
	if(verbose){
		fprintf(stderr, "Verifying...");
	}
	res = hidprog_verify(handle, info.flash_base, buf, len);
	if (res) {
		printf("Verify Failed!\n");
		exit(-1);
	} else if(verbose){
		fprintf(stderr, " Done!\n");
	}
	if(do_reset)
	{
		if(verbose) fprintf(stderr, "Resetting\n");
		res = hidprog_reset(handle);	
		if(res) ERROR("Error issuing reset command: %d\n", res);
	}
	free(buf);
	fclose(f);
}

static void enumerate(unsigned short vid, unsigned short pid)
{
	struct hid_device_info* start = hid_enumerate(vid, pid);
	if(!start)
	{
		printf("No devices detected\n");
		return;
	}
	struct hid_device_info* info = start;
	printf("Detected devices: (Use -s with SERIALNUMBER to specify device to program)\n");
	printf("%-4s %-4s %-16s %-24s %-24s\n", "VID", "PID", "SERIAL", "MANUFACTURER", "PRODUCT");
	while(info)
	{
		printf("%04X %04X %-16ls %-24ls %-24ls\n", 
			info->vendor_id, info->product_id, info->serial_number, info->manufacturer_string, info->product_string);

		info = info->next;
	}
	hid_free_enumeration(start);
}	

static unsigned long parse_hex_arg(int argc, char* argv[], int* i)
{
	unsigned long ret;
	char *end;

	if(argc < (*i)+2) ERROR("Expected hex number after %s\n", argv[*i]);
	ret = strtoul(argv[(*i)+1], &end, 16);
	if(end == argv[(*i)+1] || *end != 0) ERROR("Expected hex number after %s\n", argv[*i]);
	++(*i);
	return ret;
}

static void usage(FILE* out)
{
	fprintf(out, "Usage: %s [options] FILENAME\n", cmdname);
	fprintf(out, "Program device using HID bootloader\n");
	fprintf(out, "Options:\n");
	fprintf(out, "\t-v NUM, --vid NUM\n");
	fprintf(out, "\t\tOverrride the target USB VID\n");
	fprintf(out, "\t-p NUM, --pid NUM\n");
	fprintf(out, "\t\tOverrride the target USB PID\n");
	fprintf(out, "\t-s STRING, --serial STRING\n");
	fprintf(out, "\t\tSpecify the target device serial number\n");
	fprintf(out, "\t-m NUM, --magic  NUM\n");
	fprintf(out, "\t\tOverride the expected bootloader identifier\n");
	fprintf(out, "\t-E, --enumerate\n");
	fprintf(out, "\t\tList connected devices\n");
	fprintf(out, "\t-V, --verbose\n");
	fprintf(out, "\t\tPrint additional status and device info\n");
	fprintf(out, "\t-r, --reset\n");
	fprintf(out, "\t\tReset the device after programming\n");
	fprintf(out, "\t-h, --help\n");
	fprintf(out, "\t\tPrint this help\n");
}

int main(int argc, char *argv[]) {
    int         res;
    hid_device *handle;
	int 		do_enumerate = 0;
	int 		do_reset = 0;
	unsigned short vid = DEFAULT_VID;
	unsigned short pid = DEFAULT_PID;
	uint32_t magic = DEFAULT_MAGIC;
	wchar_t* serial = NULL;
	wchar_t  serial_buf[255];
	char* path = NULL;

	cmdname = argv[0];

	for(int i=1; i<argc; i++)
	{
		if(!strcmp(argv[i], "-p") || !strcmp(argv[i],"--pid"))
		{
			pid = parse_hex_arg(argc, argv, &i);
		} else if(!strcmp(argv[i], "-v") || !strcmp(argv[i],"--vid"))
		{
			vid = parse_hex_arg(argc, argv, &i);
		} else if(!strcmp(argv[i], "-s") || !strcmp(argv[i],"--serial"))
		{

			if(argc < i+2) ERROR("Expected serial number after %s", argv[i]);

			size_t len = mbstowcs(serial_buf, argv[++i], 255);
			if(len == (size_t)-1) ERROR("Failure processing serial number '%s'. Too long?\n", argv[i]);
			serial = serial_buf;
		} 
		else if(!strcmp(argv[i], "-m") || !strcmp(argv[i],"--magic"))
		{
			magic = parse_hex_arg(argc, argv, &i);
		} 
		else if(!strcmp(argv[i], "-E") || !strcmp(argv[i],"--enumerate"))
		{	
			do_enumerate = 1;
		}
		else if(!strcmp(argv[i], "-h") || !strcmp(argv[i],"--help"))
		{
			usage(stdout);
			exit(0);
		}
		else if(!strcmp(argv[i], "-V") || !strcmp(argv[i],"--verbose"))
		{
			verbose = 1;
		}
		else if(!strcmp(argv[i], "-r") || !strcmp(argv[i],"--reset"))
		{
			do_reset = 1;
		}
		else if(argv[i][0] != '-')
		{	
			if(path)
			{
				fprintf(stderr, "%s: Unexpected argument '%s'. Only one input file is permitted\n", cmdname, argv[i]);
				usage(stderr);
				exit(-1);
			} else 
			{
				path = argv[i];
			}
		}
		else 
		{
			fprintf(stderr, "%s: Unknown option '%s'\n", cmdname, argv[i]);
			usage(stderr);
			exit(-1);
		}

	}

	
    // Initialize the hidapi library
    res = hid_init();
	if(res) ERROR("Failed to initialize HID library: %d", res);

	if(do_enumerate)
	{
		enumerate(vid, pid);
	} else if(path)
	{
		// Open the device using the VID, PID,
		// and optionally the Serial number.
		handle = hid_open(vid, pid, serial);
		if (!handle) {
			if(serial)
				ERROR("Unable to open device with vid = 0x%04X, pid = 0x%04X and serial = '%ls'\n", vid, pid, serial);
			else 
				ERROR("Unable to open device with vid = 0x%04X and pid = 0x%04X\n", vid, pid);
		}
		program(handle, path, magic, do_reset);

		// Finalize the hidapi library
    	hid_close(handle);
	} else 
	{
		fprintf(stderr, "%s: No input file\n", cmdname);
		usage(stderr);
	}



    hid_exit();

    return 0;
}


#include "hidprog.h"

#include <stdio.h>
#include <string.h>
#include <hidapi/hidapi.h>
#include "hidprog_cmds.h"

static uint32_t get_le32(uint8_t buf[4]) {
    return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}

static void put_le32(uint8_t buf[4], uint32_t val) {
    buf[0] = (val >> 0) & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
    buf[2] = (val >> 16) & 0xFF;
    buf[3] = (val >> 24) & 0xFF;
}

int hidprog_send_command(hid_device *handle, hidprog_command_t *cmd) {
    uint8_t buf[sizeof(*cmd) + 1];

    if (!handle || !cmd)
        return -1;

    buf[0] = 0; // Report ID

    memcpy(&(buf[1]), cmd->bytes, sizeof(*cmd));

    return hid_write(handle, buf, sizeof(buf)) != sizeof(buf);
}

int hidprog_get_response(hid_device *handle, hidprog_response_t *rsp,
                         int timeout) {
    uint8_t buf[sizeof(*rsp) + 1];
    if (!handle || !rsp)
        return -1;

    int len = hid_read_timeout(handle, buf, sizeof(buf), timeout);

    if (len != sizeof(*rsp)) // Only full responses need apply
        return -1;

    memcpy(rsp->bytes, &(buf[0]), sizeof(*rsp));

    return 0;
}

int hidprog_run_command(hid_device *handle, hidprog_command_t *cmd,
                        hidprog_response_t *rsp) {
    int res = hidprog_send_command(handle, cmd);
    if (res)
        return res;
    res = hidprog_get_response(handle, rsp, 1000);
    if (res)
        return res;

    if (rsp->id != cmd->id)
        return -1;

    return 0;
}

int hidprog_program(hid_device *handle, uint32_t base_address, uint8_t *data,
                    size_t len) {
    hidprog_command_t  cmd = {0};
    hidprog_response_t rsp = {0};
    int                res;

    if (!handle || !data)
        return -1;

    if (base_address != (uint32_t)-1) {
        res = hidprog_setaddr(handle, base_address);
        if (res)
            return res;
    }
    for (size_t i = 0; i < len;) {
        cmd.id = HIDPROG_ID_WRITEDATA;

        int remain = len - i;
        if (remain > 16)
            remain = 16;

        cmd.writedata.len =
            ((remain + 3) / 4) | HIDPROG_WRITEDATA_LEN_MAY_ERASE;

        memcpy(cmd.writedata.data, data + i, remain);
        res = hidprog_run_command(handle, &cmd, &rsp);
        if (res) {
            return res;
        }
        i += remain;
    }
    return 0;
}

int hidprog_read(hid_device *handle, uint32_t base_address, uint8_t *data,
                 size_t len) {
    hidprog_command_t  cmd = {0};
    hidprog_response_t rsp = {0};
    int                res;

    if (!handle || !data)
        return -1;

    if (base_address != (uint32_t)-1) {
        res = hidprog_setaddr(handle, base_address);
        if (res)
            return res;
    }

    for (size_t i = 0; i < len;) {
        cmd.id = HIDPROG_ID_READDATA;

        int remain = len - i;
        if (remain > 16)
            remain = 16;
        cmd.readdata.len = ((remain + 3) / 4);

        res = hidprog_run_command(handle, &cmd, &rsp);
        if (res) {
            return res;
        }
        memcpy(data + i, rsp.readdata.data, remain);
        i += remain;
    }
    return 0;
}

int hidprog_verify(hid_device *handle, uint32_t base_address, uint8_t *data,
                   size_t len) {
    hidprog_command_t  cmd = {0};
    hidprog_response_t rsp = {0};
    int                res;

    if (!handle || !data)
        return -1;

    if (base_address != (uint32_t)-1) {
        res = hidprog_setaddr(handle, base_address);
        if (res)
            return res;
    }
    for (size_t i = 0; i < len;) {
        cmd.id = HIDPROG_ID_READDATA;

        int remain = len - i;
        if (remain > 16)
            remain = 16;
        cmd.readdata.len = ((remain + 3) / 4);

        res = hidprog_run_command(handle, &cmd, &rsp);
        if (res) {
            return res;
        }
        if (memcmp(data + i, rsp.readdata.data, remain) != 0)
            return -1;
        i += remain;
    }
    return 0;
}

int hidprog_setaddr(hid_device *handle, uint32_t address) {
    if (!handle)
        return -1;

    hidprog_command_t  cmd = {0};
    hidprog_response_t rsp = {0};

    cmd.setaddr.id = HIDPROG_ID_SETADDR;
    put_le32(cmd.setaddr.addr, address);
    return hidprog_run_command(handle, &cmd, &rsp);
}

int hidprog_getaddr(hid_device *handle, uint32_t *address) {
    if (!handle || !address)
        return -1;

    hidprog_command_t  cmd = {0};
    hidprog_response_t rsp = {0};
    int                res;

    cmd.getaddr.id = HIDPROG_ID_GETADDR;
    res            = hidprog_run_command(handle, &cmd, &rsp);
    if (res)
        return res;

    *address = get_le32(rsp.getaddr.addr);

    return 0;
}

int hidprog_getinfo(hid_device *handle, hidprog_info_t *info) {
    hidprog_command_t  cmd = {0};
    hidprog_response_t rsp = {0};
    int                res;

    if (!handle || !info)
        return -1;

    memset(info, 0, sizeof(*info));

    cmd.getaddr.id = HIDPROG_ID_GETINFO;
    res            = hidprog_run_command(handle, &cmd, &rsp);
    if (res)
        return res;

    info->version    = rsp.getinfo.version;
    info->magic      = get_le32(rsp.getinfo.magic);
    info->flash_base = get_le32(rsp.getinfo.flash_base);
    info->flash_size = get_le32(rsp.getinfo.flash_size);

    int i;
    for (i = 0; i < 8; i++) {
        if (rsp.getinfo.block_count[i] == 0)
            break;

        info->blocks[i].size =
            1 << (rsp.getinfo.block_size[i] & HIDPROG_GETINFO_BLOCKSIZE_SIZE);
        if (rsp.getinfo.block_size[i] & HIDPROG_GETINFO_BLOCKSIZE_READONY)
            info->blocks[i].flags |= HIDPROG_BLOCK_FLAG_READONLY;
        info->blocks[i].count = rsp.getinfo.block_count[i];
    }
    info->num_blocks = i;

    return 0;
}

int hidprog_reset(hid_device *handle) {
    if (!handle)
        return -1;

    hidprog_command_t  cmd = {0};
    hidprog_response_t rsp = {0};

    cmd.id           = HIDPROG_ID_FINISH;
    cmd.finish.flags = HIDPROG_FINISH_FLAG_REBOOT;
    return hidprog_run_command(handle, &cmd, &rsp);
}

static char *get_id_name(uint8_t id) {
    switch (id) {
        case HIDPROG_ID_PING:
            return "PING";
        case HIDPROG_ID_SETADDR:
            return "SETADDR";
        case HIDPROG_ID_GETADDR:
            return "GETADDR";
        case HIDPROG_ID_WRITEDATA:
            return "WRITEDATA";
        case HIDPROG_ID_READDATA:
            return "READDATA";
        case HIDPROG_ID_FINISH:
            return "FINISH";
        case HIDPROG_ID_GETINFO:
            return "GETINFO";
        default:
            return "UNKNOWN";
    }
}

void hidprog_dump_response(hidprog_response_t *rsp) {
    printf("Response ID %08X (%s)\n", rsp->id, get_id_name(rsp->id));
    switch (rsp->id) {
        case HIDPROG_ID_PING:
            printf("\tping.echo: 0x%02X, ping.count: %d\n", rsp->ping.echo,
                   rsp->ping.count);
            break;

        case HIDPROG_ID_GETINFO:
            printf("\tversion: %d, magic: 0x%08X\n", rsp->getinfo.version,
                   get_le32(rsp->getinfo.magic));
            printf("\tflash_size: %d bytes (0x%08X)\n",
                   get_le32(rsp->getinfo.flash_size),
                   get_le32(rsp->getinfo.flash_size));
            printf("\tFlash Blocks:\n");
            for (size_t i = 0; i < sizeof(rsp->getinfo.block_count); i++) {
                if (rsp->getinfo.block_count[i] == 0)
                    break;
                printf("\t\t %d bytes x %d %s\n",
                       1 << (rsp->getinfo.block_size[i] & 0x7F),
                       rsp->getinfo.block_count[i],
                       rsp->getinfo.block_size[i] & 0x80 ? "READ_ONLY" : "");
            };
            break;

        case HIDPROG_ID_READDATA:
            printf("\tlength: %d (%d bytes)\n", rsp->readdata.len,
                   4 * rsp->readdata.len);
            for (int i = 0; i < rsp->readdata.len; i++) {
                printf("\t0x%08X\n", get_le32(rsp->readdata.data + 4 * i));
            }
            break;
        case HIDPROG_ID_GETADDR:
            printf("\taddr: 0x%08X\n", get_le32(rsp->getaddr.addr));

            break;
    }
}

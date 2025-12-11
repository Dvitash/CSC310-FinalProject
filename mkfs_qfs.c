/* date: 11/12/25, authors: daniel vitashkevich, grant foody, purpose: create and format qfs images */
/*
**Program to make a filesystem on a blank file using the qfs parameters
**
** Usage: mkfs_qfs <disk image file> [<label>]
**
** To create a blank file of a specific size, you can use the following command:
**   dd if=/dev/zero of=<disk image file> bs=1M count=<size in MB>
**
** Example:
**   dd if=/dev/zero of=disk.img bs=1M count=4
**
** Then run:
**   mkfs_qfs disk.img MyVolume
**
** This will format 'disk.img' as a 4MB QFS filesystem with the label 'MyVolume'.
**
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "qfs.h"

static uint16_t choose_block_size(long file_size)
{
    const long mb30 = 30L * 1024L * 1024L;
    const long mb60 = 60L * 1024L * 1024L;

    if (file_size <= mb30)
    {
        return 512;
    }
    else if (file_size <= mb60)
    {
        return 1024;
    }

    return 2048;
}

int main(int argc, char *argv[])
{
    FILE *fp = fopen(argv[1], "rb+");

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    superblock_t sb;
    memset(&sb, 0, sizeof(superblock_t));
    sb.fs_type = 0x51;
    sb.bytes_per_block = choose_block_size(file_size);
    long data_region = file_size - (long)sizeof(superblock_t) - ((long)sizeof(direntry_t) * 255L);
    sb.total_blocks = (uint16_t)(data_region / sb.bytes_per_block);
    sb.available_blocks = sb.total_blocks;
    sb.total_direntries = 255;
    sb.available_direntries = 255;

    if (argc == 3)
    {
        strncpy(sb.label, argv[2], sizeof(sb.label) - 1);
        sb.label[sizeof(sb.label) - 1] = '\0';
    }

    uint8_t dir_zeros[sizeof(direntry_t) * 255] = {0};

    fseek(fp, 0, SEEK_SET);
    fwrite(&sb, sizeof(superblock_t), 1, fp);
    fwrite(dir_zeros, sizeof(dir_zeros), 1, fp);

    long data_start = (long)sizeof(superblock_t) + (long)sizeof(dir_zeros);

    for (uint16_t i = 0; i < sb.total_blocks; i++)
    {
        long off = data_start + (long)i * sb.bytes_per_block;
        fseek(fp, off, SEEK_SET);
        uint8_t free_flag = 1;
        fwrite(&free_flag, 1, 1, fp);
    }

    fflush(fp);
    fclose(fp);
    return 0;
}
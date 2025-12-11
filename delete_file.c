/* date: 11/12/25, authors: daniel vitashkevich, grant foody, purpose: delete a file from qfs image and free blocks */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "qfs.h"

static long block_offset(const superblock_t *sb, uint16_t block_index)
{
    long dir_bytes = (long)sizeof(direntry_t) * 255L;
    long data_start = (long)sizeof(superblock_t) + dir_bytes;
    return data_start + (long)block_index * sb->bytes_per_block;
}

int main(int argc, char *argv[])
{
    FILE *fp = fopen(argv[1], "rb+");

    superblock_t sb;
    fread(&sb, sizeof(superblock_t), 1, fp);

    direntry_t entry;
    long dir_pos = 0;

    for (int i = 0; i < 255; i++)
    {
        dir_pos = ftell(fp);
        fread(&entry, sizeof(direntry_t), 1, fp);
        if (entry.filename[0] != '\0' && strncmp(entry.filename, argv[2], sizeof(entry.filename)) == 0)
        {
            break;
        }
    }

    uint16_t data_len = sb.bytes_per_block - 3;
    uint32_t remaining = entry.file_size;
    uint16_t block = entry.starting_block;
    uint16_t freed = 0;

    while (remaining > 0)
    {
        long off = block_offset(&sb, block);
        fseek(fp, off + sb.bytes_per_block - 2, SEEK_SET);
        uint16_t next = 0;
        fread(&next, sizeof(uint16_t), 1, fp);

        fseek(fp, off, SEEK_SET);
        uint8_t free_flag = 1;
        fwrite(&free_flag, 1, 1, fp);
        freed++;

        if (remaining <= data_len)
        {
            break;
        }

        remaining -= data_len;
        block = next;
    }

    sb.available_blocks = (uint16_t)(sb.available_blocks + freed);
    if (sb.available_direntries < 255)
    {
        sb.available_direntries = (uint8_t)(sb.available_direntries + 1);
    }

    fseek(fp, 0, SEEK_SET);
    fwrite(&sb, sizeof(superblock_t), 1, fp);

    direntry_t empty;
    memset(&empty, 0, sizeof(empty));
    fseek(fp, dir_pos, SEEK_SET);
    fwrite(&empty, sizeof(direntry_t), 1, fp);

    fclose(fp);
    return 0;
}

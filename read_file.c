/* date: 11/12/25, authors: daniel vitashkevich, grant foody, purpose: extract a file from qfs image */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
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
    FILE *fp = fopen(argv[1], "rb");

    superblock_t sb;
    fread(&sb, sizeof(superblock_t), 1, fp);

    direntry_t entry;
    for (int i = 0; i < 255; i++)
    {
        fread(&entry, sizeof(direntry_t), 1, fp);
        if (entry.filename[0] != '\0' && strncmp(entry.filename, argv[2], sizeof(entry.filename)) == 0)
        {
            break;
        }
    }

    FILE *out = fopen(argv[3], "wb");

    uint32_t remaining = entry.file_size;
    uint16_t data_len = sb.bytes_per_block - 3;
    uint16_t block = entry.starting_block;
    uint8_t *buffer = (uint8_t *)malloc(data_len);

    while (remaining > 0)
    {
        long off = block_offset(&sb, block);
        fseek(fp, off + 1, SEEK_SET);
        size_t read = fread(buffer, 1, data_len, fp);
        size_t to_write = remaining < read ? remaining : read;
        fwrite(buffer, 1, to_write, out);

        fseek(fp, off + sb.bytes_per_block - 2, SEEK_SET);
        uint16_t next = 0;
        fread(&next, sizeof(uint16_t), 1, fp);

        if (remaining <= to_write)
        {
            break;
        }
        remaining -= (uint32_t)to_write;
        block = next;
    }

    free(buffer);
    fclose(out);
    fclose(fp);
    return 0;
}
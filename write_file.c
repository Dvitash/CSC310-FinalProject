/* date: 11/12/25, authors: daniel vitashkevich, grant foody, purpose: add a local file into a qfs image */
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
    FILE *fp = fopen(argv[1], "rb+");

    FILE *in = fopen(argv[2], "rb");

    fseek(in, 0, SEEK_END);
    long input_size = ftell(in);
    fseek(in, 0, SEEK_SET);

    superblock_t sb;
    fread(&sb, sizeof(superblock_t), 1, fp);

    direntry_t entries[255];
    fread(entries, sizeof(direntry_t), 255, fp);
    int dir_index = 0;

    for (int i = 0; i < 255; i++)
    {
        if (entries[i].filename[0] == '\0')
        {
            dir_index = i;
            break;
        }
    }

    uint16_t data_len = sb.bytes_per_block - 3;
    uint32_t blocks_needed = (uint32_t)((input_size + data_len - 1) / data_len);

    uint16_t *free_blocks = (uint16_t *)malloc(sizeof(uint16_t) * blocks_needed);
    uint32_t found = 0;
    long data_start = (long)sizeof(superblock_t) + (long)sizeof(entries);

    for (uint16_t i = 0; i < sb.total_blocks && found < blocks_needed; i++)
    {
        long off = data_start + (long)i * sb.bytes_per_block;
        fseek(fp, off, SEEK_SET);
        uint8_t flag = 0;
        fread(&flag, 1, 1, fp);

        if (flag == 1)
        {
            free_blocks[found++] = i;
        }
    }

    uint8_t *buf = (uint8_t *)malloc(data_len);

    long remaining = input_size;

    for (uint32_t idx = 0; idx < blocks_needed; idx++)
    {
        uint16_t block = free_blocks[idx];
        long off = block_offset(&sb, block);
        fseek(fp, off, SEEK_SET);
        uint8_t busy = 0;
        fwrite(&busy, 1, 1, fp);

        size_t to_read = (size_t)((remaining > data_len) ? data_len : remaining);
        size_t read = fread(buf, 1, to_read, in);
        fwrite(buf, 1, read, fp);

        if (read < data_len)
        {
            uint8_t zero = 0;
            for (size_t p = read; p < data_len; p++)
            {
                fwrite(&zero, 1, 1, fp);
            }
        }

        fseek(fp, off + sb.bytes_per_block - 2, SEEK_SET);
        uint16_t next = (idx + 1 < blocks_needed) ? free_blocks[idx + 1] : 0;
        fwrite(&next, sizeof(uint16_t), 1, fp);

        if (remaining > data_len)
        {
            remaining -= data_len;
        }
        else
        {
            remaining = 0;
        }
    }

    const char *base = argv[2];
    const char *slash = strrchr(argv[2], '\\');
    if (slash && *(slash + 1) != '\0')
    {
        base = slash + 1;
    }
    const char *fslash = strrchr(base, '/');
    if (fslash && *(fslash + 1) != '\0')
    {
        base = fslash + 1;
    }

    direntry_t new_entry;
    memset(&new_entry, 0, sizeof(new_entry));
    strncpy(new_entry.filename, base, sizeof(new_entry.filename) - 1);
    new_entry.permissions = 0;
    new_entry.owner_id = 0;
    new_entry.group_id = 0;
    new_entry.starting_block = free_blocks[0];
    new_entry.file_size = (uint32_t)input_size;

    sb.available_blocks = (uint16_t)(sb.available_blocks - blocks_needed);
    if (sb.available_direntries > 0)
    {
        sb.available_direntries = (uint8_t)(sb.available_direntries - 1);
    }

    fseek(fp, 0, SEEK_SET);
    fwrite(&sb, sizeof(superblock_t), 1, fp);

    long dir_off = (long)sizeof(superblock_t) + (long)dir_index * sizeof(direntry_t);
    fseek(fp, dir_off, SEEK_SET);
    fwrite(&new_entry, sizeof(direntry_t), 1, fp);

    free(buf);
    free(free_blocks);
    fclose(in);
    fclose(fp);

    return 0;
}
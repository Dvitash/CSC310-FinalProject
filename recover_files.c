/* date: 11/12/25, authors: daniel vitashkevich, grant foody, purpose: recover jpgs from qfs data blocks */
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

    uint16_t data_len = sb.bytes_per_block - 3;
    uint8_t *buf = (uint8_t *)malloc(data_len);
    int file_idx = 1;
    int in_file = 0;
    FILE *out = NULL;
    uint8_t prev = 0;
    int prev_set = 0;

    for (uint16_t i = 0; i < sb.total_blocks; i++)
    {
        long off = block_offset(&sb, i);
        fseek(fp, off + 1, SEEK_SET);
        size_t read = fread(buf, 1, data_len, fp);

        for (size_t j = 0; j < read; j++)
        {
            uint8_t b = buf[j];
            if (!in_file)
            {
                if (prev_set && prev == 0xFF && b == 0xD8)
                {
                    char name[64];
                    snprintf(name, sizeof(name), "recovered_file_%d.jpg", file_idx++);
                    out = fopen(name, "wb");

                    fwrite(&prev, 1, 1, out);
                    fwrite(&b, 1, 1, out);

                    in_file = 1;
                    prev_set = 0;

                    continue;
                }

                prev = b;
                prev_set = 1;
            }
            else
            {
                if (prev_set && prev == 0xFF && b == 0xD9)
                {
                    fwrite(&prev, 1, 1, out);
                    fwrite(&b, 1, 1, out);
                    fclose(out);
                    out = NULL;
                    in_file = 0;
                    prev_set = 0;
                    continue;
                }

                if (prev_set)
                {
                    fwrite(&prev, 1, 1, out);
                }

                prev = b;
                prev_set = 1;
            }
        }
    }

    if (in_file)
    {
        if (prev_set)
        {
            fwrite(&prev, 1, 1, out);
        }

        fclose(out);
    }

    free(buf);
    fclose(fp);
    return 0;
}
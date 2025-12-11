/* date: 11/12/25, authors: daniel vitashkevich, grant foody, purpose: list qfs metadata and directory */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "qfs.h"

int main(int argc, char *argv[])
{
    FILE *fp = fopen(argv[1], "rb");

    superblock_t sb;
    fseek(fp, 0, SEEK_SET);
    fread(&sb, sizeof(superblock_t), 1, fp);

    direntry_t entries[255];
    fread(entries, sizeof(direntry_t), 255, fp);

    uint16_t free_blocks = 0;
    long data_start = (long)sizeof(superblock_t) + (long)sizeof(entries);
    uint16_t data_per_block = sb.bytes_per_block;
    for (uint16_t i = 0; i < sb.total_blocks; i++)
    {
        long off = data_start + (long)i * data_per_block;
        fseek(fp, off, SEEK_SET);
        uint8_t flag = 0;
        fread(&flag, 1, 1, fp);
        if (flag == 1)
        {
            free_blocks++;
        }
    }

    uint8_t free_dir = 0;
    for (int i = 0; i < 255; i++)
    {
        if (entries[i].filename[0] == '\0')
        {
            free_dir++;
        }
    }

    printf("Block size: %u\n", sb.bytes_per_block);
    printf("Total blocks: %u\n", sb.total_blocks);
    printf("Free blocks: %u\n", free_blocks);
    printf("Total directory entries: %u\n", sb.total_direntries);
    printf("Free directory entries: %u\n", free_dir);

    for (int i = 0; i < 255; i++)
    {
        if (entries[i].filename[0] != '\0')
        {
            printf("%s %u %u\n", entries[i].filename, entries[i].file_size, entries[i].starting_block);
        }
    }

    fclose(fp);
    return 0;
}
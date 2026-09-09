#include "sys/disk/disk.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>

int disk_init()
{
    FILE *cfg = fopen("config.cfg", "r");
    if (!cfg) {
        fprintf(stderr, "Could not read config.cfg. Make sure that you are running from the correct directory...\n");
        return -1;
    }

    char disk_name[256];
    fgets(disk_name, 256, cfg); // ignore one line... we'll catch any errors next

    if (strlcpy(disk_name, "sys/", (size_t) 5) != 4) {
        fprintf(stderr, "Could not setup buffer for reading diskname\n");
        return -1;
    }

    if (fscanf(cfg, "diskname : %s", &disk_name[4]) != 1) {
        fprintf(stderr, "Error reading disk name...\n");
        return -1;
    }

    if (fclose(cfg) == EOF) {
        fprintf(stderr, "Error closing config");
    }

    disk_head = open(disk_name, O_RDONLY);
    disk_pos = 0;

    return 0;
}

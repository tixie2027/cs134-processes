#include "sys/cpu/cpu.h"
#include "sys/disk/disk.h"
#include "sys/mem/mem.h"
#include "kernel/kmain.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int
main()
{
    printf("Welcome to your CS134 OS!\n");

    // open log file and notify user where to find output
    FILE *cfg = fopen("config.cfg", "r");
    if (!cfg) {
        fprintf(stderr, "Could not read config.cfg. Make sure that you are running from the correct directory...\n");
        return -1;
    }

    char log_filename[256] = "logs/";
    if (fscanf(cfg, "logname : %s", &log_filename[5]) != 1) {
        fprintf(stderr, "Error reading log file name\n");
        return -1;
    }

    if (fclose(cfg) == EOF) {
        fprintf(stderr, "Error closing config");
        return -1;
    }

    FILE *log = fopen(log_filename, "w");
    if (!log) {
        fprintf(stderr, "Could not open output log...\n");
        return -1;
    }

    fprintf(log, "\nWaking things up...\n");

    fprintf(log, "... turning on the disk...  ");
    if (disk_init() != 0) {
        fprintf(stderr, "Error opening file for simulated disk... exiting!\n");
        return -1;
    }
    fprintf(log, "Success!\n");

    fprintf(log, "... booting up memory...  ");
    if (mem_init() != 0) {
        fprintf(stderr, "Error allocating memory for simulated memory... exiting!\n");
        return -1;
    }
    fprintf(log, "Success!\n");


    fprintf(log, "... booting up cpu...  ");
    pthread_t *cores;
    int num_cores;
    if (cpu_init(&cores, &num_cores) != 0) {
        fprintf(stderr, "Error starting threads for simulated cpu... exiting!\n");
        return -1;
    }
    fprintf(log, "Success!\n");


    fprintf(log, "... executing kernel...  ");

    for (int i = 0; i < num_cores; i++) {
        pthread_join(cores[i], NULL);
        free(&cores[i]);
    }

    return 0;
}

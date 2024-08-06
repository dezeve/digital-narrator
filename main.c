#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/statvfs.h>

#define CPU_STAT_FILE "/proc/stat"
#define MEM_STAT_FILE "/proc/meminfo"

typedef struct {
	unsigned long long user;
	unsigned long long nice;
	unsigned long long system;
	unsigned long long idle;
} cpu_stat_t;

static cpu_stat_t previous_stat, current_stat;

static void read_cpu_stat(cpu_stat_t *stat) {
	FILE *pf = fopen(CPU_STAT_FILE, "r");
	if(!pf) {
		perror("fopen");
		exit(1);
	}
	char line[128];
	if(fgets(line, sizeof(line), pf)) {
		sscanf(line, "cpu %llu %llu %llu %llu",
				&stat->user,
				&stat->nice,
				&stat->system,
				&stat->idle);
	}
	fclose(pf);
}

float get_ram_usage() {
	FILE *pf = fopen(MEM_STAT_FILE, "r");
	if(!pf) {
		perror("fopen");
		return -1.0;
	}
	char line[1024];
	long total_ram = 0;
	long free_ram = 0;
	while(fgets(line, sizeof(line), pf)){
		if(strncmp(line, "MemTotal:", 9) == 0){
			total_ram = atol(line + 9);
		} else if(strncmp(line, "MemFree:", 8) == 0){
			free_ram = atol(line + 8);
		}
		if(total_ram > 0 && free_ram > 0) {
			break;
		}
	}
	fclose(pf);
	if(total_ram > 0 && free_ram > 0){
		long used_ram = total_ram - free_ram;
		return (float)used_ram / total_ram * 100;
	} else {
		return -1.0;
	}
}

float get_disk_usage(const char *mount_point){
	struct statvfs fs;
	int result = statvfs(mount_point, &fs);
	if(result != 0){
		perror("statvfs");
		return -1.0;
	}
	unsigned long total_blocks = fs.f_blocks;
	unsigned long free_blocks = fs.f_bfree;
	float usage_percentage =
		(1 - (float)free_blocks / total_blocks) * 100;
	return usage_percentage;
}

static float calculate_cpu_usage(cpu_stat_t *previous,
		cpu_stat_t *current) {
	unsigned long long total_previous = 
		previous->user +
		previous->nice +
		previous->system +
		previous->idle;
	unsigned long long total_current = 
		current->user +
		current->nice +
		current->system +
		current->idle;
	unsigned long long idle_difference = current->idle - previous->idle;
	unsigned long long total_difference = total_current - total_previous;
	
	if(total_difference == 0) {
		return 0.0;
	}

	return (1.0 - (idle_difference / (float)total_difference)) * 100.0;
}

int main() {
	while (1) {
		read_cpu_stat(&previous_stat);
		sleep(1);
		read_cpu_stat(&current_stat);
		float cpu_usage =
			calculate_cpu_usage(&previous_stat, &current_stat);
		float ram_usage = get_ram_usage();
		float disk_usage = get_disk_usage("/");
		printf("\rCpu %.2f%% | Ram %.2f%% | Disk %.2f%%",
				cpu_usage, ram_usage, disk_usage);
		fflush(stdout);
	}
	return 0;
}

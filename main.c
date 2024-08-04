#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CPU_STAT_FILE "/proc/stat"

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
	read_cpu_stat(&previous_stat);
	sleep(1);
	read_cpu_stat(&current_stat);
	float cpu_usage = calculate_cpu_usage(&previous_stat, &current_stat);
	printf("Cpu Usage=\t%.2f%%\n", cpu_usage);
	return 0;
}


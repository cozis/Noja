#ifndef TIMING_H
#define TIMING_H

#include <stddef.h>
#include "../utils/source.h"

typedef struct {
	size_t calls;
	double time;
	size_t line;
	char  *name;
	Source *src;
} FunctionExecutionSummary;

typedef int TimingID;
typedef struct TimingTable TimingTable;
TimingTable *TimingTable_new();
void         TimingTable_free(TimingTable *table);
TimingID     TimingTable_newEntry(TimingTable *table, Source *src, size_t line, const char *name);
void         TimingTable_sumCallTime(TimingTable *table, TimingID id, double time);
TimingTable *TimingTable_getDefaultTable();
const FunctionExecutionSummary *TimingTable_getSummary(TimingTable *table, size_t *count);
#endif
#include <stdlib.h>
#include <string.h>
#include "timing.h"
#include "utils/defs.h"

struct TimingTable {
	FunctionExecutionSummary *entries;
	size_t size, used;
};

TimingTable *TimingTable_new()
{
	TimingTable *table = malloc(sizeof(TimingTable));
	if (table == NULL)
		return NULL;
	table->entries = NULL;
	table->size = 0;
	table->used = 0;
	return table;
}

void TimingTable_free(TimingTable *table)
{
	if (table != NULL) {
		for(size_t i = 0; i < table->used; i++) {
			free(table->entries[i].name);
			Source_Free(table->entries[i].src);
		}
		free(table->entries);
		free(table);
	}
}

void TimingTable_sumCallTime(TimingTable *table, TimingID id, double time)
{
	if (id < 0 || id >= (int) table->used)
		abort();
	
	table->entries[id].calls++;
	table->entries[id].time += time;
}

TimingID TimingTable_newEntry(TimingTable *table, Source *src, size_t line, const char *name)
{
	if (table->size == table->used) {
		
		size_t new_size = 2 * table->size;
		if (new_size == 0) new_size = 8;

		void *temp = realloc(table->entries, new_size * sizeof(FunctionExecutionSummary));
		if (temp == NULL) abort();
		
		table->entries = temp;
		table->size = new_size;
	}
	
	char *name2 = strdup(name);
	if (name2 == NULL)
		abort();
	
	TimingID id = (int) table->used++;
	table->entries[id].src = Source_Copy(src);
	table->entries[id].line = line;
	table->entries[id].name = name2;
	table->entries[id].time = 0;
	table->entries[id].calls = 0;
	return id;
}

const FunctionExecutionSummary*
TimingTable_getSummary(TimingTable *table, size_t *count)
{
	ASSERT(table != NULL && count != NULL);
	*count = table->used;
	return table->entries;
}
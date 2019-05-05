#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include "cachelab.h"


typedef struct {
  bool valid;
  int tag;
  int timestamp;
} line_t;

typedef struct {
  line_t *lines;
} set_t;

typedef struct {
  set_t *sets;
  size_t set_num;
  size_t line_num;
} cache_t;

cache_t cache = {};
int set_bits = 0, block_bits = 0;
size_t hits = 0, misses = 0, evictions = 0;


void simulate(int addr);
int main(int argc, char *argv[]) {
  FILE *file = 0;
  for (int opt; (opt = getopt(argc, argv, "s:E:b:t:")) != -1;) {
    switch (opt) {
      case 's':
        set_bits = atoi(optarg);
        cache.set_num = 2 << set_bits;
        break;
      case 'E':
        cache.line_num = atoi(optarg);
        break;
      case 'b':
        block_bits = atoi(optarg);
        break;
      case 't':
        if (!(file = fopen(optarg, "r"))) { return 1; }
        break;
      default:
        return 1;
    }
  }
  if (!set_bits || !cache.line_num || !block_bits || !file) { return 1; }

  cache.sets = malloc(sizeof(set_t) * cache.set_num);
  for (int i = 0; i < cache.set_num; ++i) {
    cache.sets[i].lines = calloc(sizeof(line_t), cache.line_num);
  }

  char kind;
  int addr;

  while (fscanf(file, " %c %x%*c%*d", &kind, &addr) != EOF) {
    if (kind == 'I') { continue; }

    simulate(addr);
    if ('M' == kind) { simulate(addr); }
  }
  printSummary(hits, misses, evictions);

  fclose(file);
  for (size_t i = 0; i < cache.set_num; ++i) { free(cache.sets[i].lines); }
  free(cache.sets);
  return 0;
}


void update(set_t *set, size_t line_no);
void simulate(int addr) {
  size_t set_index = (0x7fffffff >> (31 - set_bits)) & (addr >> block_bits);
  int tag = 0xffffffff & (addr >> (set_bits + block_bits));

  set_t *set = &cache.sets[set_index];

  for (size_t i = 0; i < cache.line_num; ++i) {
    line_t* line = &set->lines[i];

    if (!line->valid) { continue; }
    if (line->tag != tag) { continue; }

    ++hits;
    update(set, i);
    return;
  }
  ++misses;

  for (size_t i = 0; i < cache.line_num; ++i) {
    line_t* line = &set->lines[i];

    if (line->valid) { continue; }

    line->valid = true;
    line->tag = tag;
    update(set, i);
    return;
  }

  ++evictions;

  for (size_t i = 0; i < cache.line_num; ++i) {
    line_t* line = &set->lines[i];

    if (line->timestamp) { continue; }

    line->valid = true;
    line->tag = tag;
    update(set, i);
    return;
  }
}


void update(set_t *set, size_t line_no) {
  line_t *line = &set->lines[line_no];

  for (size_t i = 0; i < cache.line_num; ++i) {
    line_t *it = &set->lines[i];
    if (!it->valid) { continue; }
    if (it->timestamp <= line->timestamp) { continue; }

    --it->timestamp;
  }

  line->timestamp = cache.line_num - 1;
}

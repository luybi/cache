#ifndef CACHE_SIMULATOR_H
#define CACHE_SIMULATOR_H

// Estruturas de dados
typedef struct {
    int valid;        // Bit que indica se a linha é válida
    unsigned int tag; // Tag para identificar os dados
    int last_used;    // Para política LRU
    int load_time;    // Para política FIFO
} CacheLine;

typedef struct {
    CacheLine *lines; // Linhas associadas ao conjunto
    int num_lines;    // Número de linhas por conjunto (associatividade)
} CacheSet;

typedef struct {
    CacheSet *sets;    // Conjuntos na cache
    int num_sets;      // Número total de conjuntos
    int block_size;    // Tamanho do bloco em bytes
    int assoc;         // Grau de associatividade
    char replacement_policy; // Política de substituição ('R', 'F', 'L')
} Cache;

// Funções
Cache* init_cache(int num_sets, int block_size, int assoc, char replacement_policy);
int access_cache(Cache *cache, unsigned int address, int *hit, int *miss, int *miss_comp, int *miss_conf, int *miss_cap);
void read_address_file(const char *filename, Cache *cache);
void replace_line(Cache *cache, int index, unsigned int tag);
void print_stats(int total_accesses, int hit, int miss, int miss_comp, int miss_conf, int miss_cap, int flagOut);

#endif

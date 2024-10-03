#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "cache_simulator.h"

// Função de inicialização da cache
Cache* init_cache(int num_sets, int block_size, int assoc, char replacement_policy) {
    Cache *cache = (Cache*) malloc(sizeof(Cache));
    cache->num_sets = num_sets;
    cache->block_size = block_size;
    cache->assoc = assoc;
    cache->replacement_policy = replacement_policy;
    cache->sets = (CacheSet*) malloc(num_sets * sizeof(CacheSet));

    for (int i = 0; i < num_sets; i++) {
        cache->sets[i].lines = (CacheLine*) malloc(assoc * sizeof(CacheLine));
        for (int j = 0; j < assoc; j++) {
            cache->sets[i].lines[j].valid = 0;
            cache->sets[i].lines[j].last_used = 0;
            cache->sets[i].lines[j].load_time = 0;
        }
    }
    return cache;
}

// Função de simulação de acesso ao cache
int access_cache(Cache *cache, unsigned int address, int *hit, int *miss, int *miss_comp, int *miss_conf, int *miss_cap) {
    unsigned int block_offset = address % cache->block_size;
    unsigned int index = (address / cache->block_size) % cache->num_sets;
    unsigned int tag = (address / cache->block_size) / cache->num_sets;

    CacheSet *set = &cache->sets[index];
    int found_free_line = 0;   // Indica se há uma linha livre no conjunto

    // Verifica por hits
    for (int i = 0; i < cache->assoc; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            *hit += 1;
            set->lines[i].last_used = 0; // Para LRU
            return 1;  // Cache hit
        }
    }

    // Caso contrário, é um miss
    *miss += 1;

    // Verifica se o miss é compulsório ou de conflito/capacidade
    for (int i = 0; i < cache->assoc; i++) {
        if (!set->lines[i].valid) {
            *miss_comp += 1;   // Miss compulsório: a linha não foi preenchida antes
            found_free_line = 1;
            break;
        }
    }

    if (!found_free_line) {
        // Se não houver linhas livres, classifica o miss como de conflito ou capacidade
        if (cache->assoc == 1) {
            *miss_cap += 1;  // Para caches diretamente mapeadas, todos os misses adicionais são de capacidade
        } else {
            *miss_conf += 1;  // Para caches associativas, são de conflito
        }
    }

    // Substituição de linha (Random, FIFO, LRU)
    replace_line(cache, index, tag);

    return 0;  // Cache miss
}

// Função para substituir linha no cache
void replace_line(Cache *cache, int index, unsigned int tag) {
    CacheSet *set = &cache->sets[index];
    int line_to_replace = 0;

    // Implementar política de substituição
    if (cache->replacement_policy == 'R') {
        // Randômica
        line_to_replace = rand() % cache->assoc;
    } else if (cache->replacement_policy == 'F') {
        // FIFO
        int oldest = set->lines[0].load_time;
        for (int i = 1; i < cache->assoc; i++) {
            if (set->lines[i].load_time < oldest) {
                oldest = set->lines[i].load_time;
                line_to_replace = i;
            }
        }
    } else if (cache->replacement_policy == 'L') {
        // LRU
        int lru = set->lines[0].last_used;
        for (int i = 1; i < cache->assoc; i++) {
            if (set->lines[i].last_used > lru) {
                lru = set->lines[i].last_used;
                line_to_replace = i;
            }
        }
    }

    // Substitui a linha
    set->lines[line_to_replace].valid = 1;
    set->lines[line_to_replace].tag = tag;
    set->lines[line_to_replace].load_time = clock();  // Atualiza o tempo de carregamento
    set->lines[line_to_replace].last_used = 0;        // Atualiza o uso para LRU
}

// Função para ler arquivo de endereços e simular acessos
void read_address_file(const char *filename, Cache *cache) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Erro ao abrir o arquivo de entrada.\n");
        exit(1);
    }

    unsigned int address;
    int hit = 0, miss = 0, miss_comp = 0, miss_conf = 0, miss_cap = 0;
    int total_accesses = 0;
    
    while (fread(&address, sizeof(unsigned int), 1, file)) {
        address = __builtin_bswap32(address);  // Converter big endian para little endian
        access_cache(cache, address, &hit, &miss, &miss_comp, &miss_conf, &miss_cap);
        total_accesses++;
    }

    fclose(file);
    print_stats(total_accesses, hit, miss, miss_comp, miss_conf, miss_cap, 1);
}

// Função para imprimir as estatísticas do cache
void print_stats(int total_accesses, int hit, int miss, int miss_comp, int miss_conf, int miss_cap, int flagOut) {
    float hit_rate = (float) hit / total_accesses;
    float miss_rate = (float) miss / total_accesses;
    float comp_rate = miss > 0 ? (float) miss_comp / miss : 0;
    float conf_rate = miss > 0 ? (float) miss_conf / miss : 0;
    float cap_rate = miss > 0 ? (float) miss_cap / miss : 0;

    if (flagOut == 0) {
        printf("Total de acessos: %d\n", total_accesses);
        printf("Taxa de hit: %.6f%%\n", hit_rate * 100);
        printf("Taxa de miss: %.6f%%\n", miss_rate * 100);
        printf("Misses compulsórios: %.6f%%\n", comp_rate * 100);
        printf("Misses de conflito: %.6f%%\n", conf_rate * 100);
        printf("Misses de capacidade: %.6f%%\n", cap_rate * 100);
    } else {
        // Formato padrão
        printf("%d, %.6f, %.6f, %.6f, %.6f, %.6f\n", total_accesses, hit_rate, miss_rate, comp_rate, conf_rate, cap_rate);
    }
}

// Função principal
int main( int argc, char *argv[] ) {
    if (argc != 7) {
        printf("Numero de argumentos incorreto. Utilize:\n");
        printf("./cache_simulator <nsets> <bsize> <assoc> <substituição> <flag_saida> arquivo_de_entrada\n");
        exit(EXIT_FAILURE);
    }

    int nsets = atoi(argv[1]);
    int bsize = atoi(argv[2]);
    int assoc = atoi(argv[3]);
    char subst = argv[4][0];  // Considera que apenas uma letra foi passada
    int flagOut = atoi(argv[5]);
    char *arquivoEntrada = argv[6];

    printf("nsets = %d\n", nsets);
    printf("bsize = %d\n", bsize);
    printf("assoc = %d\n", assoc);
        printf("subst = %c\n", subst);
    printf("flagOut = %d\n", flagOut);
    printf("arquivo = %s\n", arquivoEntrada);

    // Inicializa a cache com os parâmetros fornecidos
    Cache *cache = init_cache(nsets, bsize, assoc, subst);

    // Simula os acessos ao cache com o arquivo de endereços fornecido
    read_address_file(arquivoEntrada, cache);

    // Libera a memória alocada
    for (int i = 0; i < nsets; i++) {
        free(cache->sets[i].lines);
    }
    free(cache->sets);
    free(cache);

    return 0;
}


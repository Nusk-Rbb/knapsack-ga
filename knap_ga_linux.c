#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// ナップサック問題の定義
#define KNAP_CAPACITY 100  // ナップサックの容量
#define ITEM_NUM 10        // 品物の個数

// GAパラメータの定義
#define POP_SIZE 10               // 初期に生成する染色体の個数
#define S_POP_SIZE POP_SIZE / 2   // 選択で残す染色体の個数
#define GENE_LENGTH ITEM_NUM      // 染色体の長さ（遺伝子の数）
double ELITE_RATE = 0.0;          // エリート戦略で保存する割合
double MUTATION_RATE = 0.0;       // 突然変異確率

typedef struct {
    int value;   // 品物の価値
    int weight;  // 品物の重さ
} Item;

typedef struct {
    bool genes[GENE_LENGTH];
    int fitness;
} Chromosome;

typedef struct {
    Chromosome chromosomes[POP_SIZE];
} Population;

typedef struct {
    Chromosome chromosomes[S_POP_SIZE];
    int fitness;
} SelectedPopulation;

// プロトタイプ宣言
int initialize_population(Population *pop);
int calculate_fitness(Item items[ITEM_NUM], Population *pop);
int select_chromosomes(Population *pop, SelectedPopulation *selected_pop, 
                      Population *next_pop, int elite_count);
int perform_crossover(SelectedPopulation *selected_pop, Population *next_pop, int elite_count);
int perform_mutation(Population *next_pop, int elite_count);
int copy_population(Population *dest_pop, Population *src_pop);
int process_final_generation(Population *pop);
int sort_fitness(Population *current_pop, Population *sorted_pop);
int get_max(int a, int b);

// drand48関連
static long long rand_seed = 0x1234ABCD330E;
double drand48(void) {
    rand_seed = rand_seed * 0x5DEECE66D + 0xB;
    return (rand_seed & 0xFFFFFFFFFFFF) * (1.0 / 281474976710656.0);
}

void srand48(long seed) {
    rand_seed = seed;
    rand_seed = (rand_seed << 16) + 0x330E;
}

int main(int argc, char *argv[]) {
    Item items[ITEM_NUM] = {
        {10, 6}, {80, 30}, {25, 15}, {22, 18},
        {5, 10}, {75, 35}, {70, 35}, {60, 20},
        {30, 11}, {25, 30}
    };

    Population current_pop;
    Population next_pop;
    SelectedPopulation selected_pop;
    int generation_count;

    srand((unsigned)time(NULL));
    srand48((unsigned)time(NULL));

    if (argc == 4) {
        generation_count = atoi(argv[1]);
        ELITE_RATE = atof(argv[2]);
        MUTATION_RATE = atof(argv[3]);
    } else {
        printf("%s <generation count> <elite rate> <mutation rate>\n", argv[0]);
        exit(1);
    }

    int elite_count = (int)(POP_SIZE * ELITE_RATE + 0.5);

    initialize_population(&current_pop);
    printf("#generation %d, elite rate %lf, mutation rate %lf\n", 
           generation_count, ELITE_RATE, MUTATION_RATE);

    printf("#generation,best fitness,average,");
    for (int i = 0; i < ITEM_NUM; i++) {
        printf("%d,", i);
    }
    printf("\n");

    for (int gen = 0; gen < generation_count; gen++) {
        int max_fitness = calculate_fitness(items, &current_pop);

        printf("%d max,%d,", gen, max_fitness);
        for (int i = 0; i < POP_SIZE; i++) {
            printf("%4d,", current_pop.chromosomes[i].fitness);
        }
        printf("\n");

        select_chromosomes(&current_pop, &selected_pop, &next_pop, elite_count);
        perform_crossover(&selected_pop, &next_pop, elite_count);
        perform_mutation(&next_pop, elite_count);
        copy_population(&current_pop, &next_pop);
    }

    calculate_fitness(items, &current_pop);
    process_final_generation(&current_pop);

    return 0;
}

int initialize_population(Population *pop) {
    for (int chrom_idx = 0; chrom_idx < POP_SIZE; chrom_idx++) {
        for (int gene_idx = 0; gene_idx < GENE_LENGTH; gene_idx++) {
            pop->chromosomes[chrom_idx].genes[gene_idx] = (rand() % 2) == 1;
        }
    }

    for (int chrom_idx = 0; chrom_idx < POP_SIZE; chrom_idx++) {
        printf("%2d番目の染色体：", chrom_idx);
        for (int gene_idx = 0; gene_idx < GENE_LENGTH; gene_idx++) {
            printf("%d", pop->chromosomes[chrom_idx].genes[gene_idx]);
        }
        printf("\n");
    }
    return 0;
}

int calculate_fitness(Item items[ITEM_NUM], Population *pop) {
    int max_fitness = 0;

    for (int chrom_idx = 0; chrom_idx < POP_SIZE; chrom_idx++) {
        int total_weight = 0;
        int total_value = 0;

        for (int gene_idx = 0; gene_idx < GENE_LENGTH; gene_idx++) {
            if (pop->chromosomes[chrom_idx].genes[gene_idx]) {
                total_weight += items[gene_idx].weight;
            }
        }

        if (total_weight <= KNAP_CAPACITY) {
            for (int gene_idx = 0; gene_idx < GENE_LENGTH; gene_idx++) {
                if (pop->chromosomes[chrom_idx].genes[gene_idx]) {
                    total_value += items[gene_idx].value;
                }
            }
            pop->chromosomes[chrom_idx].fitness = total_value;
        } else {
            pop->chromosomes[chrom_idx].fitness = 1;
        }

        max_fitness = get_max(max_fitness, pop->chromosomes[chrom_idx].fitness);
    }

    return max_fitness;
}

int select_chromosomes(Population *pop, SelectedPopulation *selected_pop, 
                      Population *next_pop, int elite_count) {
    Population sorted_pop;
    sort_fitness(pop, &sorted_pop);

    // エリート選択
    for (int i = 0; i < elite_count; i++) {
        for (int gene_idx = 0; gene_idx < GENE_LENGTH; gene_idx++) {
            next_pop->chromosomes[i].genes[gene_idx] = sorted_pop.chromosomes[i].genes[gene_idx];
        }
    }

    // ルーレット選択
    for (int i = 0; i < S_POP_SIZE; i++) {
        int fitness_sum = 0;
        for (int j = 0; j < POP_SIZE; j++) {
            fitness_sum += pop->chromosomes[j].fitness;
        }

        double threshold = 0.0;
        double random_value = drand48();
        int selected_idx = 0;

        for (int j = 0; j < POP_SIZE; j++) {
            threshold += (double)pop->chromosomes[j].fitness / fitness_sum;
            if (random_value <= threshold) {
                selected_idx = j;
                break;
            }
        }

        for (int gene_idx = 0; gene_idx < GENE_LENGTH; gene_idx++) {
            selected_pop->chromosomes[i].genes[gene_idx] = 
                pop->chromosomes[selected_idx].genes[gene_idx];
        }
        pop->chromosomes[selected_idx].fitness = 0;
    }
    return 0;
}

int perform_crossover(SelectedPopulation *selected_pop, Population *next_pop, int elite_count) {
    int current_child = elite_count;

    while (current_child < POP_SIZE) {
        int parent1_idx, parent2_idx;
        do {
            parent1_idx = (int)(drand48() * S_POP_SIZE);
            parent2_idx = (int)(drand48() * S_POP_SIZE);
        } while (parent1_idx == parent2_idx);

        int cross_point1, cross_point2;
        do {
            cross_point1 = (int)(drand48() * (GENE_LENGTH + 1));
            cross_point2 = (int)(drand48() * (GENE_LENGTH + 1));
        } while (cross_point1 >= cross_point2);

        Chromosome child1, child2;

        // 子染色体の生成
        for (int i = 0; i < GENE_LENGTH; i++) {
            if (i >= cross_point1 && i < cross_point2) {
                child1.genes[i] = selected_pop->chromosomes[parent2_idx].genes[i];
                child2.genes[i] = selected_pop->chromosomes[parent1_idx].genes[i];
            } else {
                child1.genes[i] = selected_pop->chromosomes[parent1_idx].genes[i];
                child2.genes[i] = selected_pop->chromosomes[parent2_idx].genes[i];
            }
        }

        // 子を次世代に追加
        if (current_child < POP_SIZE) {
            for (int i = 0; i < GENE_LENGTH; i++) {
                next_pop->chromosomes[current_child].genes[i] = child1.genes[i];
            }
            current_child++;
        }

        if (current_child < POP_SIZE) {
            for (int i = 0; i < GENE_LENGTH; i++) {
                next_pop->chromosomes[current_child].genes[i] = child2.genes[i];
            }
            current_child++;
        }
    }
    return 0;
}

int perform_mutation(Population *next_pop, int elite_count) {
    for (int chrom_idx = elite_count; chrom_idx < POP_SIZE; chrom_idx++) {
        for (int gene_idx = 0; gene_idx < GENE_LENGTH; gene_idx++) {
            if (drand48() < MUTATION_RATE) {
                next_pop->chromosomes[chrom_idx].genes[gene_idx] = 
                    !next_pop->chromosomes[chrom_idx].genes[gene_idx];
            }
        }
    }
    return 0;
}

int copy_population(Population *dest_pop, Population *src_pop) {
    for (int chrom_idx = 0; chrom_idx < POP_SIZE; chrom_idx++) {
        for (int gene_idx = 0; gene_idx < GENE_LENGTH; gene_idx++) {
            dest_pop->chromosomes[chrom_idx].genes[gene_idx] = 
                src_pop->chromosomes[chrom_idx].genes[gene_idx];
        }
    }
    return 0;
}

int sort_fitness(Population *pop, Population *sorted_pop) {

    for (int sort_idx = 0; sort_idx < POP_SIZE; sort_idx++) {
        int max_value = pop->chromosomes[sort_idx].fitness;
        int max_position = 0;
        
        for (int search_idx = sort_idx + 1; search_idx < POP_SIZE; search_idx++) {
            if (max_value < pop->chromosomes[search_idx].fitness) {
                max_value = pop->chromosomes[search_idx].fitness;
                max_position = search_idx;
            }
        }
        sorted_pop->chromosomes[sort_idx] = pop->chromosomes[max_position];
    }
    return 0;
}

Chromosome get_best_chromosome(Population *pop) {
    int best_fitness = 0;
    int best_idx = 0;
    for (int i = 0; i < POP_SIZE; i++) {
        if (best_fitness < pop->chromosomes[i].fitness) {
            best_fitness = pop->chromosomes[i].fitness;
            best_idx = i;
        }
    }
    return pop->chromosomes[best_idx];
}

int process_final_generation(Population *pop) {
    for (int chrom_idx = 0; chrom_idx < POP_SIZE; chrom_idx++) {
        printf("%2d chromosome,", chrom_idx);
        for (int gene_idx = 0; gene_idx < GENE_LENGTH; gene_idx++) {
            printf("%d", (int)pop->chromosomes[chrom_idx].genes[gene_idx]);
        }
        printf(",%d\n", pop->chromosomes[chrom_idx].fitness);
    }
    return 0;
}

int get_max(int a, int b) {
    return (a > b) ? a : b;
}

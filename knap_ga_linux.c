#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// ===== ナップサック問題の定義 =====
#define KNAP_CAPACITY 100  // ナップサックの容量
#define ITEM_NUM 10        // 品物の個数

// ===== GAパラメータの定義 =====
#define POP_SIZE 10                 // 1世代の染色体数
#define S_POP_SIZE (POP_SIZE / 2)   // 選択（親プール）に残す染色体数
#define GENE_LENGTH ITEM_NUM        // 染色体の長さ（遺伝子の数）

// コマンドライン引数で上書きされる実行時パラメータ
double ELITE_RATE = 0.0;     // エリート戦略で保存する割合
double MUTATION_RATE = 0.0;  // 突然変異確率

// 容量オーバー個体に与えるペナルティ適応度（ルーレットでほぼ選ばれない）
#define PENALTY_FITNESS 0

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

// 選択された親候補のプール
typedef struct {
    Chromosome chromosomes[S_POP_SIZE];
} SelectedPopulation;

// ===== プロトタイプ宣言 =====
void initialize_population(Population *pop);
int  calculate_fitness(const Item items[ITEM_NUM], Population *pop);
double average_fitness(const Population *pop);
void sort_population_desc(const Population *pop, Population *sorted_pop);
void select_chromosomes(const Population *pop, SelectedPopulation *selected_pop,
                        Population *next_pop, int elite_count);
void perform_crossover(const SelectedPopulation *selected_pop, Population *next_pop,
                       int elite_count);
void perform_mutation(Population *next_pop, int elite_count);
void print_final_generation(const Population *pop);

// ===== 簡易乱数生成器（drand48相当のLCG） =====
// 標準ライブラリの drand48/srand48 と衝突しないよう独自名にしている。
static unsigned long long rng_state = 0x1234ABCD330EULL;

static void rng_seed(unsigned long seed) {
    rng_state = ((unsigned long long)seed << 16) + 0x330EULL;
}

// [0.0, 1.0) の一様乱数を返す
static double rng_next(void) {
    rng_state = rng_state * 0x5DEECE66DULL + 0xBULL;
    return (rng_state & 0xFFFFFFFFFFFFULL) * (1.0 / 281474976710656.0);
}

// [0, n) の整数乱数を返す
static int rng_below(int n) {
    return (int)(rng_next() * n);
}

int main(int argc, char *argv[]) {
    Item items[ITEM_NUM] = {
        {10, 6}, {80, 30}, {25, 15}, {22, 18},
        {5, 10}, {75, 35}, {70, 35}, {60, 20},
        {30, 11}, {25, 30}
    };

    if (argc != 4) {
        printf("usage: %s <generation count> <elite rate> <mutation rate>\n", argv[0]);
        return 1;
    }
    int generation_count = atoi(argv[1]);
    ELITE_RATE = atof(argv[2]);
    MUTATION_RATE = atof(argv[3]);

    rng_seed((unsigned long)time(NULL));

    // エリート数を [0, POP_SIZE] に収める
    int elite_count = (int)(POP_SIZE * ELITE_RATE + 0.5);
    if (elite_count < 0) elite_count = 0;
    if (elite_count > POP_SIZE) elite_count = POP_SIZE;

    Population current_pop;
    Population next_pop;
    SelectedPopulation selected_pop;

    initialize_population(&current_pop);

    printf("#generation %d, elite rate %f, mutation rate %f\n",
           generation_count, ELITE_RATE, MUTATION_RATE);

    // CSVヘッダー（各列の意味とデータ行を一致させる）
    printf("#generation,best,average");
    for (int i = 0; i < POP_SIZE; i++) {
        printf(",chrom%d", i);
    }
    printf("\n");

    for (int gen = 0; gen < generation_count; gen++) {
        int max_fitness = calculate_fitness(items, &current_pop);
        double avg_fitness = average_fitness(&current_pop);

        printf("%d,%d,%.2f", gen, max_fitness, avg_fitness);
        for (int i = 0; i < POP_SIZE; i++) {
            printf(",%d", current_pop.chromosomes[i].fitness);
        }
        printf("\n");

        select_chromosomes(&current_pop, &selected_pop, &next_pop, elite_count);
        perform_crossover(&selected_pop, &next_pop, elite_count);
        perform_mutation(&next_pop, elite_count);

        current_pop = next_pop;  // 次世代へ更新
    }

    calculate_fitness(items, &current_pop);
    print_final_generation(&current_pop);

    return 0;
}

// 集団をランダムな遺伝子で初期化し、内容を表示する
void initialize_population(Population *pop) {
    for (int c = 0; c < POP_SIZE; c++) {
        for (int g = 0; g < GENE_LENGTH; g++) {
            pop->chromosomes[c].genes[g] = (rng_below(2) == 1);
        }
        pop->chromosomes[c].fitness = 0;
    }

    for (int c = 0; c < POP_SIZE; c++) {
        printf("#%2d番目の染色体：", c);
        for (int g = 0; g < GENE_LENGTH; g++) {
            printf("%d", pop->chromosomes[c].genes[g]);
        }
        printf("\n");
    }
}

// 各染色体の適応度を計算し、集団中の最大適応度を返す
int calculate_fitness(const Item items[ITEM_NUM], Population *pop) {
    int max_fitness = 0;

    for (int c = 0; c < POP_SIZE; c++) {
        int total_weight = 0;
        int total_value = 0;

        for (int g = 0; g < GENE_LENGTH; g++) {
            if (pop->chromosomes[c].genes[g]) {
                total_weight += items[g].weight;
                total_value += items[g].value;
            }
        }

        // 容量を超えた解は無効としてペナルティを与える
        pop->chromosomes[c].fitness =
            (total_weight <= KNAP_CAPACITY) ? total_value : PENALTY_FITNESS;

        if (pop->chromosomes[c].fitness > max_fitness) {
            max_fitness = pop->chromosomes[c].fitness;
        }
    }

    return max_fitness;
}

// 集団の平均適応度を返す
double average_fitness(const Population *pop) {
    long sum = 0;
    for (int c = 0; c < POP_SIZE; c++) {
        sum += pop->chromosomes[c].fitness;
    }
    return (double)sum / POP_SIZE;
}

// 適応度の降順に並べ替えた集団を sorted_pop に作る（選択ソート）
void sort_population_desc(const Population *pop, Population *sorted_pop) {
    *sorted_pop = *pop;  // まるごとコピーしてから並べ替える

    for (int i = 0; i < POP_SIZE - 1; i++) {
        int max_pos = i;
        for (int j = i + 1; j < POP_SIZE; j++) {
            if (sorted_pop->chromosomes[j].fitness >
                sorted_pop->chromosomes[max_pos].fitness) {
                max_pos = j;
            }
        }
        if (max_pos != i) {
            Chromosome tmp = sorted_pop->chromosomes[i];
            sorted_pop->chromosomes[i] = sorted_pop->chromosomes[max_pos];
            sorted_pop->chromosomes[max_pos] = tmp;
        }
    }
}

// エリート選択 + ルーレット選択で親プールを構築する
void select_chromosomes(const Population *pop, SelectedPopulation *selected_pop,
                        Population *next_pop, int elite_count) {
    // --- エリート選択：上位 elite_count 個をそのまま次世代へ ---
    Population sorted_pop;
    sort_population_desc(pop, &sorted_pop);
    for (int i = 0; i < elite_count; i++) {
        next_pop->chromosomes[i] = sorted_pop.chromosomes[i];
    }

    // --- ルーレット選択：親プールを S_POP_SIZE 個構築する ---
    // 元の集団を壊さないよう、適応度を作業用配列にコピーして
    // 選んだ個体は0にする（非復元抽出）。
    int fitness[POP_SIZE];
    for (int j = 0; j < POP_SIZE; j++) {
        fitness[j] = pop->chromosomes[j].fitness;
    }

    for (int i = 0; i < S_POP_SIZE; i++) {
        int fitness_sum = 0;
        for (int j = 0; j < POP_SIZE; j++) {
            fitness_sum += fitness[j];
        }

        int selected_idx;
        if (fitness_sum <= 0) {
            // 全員ペナルティ等で合計0なら一様ランダムに選ぶ（ゼロ除算回避）
            selected_idx = rng_below(POP_SIZE);
        } else {
            double random_value = rng_next();
            double threshold = 0.0;
            selected_idx = POP_SIZE - 1;  // 丸め誤差で選ばれなかった場合の保険
            for (int j = 0; j < POP_SIZE; j++) {
                threshold += (double)fitness[j] / fitness_sum;
                if (random_value <= threshold) {
                    selected_idx = j;
                    break;
                }
            }
        }

        selected_pop->chromosomes[i] = pop->chromosomes[selected_idx];
        fitness[selected_idx] = 0;  // 同じ個体を再選択しない
    }
}

// 親プールから二点交叉で子を作り、エリート以降を埋める
void perform_crossover(const SelectedPopulation *selected_pop, Population *next_pop,
                       int elite_count) {
    int child_idx = elite_count;

    while (child_idx < POP_SIZE) {
        // 異なる2つの親を選ぶ
        int parent1, parent2;
        do {
            parent1 = rng_below(S_POP_SIZE);
            parent2 = rng_below(S_POP_SIZE);
        } while (parent1 == parent2);

        // 交叉区間 [cross1, cross2) を決める
        int cross1, cross2;
        do {
            cross1 = rng_below(GENE_LENGTH + 1);
            cross2 = rng_below(GENE_LENGTH + 1);
        } while (cross1 >= cross2);

        // 区間内の遺伝子だけ親を入れ替えて2つの子を作る
        Chromosome child1, child2;
        for (int g = 0; g < GENE_LENGTH; g++) {
            bool swap = (g >= cross1 && g < cross2);
            child1.genes[g] = selected_pop->chromosomes[swap ? parent2 : parent1].genes[g];
            child2.genes[g] = selected_pop->chromosomes[swap ? parent1 : parent2].genes[g];
        }
        child1.fitness = 0;
        child2.fitness = 0;

        next_pop->chromosomes[child_idx++] = child1;
        if (child_idx < POP_SIZE) {
            next_pop->chromosomes[child_idx++] = child2;
        }
    }
}

// エリート以外の遺伝子をMUTATION_RATEの確率で反転させる
void perform_mutation(Population *next_pop, int elite_count) {
    for (int c = elite_count; c < POP_SIZE; c++) {
        for (int g = 0; g < GENE_LENGTH; g++) {
            if (rng_next() < MUTATION_RATE) {
                next_pop->chromosomes[c].genes[g] = !next_pop->chromosomes[c].genes[g];
            }
        }
    }
}

// 最終世代の各染色体（遺伝子と適応度）を表示する
void print_final_generation(const Population *pop) {
    for (int c = 0; c < POP_SIZE; c++) {
        printf("%2d chromosome,", c);
        for (int g = 0; g < GENE_LENGTH; g++) {
            printf("%d", pop->chromosomes[c].genes[g]);
        }
        printf(",%d\n", pop->chromosomes[c].fitness);
    }
}

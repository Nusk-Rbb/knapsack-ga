#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

// ナップサック問題の定義
#define KNAP_CAPA 100 // ナップサックの容量
#define ITEM_NUM 10   // 品物の個数

// GAパラメータの定義
#define POP_SIZE 10             // 初期に生成する染色体の個数
#define S_POP_SIZE POP_SIZE / 2 // 選択で残す染色体の個数
#define GENE_LENGTH ITEM_NUM    // 染色体の長さ（遺伝子の数）
double ELITE_COPY = 0.0;        // エリート戦略で保存する割合
double MUTE_RATE = 0.0;         // 突然変異確率

typedef struct {
    int value;    // 品物の価値
    int weight;   // 品物の重さ
} Item;

typedef struct {
    bool genes[GENE_LENGTH];
} Chromosome;

typedef struct {
    Chromosome chromosomes[POP_SIZE];
} GenePool;

typedef struct {
    Chromosome chromosomes[S_POP_SIZE];
} SelectGenePool;

typedef struct {
    int fitnessValues[POP_SIZE];
} FitnessValues;

typedef struct {
    double fitnessValues[POP_SIZE];
} DoubleFitnessValues;

// プロトタイプ宣言
int initialize_gene(GenePool *GENE);
int calc_fitness(Item ITEM[ITEM_NUM], GenePool *GENE, FitnessValues *FITNESS);
int selection(GenePool *GENE,
              SelectGenePool *SELECT_GENE,
              GenePool *NEXT_GENE, 
              FitnessValues *FITNESS, int e_num);
int crossover(SelectGenePool *SELECT_GENE,
              GenePool *NEXT_GENE, int e_num);
int mutation(GenePool *NEXT_GENE, int e_num);
int gene_copy(GenePool *GENE,
              GenePool *NEXT_GENE);
int after_proc(GenePool *GENE, FitnessValues *FITNESS);
int sort(FitnessValues *FITNESS, FitnessValues *SORT_FIT);
int cmp(int, int);

// drand48関連
static long long x = 0x1234ABCD330E;
double drand48() /* 0.0以上1.0未満 */
{
  x = x * 0x5DEECE66D + 0xB;
  return (x & 0xFFFFFFFFFFFF) * (1.0 / 281474976710656.0);
}
long lrand48() /* 0以上2147483647以下 */
{
  x = x * 0x5DEECE66D + 0xB;
  return (long)(x >> 17) & 0x7FFFFFFF;
}
long mrand48() /* -2147483648以上214748367以下 */
{
  x = x * 0x5DEECE66D + 0xB;
  return (long)(x >> 16) & 0xFFFFFFFF;
}
void srand48(long s) {
  x = s;
  x = (x << 16) + 0x330E;
}


/*---------------データ採取用の変数など---------------*/
int reset_data();

int main(int argc, char *argv[]) {

  Item ITEM[ITEM_NUM] = {{10, 6},  {80, 30}, {25, 15}, {22, 18},
                         {5, 10},  {75, 35}, {70, 35}, {60, 20},
                         {30, 11}, {25, 30}}; // 品物初期化(値段,重さ)

  GenePool GENE_POOL;                     // 現世代の染色体
  GenePool NEXT_GENE_POOL;                // 次世代の染色体
  SelectGenePool SELECTED_GENE_POOL;      // 選択された染色体
  FitnessValues FITNESS_VALUES;           // 適応度
  int GENERATION_NUM;                     // 終了世代数

  srand((unsigned)time(NULL));
  srand48((unsigned)time(NULL));

  // knap パラメータの読み込み
  if (argc == 4) {
    GENERATION_NUM = (int)atoi(argv[1]);
    ELITE_COPY = atof(argv[2]);
    MUTE_RATE = atof(argv[3]);

  } else {
    printf("%s <generation num> <elite copy> <mutation rate>\n", argv[0]);
    exit(1);
  }

  // エリート戦略で残す染色体の数を計算
  int e_num = (int)(POP_SIZE * ELITE_COPY + 0.5);

  initialize_gene(&GENE_POOL); // 初期母集団生成;
  printf("#generation %d, elite copy %lf, muterate %lf\n", GENERATION_NUM,
         ELITE_COPY, MUTE_RATE);

  printf("#generation,best fitness,average,");
  for (int j = 0; j < ITEM_NUM; j++) {
    printf("%d,", j);
  }
  printf("\n");

  int max_index = 0;
  for (int i = 0; i < GENERATION_NUM; i++) {
    int max = calc_fitness(ITEM, &GENE_POOL, &FITNESS_VALUES);
    max_index = i;

    // 世代数と最優良評価値の画面表示
    printf("%d max,%d,", i, max);
    for (int k = 0; k < POP_SIZE; k++) {
      printf("%4d,", FITNESS_VALUES.fitnessValues[k]);
    }
    printf("\n");

    selection(&GENE_POOL, &SELECTED_GENE_POOL, &NEXT_GENE_POOL, &FITNESS_VALUES, e_num);
    crossover(&SELECTED_GENE_POOL, &NEXT_GENE_POOL, e_num);
    mutation(&NEXT_GENE_POOL, e_num);
    gene_copy(&GENE_POOL, &NEXT_GENE_POOL);
    // after_proc(GENE,FITNESS);/////////
  }
  calc_fitness(ITEM, &GENE_POOL, &FITNESS_VALUES);
  after_proc(&GENE_POOL, &FITNESS_VALUES);

  return 0;
}

int cmp(int a, int b) {
  if(a > b) {
    return a;
  } else {
    return b;
  }
}

// 初期母集団生成
int initialize_gene(GenePool *GENE) {

  for (int i = 0; i < POP_SIZE; i++) {
    for(int j = 0; j < GENE_LENGTH; j++){
      int b = rand() % 2;
      bool unit_chromosome;
      if(b == 0){
        unit_chromosome = 0;
      } else {
        unit_chromosome = 1;
      }
      GENE->chromosomes[i].genes[j] = unit_chromosome;
    }
  }
  /*
  for(i=0;i<POP_SIZE;i++){
    printf("%2d番目の染色体：",i);
    for(j=0;j<GENE_LENGTH;j++){
      printf("%d",GENE[i][j]);  //染色体の遺伝子表示
    }
    printf("\n");
  }
*/

  return 0;
}

// 適応度計算
int calc_fitness(Item ITEM[ITEM_NUM], GenePool *GENE, FitnessValues *FITNESS) {

  int WEIGHT_SUM, VAL_SUM, MAX_FITNESS = 0;

  for (int i = 0; i < POP_SIZE; i++) {

    WEIGHT_SUM = 0;
    VAL_SUM = 0;

    // 入れた品物の重さの総和
    for (int j = 0; j < GENE_LENGTH; j++) {
      WEIGHT_SUM += ITEM[j].weight * (int)(GENE->chromosomes[i].genes[j]); 
    }

    // 入れた品物の重さが上限を超しているかの判定
    if (WEIGHT_SUM <= KNAP_CAPA) { 

      // 入れた品物の値段の総和
      for (int j = 0; j < GENE_LENGTH; j++) {
        VAL_SUM += ITEM[j].value * (int)(GENE->chromosomes[i].genes[j]);
      }
      FITNESS->fitnessValues[i] = VAL_SUM;
    } else {
      FITNESS->fitnessValues[i] = 1;
    }
    if (MAX_FITNESS < FITNESS->fitnessValues[i]) {
      MAX_FITNESS = FITNESS->fitnessValues[i];
    }
  }

  return MAX_FITNESS;
}

// 選択：エリート戦略とルーレット選択の併用（全染色体の半分を次世代に残す）
int selection(GenePool *GENE, SelectGenePool *SELECT_GENE,
              GenePool *NEXT_GENE, FitnessValues *FITNESS, int e_num) {
  int num_tmp;                   // 選ばれた染色体の番号を代入

  // エリート戦略用
  FitnessValues SORTED_FITNESS_POSITIONS; // 染色体の番号を、適応度で降順に並び替えたものを代入

  // ルーレット選択用
  FitnessValues THRESHOLD; // しきい値を代入
  double r;                  // ルーレットの出る値

  // エリート戦略で残す染色体の内1割の染色体を残す

  sort(FITNESS, &SORTED_FITNESS_POSITIONS); // 適応度を降順に並び替える

  for (int i = 0; i < e_num; i++) {
    num_tmp = SORTED_FITNESS_POSITIONS.fitnessValues[i]; // 適応度の上位から染色体の番号を代入

    for (int j = 0; j < GENE_LENGTH; j++) {
      NEXT_GENE->chromosomes[i].genes[j] = GENE->chromosomes[num_tmp].genes[j];
    }
    // FITNESS_COPY[num_tmp]=0.0; //エリートを交叉に参加させないための処理
  }

  // ルーレット選択で残りの染色体を選出；(S_POP_SIZE-e_num)個の染色体を選出

  // iはe_num~S_POP_SIZEでは？
  for (int i = 0; i < S_POP_SIZE; i++) {
    // 全染色体の適応度の総和
    int FIT_SUM = 0;

    for (int j = 0; j < POP_SIZE; j++) {
      FIT_SUM += FITNESS->fitnessValues[j]; // 適応度の総和を計算
    }

    for (int j = 0; j < POP_SIZE; j++) { // 初期化
      THRESHOLD.fitnessValues[j] = 0.0;
    }

    THRESHOLD.fitnessValues[0] = FITNESS->fitnessValues[0] / FIT_SUM;
    for (int j = 1; j < POP_SIZE; j++) {
      // しきい値（適応度の割合）を計算して代入
      THRESHOLD.fitnessValues[j] = THRESHOLD.fitnessValues[j - 1] + THRESHOLD.fitnessValues[j] / (double)(FIT_SUM); 
    }

    r = drand48(); // ルーレットの値を生成

    int k = 0;
    while (1) {
      if (r <= THRESHOLD.fitnessValues[k]) {
        break; // ルーレットの値が止まるところでbreak
      }
      k++;
    }
    num_tmp = k;
    for (int j = 0; j < GENE_LENGTH; j++) {
      NEXT_GENE->chromosomes[i].genes[j] = GENE->chromosomes[num_tmp].genes[j]; // 選ばれた染色体を次世代に残す
    }
    FITNESS->fitnessValues[num_tmp] = 0.0; // 染色体がかぶらないようにする
  }
  return 0;
}

/*
  交叉:2点交叉を使用、交叉点は乱数で決定、1回の交叉につき子が1体できる
  //採取２体の間違い? 選択された染色体からPOP_SIZE体の子を作成
*/
int crossover(SelectGenePool *SELECT_GENE, GenePool *NEXT_GENE, int e_num) {

  Chromosome CH_1, CH_2;
  int FATHER = 0, MOTHER = 0;
  int CROSS_POINT_1 = 0, CROSS_POINT_2 = 0;
  int r_tmp;

  int i = e_num;

  while (i < POP_SIZE) {

    // 親の決定
    do {
      r_tmp = drand48() * S_POP_SIZE;
      FATHER = r_tmp % S_POP_SIZE;

      r_tmp = drand48() * S_POP_SIZE;
      MOTHER = r_tmp % S_POP_SIZE;
    } while (FATHER == MOTHER);

    // 親の遺伝子を子の遺伝子にコピー
    for (int j = 0; j < GENE_LENGTH; j++) {
      CH_1.genes[j] = SELECT_GENE->chromosomes[FATHER].genes[j];
      CH_2.genes[j] = SELECT_GENE->chromosomes[MOTHER].genes[j];
    }

    // 交叉点を決める採取
    do {
      r_tmp = drand48() * (GENE_LENGTH + 1);
      CROSS_POINT_1 = r_tmp % (GENE_LENGTH + 1);

      r_tmp = drand48() * (GENE_LENGTH + 1);
      CROSS_POINT_2 = r_tmp % (GENE_LENGTH + 1);
    } while (CROSS_POINT_1 >= CROSS_POINT_2);

    // 子の遺伝子組み換え
    for (int j = CROSS_POINT_1; j < CROSS_POINT_2; j++) {
      CH_1.genes[j] = SELECT_GENE->chromosomes[FATHER].genes[j];
      CH_2.genes[j] = SELECT_GENE->chromosomes[MOTHER].genes[j];
    }

    // 子を次世代に残す
    for (int j = 0; j < GENE_LENGTH; j++) {
      NEXT_GENE->chromosomes[i].genes[j] = CH_1.genes[j];
    }
    i++;
    if (i >= POP_SIZE)
      break;

    for (int j = 0; j < GENE_LENGTH; j++) {
      NEXT_GENE->chromosomes[i].genes[j] = CH_2.genes[j];
    }
    i++;
  }
  return 0;
}

// 突然変異
// 1点突然変異を使用　確率で、染色体の持つ遺伝子の一箇所を反転、反転場所採取採取は乱数で決定
int mutation(GenePool *NEXT_GENE, int e_num) {

  int i, j, r_tmp;
  int MUTE_POINT;
  double r;

  for (i = e_num; i < POP_SIZE; i++) {
    for (j = 0; j < GENE_LENGTH; j++) {

      r = drand48();
      if (r < MUTE_RATE) {
        // 遺伝子を反転
        NEXT_GENE->chromosomes[i].genes[j] = !NEXT_GENE->chromosomes[i].genes[j];
      }
    }
  }
  return 0;
}

// NEXT_GENEの中身を全てGENEに移す
int gene_copy(GenePool *GENE, GenePool *NEXT_GENE) {

  int i, j;

  for (i = 0; i < POP_SIZE; i++) {
    for (j = 0; j < GENE_LENGTH; j++) {
      GENE->chromosomes[i].genes[j] = NEXT_GENE->chromosomes[i].genes[j];
    }
  }

  return 0;
}

// ソート
int sort(FitnessValues *FITNESS, FitnessValues *SORTED_FITNESS_POSITIONS) {
  FitnessValues FITNESS_COPY; // int型でFITNESSのコピーを作成

  // コピー
  for (int i = 0; i < POP_SIZE; i++) {
    FITNESS_COPY.fitnessValues[i] = FITNESS->fitnessValues[i];
  }

  for (int i = 0; i < POP_SIZE; i++) {
    int maxValue = FITNESS_COPY.fitnessValues[i];
    int maxPosition = i;
    for (int j = i + 1; j < POP_SIZE; j++) {
      if (maxValue < FITNESS_COPY.fitnessValues[j]) { // 降順
        maxValue = FITNESS_COPY.fitnessValues[j];
        maxPosition = j;
      }
    }
    FITNESS_COPY.fitnessValues[maxPosition] = FITNESS_COPY.fitnessValues[i];
    FITNESS_COPY.fitnessValues[i] = maxValue;
    SORTED_FITNESS_POSITIONS->fitnessValues[i] = maxPosition;
  }
  return 0;
}

// 最終世代の画面表示
int after_proc(GenePool *GENE, FitnessValues *FITNESS) {

  int i, j;

  for (i = 0; i < POP_SIZE; i++) {
    printf("%2d chromosome,", i);
    for (j = 0; j < GENE_LENGTH; j++) {
      printf("%d", (int)GENE->chromosomes[i].genes[j]); // 染色体の遺伝子表示
    }
    printf(",%d\n", FITNESS->fitnessValues[i]);
  }

  return 0;
}

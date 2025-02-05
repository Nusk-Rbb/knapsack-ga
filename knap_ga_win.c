#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// ナップサック問題の定義
#define KNAP_CAPA 100 // ナップサックの容量
#define ITEM_NUM 10   // 品物の個数

// GAパラメータの定義
#define POP_SIZE 10             // 初期に生成する染色体の個数
#define S_POP_SIZE POP_SIZE / 2 // 選択で残す染色体の個数
#define GENE_LENGTH ITEM_NUM    // 染色体の長さ（遺伝子の数）
double ELITE_COPY = 0.0;        // エリート戦略で保存する割合
double MUTE_RATE = 0.0;         // 突然変異確率

// プロトタイプ宣言
int initialize_gene(int GENE[POP_SIZE][GENE_LENGTH]);
int calc_fitness(int ITEM[][2], int GENE[][GENE_LENGTH], int *, int);
int selection(int GENE[POP_SIZE][GENE_LENGTH],
              int SELECT_GENE[POP_SIZE][GENE_LENGTH],
              int NEXT_GENE[POP_SIZE][GENE_LENGTH], int *);
int crossover(int SELECT_GENE[POP_SIZE][GENE_LENGTH],
              int NEXT_GENE[POP_SIZE][GENE_LENGTH]);
int mutation(int NEXT_GENE[POP_SIZE][GENE_LENGTH]);
int gene_copy(int GENE[POP_SIZE][GENE_LENGTH],
              int NEXT_GENE[POP_SIZE][GENE_LENGTH]);
int after_proc(int GENE[POP_SIZE][GENE_LENGTH], int *);
int sort(int *FITNESS, int *SORT_FIT);

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

int e_num; // エリート戦略で残す染色体の数を計算

/*---------------データ採取用の変数など---------------*/
int reset_data();

int main(int argc, char *argv[]) {

  int ITEM[ITEM_NUM][2] = {{10, 6},  {80, 30}, {25, 15}, {22, 18},
                           {5, 10},  {75, 35}, {70, 35}, {60, 20},
                           {30, 11}, {25, 30}}; // 品物初期化(値段,重さ)

  int GENE[POP_SIZE][GENE_LENGTH]; // 現世代の染色体

  int SELECT_GENE[S_POP_SIZE][GENE_LENGTH] = {0}; // 選択で残す染色体
  int NEXT_GENE[POP_SIZE][GENE_LENGTH];           // 次世代に残す染色体
  int FITNESS[POP_SIZE];                          // 適応度
  int GENERATION_NUM;                             // 終了世代数
  int i, j, k;
  int max = 0;

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
  e_num = (int)(POP_SIZE * ELITE_COPY + 0.5);

  initialize_gene(GENE);
  printf("#generation %d, elite copy %lf, muterate %lf\n", GENERATION_NUM,
         ELITE_COPY, MUTE_RATE);

  printf("#generation,best fitness,average,");
  for (j = 0; j < ITEM_NUM; j++) {
    printf("%d,", j);
  }
  printf("\n");

  for (i = 0; i < GENERATION_NUM; i++) {
    max = calc_fitness(ITEM, GENE, FITNESS, i);

    // 世代数と最優良評価値の画面表示
    printf("%d max,%d,", i, max);
    for (k = 0; k < POP_SIZE; k++)
      printf(" %4d,", FITNESS[k]);
    printf("\n");

    selection(GENE, SELECT_GENE, NEXT_GENE, FITNESS);
    crossover(SELECT_GENE, NEXT_GENE);
    mutation(NEXT_GENE);
    gene_copy(GENE, NEXT_GENE);
    // after_proc(GENE,FITNESS);/////////
  }
  calc_fitness(ITEM, GENE, FITNESS, i);
  after_proc(GENE, FITNESS);

  return 0;
}

// 初期母集団生成
int initialize_gene(int GENE[][GENE_LENGTH]) {

  int i, j;

  for (i = 0; i < POP_SIZE; i++) {
    for (j = 0; j < GENE_LENGTH; j++) {
      GENE[i][j] = rand() % 2; // 遺伝子初期化
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
int calc_fitness(int ITEM[][2], int GENE[][GENE_LENGTH], int *FITNESS,
                 int GN_num) {

  int i, j, WEIGHT_SUM, VAL_SUM, MAX_FITNESS = 0;

  for (i = 0; i < POP_SIZE; i++) {

    WEIGHT_SUM = 0;
    VAL_SUM = 0;

    for (j = 0; j < GENE_LENGTH; j++) {
      WEIGHT_SUM += ITEM[j][1] * GENE[i][j]; // 入れた品物の重さの総和
    }

    if (WEIGHT_SUM <= KNAP_CAPA) { // 入れた品物の重さが上限を超しているかの判定

      for (j = 0; j < GENE_LENGTH; j++) {
        VAL_SUM += ITEM[j][0] * GENE[i][j]; // 入れた品物の値段の総和
      }
      FITNESS[i] = VAL_SUM;
    } else {
      FITNESS[i] = 1;
    }
    if (MAX_FITNESS < FITNESS[i]) {
      MAX_FITNESS = FITNESS[i];
    }
  }

  return MAX_FITNESS;
}

// 選択：エリート戦略とルーレット選択の併用（全染色体の半分を次世代に残す）
int selection(int GENE[][GENE_LENGTH], int SELECT_GENE[][GENE_LENGTH],
              int NEXT_GENE[][GENE_LENGTH], int *FITNESS) {

  int i, j;
  double FITNESS_COPY[POP_SIZE]; // double型でFITNESSのコピーを作成
  int num_tmp;                   // 選ばれた染色体の番号を代入

  // エリート戦略用
  int SORT_FIT[POP_SIZE]; // 染色体の番号を、適応度で降順に並び替えたものを代入

  // ルーレット選択用
  double FIT_RATE[POP_SIZE]; // しきい値を代入
  int FIT_SUM;               // 全染色体の適応度の総和
  double r;                  // ルーレットの出る値

  for (i = 0; i < POP_SIZE; i++) {
    FITNESS_COPY[i] = FITNESS[i]; // コピー
  }

  // エリート戦略で残す染色体の内1割の染色体を残す

  sort(FITNESS, SORT_FIT); // 適応度を降順に並び替える

  for (i = 0; i < e_num; i++) {
    num_tmp = SORT_FIT[i]; // 適応度の上位から染色体の番号を代入

    for (j = 0; j < GENE_LENGTH; j++) {
      NEXT_GENE[i][j] = GENE[num_tmp][j];
    }
    // FITNESS_COPY[num_tmp]=0.0; //エリートを交叉に参加させないための処理
  }

  // ルーレット選択で残りの染色体を選出；(S_POP_SIZE-e_num)個の染色体を選出

  // iはe_num~S_POP_SIZEでは？
  for (i = 0; i < S_POP_SIZE; i++) {
    FIT_SUM = 0;
    for (j = 0; j < POP_SIZE; j++) {
      FIT_SUM += FITNESS_COPY[j]; // 適応度の総和を計算
    }

    for (j = 0; j < POP_SIZE; j++) { // 初期化
      FIT_RATE[j] = 0;
    }

    FIT_RATE[0] = FITNESS_COPY[0] / FIT_SUM;
    for (j = 1; j < POP_SIZE; j++) {
      FIT_RATE[j] = FIT_RATE[j - 1] +
                    FITNESS_COPY[j] / FIT_SUM; /* しきい値（適応度の割合）
         　 を計算して代入     */
    }

    r = drand48(); // ルーレットの値を生成

    j = 0;
    while (1) {
      if (r <= FIT_RATE[j]) {
        break; // ルーレットの値が止まるところでbreak
      }
      j++;
    }
    num_tmp = j;
    for (j = 0; j < GENE_LENGTH; j++) {
      SELECT_GENE[i][j] = GENE[num_tmp][j]; // 選ばれた染色体を次世代に残す
    }
    FITNESS_COPY[num_tmp] = 0.0; // 染色体がかぶらないようにする
  }
  return 0;
}

/*
  交叉:2点交叉を使用、交叉点は乱数で決定、1回の交叉につき子が1体できる
  //採取２体の間違い? 選択された染色体からPOP_SIZE体の子を作成
*/
int crossover(int SELECT_GENE[][GENE_LENGTH], int NEXT_GENE[][GENE_LENGTH]) {

  int FATHER = 0, MOTHER = 0;
  int CH_1_GENE[GENE_LENGTH], CH_2_GENE[GENE_LENGTH];
  int CROSS_POINT_1 = 0, CROSS_POINT_2 = 0;
  int i, j, r_tmp;

  i = e_num;

  while (i < POP_SIZE) {

    // 親の決定
    do {
      r_tmp = drand48() * S_POP_SIZE;
      FATHER = r_tmp % S_POP_SIZE;

      r_tmp = drand48() * S_POP_SIZE;
      MOTHER = r_tmp % S_POP_SIZE;
    } while (FATHER == MOTHER);

    // 親の遺伝子を子の遺伝子にコピー
    for (j = 0; j < GENE_LENGTH; j++) {
      CH_1_GENE[j] = SELECT_GENE[FATHER][j];
      CH_2_GENE[j] = SELECT_GENE[MOTHER][j];
    }

    // 交叉点を決める採取
    do {
      r_tmp = drand48() * (GENE_LENGTH + 1);
      CROSS_POINT_1 = r_tmp % (GENE_LENGTH + 1);

      r_tmp = drand48() * (GENE_LENGTH + 1);
      CROSS_POINT_2 = r_tmp % (GENE_LENGTH + 1);
    } while (CROSS_POINT_1 >= CROSS_POINT_2);

    // 子の遺伝子組み換え
    for (j = CROSS_POINT_1; j < CROSS_POINT_2; j++) {
      CH_1_GENE[j] = SELECT_GENE[MOTHER][j];
      CH_2_GENE[j] = SELECT_GENE[FATHER][j];
    }

    // 子を次世代に残す
    for (j = 0; j < GENE_LENGTH; j++) {
      NEXT_GENE[i][j] = CH_1_GENE[j];
    }
    i++;
    if (i >= POP_SIZE)
      break;

    for (j = 0; j < GENE_LENGTH; j++) {
      NEXT_GENE[i][j] = CH_2_GENE[j];
    }
    i++;
  }
  return 0;
}

// 突然変異
// 1点突然変異を使用　確率で、染色体の持つ遺伝子の一箇所を反転、反転場所採取採取は乱数で決定
int mutation(int NEXT_GENE[][GENE_LENGTH]) {

  int i, j, r_tmp;
  int MUTE_POINT;
  double r;

  for (i = e_num; i < POP_SIZE; i++) {
    for (j = 0; j < GENE_LENGTH; j++) {

      r = drand48();
      if (r < MUTE_RATE) {
        // 遺伝子を反転
        NEXT_GENE[i][j] ^= 1;
      }
    }
  }
  return 0;
}

// NEXT_GENEの中身を全てGENEに移す
int gene_copy(int GENE[][GENE_LENGTH], int NEXT_GENE[][GENE_LENGTH]) {

  int i, j;

  for (i = 0; i < POP_SIZE; i++) {
    for (j = 0; j < GENE_LENGTH; j++) {
      GENE[i][j] = NEXT_GENE[i][j];
    }
  }

  return 0;
}

// ソート
int sort(int *FITNESS, int *SORT_FIT) {
  int i, j, maxv, maxp;
  int FITNESS_COPY[POP_SIZE]; // int型でFITNESSのコピーを作成

  for (i = 0; i < POP_SIZE; i++) {
    FITNESS_COPY[i] = FITNESS[i]; // コピー
  }

  for (i = 0; i < POP_SIZE; i++) {
    maxv = FITNESS_COPY[i];
    maxp = i;
    for (j = i + 1; j < POP_SIZE; j++) {
      if (maxv < FITNESS_COPY[j]) { // 降順
        maxv = FITNESS_COPY[j];
        maxp = j;
      }
    }
    FITNESS_COPY[maxp] = FITNESS_COPY[i];
    FITNESS_COPY[i] = maxv;
    SORT_FIT[i] = maxp;
  }
  return 0;
}

// 最終世代の画面表示
int after_proc(int GENE[][GENE_LENGTH], int *FITNESS) {

  int i, j;

  for (i = 0; i < POP_SIZE; i++) {
    printf("%2d番目の染色体：", i);
    for (j = 0; j < GENE_LENGTH; j++) {
      printf("%d", GENE[i][j]); // 染色体の遺伝子表示
    }
    printf("\t%d\n", FITNESS[i]);
  }

  return 0;
}
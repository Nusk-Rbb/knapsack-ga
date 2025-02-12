# 実行する世代数
GENERATION_NUM=$1

# 探索するパラメータの範囲
ELITE_COPY_MIN=0.0
ELITE_COPY_MAX=1.0
ELITE_COPY_STEP=0.1

MUTE_RATE_MIN=0.0
MUTE_RATE_MAX=1.0
MUTE_RATE_STEP=0.1

# 最適値
MAX_FITNESS=245

# 結果を保存するファイル
RESULT_FILE="report/parameter_results/parameter_results.csv"

# ヘッダー行を書き込む
echo "#Generation Number,$GENERATION_NUM" > $RESULT_FILE
echo "elite_copy,mute_rate,best_fitness" >> $RESULT_FILE

# パラメータを変化させて実行
for ELITE_COPY in $(seq $ELITE_COPY_MIN $ELITE_COPY_STEP $ELITE_COPY_MAX); do
  for MUTE_RATE in $(seq $MUTE_RATE_MIN $MUTE_RATE_STEP $MUTE_RATE_MAX); do
    # 遺伝的アルゴリズムを実行
    ./knap_ga_win $GENERATION_NUM $ELITE_COPY $MUTE_RATE | \
      # 最適値を抽出
      grep "max," | tail -n 1 | awk -F',' '{print $2}' | \
      # 結果をファイルに書き込む
      while read BEST_FITNESS; do
        if [ $BEST_FITNESS -eq $MAX_FITNESS ]; then
            echo "$ELITE_COPY,$MUTE_RATE" >> $RESULT_FILE
        fi
      done
  done
done

echo "Optimization finished. Results are saved in $RESULT_FILE"
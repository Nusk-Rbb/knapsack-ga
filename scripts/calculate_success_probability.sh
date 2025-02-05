# パラメータ出力ファイル名
PARAM_FILE="report/parameter_results/parameter_results.csv"

# 結果ファイル名
RESULT_FILE="report/parameter_results/success_probabilities.csv"

# 世代数
GENERATION_NUM=$1

# 試行回数
TRIAL_COUNT=1000

# 最適値
MAX_FITNESS=245

echo "Elite copy, Mutation rate, Success probability" > $RESULT_FILE

# 各パラメータに対して試行
while IFS=',' read -r ELITE_COPY MUTE_RATE; do
  # コメント行はスキップ
  if [[ $ELITE_COPY = \#* ]]; then
    continue
  fi

  # ヘッダー行はスキップ
  if [[ $ELITE_COPY = "elite_copy" ]]; then
    continue
  fi

  echo "Testing parameters: Elite copy = $ELITE_COPY, Mutation rate = $MUTE_RATE"

  # 成功回数をカウント
  SUCCESS_COUNT=0

  # 試行回数分繰り返す
  for i in $(seq 1 $TRIAL_COUNT); do
    # 遺伝的アルゴリズムを実行
    CURRENT_FITNESS=$(./knap_ga_win $GENERATION_NUM $ELITE_COPY $MUTE_RATE | grep "max," | tail -n 1 | awk -F',' '{print $2}')
    # 最適値が$MAX_FITNESSならカウントアップ
    if [ $CURRENT_FITNESS = "$MAX_FITNESS" ]; then
      SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
    fi
  done

  # 成功確率を計算して出力
  SUCCESS_PROBABILITY=$(bc <<< "scale=2; $SUCCESS_COUNT / $TRIAL_COUNT * 100")
  echo "Success probability: $SUCCESS_PROBABILITY%"
  echo "$ELITE_COPY,$MUTE_RATE,$SUCCESS_PROBABILITY%" >> "$RESULT_FILE"
done < "$PARAM_FILE"
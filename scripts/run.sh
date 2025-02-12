GENERATION_NUM=$1

./scripts/parameter_search.sh $GENERATION_NUM

./scripts/calculate_success_probability.sh $GENERATION_NUM

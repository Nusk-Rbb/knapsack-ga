if [ $# -ne 1 ]; then
    echo "Example: $0 <generation number>"
    echo "Example: $0 10"
    exit 1
fi

./scripts/run.sh $1
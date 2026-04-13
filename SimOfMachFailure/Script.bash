#!/bin/bash

NUM_RUNS=3
THREADS=(1 2 4 8 16)
AVAILABLE_CORES=$(nproc)

if [[ $AVAILABLE_CORES -lt 16 ]]; then
    echo "Доступных ядер: $AVAILABLE_CORES. Будет использоваться oversubscribe."
    OVERSUBSCRIBE="--oversubscribe"
else
    OVERSUBSCRIBE=""
fi

for THREAD_COUNT in "${THREADS[@]}"
do
    echo "Запуск для $THREAD_COUNT потоков..."

    for ((i=1; i<=NUM_RUNS; i++))
    do
        echo "Запуск $i из $NUM_RUNS для $THREAD_COUNT потоков..."
        mpirun $OVERSUBSCRIBE -np $THREAD_COUNT ./ParallelProgram
        sleep 2
    done
done

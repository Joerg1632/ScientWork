#!/bin/bash

# Количество запусков для каждого числа потоков
NUM_RUNS=3
# Массив чисел потоков
THREADS=(1 2 4 8 16)

# Получаем количество доступных ядер
AVAILABLE_CORES=$(nproc)

# Если доступных ядер меньше, чем запрашиваемых процессов, используем --oversubscribe
if [[ $AVAILABLE_CORES -lt 16 ]]; then
    echo "Доступных ядер: $AVAILABLE_CORES. Будет использоваться oversubscribe."
    OVERSUBSCRIBE="--oversubscribe"
else
    OVERSUBSCRIBE=""
fi

# Запуск программы для каждого числа потоков
for THREAD_COUNT in "${THREADS[@]}"
do
    echo "Запуск для $THREAD_COUNT потоков..."
    
    # Запуск программы несколько раз для текущего числа потоков
    for ((i=1; i<=NUM_RUNS; i++))
    do
        echo "Запуск $i из $NUM_RUNS для $THREAD_COUNT потоков..."
        
        # Запуск программы с нужным количеством процессов
        mpirun $OVERSUBSCRIBE -np $THREAD_COUNT ./ParallelProgram
        
        # Ожидание между запусками (например, 2 секунд)
        sleep 2
    done
done

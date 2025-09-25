#include <iostream>
#include <random>
#include <vector>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <windows.h>
#include <chrono>

// Параметры модели
int initial_N;         // Кол-во узлов
double delta_t;        // Шаг по времени
int T;                 // Всего шагов времени
int num_experiments;   // Кол-во экспериментов
double weibull_lambda; // Параметр масштаба для Вейбулла
double weibull_k;      // Параметр формы для Вейбулла

std::default_random_engine generator;

// ========================== Симуляция Вейбулла ==========================
struct Node {
    double time_to_failure;
    double time_to_recovery;
    bool working;
};

// Генерация случайного времени по Вейбуллу
double weibullTime(double lambda, double k) {
    std::weibull_distribution<double> dist(k, lambda);
    return dist(generator);
}

// Симуляция одного эксперимента
std::vector<int> simulate() {
    int num_steps = T / delta_t;
    std::vector<int> working_machines(num_steps, initial_N);

    // Инициализация узлов
    std::vector<Node> nodes(initial_N);
    for (auto &node : nodes) {
        node.working = true;
        node.time_to_failure = weibullTime(weibull_lambda, weibull_k);
        node.time_to_recovery = 0.0;
    }

    for (int step = 0; step < num_steps; ++step) {
        double current_time = step * delta_t;

        for (auto &node : nodes) {
            if (node.working) {
                // Проверяем, не наступил ли отказ
                if (node.time_to_failure <= current_time) {
                    node.working = false;
                    node.time_to_recovery = current_time + weibullTime(weibull_lambda, weibull_k);
                }
            } else {
                // Проверяем, не наступило ли восстановление
                if (node.time_to_recovery <= current_time) {
                    node.working = true;
                    node.time_to_failure = current_time + weibullTime(weibull_lambda, weibull_k);
                }
            }
        }

        // Подсчет работающих узлов
        int count = std::count_if(nodes.begin(), nodes.end(), [](const Node &n){ return n.working; });
        working_machines[step] = count;
    }

    return working_machines;
}

// ========================== Средние и дисперсии ==========================
std::vector<double> calculateMean(const std::vector<std::vector<int>>& experiments) {
    int num_steps = T/delta_t;
    std::vector<double> mean(num_steps, 0.0);
    for (int i = 0; i < num_steps; ++i) {
        for (const auto &experiment : experiments) mean[i] += experiment[i];
        mean[i] /= experiments.size();
    }
    return mean;
}

std::vector<double> calculateVariance(const std::vector<std::vector<int>>& experiments, const std::vector<double>& mean) {
    int num_steps = T/delta_t;
    std::vector<double> var(num_steps, 0.0);
    for (int i = 0; i < num_steps; ++i) {
        for (const auto &experiment : experiments)
            var[i] += std::pow(experiment[i] - mean[i], 2);
        var[i] /= experiments.size();
    }
    return var;
}

// ========================== CSV ==========================
std::string formatNumber(double number) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << number;
    std::string result = out.str();
    std::replace(result.begin(), result.end(), '.', ',');
    return result;
}

void saveToCSV(const std::vector<double>& mean, const std::vector<double>& var) {
    std::ofstream outfile("resultOfManyExperiments.csv", std::ios::out | std::ios::binary);
    outfile << "\xEF\xBB\xBF"; // UTF-8 BOM

    outfile << "Parameters:\n";
    outfile << "Initial N;" << initial_N << "\n";
    outfile << "Delta t;" << delta_t << "\n";
    outfile << "T (time steps);" << T << "\n";
    outfile << "Number of experiments;" << num_experiments << "\n";
    outfile << "Weibull lambda;" << weibull_lambda << "\n";
    outfile << "Weibull k;" << weibull_k << "\n\n";

    outfile << "Time;WorkingMean;WorkingMean+Sqrt(Var);\n";

    int num_steps = T/delta_t;
    for (int i = 0; i < num_steps; ++i) {
        outfile << formatNumber(i * delta_t) << ";"
                << formatNumber(mean[i]) << ";"
                << formatNumber(mean[i] + std::sqrt(var[i])) << ";\n";
    }

    outfile.close();
}

// ========================== Чтение конфигурации ==========================
void readInput(const std::string& filename) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл " << filename << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string line;
    int line_count = 0;

    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        if (line_count == 0) iss >> initial_N;
        else if (line_count == 1) iss >> delta_t;
        else if (line_count == 2) iss >> T;
        else if (line_count == 3) iss >> num_experiments;
        else if (line_count == 4) iss >> weibull_lambda;
        else if (line_count == 5) iss >> weibull_k;

        if (iss.fail()) {
            std::cerr << "Ошибка: не удалось считать параметр из строки: " << line << std::endl;
            exit(EXIT_FAILURE);
        }

        line_count++;
    }

    if (line_count < 6) {
        std::cerr << "Ошибка: недостаточно параметров в файле. Ожидалось 6, считано " << line_count << std::endl;
        exit(EXIT_FAILURE);
    }

    infile.close();
}

// ========================== MAIN ==========================
int main() {
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "ru_RU.UTF-8");

    readInput("D:/ScientWork/SimOfMachFailure/config2.txt");

    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<std::vector<int>> experiments;
    for (int i = 0; i < num_experiments; ++i) {
        experiments.push_back(simulate());
    }

    auto mean = calculateMean(experiments);
    auto var = calculateVariance(experiments, mean);

    saveToCSV(mean, var);

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end_time - start_time;

    std::cout << "Симуляция завершена. Данные сохранены в файл resultOfManyExperiments.csv" << std::endl;
    std::cout << "Время выполнения: " << elapsed_seconds.count() << " секунд." << std::endl;

    return 0;
}

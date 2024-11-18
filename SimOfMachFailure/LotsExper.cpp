#include <iostream>
#include <random>
#include <vector>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <windows.h>

double lambda;         // failure rate -- exp^(-lambda * delta_t * cur_N)
int initial_N;         // Initial number of machines
double delta_t;        // step in second
int T;                 // 100 sec, total time = T * delta_t
int num_experiments;   // Number of experiments
double nu;             // intensity of recovery

std::default_random_engine generator;
std::uniform_real_distribution distribution(0.0, 1.0);

double calculateFailureProbability(int cur_N) {
    return 1 - std::exp(-lambda * delta_t * cur_N);
}

double calculateRecoveryProbability() {
    return 1 - std::exp(-nu * delta_t);
}

std::vector<int> simulateFailures() {
    int current_N = initial_N;
    std::vector<int> working_machines(T, initial_N);

    for (int i = 0; i < T; ++i) {

        double R_t = calculateFailureProbability(current_N);
        double z_failure = distribution(generator);

        if (z_failure < R_t && current_N > 0) {
            current_N--;
        }


        double recovery_t = calculateRecoveryProbability();
        double z_recovery = distribution(generator);

        if (z_recovery < recovery_t && current_N < initial_N) {
            current_N++;
        }

        working_machines[i] = current_N;
    }

    return working_machines;
}

std::vector<double> calculateMean(const std::vector<std::vector<int>>& experiments) {
    std::vector<double> mean(T, 0.0);
    for (int i = 0; i < T; ++i) {
        double sum = 0.0;
        for (const auto& experiment : experiments) {
            sum += experiment[i];
        }
        mean[i] = sum / num_experiments;
    }
    return mean;
}

std::vector<double> calculateVariance(const std::vector<std::vector<int>>& experiments, const std::vector<double>& mean) {
    std::vector<double> variance(T, 0.0);
    for (int i = 0; i < T; ++i) {
        double sum = 0.0;
        for (const auto& experiment : experiments) {
            sum += std::pow(experiment[i] - mean[i], 2);
        }
        variance[i] = sum / num_experiments;
    }
    return variance;
}

std::string formatNumber(double number) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << number;
    std::string result = out.str();
    std::replace(result.begin(), result.end(), '.', ',');
    return result;
}

void saveToCSV(const std::vector<double>& mean, const std::vector<double>& variance, const std::vector<std::vector<int>>& experiments) {
    std::ofstream outfile("resultOfManyExperiments.csv", std::ios::out | std::ios::binary);
    outfile << "\xEF\xBB\xBF"; // UTF-8 BOM
    outfile << "Time;Mean;Mean+Sqrt(Var);Mean-Sqrt(Var);Average Working Machines\n";

    for (int i = 0; i < T; ++i) {
        if (i % 50 == 0) {
            outfile << formatNumber(i * delta_t) << ";";
        } else {
            outfile << ";";
        }

        outfile << formatNumber(mean[i]) << ";"
                << formatNumber(mean[i] + std::sqrt(variance[i])) << ";"
                << formatNumber(mean[i] - std::sqrt(variance[i])) << ";";

        int total_working = 0;
        for (const auto& experiment : experiments) {
            total_working += experiment[i];
        }
        int average_working = total_working / num_experiments;

        outfile << average_working << "\n";
    }
    outfile.close();
}

void readInput(const std::string& filename) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл " << filename << std::endl;
        exit(EXIT_FAILURE);
    }

    infile >> lambda;
    infile >> initial_N;
    infile >> delta_t;
    infile >> T;
    infile >> num_experiments;
    infile >> nu;

    infile.close();
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "ru_RU.UTF-8");

    readInput("D:/ScientWork/SimOfMachFailure/config.txt");

    std::vector<std::vector<int>> experiments;
    for (int i = 0; i < num_experiments; ++i) {
        experiments.push_back(simulateFailures());
    }

    std::vector<double> mean = calculateMean(experiments);
    std::vector<double> variance = calculateVariance(experiments, mean);

    saveToCSV(mean, variance, experiments);

    std::cout << "Симуляция завершена. Данные сохранены в файл resultOfManyExperiments.csv" << std::endl;
    return 0;
}



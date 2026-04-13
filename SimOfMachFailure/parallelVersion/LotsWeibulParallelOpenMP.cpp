#include <iostream>
#include <random>
#include <vector>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <omp.h>
#include <chrono>

double lambda, beta_f;
int initial_N;
double delta_t;
double T;
int num_experiments;
double nu;

double calculateFailureProbability(double t, int cur_N) {
    double delta = std::pow(t + delta_t, beta_f) - std::pow(t, beta_f);
    return 1.0 - std::exp(-lambda * delta * cur_N);
}

double calculateRecoveryProbability(int cur_N) {
    return 1.0 - std::exp(-nu * delta_t * (initial_N - cur_N));
}

std::vector<int> simulate(unsigned int seed) {
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    int current_N = initial_N;
    int num_steps = static_cast<int>(T / delta_t);
    std::vector<int> working_machines(num_steps);

    for (int i = 0; i < num_steps; ++i) {
        double t = i * delta_t;

        if (dist(gen) < calculateFailureProbability(t, current_N) && current_N > 0) {
            current_N--;
        }

        if (dist(gen) < calculateRecoveryProbability(current_N) && current_N < initial_N) {
            current_N++;
        }

        working_machines[i] = current_N;
    }
    return working_machines;
}

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
        if (line_count == 0) iss >> lambda;
        else if (line_count == 1) iss >> beta_f;
        else if (line_count == 2) iss >> initial_N;
        else if (line_count == 3) iss >> delta_t;
        else if (line_count == 4) iss >> T;
        else if (line_count == 5) iss >> num_experiments;
        else if (line_count == 6) iss >> nu;

        if (iss.fail()) {
            std::cerr << "Ошибка чтения параметра в строке: " << line << std::endl;
            exit(EXIT_FAILURE);
        }
        line_count++;
    }
    infile.close();
}

std::string formatNumber(double number) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << number;
    std::string result = out.str();
    std::replace(result.begin(), result.end(), '.', ',');
    return result;
}

void saveToCSV(const std::vector<double>& mean_working,
               const std::vector<double>& variance_working,
               int num_threads) {
    std::ostringstream filename;
    filename << "resultOfOpenMP_" << num_threads << "Threads.csv";

    std::ofstream outfile(filename.str(), std::ios::out | std::ios::binary);
    outfile << "\xEF\xBB\xBF";

    outfile << "Parameters:\n";
    outfile << "Lambda_f;" << lambda << "\n";
    outfile << "Beta_f;" << beta_f << "\n";
    outfile << "Initial N;" << initial_N << "\n";
    outfile << "Delta t;" << delta_t << "\n";
    outfile << "T;" << T << "\n";
    outfile << "Number of experiments;" << num_experiments << "\n";
    outfile << "Nu;" << nu << "\n\n";

    outfile << "Time;FailureMean;FailureMean+Sqrt(Var);\n";

    int num_steps = static_cast<int>(T / delta_t);
    int step_interval = 100;

    for (int i = 0; i < num_steps; ++i) {
        if (i % step_interval == 0) {
            double time_in_hours = (i * delta_t) / 3.6;
            if (std::fmod(time_in_hours, 0.5) == 0) {
                outfile << formatNumber(i * delta_t / 3.6) << ";"
                        << formatNumber(initial_N - mean_working[i]) << ";"
                        << formatNumber(initial_N - mean_working[i] + std::sqrt(variance_working[i])) << ";\n";
            } else {
                outfile << ";"
                        << formatNumber(initial_N - mean_working[i]) << ";"
                        << formatNumber(initial_N - mean_working[i] + std::sqrt(variance_working[i])) << ";\n";
            }
        }
    }
    outfile.close();
}

int main() {
    readInput("/mnt/d/ScientWork/SimOfMachFailure/config/configWeibull.txt");

    int num_steps = static_cast<int>(T / delta_t);

    std::vector<double> sum_working(num_steps, 0.0);
    std::vector<double> sum_sq_working(num_steps, 0.0);

    auto start_time = std::chrono::high_resolution_clock::now();

    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        int num_threads = omp_get_num_threads();

        std::vector<double> local_sum(num_steps, 0.0);
        std::vector<double> local_sum_sq(num_steps, 0.0);

        #pragma omp for schedule(dynamic) nowait
        for (int exp = 0; exp < num_experiments; ++exp) {
            unsigned int seed = static_cast<unsigned int>(std::time(nullptr)) + exp + thread_id * 1000000;
            auto trajectory = simulate(seed);

            for (int t = 0; t < num_steps; ++t) {
                double val = trajectory[t];
                local_sum[t] += val;
                local_sum_sq[t] += val * val;
            }
        }

        #pragma omp critical
        {
            for (int t = 0; t < num_steps; ++t) {
                sum_working[t] += local_sum[t];
                sum_sq_working[t] += local_sum_sq[t];
            }
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    std::vector<double> mean_working(num_steps);
    std::vector<double> variance_working(num_steps);

    for (int t = 0; t < num_steps; ++t) {
        mean_working[t] = sum_working[t] / num_experiments;
        variance_working[t] = (sum_sq_working[t] / num_experiments) - (mean_working[t] * mean_working[t]);
    }

    int num_threads_used = omp_get_max_threads();
    saveToCSV(mean_working, variance_working, num_threads_used);

    std::cout << "Симуляция завершена за " << elapsed.count() << " секунд.\n";
    std::cout << "Использовано потоков: " << num_threads_used << "\n";
    std::cout << "Результат сохранён в resultOfOpenMP_" << num_threads_used << "Threads.csv\n";

    return 0;
}
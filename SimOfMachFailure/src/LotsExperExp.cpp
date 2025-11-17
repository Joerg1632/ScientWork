#include <iostream>
#include <random>
#include <vector>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <windows.h>
#include <chrono>
#include <filesystem>

double lambda;         // failure rate 1 - exp^(-lambda * delta_t * cur_N)
int initial_N;         // Initial number of machines
double delta_t;        // step in second
double T;                 // sec, total time = T * delta_t
int num_experiments;   // Number of experiments
double nu;             // intensity of recovery

std::default_random_engine generator;
std::uniform_real_distribution distribution(0.0, 1.0);

double calculateFailureProbability(int cur_N) {
    return 1 - std::exp(-lambda * delta_t * cur_N);
}

double calculateRecoveryProbability(int cur_N) {
    return 1 - std::exp(-nu * delta_t * (initial_N - cur_N));
}

std::vector<double> calculateAnalyticalM() {
    int num_steps = T/delta_t;
    std::vector analytical_mean(num_steps, 0.0);
    for (int t = 0; t < num_steps; ++t) {
        double time = t * delta_t;
        double coeff1 = (lambda * initial_N) / (lambda + nu);
        double coeff2 = (0 * nu - (initial_N - 0) * lambda) / (lambda + nu);
        analytical_mean[t] = coeff1 + coeff2 * std::exp(-(lambda + nu) * time);
    }
    return analytical_mean;
}

std::vector<double> calculateAnalyticalD() {
    int num_steps = T/delta_t;
    std::vector analytical_variance(num_steps, 0.0);
    for (int t = 0; t < num_steps; ++t) {
        double coeff1 = (initial_N * lambda * nu) / std::pow(lambda + nu, 2);
        double coeff2 = std::pow(lambda,2)*(initial_N - 0) + nu * (0 * nu -lambda * initial_N) / std::pow(lambda + nu, 2);
        double coeff3 = std::pow(lambda,2)*(initial_N - 0) + 0 * pow(nu,2) / std::pow(lambda + nu, 2);
        analytical_variance[t] = coeff1 + coeff2 * std::exp(-(lambda + nu) * t)
                                 - coeff3 * std::exp(-2 * (lambda + nu) * t);
    }
    return analytical_variance;
}

std::vector<int> simulate() {
    int current_N = initial_N;
    int num_steps = T/delta_t;
    std::vector working_machines(num_steps, initial_N);

    for (int i = 0; i < num_steps; ++i) {

        double R_t = calculateFailureProbability(current_N);
        double z_failure = distribution(generator);

        if (z_failure < R_t && current_N > 0) {
            current_N--;
        }

        double recovery_t = calculateRecoveryProbability(current_N);
        double z_recovery = distribution(generator);
        if (z_recovery < recovery_t && current_N < initial_N) {
            current_N++;
        }

        working_machines[i] = current_N;
    }

    return working_machines;
}

std::vector<double> calculateMean(const std::vector<std::vector<int>>& experiments) {
    int num_steps = T/delta_t;
    std::vector mean(num_steps, 0.0);
    for (int i = 0; i < num_steps; ++i) {
        double sum = 0.0;
        for (const auto& experiment : experiments) {
            sum += experiment[i];
        }
        mean[i] = sum / num_experiments;
    }
    return mean;
}

std::vector<double> calculateVariance(const std::vector<std::vector<int>>& experiments, const std::vector<double>& mean) {
    int num_steps = T/delta_t;
    std::vector variance(num_steps, 0.0);
    for (int i = 0; i < num_steps; ++i) {
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

std::string getUniqueFilename(const std::string& base_path) {
    namespace fs = std::filesystem;

    fs::create_directories(fs::path(base_path).parent_path());

    std::string filename = base_path;
    std::string base = fs::path(base_path).stem().string();
    std::string ext = fs::path(base_path).extension().string();
    std::string dir = fs::path(base_path).parent_path().string();

    int counter = 0;
    while (fs::exists(filename)) {
        ++counter;
        filename = dir + "/" + base + std::to_string(counter) + ext;
    }

    return filename;
}

void saveToCSV(const std::vector<double>& failure_mean,
               const std::vector<double>& failure_variance,
               const std::vector<double>& analiticalMean,
               const std::vector<double>& analiticalVar) {
    const std::string unique_path = getUniqueFilename("D:/ScientWork/SimOfMachFailure/results/resultOfManyExperiments.csv");

    std::ofstream outfile(unique_path, std::ios::out | std::ios::binary);
    if (!outfile.is_open()) {
        std::cerr << "Ошибка: не удалось создать файл: " << unique_path << std::endl;
        return;
    }
    outfile << "\xEF\xBB\xBF"; // UTF-8 BOM

    outfile << "Parameters:\n";
    outfile << "Lambda;" << lambda << "\n";
    outfile << "Initial N;" << initial_N << "\n";
    outfile << "Delta t;" << delta_t << "\n";
    outfile << "T (time steps);" << T << "\n";
    outfile << "Number of experiments;" << num_experiments << "\n";
    outfile << "Nu;" << nu << "\n\n";

    outfile << "Time;FailureMean;FailureMean+Sqrt(Var);"
               "AnalyticalMean;AnalyticalMean+sqrt(Var);\n";

    const int num_steps = T/delta_t;

    for (int i = 0; i < num_steps; ++i) {
        if(int step_interval = 100; i %step_interval == 0) {
            double time_in_hours = (i * delta_t) / 3.6;
            if (std::fmod(time_in_hours, 0.5) == 0){
                outfile << formatNumber(i * delta_t / 3.6) << ";"
                << formatNumber(initial_N - failure_mean[i]) << ";"
                << formatNumber(initial_N - failure_mean[i] + std::sqrt(failure_variance[i])) << ";"
                << formatNumber(analiticalMean[i]) << ";"
                << formatNumber(analiticalMean[i] + std::sqrt(analiticalVar[i])) << ";\n";
            }
            else {
                outfile << ";"
                << formatNumber(initial_N - failure_mean[i]) << ";"
                << formatNumber(initial_N - failure_mean[i] + std::sqrt(failure_variance[i])) << ";"
                << formatNumber(analiticalMean[i]) << ";"
                << formatNumber(analiticalMean[i] + std::sqrt(analiticalVar[i])) << ";\n";
            }
        }
    }

    outfile.close();
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
        else if (line_count == 1) iss >> initial_N;
        else if (line_count == 2) iss >> delta_t;
        else if (line_count == 3) iss >> T;
        else if (line_count == 4) iss >> num_experiments;
        else if (line_count == 5) iss >> nu;

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

int main() {
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "ru_RU.UTF-8");

    auto start_time = std::chrono::high_resolution_clock::now();

    readInput("D:/ScientWork/SimOfMachFailure/config/configExp.txt");

    std::vector<std::vector<int>> experiments;
    std::vector<int> working_machines;

    for (int i = 0; i < num_experiments; ++i) {
        working_machines = simulate();
        experiments.push_back(working_machines);
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end_time - start_time;

    std::vector<double> failure_mean = calculateMean(experiments);
    std::vector<double> variance_mean = calculateVariance(experiments, failure_mean);
    std::vector<double> analitical_mean = calculateAnalyticalM();
    std::vector<double> analitical_var = calculateAnalyticalD();

    saveToCSV(failure_mean,variance_mean,analitical_mean, analitical_var);

    std::cout << "Симуляция завершена. Данные сохранены в файл resultOfManyExperiments.csv в папке results" << std::endl;
    std::cout << "Время выполнения: " << elapsed_seconds.count() << " секунд." << std::endl;
    return 0;
}


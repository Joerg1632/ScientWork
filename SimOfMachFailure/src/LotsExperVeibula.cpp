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
#include <sstream>

double delta_shape;
double lambda_failure;
double lambda_recovery;
int    initial_N;
double delta_t;
double T;
int    num_experiments;

std::default_random_engine generator(std::random_device{}());
std::uniform_real_distribution<double> distribution(0.0, 1.0);

double conditionalProbability(double age, double lambda, double shape) {
    if (age < 0.0)
        age = 0.0;
    double H_now  = std::pow(lambda * age, shape);
    double H_next = std::pow(lambda * (age + delta_t), shape);
    return 1.0 - std::exp(-(H_next - H_now));
}

struct MachineState {
    bool   working;
    double age;
};

std::vector<int> simulate() {
    int num_steps = static_cast<int>(T / delta_t);
    std::vector<int> working_count(num_steps);

    std::vector<MachineState> machines(initial_N);
    for (auto& m : machines) {
        m.working = true;
        m.age     = 0.0;
    }

    for (int step = 0; step < num_steps; ++step) {
        for (auto& m : machines) {
            double p = m.working
                ? conditionalProbability(m.age, lambda_failure,  delta_shape)
                : conditionalProbability(m.age, lambda_recovery, delta_shape);

            if (distribution(generator) < p) {
                m.working = !m.working;
                m.age     = 0.0;
            } else {
                m.age += delta_t;
            }
        }

        int cnt = 0;
        for (const auto& m : machines) cnt += m.working ? 1 : 0;
        working_count[step] = cnt;
    }

    return working_count;
}

std::vector<double> calculateMean(const std::vector<std::vector<int>>& experiments) {
    int num_steps = static_cast<int>(T / delta_t);
    std::vector<double> mean(num_steps, 0.0);
    for (int i = 0; i < num_steps; ++i) {
        double sum = 0.0;
        for (const auto& exp : experiments) sum += exp[i];
        mean[i] = sum / num_experiments;
    }
    return mean;
}

std::vector<double> calculateVariance(const std::vector<std::vector<int>>& experiments,
                                      const std::vector<double>& mean) {
    int num_steps = static_cast<int>(T / delta_t);
    std::vector<double> variance(num_steps, 0.0);
    for (int i = 0; i < num_steps; ++i) {
        double sum = 0.0;
        for (const auto& exp : experiments)
            sum += std::pow(exp[i] - mean[i], 2);
        variance[i] = sum / num_experiments;
    }
    return variance;
}

std::string formatNumber(double number) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(4) << number;
    std::string result = out.str();
    std::replace(result.begin(), result.end(), '.', ',');
    return result;
}

std::string getUniqueFilename(const std::string& base_path) {
    namespace fs = std::filesystem;
    fs::create_directories(fs::path(base_path).parent_path());
    std::string filename = base_path;
    std::string base = fs::path(base_path).stem().string();
    std::string ext  = fs::path(base_path).extension().string();
    std::string dir  = fs::path(base_path).parent_path().string();
    int counter = 0;
    while (fs::exists(filename)) {
        ++counter;
        filename = dir + "/" + base + std::to_string(counter) + ext;
    }
    return filename;
}

void saveToCSV(const std::vector<double>& mean,
               const std::vector<double>& variance) {
    const std::string unique_path = getUniqueFilename(
        "D:/ScientWork/SimOfMachFailure/results/resultWeibullAged.csv");

    std::ofstream outfile(unique_path, std::ios::out | std::ios::binary);
    if (!outfile.is_open()) {
        std::cerr << "Ошибка: не удалось создать файл: " << unique_path << std::endl;
        return;
    }
    outfile << "\xEF\xBB\xBF";

    outfile << "Parameters:\n";
    outfile << "delta_shape;"     << delta_shape     << "\n";
    outfile << "lambda_failure;"  << lambda_failure   << "\n";
    outfile << "lambda_recovery;" << lambda_recovery  << "\n";
    outfile << "Initial N;"       << initial_N        << "\n";
    outfile << "Delta t;"         << delta_t          << "\n";
    outfile << "T;"               << T                << "\n";
    outfile << "Num experiments;" << num_experiments  << "\n\n";

    outfile << "Time;Mean working;Mean+Sqrt(Var)\n";

    int num_steps = static_cast<int>(T / delta_t);
    constexpr int step_interval = 100;

    for (int i = 0; i < num_steps; ++i) {
        if (i % step_interval != 0) continue;
        double time = i * delta_t;
        outfile << formatNumber(time)                              << ";"
                << formatNumber(mean[i])                           << ";"
                << formatNumber(mean[i] + std::sqrt(variance[i])) << "\n";
    }

    outfile.close();
    std::cout << "Результаты сохранены в: " << unique_path << std::endl;
}

void readInput(const std::string& filename) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл " << filename << std::endl;
        exit(EXIT_FAILURE);
    }

    // Формат configExp.txt (7 строк):
    // delta_shape
    // lambda_failure
    // lambda_recovery
    // initial_N
    // delta_t
    // T
    // num_experiments

    std::string line;
    int line_count = 0;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        switch (line_count) {
            case 0: iss >> delta_shape;     break;
            case 1: iss >> lambda_failure;  break;
            case 2: iss >> lambda_recovery; break;
            case 3: iss >> initial_N;       break;
            case 4: iss >> delta_t;         break;
            case 5: iss >> T;               break;
            case 6: iss >> num_experiments; break;
        }
        if (iss.fail()) {
            std::cerr << "Ошибка в строке: " << line << std::endl;
            exit(EXIT_FAILURE);
        }
        ++line_count;
    }
    if (line_count < 7) {
        std::cerr << "Ожидалось 7 параметров, считано " << line_count << std::endl;
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
    experiments.reserve(num_experiments);
    for (int i = 0; i < num_experiments; ++i)
        experiments.push_back(simulate());

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    auto mean     = calculateMean(experiments);
    auto variance = calculateVariance(experiments, mean);

    saveToCSV(mean, variance);

    std::cout << "Симуляция завершена.\n";
    std::cout << "Время выполнения: " << elapsed.count() << " сек.\n";
    return 0;
}

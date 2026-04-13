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
#include <omp.h>

double lambda;
double mu, mu_rec;
double sigma, sigma_rec;
double nu;
int    initial_N;
double delta_t;
double T;
int    num_experiments;

double normalCDF(double x) {
    return 0.5 * (1.0 + erf(x / std::sqrt(2.0)));
}

double lognormalSurvival(double t, double mu, double sigma) {
    if (t <= 0.0) return 1.0;
    double z = (std::log(t) - mu) / sigma;
    return 1.0 - normalCDF(z);
}

double H(double t, double mu, double sigma) {
    double S = lognormalSurvival(t, mu, sigma);
    if (S <= 1e-12) S = 1e-12;
    return -std::log(S);
}

double calculateFailureProbability(int cur_N, double t) {
    if (t <= 0.0) t = 1e-9;

    double H_now  = cur_N * H(t, mu, sigma);
    double H_next = cur_N * H(t + delta_t, mu, sigma);

    return 1.0 - std::exp(-(H_next - H_now));
}

double calculateRecoveryProbability(int cur_N, double t) {
    int failed_N = initial_N - cur_N;
    if (failed_N <= 0) return 0.0;
    if (t <= 0.0) t = 1e-9;

    double H_now  = failed_N * H(t, mu_rec, sigma_rec);
    double H_next = failed_N * H(t + delta_t, mu_rec, sigma_rec);

    return 1.0 - std::exp(-(H_next - H_now));
}

std::vector<int> simulate(std::default_random_engine& rng) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    int current_N = initial_N;
    int num_steps = static_cast<int>(T / delta_t);
    std::vector<int> working_machines(num_steps, initial_N);

    for (int i = 0; i < num_steps; ++i) {
        double t = (i == 0) ? 1e-9 : i * delta_t;

        double p_fail = calculateFailureProbability(current_N, t);
        if (dist(rng) < p_fail && current_N > 0)
            current_N--;

        double p_rec = calculateRecoveryProbability(current_N, t);
        if (dist(rng) < p_rec && current_N < initial_N)
            current_N++;

        working_machines[i] = current_N;
    }

    return working_machines;
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

std::vector<double> calculateAnalyticalM() {
    int num_steps = static_cast<int>(T / delta_t);
    std::vector<double> m(num_steps, 0.0);
    for (int i = 0; i < num_steps; ++i) {
        double t      = i * delta_t;
        double coeff1 = (lambda * initial_N) / (lambda + nu);
        double coeff2 = -initial_N * lambda   / (lambda + nu);
        m[i] = coeff1 + coeff2 * std::exp(-(lambda + nu) * t);
    }
    return m;
}

std::vector<double> calculateAnalyticalD() {
    int num_steps = static_cast<int>(T / delta_t);
    std::vector<double> d(num_steps, 0.0);
    for (int i = 0; i < num_steps; ++i) {
        double t      = i * delta_t;
        double coeff1 = (initial_N * lambda * nu) / std::pow(lambda + nu, 2);
        double coeff2 = std::pow(lambda, 2) * initial_N / std::pow(lambda + nu, 2);
        d[i] = coeff1
             - coeff2 * std::exp(-(lambda + nu) * t)
             - coeff2 * std::exp(-2.0 * (lambda + nu) * t);
    }
    return d;
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
               const std::vector<double>& variance,
               const std::vector<double>& analytical_mean,
               const std::vector<double>& analytical_var) {
    const std::string unique_path = getUniqueFilename(
        "SimOfMachFailure/results/resultLognormalGroup.csv");

    std::ofstream outfile(unique_path, std::ios::out | std::ios::binary);
    if (!outfile.is_open()) {
        std::cerr << "Ошибка: не удалось создать файл: " << unique_path << std::endl;
        return;
    }
    outfile << "\xEF\xBB\xBF";

    outfile << "Parameters:\n";
    outfile << "lambda;"          << lambda        << "\n";
    outfile << "mu;"              << mu            << "\n";
    outfile << "sigma;"           << sigma         << "\n";
    outfile << "nu;"              << nu            << "\n";
    outfile << "Initial N;"       << initial_N     << "\n";
    outfile << "Delta t;"         << delta_t       << "\n";
    outfile << "T;"               << T             << "\n";
    outfile << "Num experiments;" << num_experiments << "\n";
    outfile << "Threads used;"    << omp_get_max_threads() << "\n\n";

    outfile << "Time;LN mean;LN mean+Sqrt(Var);"
               "Analytical mean;Analytical mean+Sqrt(Var)\n";

    int num_steps = static_cast<int>(T / delta_t);
    constexpr int step_interval = 1000;

    for (int i = 0; i < num_steps; ++i) {
        if (i % step_interval != 0) continue;
        double t = i * delta_t;

        if (std::fmod(t, 50.0) < 1e-9)
            outfile << formatNumber(t) << ";";
        else
            outfile << ";";

        outfile << formatNumber(initial_N - mean[i])                          << ";"
                << formatNumber(initial_N - mean[i] + std::sqrt(variance[i])) << ";"
                << formatNumber(analytical_mean[i])                            << ";"
                << formatNumber(analytical_mean[i] +
                   std::sqrt(std::abs(analytical_var[i])))                     << "\n";
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

    std::string line;
    int line_count = 0;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        switch (line_count) {
            case 0: iss >> lambda;          break;
            case 1: iss >> mu;              break;
            case 2: iss >> sigma;           break;
            case 3: iss >> mu_rec;              break;
            case 4: iss >> sigma_rec;           break;
            case 5: iss >> nu;              break;
            case 6: iss >> initial_N;       break;
            case 7: iss >> delta_t;         break;
            case 8: iss >> T;               break;
            case 9: iss >> num_experiments; break;
        }
        if (iss.fail()) {
            std::cerr << "Ошибка в строке: " << line << std::endl;
            exit(EXIT_FAILURE);
        }
        ++line_count;
    }
    if (line_count < 10) {
        std::cerr << "Ожидалось 10 параметров, считано " << line_count << std::endl;
        exit(EXIT_FAILURE);
    }
    infile.close();
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "ru_RU.UTF-8");

    auto start_time = std::chrono::high_resolution_clock::now();

    readInput("SimOfMachFailure/config/configLogNormal.txt");

    std::vector<std::vector<int>> experiments(num_experiments);

    #pragma omp parallel
    {
        std::default_random_engine rng(
            std::random_device{}() ^
            static_cast<unsigned>(omp_get_thread_num() * 2654435761u)
        );

        #pragma omp for schedule(dynamic, 10)
        for (int i = 0; i < num_experiments; ++i) {
            experiments[i] = simulate(rng);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    auto mean         = calculateMean(experiments);
    auto variance     = calculateVariance(experiments, mean);
    auto analytical_m = calculateAnalyticalM();
    auto analytical_d = calculateAnalyticalD();

    saveToCSV(mean, variance, analytical_m, analytical_d);

    std::cout << "Симуляция завершена.\n";
    std::cout << "Потоков использовано: " << omp_get_max_threads() << "\n";
    std::cout << "Время выполнения: " << elapsed.count() << " сек.\n";
    return 0;
}
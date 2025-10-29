#include <iostream>
#include <random>
#include <vector>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <iomanip>
#include <windows.h>
#include <chrono>
#include <queue>

double mu_f, mu_r;            // scale for Weibull (for failure)
int initial_N;         // Initial number of machines
double delta_t;        // step in seconds
int T;                 // total seconds
int num_experiments;   // Number of experiments
double sigma_f, sigma_r;           // shape for Weibull (for failure)

std::default_random_engine generator;

struct Machine {
    bool working = true;
    double event_time = -1.0;
};

std::vector<int> simulate() {
    int num_steps = static_cast<int>(T / delta_t);
    std::vector working_machines(num_steps, 0);

    std::lognormal_distribution failure_dist(mu_f, sigma_f);
    std::lognormal_distribution repair_dist(mu_r, sigma_r);

    std::vector<Machine> machines(initial_N);
    std::priority_queue<std::pair<double, int>, std::vector<std::pair<double, int>>, std::greater<>> events;

    for (int i = 0; i < initial_N; ++i) {
        double ttf = failure_dist(generator);
        machines[i].event_time = ttf;
        events.push({ttf, i});
    }

    double current_time = 0.0;
    int current_working = initial_N;
    int step = 0;

    while (!events.empty()) {
        auto [time, mid] = events.top();
        events.pop();
        if (time > T)
            break;

        while (step < num_steps && step * delta_t < time)
            working_machines[step++] = current_working;

        current_time = time;
        Machine& m = machines[mid];
        if (std::abs(m.event_time - time) > 1e-9)
            continue;

        if (m.working)
        {
            m.working = false;
            --current_working;
            double ttr = repair_dist(generator);
            m.event_time = current_time + ttr;
            events.push({m.event_time, mid});
        }
        else
        {
            m.working = true;
            ++current_working;
            double ttf = failure_dist(generator);
            m.event_time = current_time + ttf;
            events.push({m.event_time, mid});
        }
    }

    while (step < num_steps)
        working_machines[step++] = current_working;

    return working_machines;
}

std::vector<double> calculateMean(const std::vector<std::vector<int>>& experiments) {
    int num_steps = static_cast<int>(T / delta_t);
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
    int num_steps = static_cast<int>(T / delta_t);
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
               const std::vector<double>& failure_variance) {
    const std::string unique_path = getUniqueFilename("D:/ScientWork/SimOfMachFailure/results/resultOfManyExperiments.csv");

    std::ofstream outfile(unique_path, std::ios::out | std::ios::binary);
    if (!outfile.is_open()) {
        std::cerr << "Ошибка: не удалось создать файл: " << unique_path << std::endl;
        return;
    }

    outfile << "\xEF\xBB\xBF";

    outfile << "Parameters:\n";
    outfile << "mu_f;" << mu_f << "\n";
    outfile << "sigma_f;" << sigma_f << "\n";
    outfile << "mu_r;" << mu_r << "\n";
    outfile << "sigma_r;" << sigma_r << "\n";
    outfile << "Initial N;" << initial_N << "\n";
    outfile << "Delta t;" << delta_t << "\n";
    outfile << "T (seconds);" << T << "\n";
    outfile << "Number of experiments;" << num_experiments << "\n\n";

    outfile << "Time;FailureMean;FailureMean+Sqrt(Var);\n";

    int num_steps = static_cast<int>(T / delta_t);
    int step_interval = 100;

    for (int i = 0; i < num_steps; ++i) {
        if(i %step_interval == 0) {
            double time_in_hours = (i * delta_t) / 3600.0;
            if (std::fmod(time_in_hours, 0.5) == 0){
                outfile << formatNumber(i * delta_t/3600) << ";"
                << formatNumber(initial_N - failure_mean[i]) << ";"
                << formatNumber(initial_N - failure_mean[i] + std::sqrt(failure_variance[i])) << ";\n";
            }
            else {
                outfile << ";"
                << formatNumber(initial_N - failure_mean[i]) << ";"
                << formatNumber(initial_N - failure_mean[i] + std::sqrt(failure_variance[i])) << ";\n";
            }
        }
    }

    outfile.close();
}

void readInput(const std::string& filename) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        throw std::runtime_error("Input file is not open");
    }

    std::string line;
    int line_count = 0;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        if (line_count == 0) iss >> mu_f;
        else if (line_count == 1) iss >> sigma_f;
        else if (line_count == 2) iss >> mu_r;
        else if (line_count == 3) iss >> sigma_r;
        else if (line_count == 4) iss >> initial_N;
        else if (line_count == 5) iss >> delta_t;
        else if (line_count == 6) iss >> T;
        else if (line_count == 7) iss >> num_experiments;
        line_count++;
    }
    if (line_count < 8) {
        throw std::runtime_error("Input file is too small");
    }
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "ru_RU.UTF-8");

    const auto start_time = std::chrono::high_resolution_clock::now();

    readInput("D:/ScientWork/SimOfMachFailure/config/configLogNormal.txt");

    std::vector<std::vector<int>> experiments;
    std::vector<int> working_machines;

    for (int i = 0; i < num_experiments; ++i) {
        working_machines = simulate();
        experiments.push_back(working_machines);
    }
    const auto end_time = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> elapsed_seconds = end_time - start_time;

    const std::vector<double> failure_mean = calculateMean(experiments);
    const std::vector<double> failure_variance = calculateVariance(experiments, failure_mean);

    saveToCSV(failure_mean, failure_variance);

    std::cout << "Симуляция завершена. Данные сохранены в файл resultOfManyExperiments.csv в папке results" << std::endl;
    std::cout << "Время выполнения: " << elapsed_seconds.count() << " секунд." << std::endl;
    return 0;
}
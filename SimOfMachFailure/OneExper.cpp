#include <iostream>
#include <random>
#include <vector>
#include <cmath>
#include <numeric>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <windows.h>

const double lambda = 0.001; // exp^(-lambda * delta_t * cur_N)
const int initial_N = 100; //num_initial_work_machine
const double delta_t = 0.1; //second
const int T = 1000; // 100 sec, total time = T * delta_t

std::default_random_engine generator;
std::uniform_real_distribution distribution(0.0, 1.0);

double calculateFailureProbability(int cur_N) {
    return 1 - std::exp(-lambda * delta_t * cur_N);
}

void simulateFailures(std::vector<int>& working_machines) {
    int current_N = initial_N;

    for (int i = 0; i < T; ++i) {
        double R_t = calculateFailureProbability(current_N);
        double z = distribution(generator);

        if (z < R_t && current_N > 0) {
            current_N--;
        }

        working_machines[i] = current_N;
    }
}

std::string formatNumber(double number) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << number;
    std::string result = out.str();
    std::replace(result.begin(), result.end(), '.', ',');
    return result;
}


void saveToCSV(const std::vector<int>& working_machines) {
    std::ofstream outfile("resultsOfOneExperiment.csv", std::ios::out | std::ios::binary);
    outfile << "\xEF\xBB\xBF";
    outfile << "Time;N(t)\n";

    for (int i = 0; i < T; ++i) {
        if (i % 50 == 0) {
            outfile << formatNumber(i * delta_t) << ";" << working_machines[i] << "\n";
        } else {
            outfile << ";" << working_machines[i] << "\n";
        }
    }
    outfile.close();
}



int main() {
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "ru_RU.UTF-8");
    std::vector<int> working_machines(T, initial_N);

    simulateFailures(working_machines);

    saveToCSV(working_machines);

    std::cout << "Симуляция завершена. Данные сохранены в файл results.csv" << std::endl;
    return 0;
}

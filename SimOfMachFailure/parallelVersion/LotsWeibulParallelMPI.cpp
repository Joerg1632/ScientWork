#include <iostream>
#include <random>
#include <vector>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <mpi.h>
#include <sstream>
#include <algorithm>

double lambda,beta_f;         // failure rate 1 - exp^(-lambda * delta_t * cur_N)
int initial_N;         // Initial number of machines
double delta_t;        // step in second
double T;                 // sec, total time = T * delta_t
int num_experiments;   // Number of experiments
double nu;             // intensity of recovery

std::default_random_engine generator;
std::uniform_real_distribution distribution(0.0, 1.0);

double calculateFailureProbability(double t, int cur_N) {
    double delta = std::pow(t + delta_t, beta_f) - std::pow(t, beta_f);
    return 1.0 - std::exp(-lambda * delta * cur_N);
}

double calculateRecoveryProbability(int cur_N) {
    return 1 - std::exp(-nu * delta_t * (initial_N - cur_N));
}

std::vector<int> simulate() {
    int current_N = initial_N;
    int num_steps = T / delta_t;
    std::vector<int> working_machines(num_steps, initial_N);

    for (int i = 0; i < num_steps; ++i) {
        if (distribution(generator) < calculateFailureProbability(i * delta_t, current_N) && current_N > 0) {
            current_N--;
        }
        if (distribution(generator) < calculateRecoveryProbability(current_N) && current_N < initial_N) {
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
        if (line_count == 1) iss >> beta_f;
        else if (line_count == 2) iss >> initial_N;
        else if (line_count == 3) iss >> delta_t;
        else if (line_count == 4) iss >> T;
        else if (line_count == 5) iss >> num_experiments;
        else if (line_count == 6) iss >> nu;

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

std::string formatNumber(double number) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << number;
    std::string result = out.str();
    std::replace(result.begin(), result.end(), '.', ',');
    return result;
}

void saveToCSV(std::vector<double>& failure_mean,
               std::vector<double>& failure_variance,int size) {
    std::ostringstream filename;
    filename << "resultOfParallelExperiments_" << size << "Threads.csv";

    std::ofstream outfile(filename.str(), std::ios::out | std::ios::binary);
    outfile << "\xEF\xBB\xBF"; // UTF-8 BOM

    outfile << "Parameters:\n";
    outfile << "Lambda_f;" << lambda << "\n";
    outfile << "Beta_f;" << beta_f << "\n";
    outfile << "Initial N;" << initial_N << "\n";
    outfile << "Delta t;" << delta_t << "\n";
    outfile << "T (time steps);" << T << "\n";
    outfile << "Number of experiments;" << num_experiments << "\n";
    outfile << "Nu;" << nu << "\n\n";

    outfile << "Time;FailureMean;FailureMean+Sqrt(Var);\n";

    int num_steps = T / delta_t;
    int step_interval = 100;

    for (int i = 0; i < num_steps; ++i) {
        failure_mean[i] /= size;
        failure_variance[i] /= size;
        if (i % step_interval == 0) {
            double time_in_hours = (i * delta_t) / 3.6;
            if (std::fmod(time_in_hours, 0.5) == 0) {
                outfile << formatNumber(i * delta_t / 3.6) << ";"
                        << formatNumber(initial_N - failure_mean[i]) << ";"
                        << formatNumber(initial_N - failure_mean[i] + std::sqrt(failure_variance[i])) << ";\n";
            } else {
                outfile << ";"
                        << formatNumber(initial_N - failure_mean[i]) << ";"
                        << formatNumber(initial_N - failure_mean[i] + std::sqrt(failure_variance[i])) << ";\n";
            }
        }
    }

    outfile.close();
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    double start_time = 0;

    if (rank == 0) {
        readInput("/mnt/d/ScientWork/SimOfMachFailure/config/configWeibull.txt");
        start_time = MPI_Wtime();
    }

    MPI_Bcast(&lambda, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&initial_N, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&delta_t, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&T, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&num_experiments, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&nu, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&beta_f, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    int num_steps = T / delta_t;
    int experiments_per_process = num_experiments / size;
    int remaining_experiments = num_experiments % size;

    // Индексы для каждого процесса
    int start_index = rank * experiments_per_process + std::min(rank, remaining_experiments);
    int end_index = start_index + experiments_per_process + (rank < remaining_experiments ? 1 : 0);

    // Массив для хранения результатов для этого процесса (двумерный вектор)
    int local_num_experiments = end_index - start_index;
    std::vector<std::vector<int>> local_results(local_num_experiments, std::vector<int>(num_steps));

    std::cout << "Process " << rank << ": "
              << "Start index = " << start_index << ", "
              << "End index = " << end_index << ", "
              << "Experiments = " << local_num_experiments << "\n";

    for (int i = start_index; i < end_index; ++i) {
        local_results[i - start_index] = simulate();
    }

    std::vector<double> local_mean(num_steps, 0.0);
    std::vector<double> local_variance(num_steps, 0.0);

    for (int i = 0; i < num_steps; ++i) {
        double sum = 0.0, sum_sq = 0.0;
        for (const auto& experiment : local_results) {
            sum += experiment[i];
            sum_sq += experiment[i] * experiment[i];
        }
        local_mean[i] = sum / local_num_experiments;
        local_variance[i] = (sum_sq / local_num_experiments) - (local_mean[i] * local_mean[i]);
    }

    std::vector<double> global_mean(num_steps, 0.0);
    std::vector<double> global_variance(num_steps, 0.0);

    MPI_Reduce(local_mean.data(), global_mean.data(), num_steps, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(local_variance.data(), global_variance.data(), num_steps, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        const double end_time = MPI_Wtime();
        saveToCSV(global_mean, global_variance, size);
        std::cout << "Время выполнения: " <<end_time - start_time <<" секунд."<< "\n";
    }

    MPI_Finalize();
    return 0;
}

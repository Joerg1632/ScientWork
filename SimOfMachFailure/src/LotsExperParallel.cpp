#include <iostream>
#include <random>
#include <vector>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <mpi.h>
#include <sstream>
#include <algorithm>

double lambda;
int initial_N;
double delta_t;
int T;
int num_experiments;
double nu;

std::default_random_engine generator;
std::uniform_real_distribution distribution(0.0, 1.0);

double calculateFailureProbability(int cur_N) {
    return 1 - std::exp(-lambda * delta_t * cur_N);
}

double calculateRecoveryProbability(int cur_N) {
    return 1 - std::exp(-nu * delta_t * (initial_N - cur_N));
}

std::vector<double> calculateAnalyticalM() {
    int num_steps = T / delta_t;
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
    int num_steps = T / delta_t;
    std::vector<double> analytical_variance(num_steps, 0.0);
    for (int t = 0; t < num_steps; ++t) {
        double coeff1 = (initial_N * lambda * nu) / std::pow(lambda + nu, 2);
        double coeff2 = std::pow(lambda, 2) * (initial_N - 0) + nu * (0 * nu - lambda * initial_N) / std::pow(lambda + nu, 2);
        double coeff3 = std::pow(lambda, 2) * (initial_N - 0) + 0 * std::pow(nu, 2) / std::pow(lambda + nu, 2);
        analytical_variance[t] = coeff1 + coeff2 * std::exp(-(lambda + nu) * t)
                                 - coeff3 * std::exp(-2 * (lambda + nu) * t);
    }
    return analytical_variance;
}

std::vector<int> simulate() {
    int current_N = initial_N;
    int num_steps = T / delta_t;
    std::vector<int> working_machines(num_steps, initial_N);

    for (int i = 0; i < num_steps; ++i) {
        if (distribution(generator) < calculateFailureProbability(current_N) && current_N > 0) {
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

std::string formatNumber(double number) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << number;
    std::string result = out.str();
    std::replace(result.begin(), result.end(), '.', ',');
    return result;
}

void saveToCSV(std::vector<double>& failure_mean,
               std::vector<double>& failure_variance,
               const std::vector<double>& analiticalMean,
               const std::vector<double>& analiticalVar, int size) {
    std::ostringstream filename;
    filename << "resultOfParallelExperiments_" << size << "Threads.csv";

    std::ofstream outfile(filename.str(), std::ios::out | std::ios::binary);
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
                        << formatNumber(initial_N - failure_mean[i] + std::sqrt(failure_variance[i])) << ";"
                        << formatNumber(analiticalMean[i]) << ";"
                        << formatNumber(analiticalMean[i] + std::sqrt(analiticalVar[i])) << ";\n";
            } else {
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

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    double start_time = 0;

    if (rank == 0) {
        readInput("/mnt/d/ScientWork/SimOfMachFailure/configExp.txt");
        start_time = MPI_Wtime();
    }

    MPI_Bcast(&lambda, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&initial_N, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&delta_t, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&T, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&num_experiments, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&nu, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

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
        std::vector<double> analitical_mean = calculateAnalyticalM();
        std::vector<double> analitical_var = calculateAnalyticalD();
        saveToCSV(global_mean, global_variance, analitical_mean, analitical_var, size);
        std::cout << "Время выполнения: " <<end_time - start_time <<" секунд."<< "\n";
    }

    MPI_Finalize();
    return 0;
}

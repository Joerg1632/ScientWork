 #include <iostream>
 #include <random>
 #include <vector>
 #include <cmath>
 #include <numeric>
 #include <fstream>
 #include <algorithm>
 #include <iomanip>
 #include <sstream>

 const double lambda = 0.1;
 const int initial_N = 100;
 const double delta_t = 0.1;
 const int T = 200;

 std::default_random_engine generator;
 std::uniform_real_distribution<double> distribution(0.0, 1.0);

 double calculateFailureProbability(int cur_t) {
     return 1 - std::exp(-lambda * cur_t);
 }

 void simulateFailures(std::vector<int>& working_machines) {
     int current_N = initial_N;

     for (int i = 0; i < T; ++i) {
         double t = i * delta_t;

         double R_t = calculateFailureProbability(t);
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

 void saveAveragesToCSV(const std::vector<double>& avg_working_machines, const std::vector<double>& avg_mean_values, const std::vector<double>& avg_variance_values) {
     std::ofstream outfile("results_averages.csv", std::ios::out | std::ios::binary);

     outfile << "\xEF\xBB\xBF";
     outfile << "Time;Avg_N(t);Avg_Mean;Avg_Variance\n";

     for (int i = 0; i < T; ++i) {
         outfile << formatNumber(i * delta_t) << ";"
                 << formatNumber(avg_working_machines[i]) << ";"
                 << formatNumber(avg_mean_values[i]) << ";"
                 << formatNumber(avg_variance_values[i]) << "\n";
     }

     outfile.close();
 }

 int main() {
     const int num_experiments = 100;

     std::vector<double> total_working_machines(T, 0.0);
     std::vector<double> total_variance_values(T, 0.0);
     std::vector<std::vector<int>> all_working_machines(num_experiments, std::vector<int>(T, 0));

     for (int exp = 0; exp < num_experiments; ++exp) {
         std::vector<int> working_machines(T, initial_N);
         simulateFailures(working_machines);
         all_working_machines[exp] = working_machines;

         for (int i = 0; i < T; ++i) {
             total_working_machines[i] += working_machines[i];
         }
     }

     std::vector<double> avg_working_machines(T);
     for (int i = 0; i < T; ++i) {
         avg_working_machines[i] = total_working_machines[i] / num_experiments;
     }

     for (int i = 0; i < T; ++i) {
         double cumulative_variance = 0.0;
         for (int exp = 0; exp < num_experiments; ++exp) {
             cumulative_variance += (all_working_machines[exp][i] - avg_working_machines[i]) * (all_working_machines[exp][i] - avg_working_machines[i]);
         }
         total_variance_values[i] = cumulative_variance / num_experiments;
     }

     saveAveragesToCSV(avg_working_machines, avg_working_machines, total_variance_values);

     std::cout << "Симуляция завершена. Итоговые данные сохранены в файл results_averages.csv" << std::endl;

     return 0;
 }

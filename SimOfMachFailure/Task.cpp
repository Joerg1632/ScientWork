//  //3
// #include <iostream>
// #include <vector>
// #include <queue>
// #include <algorithm>
// using namespace std;
//
// int main() {
//     int p, input;
//     cin >> p >> input;
//     vector<long long> ebv(p), m(p), k(p);
//     for (int i = 0; i < p; i++) {
//         cin >> ebv[i] >> m[i] >> k[i];
//     }
//
//     priority_queue<long long, vector<long long>, greater<long long>> cats;
//     for (int i = 0; i < input; i++) {
//         cats.push(0);
//     }
//     priority_queue<pair<long long, int>, vector<pair<long long, int>>, greater<pair<long long, int>>> to_statue;
//
//     for (int i = 0; i < p; i++) {
//         long long free_time = cats.top();
//         cats.pop();
//         long long start_pet = max(ebv[i], free_time);
//         long long end_pet = start_pet + m[i];
//         cats.push(end_pet);
//         to_statue.push({end_pet, i});
//     }
//
//     vector<long long> answer(p);
//     long long statue_free = 0;
//     while (!to_statue.empty()) {
//         auto [end_pet, index] = to_statue.top();
//         to_statue.pop();
//         long long start_statue = max(statue_free, end_pet);
//         long long end_statue = start_statue + k[index];
//         answer[index] = end_statue;
//         statue_free = end_statue;
//     }
//
//     for (long long res : answer) {
//         cout << res << "\n";
//     }
//     return 0;
// }
// //2
// #include <bits/stdc++.h>
// using namespace std;
//
// int main() {
//     int n;
//     cin >> n;
//     vector<pair<long long, long long>> intervals;
//     intervals.reserve(n);
//
//     for (int i = 0; i < n; i++) {
//         long long q, w;
//         cin >> q >> w;
//         intervals.emplace_back(q, q + w);
//     }
//     sort(intervals.begin(), intervals.end());
//     priority_queue<long long, vector<long long>, greater<long long>> pq;
//     int answer = 0;
//
//     for (auto [start, end] : intervals) {
//         while (!pq.empty() && pq.top() <= start) {
//             pq.pop();
//         }
//         pq.push(end);
//         answer = max(answer, (int)pq.size());
//     }
//
//     cout << answer;
//     return 0;
// }

//1
// #include <bits/stdc++.h>
// using namespace std;
//
// int main() {
//     int N, R;
//     cin >> N >> R;
//     vector<int> T(N);
//     for(int i = 0; i < N; i++) cin >> T[i];
//
//     vector<vector<int>> adj(N);
//     vector<int> indeg(N, 0);
//     for(int i = 0; i < R; i++){
//         int u, v;
//         cin >> u >> v;
//         u--; v--;
//         adj[u].push_back(v);
//         indeg[v]++;
//     }
//
//     int total = accumulate(T.begin(), T.end(), 0);
//
//     int min_total = total;
//
//     for(int remove = 0; remove < N; remove++){
//
//         vector<int> in = indeg;
//         queue<int> q;
//
//         for(int i = 0; i < N; i++){
//             if(i == remove) continue;
//             if(in[i]==0) q.push(i);
//         }
//
//         int cnt = 0;
//         int sum_time = 0;
//
//         while(!q.empty()){
//             int u = q.front(); q.pop();
//             cnt++;
//             sum_time += T[u];
//             for(int v: adj[u]){
//                 if(v == remove) continue;
//                 in[v]--;
//                 if(in[v]==0) q.push(v);
//             }
//         }
//
//         if(cnt == N-1){
//             min_total = min(min_total, sum_time);
//         }
//     }
//
//     cout << min_total << "\n";
//     return 0;
// }


// //5
// #include <algorithm>
// #include <charconv>
// #include <cmath>
// #include <iomanip>
// #include <ios>
// #include <iostream>
// #include <math.h>
// #include <stdio.h>
// #include <vector>
//
// using namespace std;
// const double e = 1e-9;
//
// vector<long long> lx, dx;
//
// bool check(double k, long long N, long long L, long long D) {
//     double rl = 0, rw = 0, tw = 0;
//     for (int i = 0; i < N; i++) {
//         double l1 = k * lx[i];
//         double d1 = k * dx[i];
//         if (l1 > L + e || d1 > D + e) return false;
//
//         if (rl < e) {
//             rw = d1;
//             rl = l1;
//         } else if (fabs(d1 - rw) < e && rl + l1 <= L + e) {
//             rl += l1;
//         } else {
//             tw += rw;
//             if (tw > D + e) return false;
//             rw = d1;
//             rl = l1;
//         }
//     }
//     tw += rw;
//     return tw <= D + e;
// }
//
// int main() {
//     int N;
//     long long L, D;
//     cin >> N >> L >> D;
//     lx.resize(N);
//     dx.resize(N);
//     for (int i = 0; i < N; i++) cin >> lx[i] >> dx[i];
//     double lo = 0, hi = 1e18;
//     for (int i = 0; i < N; i++) {
//         hi = min(hi, (double)L / lx[i]);
//         hi = min(hi, (double)D / dx[i]);
//     }
//     hi = min(hi * 2, 1e18);
//     for (int i = 0; i < 120; i++) {
//         double m = (lo + hi) / 2;
//         if (check(m, N, L, D)) lo = m;
//         else hi = m;
//     }
//
//     cout << fixed << setprecision(10) << lo << "\n";
//     return 0;
// }

//4
#include <bits/stdc++.h>
using namespace std;

struct State {
    int value;
    int steps;
};

int main() {
    int B, S;
    cin >> B >> S;
    set<int> results;
    queue<State> q;
    q.push({B * 100, S});

    int max_value = B * 100 + S * 50; // максимально возможное значение для visited
    vector<vector<bool>> visited(S + 1, vector<bool>(max_value + 1, false));

    while (!q.empty()) {
        State cur = q.front(); q.pop();
        if (cur.steps == 0) {
            results.insert(cur.value);
            continue;
        }
        if (visited[cur.steps][cur.value]) continue;
        visited[cur.steps][cur.value] = true;

        if (cur.value % 100 == 0) {
            // вариант 1: снять половину
            int half = cur.value / 2;
            if (half >= 0) q.push({half, cur.steps - 1});

            // вариант 2: снять 0.5
            int minusHalf = cur.value - 50;
            if (minusHalf >= 0) q.push({minusHalf, cur.steps - 1});
        } else {
            // дробное: снять только 0.5
            int minusHalf = cur.value - 50;
            if (minusHalf >= 0) q.push({minusHalf, cur.steps - 1});
        }
    }

    cout << results.size() << "\n";
    for (int val : results) {
        cout << fixed << setprecision(1) << val / 100.0 << " ";
    }
    cout << "\n";

    return 0;
}

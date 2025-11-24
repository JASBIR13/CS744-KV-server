#include <atomic>
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "../httplib.h"

using namespace std;
using namespace std::chrono;

atomic<bool> stop_flag(false);

struct Stats {
    atomic<uint64_t> requests{0};
    atomic<uint64_t> success{0};
    atomic<uint64_t> fail{0};
    atomic<uint64_t> total_latency_ns{0};
};

void worker_thread(int id, string server, string op, int keyspace,
                   Stats& stats) {
    httplib::Client cli(server.c_str());
    cli.set_connection_timeout(3, 0);

    mt19937_64 rng(id + 12345);
    uniform_int_distribution<int> key_rand(1, keyspace);

    while (!stop_flag.load()) {
        string key = "k" + to_string(key_rand(rng));
        auto start = high_resolution_clock::now();

        bool ok = false;

        if (op == "get") {
            auto res = cli.Get(("/kv?key=" + key).c_str());
            ok = res && res->status < 400;
        } else if (op == "put") {
            string body = "key=" + key + "&value=val" + to_string(id);
            auto res =
                cli.Post("/kv", body, "application/x-www-form-urlencoded");
            ok = res && res->status < 400;
        } else if (op == "delete") {
            auto res = cli.Delete(("/kv?key=" + key).c_str());
            ok = res && res->status < 400;
        } else if (op == "mixed") {
            int r = key_rand(rng) % 100;
            if (r < 50) {
                auto res = cli.Get(("/kv?key=" + key).c_str());
                ok = res && res->status < 400;
            } else if (r < 80) {
                string body = "key=" + key + "&value=val" + to_string(id);
                auto res =
                    cli.Post("/kv", body, "application/x-www-form-urlencoded");
                ok = res && res->status < 400;
            } else {
                auto res = cli.Delete(("/kv?key=" + key).c_str());
                ok = res && res->status < 400;
            }
        }

        auto end = high_resolution_clock::now();
        uint64_t ns = duration_cast<nanoseconds>(end - start).count();

        stats.total_latency_ns += ns;
        stats.requests++;
        if (ok)
            stats.success++;
        else
            stats.fail++;
    }
}

int main() {
    cout << "\n====== INTERACTIVE KV CLIENT ======\n";

    string server_url;
    cout << "Enter server URL (example http://localhost:8080): ";
    cin >> server_url;

    int threads;
    cout << "Enter number of threads: ";
    cin >> threads;

    string op;
    cout << "Operation type (put/get/delete/mixed): ";
    cin >> op;

    int keyspace;
    cout << "Enter key space size (example 10000): ";
    cin >> keyspace;

    int duration;
    cout << "Test duration in seconds: ";
    cin >> duration;

    cout << "\nStarting load test...\n";

    Stats stats;
    stop_flag = false;

    vector<thread> workers;
    for (int i = 0; i < threads; i++)
        workers.emplace_back(worker_thread, i, server_url, op, keyspace,
                             ref(stats));

    this_thread::sleep_for(seconds(duration));
    stop_flag = true;

    for (auto& t : workers) t.join();

    double avg_latency_ms = 0.0;
    if (stats.requests > 0)
        avg_latency_ms = (double)stats.total_latency_ns / stats.requests / 1e6;

    cout << "\n====== RESULTS ======\n";
    cout << "Total requests:      " << stats.requests.load() << endl;
    cout << "Success count:       " << stats.success.load() << endl;
    cout << "Fail count:          " << stats.fail.load() << endl;
    cout << "Throughput (req/s):  " << (stats.requests.load() / duration)
         << endl;

    cout << "Avg response time:   " << avg_latency_ms << " ms" << endl;

    cout << "=================================\n";

    return 0;
}

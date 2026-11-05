#include "../AVL/AVLTree.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <tuple>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifndef AVL_BUILD_MODE
#define AVL_BUILD_MODE "unknown"
#endif

namespace {
using Clock = std::chrono::steady_clock;
volatile std::uint64_t observable = 0;

struct Settings {
    std::vector<std::size_t> sizes{10000, 100000, 1000000};
    std::size_t repeats = 5;
    std::size_t queries = 10000;
    std::uint32_t seed = 42;
    std::size_t maxMemoryMB = 512;
    std::filesystem::path output = "results";
};

std::size_t number(const std::string& text, bool allowZero = false) {
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument("Expected a nonnegative integer: " + text);
    const auto value = std::stoull(text);
    if ((!allowZero && value == 0) || value > std::numeric_limits<std::size_t>::max())
        throw std::invalid_argument("Integer is out of range: " + text);
    return static_cast<std::size_t>(value);
}

Settings arguments(int argc, char** argv) {
    Settings settings;
    for (int i = 1; i < argc; ++i) {
        const std::string key = argv[i];
        if (key == "--help") {
            std::cout << "avl_bench [--sizes 10000,100000,1000000] [--repeats 5]\n"
                      << "          [--queries 10000] [--seed 42] [--max-memory-mb 512]\n"
                      << "          [--output results]\n"
                      << "Memory estimate is an admission check, not an OS memory limit.\n"
                      << "Exit 2 means that some requested sizes were skipped.\n";
            std::exit(0);
        }
        if (++i >= argc) throw std::invalid_argument("Missing value for " + key);
        const std::string value = argv[i];
        if (key == "--sizes") {
            settings.sizes.clear();
            if (value.empty() || value.back() == ',') throw std::invalid_argument("Invalid size list");
            std::istringstream input(value);
            std::string item;
            while (std::getline(input, item, ',')) settings.sizes.push_back(number(item));
            if (settings.sizes.empty()) throw std::invalid_argument("Empty size list");
            std::sort(settings.sizes.begin(), settings.sizes.end());
            settings.sizes.erase(std::unique(settings.sizes.begin(), settings.sizes.end()), settings.sizes.end());
        } else if (key == "--repeats") settings.repeats = number(value);
        else if (key == "--queries") settings.queries = number(value);
        else if (key == "--seed") {
            const auto parsed = number(value, true);
            if (parsed > std::numeric_limits<std::uint32_t>::max()) throw std::invalid_argument("Seed out of range");
            settings.seed = static_cast<std::uint32_t>(parsed);
        } else if (key == "--max-memory-mb") settings.maxMemoryMB = number(value);
        else if (key == "--output") settings.output = value;
        else throw std::invalid_argument("Unknown option: " + key);
    }
    if (settings.repeats > 1000) throw std::invalid_argument("At most 1000 repeats are supported");
    for (auto n : settings.sizes) {
        if (n > static_cast<std::size_t>(std::numeric_limits<int>::max() / 2))
            throw std::invalid_argument("Size exceeds the range of distinct int keys and absent keys");
    }
    return settings;
}

struct Sample {
    std::size_t n;
    std::string input;
    std::string operation;
    std::size_t repeat;
    std::size_t operations;
    double totalMS;
};

template<class F>
double milliseconds(F&& action) {
    const auto start = Clock::now();
    action();
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

std::vector<Sample> run(std::size_t n, bool randomInput, std::size_t repeat,
                        const Settings& settings) {
    std::mt19937 engine(settings.seed + static_cast<std::uint32_t>(repeat));
    std::vector<int> data(n);
    std::iota(data.begin(), data.end(), 0);
    if (randomInput) std::shuffle(data.begin(), data.end(), engine);
    const std::size_t queries = std::min(n, settings.queries);
    std::vector<int> keys(queries);
    // Uniformly spaced distinct keys, shuffled before timing. Same keys for
    // sorted/random scenarios and distinct keys for successful deletions.
    for (std::size_t i = 0; i < queries; ++i)
        keys[i] = static_cast<int>((static_cast<std::uint64_t>(i) * n) / queries);
    std::mt19937 queryEngine(settings.seed + static_cast<std::uint32_t>(repeat) + 12345);
    std::shuffle(keys.begin(), keys.end(), queryEngine);

    AVLTree<int> tree;
    std::vector<Sample> samples;
    const std::string input = randomInput ? "random" : "sorted";
    const auto record = [&](const std::string& operation, std::size_t count, double ms) {
        samples.push_back({n, input, operation, repeat + 1, count, ms});
    };
    record("build", n, milliseconds([&] {
        for (int value : data) tree.insert(value);
    }));
    if (tree.size() != n) throw std::runtime_error("Build produced an incorrect size");

    std::uint64_t hits = 0;
    record("find_hit", queries, milliseconds([&] {
        for (int value : keys) hits += tree.contains(value) ? 1u : 0u;
    }));
    observable = hits;
    if (hits != queries) throw std::runtime_error("Successful search failed");

    std::uint64_t unexpectedHits = 0;
    record("find_miss", queries, milliseconds([&] {
        for (int value : keys) unexpectedHits += tree.contains(static_cast<int>(n) + value) ? 1u : 0u;
    }));
    observable = unexpectedHits;
    if (unexpectedHits != 0) throw std::runtime_error("Absent key was found");

    std::uint64_t erased = 0;
    record("erase", queries, milliseconds([&] {
        for (int value : keys) erased += tree.erase(value) ? 1u : 0u;
    }));
    observable = erased;
    if (erased != queries || tree.size() != n - queries)
        throw std::runtime_error("Erase produced an incorrect result");
    // Destruction, input generation, validation and CSV output are not timed.
    return samples;
}

void metadata(const Settings& settings) {
    std::ofstream out(settings.output / "metadata.txt");
    if (!out) throw std::runtime_error("Cannot write metadata");
    const auto now = std::time(nullptr);
    out << "Run time (UTC): " << std::put_time(std::gmtime(&now), "%Y-%m-%d %H:%M:%S") << '\n'
        << "Compiler: "
#ifdef __clang__
        << "Clang " << __clang_version__
#elif defined(__GNUC__)
        << "GCC " << __VERSION__
#elif defined(_MSC_VER)
        << "MSVC " << _MSC_VER
#else
        << "unknown"
#endif
        << "\nBuild configuration: " << AVL_BUILD_MODE
        << "\nHardware concurrency: " << std::thread::hardware_concurrency()
        << "\nPointer bytes: " << sizeof(void*)
        << "\nSeed: " << settings.seed << "\nRepeats: " << settings.repeats
        << "\nRequested queries: " << settings.queries
        << "\nEstimated memory admission threshold MiB: " << settings.maxMemoryMB
        << "\nSizes:";
    for (auto n : settings.sizes) out << ' ' << n;
    out << "\nType: int, distinct keys 0..n-1.\n"
        << "Each repeat uses a new tree; seed = base seed + repeat index.\n"
        << "Input: ascending or shuffled. Scenarios alternate order by repeat.\n"
        << "Search batches: min(n, queries), uniformly spaced keys shuffled before timing.\n"
        << "Miss keys: n + successful key. Erase: successful query keys without duplicates.\n"
        << "Erase measurements start at n nodes and end at n - batch nodes.\n"
        << "Time: steady_clock, build total and batches; no generation/printing/destruction.\n"
        << "Checksums are consumed after timing. One warmup of 1000 keys precedes measurements.\n"
        << "Memory admission estimate: n * (sizeof(int)+sizeof(void*)*8) + queries*sizeof(int).\n"
        << "This estimate is not measured RSS and not a hard memory cap.\n"
        << "Record CPU model, available RAM and power settings separately for the report.\n";
    // Read standard Linux machine information when available; other OSes use manual notes.
    std::ifstream cpu("/proc/cpuinfo");
    for (std::string line; std::getline(cpu, line);) {
        if (line.find("model name") == 0) { out << "CPU: " << line << '\n'; break; }
    }
    std::ifstream memory("/proc/meminfo");
    for (std::string line; std::getline(memory, line);) {
        if (line.find("MemTotal:") == 0 || line.find("MemAvailable:") == 0) out << line << '\n';
    }
}

void summaries(const std::vector<Sample>& all, const std::filesystem::path& output) {
    std::map<std::tuple<std::size_t, std::string, std::string>, std::vector<Sample>> groups;
    for (const auto& sample : all) groups[{sample.n, sample.input, sample.operation}].push_back(sample);
    std::ofstream out(output / "summary.csv");
    if (!out) throw std::runtime_error("Cannot write summary");
    out << "n,input,operation,operations,repeats,median_total_ms,min_total_ms,max_total_ms,median_ns_per_op\n";
    out << std::setprecision(12);
    for (auto& entry : groups) {
        auto& samples = entry.second;
        std::sort(samples.begin(), samples.end(), [](const auto& a, const auto& b) { return a.totalMS < b.totalMS; });
        const auto& sample = samples.front();
        const auto middle = samples.size() / 2;
        const double median = samples.size() % 2 ? samples[middle].totalMS
            : (samples[middle - 1].totalMS + samples[middle].totalMS) / 2;
        out << sample.n << ',' << sample.input << ',' << sample.operation << ',' << sample.operations
            << ',' << samples.size() << ',' << median << ',' << sample.totalMS << ',' << samples.back().totalMS
            << ',' << median * 1e6 / static_cast<double>(sample.operations) << '\n';
    }
    if (!out) throw std::runtime_error("Cannot finish summary");
}
}

int main(int argc, char** argv) {
    try {
        const Settings settings = arguments(argc, argv);
        std::filesystem::create_directories(settings.output);
        metadata(settings);
        std::ofstream raw(settings.output / "raw.csv");
        std::ofstream skipped(settings.output / "skipped.csv");
        if (!raw || !skipped) throw std::runtime_error("Cannot create results files");
        raw << "n,input,operation,repeat,operations,total_ms,ns_per_op\n" << std::setprecision(12);
        skipped << "n,reason\n";
        {
            AVLTree<int> warmup;
            for (int i = 0; i < 1000; ++i) warmup.insert(i);
            for (int i = 0; i < 1000; ++i) observable = warmup.contains(i);
            for (int i = 0; i < 1000; ++i) warmup.erase(i);
        }
        std::vector<Sample> all;
        bool incomplete = false;
        for (auto n : settings.sizes) {
            const long double estimated = static_cast<long double>(n) * (sizeof(int) + sizeof(void*) * 8)
                + static_cast<long double>(std::min(n, settings.queries)) * sizeof(int);
            const long double limit = static_cast<long double>(settings.maxMemoryMB) * 1024 * 1024;
            if (estimated > limit) {
                skipped << n << ",estimated_memory_above_threshold\n";
                std::cerr << "SKIP n=" << n << ": estimated memory exceeds threshold\n";
                incomplete = true;
                continue;
            }
            try {
                for (std::size_t repeat = 0; repeat < settings.repeats; ++repeat) {
                    for (int scenario = 0; scenario < 2; ++scenario) {
                        const bool randomInput = (scenario + repeat) % 2 != 0;
                        std::cout << "n=" << n << " " << (randomInput ? "random" : "sorted")
                                  << " repeat=" << repeat + 1 << '/' << settings.repeats << std::endl;
                        const auto samples = run(n, randomInput, repeat, settings);
                        for (const auto& sample : samples) {
                            raw << sample.n << ',' << sample.input << ',' << sample.operation << ',' << sample.repeat
                                << ',' << sample.operations << ',' << sample.totalMS << ','
                                << sample.totalMS * 1e6 / static_cast<double>(sample.operations) << '\n';
                            all.push_back(sample);
                        }
                        raw.flush();
                        if (!raw) throw std::runtime_error("Cannot write raw measurements");
                    }
                }
            } catch (const std::bad_alloc&) {
                skipped << n << ",allocation_failed_partial_results_may_exist\n";
                incomplete = true;
                std::cerr << "SKIP n=" << n << ": allocation failed\n";
            }
        }
        summaries(all, settings.output);
        {
            std::ofstream report(settings.output / "metadata.txt", std::ios::app);
            std::ifstream status("/proc/self/status");
            for (std::string line; std::getline(status, line);) {
                if (line.find("VmHWM:") == 0)
                    report << "Process peak RSS (entire run, includes generators): " << line << '\n';
            }
        }
        skipped.close(); raw.close();
        if (!skipped || !raw) throw std::runtime_error("Cannot finish result files");
        std::cout << "Results: " << settings.output.string() << '\n';
        return incomplete ? 2 : 0;
    } catch (const std::exception& error) {
        std::cerr << "Benchmark error: " << error.what() << '\n';
        return 1;
    }
}
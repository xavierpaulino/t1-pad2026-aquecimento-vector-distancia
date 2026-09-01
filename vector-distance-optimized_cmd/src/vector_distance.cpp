#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <time.h>
#include <vector>

#if defined(__AVX__)
#include <immintrin.h>
#endif

namespace {

enum class KernelVariant { Baseline, Optimized };

struct Options {
    // The original assignment keeps N constant. In the late-submission CLI,
    // T is positional and N therefore remains an internal fixed parameter.
    // The legacy experimental interface may still override N with --n.
    std::size_t n = 8192;
    std::size_t t = 0;
    std::size_t repetitions = 30;
    std::size_t warmup = 2;
    std::uint64_t seed = 42;
    std::string csv_path;
    bool append = false;
    bool self_test = false;
    bool quiet = false;
    bool timer_probe = false;
    std::size_t timer_samples = 100000;
    std::size_t batch = 1;
    std::size_t order_position = 1;
    std::size_t repetition_offset = 0;
    KernelVariant variant = KernelVariant::Baseline;
    bool submission_mode = false;
};

[[noreturn]] void fail(const std::string& message) {
    throw std::runtime_error(message);
}

std::uint64_t parse_u64(std::string_view text, const char* name) {
    if (text.empty()) fail(std::string("empty value for ") + name);
    std::size_t pos = 0;
    unsigned long long value = 0;
    try {
        value = std::stoull(std::string(text), &pos, 10);
    } catch (const std::exception&) {
        fail(std::string("invalid integer for ") + name + ": " + std::string(text));
    }
    if (pos != text.size()) fail(std::string("invalid integer for ") + name + ": " + std::string(text));
    return static_cast<std::uint64_t>(value);
}

const char* variant_name(KernelVariant v) noexcept {
    return v == KernelVariant::Baseline ? "baseline_v3" : "optimized_v4";
}

KernelVariant parse_variant(std::string_view text) {
    if (text == "baseline" || text == "baseline_v3") return KernelVariant::Baseline;
    if (text == "optimized" || text == "optimized_v4") return KernelVariant::Optimized;
    fail("invalid --variant: " + std::string(text) + " (expected baseline or optimized)");
}

void usage(const char* argv0) {
    std::cerr
        << "Submission usage: " << argv0 << " T [options]\n"
        << "Experimental usage: " << argv0 << " --n N --t T [options]\n"
        << "Options:\n"
        << "  --variant V       baseline or optimized (default: baseline)\n"
        << "  --repetitions R   measured repetitions (default: 30)\n"
        << "  --warmup W        unmeasured warm-up runs (default: 2)\n"
        << "  --seed S          deterministic PRNG seed (default: 42)\n"
        << "  --csv PATH        write raw measurements to CSV\n"
        << "  --append           append to CSV (header written only if needed)\n"
        << "  --quiet            suppress human-readable summary\n"
        << "  --batch B          experimental block identifier (default: 1)\n"
        << "  --order-position P position inside the randomized block (default: 1)\n"
        << "  --repetition-offset K  add K to repetition numbering (default: 0)\n"
        << "  --self-test        run deterministic correctness tests for both kernels and exit\n"
        << "  --timer-probe      characterize CLOCK_MONOTONIC_RAW and exit\n"
        << "  --timer-samples K  samples for --timer-probe (default: 100000)\n";
}

Options parse_args(int argc, char** argv) {
    Options o;

    // Late specification: the first positional argument is T.
    // This mode is intentionally silent except for the single required line.
    int first_option = 1;
    if (argc > 1 && argv[1][0] != '-') {
        o.t = parse_u64(argv[1], "T");
        o.variant = KernelVariant::Optimized;
        o.repetitions = 1;
        o.submission_mode = true;
        first_option = 2;
    }

    for (int i = first_option; i < argc; ++i) {
        const std::string arg = argv[i];
        auto require_value = [&](const char* option) -> std::string_view {
            if (i + 1 >= argc) fail(std::string("missing value after ") + option);
            return argv[++i];
        };
        if (arg == "--n") o.n = parse_u64(require_value("--n"), "N");
        else if (arg == "--t") o.t = parse_u64(require_value("--t"), "T");
        else if (arg == "--variant") o.variant = parse_variant(require_value("--variant"));
        else if (arg == "--repetitions") o.repetitions = parse_u64(require_value("--repetitions"), "repetitions");
        else if (arg == "--warmup") o.warmup = parse_u64(require_value("--warmup"), "warmup");
        else if (arg == "--seed") o.seed = parse_u64(require_value("--seed"), "seed");
        else if (arg == "--csv") o.csv_path = std::string(require_value("--csv"));
        else if (arg == "--append") o.append = true;
        else if (arg == "--quiet") o.quiet = true;
        else if (arg == "--batch") o.batch = parse_u64(require_value("--batch"), "batch");
        else if (arg == "--order-position") o.order_position = parse_u64(require_value("--order-position"), "order-position");
        else if (arg == "--repetition-offset") o.repetition_offset = parse_u64(require_value("--repetition-offset"), "repetition-offset");
        else if (arg == "--self-test") o.self_test = true;
        else if (arg == "--timer-probe") o.timer_probe = true;
        else if (arg == "--timer-samples") o.timer_samples = parse_u64(require_value("--timer-samples"), "timer-samples");
        else if (arg == "--help" || arg == "-h") { usage(argv[0]); std::exit(EXIT_SUCCESS); }
        else fail("unknown option: " + arg);
    }
    return o;
}

std::uint64_t monotonic_raw_ns() {
    timespec ts{};
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts) != 0) {
        fail(std::string("clock_gettime(CLOCK_MONOTONIC_RAW) failed: ") + std::strerror(errno));
    }
    return static_cast<std::uint64_t>(ts.tv_sec) * 1'000'000'000ULL
         + static_cast<std::uint64_t>(ts.tv_nsec);
}

// Exact Part-1/v3 kernel. Keep this function unchanged so the new campaign
// has an internal baseline that is implementation-identical to the original.
#if defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
void squared_distances_baseline(const double* q,
                                const double* x,
                                double* distances,
                                std::size_t n,
                                std::size_t t) noexcept {
    for (std::size_t i = 0; i < n; ++i) {
        const double* row = x + i * t;
        double sum = 0.0;
        for (std::size_t j = 0; j < t; ++j) {
            const double d = q[j] - row[j];
            sum += d * d;
        }
        distances[i] = sum;
    }
}

#if defined(__AVX__)
inline double horizontal_sum_pd(__m256d v) noexcept {
    const __m128d low = _mm256_castpd256_pd128(v);
    const __m128d high = _mm256_extractf128_pd(v, 1);
    const __m128d pair = _mm_add_pd(low, high);
    const __m128d high64 = _mm_unpackhi_pd(pair, pair);
    return _mm_cvtsd_f64(_mm_add_sd(pair, high64));
}
#endif

// Single-thread optimized kernel for the same mathematical computation.
// The optimization keeps the same row-major data layout and resource budget
// as v3. On AVX-capable x86 builds it processes 16 doubles per inner-loop
// iteration with four independent vector accumulators, reducing the serial
// dependency chain of the floating-point reduction. A scalar tail handles
// arbitrary T values. No fast-math flags are required.
#if defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
void squared_distances_optimized(const double* __restrict__ q,
                                 const double* __restrict__ x,
                                 double* __restrict__ distances,
                                 std::size_t n,
                                 std::size_t t) noexcept {
    for (std::size_t i = 0; i < n; ++i) {
        const double* __restrict__ row = x + i * t;
        std::size_t j = 0;
        double sum = 0.0;

#if defined(__AVX__)
        __m256d acc0 = _mm256_setzero_pd();
        __m256d acc1 = _mm256_setzero_pd();
        __m256d acc2 = _mm256_setzero_pd();
        __m256d acc3 = _mm256_setzero_pd();

        for (; j + 15 < t; j += 16) {
            const __m256d q0 = _mm256_loadu_pd(q + j);
            const __m256d x0 = _mm256_loadu_pd(row + j);
            const __m256d d0 = _mm256_sub_pd(q0, x0);
            acc0 = _mm256_add_pd(acc0, _mm256_mul_pd(d0, d0));

            const __m256d q1 = _mm256_loadu_pd(q + j + 4);
            const __m256d x1 = _mm256_loadu_pd(row + j + 4);
            const __m256d d1 = _mm256_sub_pd(q1, x1);
            acc1 = _mm256_add_pd(acc1, _mm256_mul_pd(d1, d1));

            const __m256d q2 = _mm256_loadu_pd(q + j + 8);
            const __m256d x2 = _mm256_loadu_pd(row + j + 8);
            const __m256d d2 = _mm256_sub_pd(q2, x2);
            acc2 = _mm256_add_pd(acc2, _mm256_mul_pd(d2, d2));

            const __m256d q3 = _mm256_loadu_pd(q + j + 12);
            const __m256d x3 = _mm256_loadu_pd(row + j + 12);
            const __m256d d3 = _mm256_sub_pd(q3, x3);
            acc3 = _mm256_add_pd(acc3, _mm256_mul_pd(d3, d3));
        }

        __m256d acc = _mm256_add_pd(_mm256_add_pd(acc0, acc1), _mm256_add_pd(acc2, acc3));

        // Consume any remaining full AVX vectors before the scalar tail.
        for (; j + 3 < t; j += 4) {
            const __m256d qv = _mm256_loadu_pd(q + j);
            const __m256d xv = _mm256_loadu_pd(row + j);
            const __m256d d = _mm256_sub_pd(qv, xv);
            acc = _mm256_add_pd(acc, _mm256_mul_pd(d, d));
        }
        sum = horizontal_sum_pd(acc);
#else
        // Portable fallback: four independent scalar accumulators reduce the
        // dependency chain even when AVX is unavailable.
        double a0 = 0.0, a1 = 0.0, a2 = 0.0, a3 = 0.0;
        for (; j + 3 < t; j += 4) {
            const double d0 = q[j] - row[j];
            const double d1 = q[j + 1] - row[j + 1];
            const double d2 = q[j + 2] - row[j + 2];
            const double d3 = q[j + 3] - row[j + 3];
            a0 += d0 * d0;
            a1 += d1 * d1;
            a2 += d2 * d2;
            a3 += d3 * d3;
        }
        sum = (a0 + a1) + (a2 + a3);
#endif

        for (; j < t; ++j) {
            const double d = q[j] - row[j];
            sum += d * d;
        }
        distances[i] = sum;
    }
}

using KernelFn = void (*)(const double*, const double*, double*, std::size_t, std::size_t) noexcept;

KernelFn selected_kernel(KernelVariant v) noexcept {
    return v == KernelVariant::Baseline ? squared_distances_baseline : squared_distances_optimized;
}

double checksum(const std::vector<double>& values) {
    long double acc = 0.0L;
    for (double v : values) acc += static_cast<long double>(v);
    return static_cast<double>(acc);
}

void fill_data(std::vector<double>& q,
               std::vector<double>& x,
               std::vector<double>& distances,
               std::uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    for (double& v : q) v = dist(rng);
    for (double& v : x) v = dist(rng);
    std::fill(distances.begin(), distances.end(), 0.0);
}

void run_timer_probe(std::size_t samples) {
    if (samples == 0) fail("timer-samples must be greater than zero");

    timespec res{};
    if (clock_getres(CLOCK_MONOTONIC_RAW, &res) != 0) {
        fail(std::string("clock_getres(CLOCK_MONOTONIC_RAW) failed: ") + std::strerror(errno));
    }
    const std::uint64_t resolution_ns =
        static_cast<std::uint64_t>(res.tv_sec) * 1'000'000'000ULL +
        static_cast<std::uint64_t>(res.tv_nsec);

    std::vector<std::uint64_t> deltas;
    deltas.reserve(samples);
    for (std::size_t i = 0; i < samples; ++i) {
        const std::uint64_t a = monotonic_raw_ns();
        const std::uint64_t b = monotonic_raw_ns();
        deltas.push_back(b - a);
    }
    std::sort(deltas.begin(), deltas.end());
    const std::uint64_t median = deltas[deltas.size() / 2];
    const std::size_t p95_index = static_cast<std::size_t>(
        std::ceil(0.95 * static_cast<double>(deltas.size())) - 1.0);
    const std::uint64_t p95 = deltas[std::min(p95_index, deltas.size() - 1)];
    std::uint64_t min_positive = 0;
    for (const auto d : deltas) {
        if (d > 0) { min_positive = d; break; }
    }

    std::cout << "TIMER_PROBE"
              << " samples=" << samples
              << " clock_resolution_ns=" << resolution_ns
              << " min_positive_delta_ns=" << min_positive
              << " median_pair_delta_ns=" << median
              << " p95_pair_delta_ns=" << p95
              << '\n';
}

bool nearly_equal(double a, double b) noexcept {
    const double scale = 1.0 + std::max(std::abs(a), std::abs(b));
    return std::abs(a - b) <= 2e-12 * scale;
}

void compare_kernels(const std::vector<double>& q,
                     const std::vector<double>& x,
                     std::size_t n,
                     std::size_t t) {
    std::vector<double> ref(n, -1.0);
    std::vector<double> opt(n, -1.0);
    squared_distances_baseline(q.data(), x.data(), ref.data(), n, t);
    squared_distances_optimized(q.data(), x.data(), opt.data(), n, t);
    for (std::size_t i = 0; i < n; ++i) {
        if (!nearly_equal(ref[i], opt[i])) {
            fail("optimized kernel differs from baseline at vector " + std::to_string(i) +
                 ", T=" + std::to_string(t));
        }
    }
}

void run_self_test() {
    {
        const std::vector<double> q{1.0, 2.0};
        const std::vector<double> x{1.0, 2.0, 2.0, 4.0};
        std::vector<double> out(2, -1.0);
        squared_distances_baseline(q.data(), x.data(), out.data(), 2, 2);
        if (std::abs(out[0] - 0.0) > 1e-12 || std::abs(out[1] - 5.0) > 1e-12) {
            fail("baseline self-test failed");
        }
        squared_distances_optimized(q.data(), x.data(), out.data(), 2, 2);
        if (std::abs(out[0] - 0.0) > 1e-12 || std::abs(out[1] - 5.0) > 1e-12) {
            fail("optimized self-test failed");
        }
    }

    // Exercise vectorized and tail paths, including non-power-of-two T.
    const std::vector<std::size_t> ts{1, 2, 3, 4, 5, 7, 8, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127};
    for (const auto t : ts) {
        const std::size_t n = 7;
        std::vector<double> q(t);
        std::vector<double> x(n * t);
        std::vector<double> dummy(n);
        fill_data(q, x, dummy, 1234 + t);
        compare_kernels(q, x, n, t);
    }

    std::cout << "SELF_TEST_OK\n";
}

bool file_exists_and_nonempty(const std::string& path) {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0 && st.st_size > 0;
}

void write_csv(const Options& o,
               const std::vector<std::uint64_t>& elapsed_ns,
               double final_checksum) {
    if (o.csv_path.empty()) return;

    const bool had_content = o.append && file_exists_and_nonempty(o.csv_path);
    std::ios::openmode mode = std::ios::out;
    mode |= o.append ? std::ios::app : std::ios::trunc;
    std::ofstream out(o.csv_path, mode);
    if (!out) fail("cannot open CSV output: " + o.csv_path);

    if (!had_content) {
        out << "variant,N,T,batch,order_position,repetition,elapsed_ns,seed,checksum\n";
    }
    out << std::setprecision(17);
    for (std::size_t r = 0; r < elapsed_ns.size(); ++r) {
        out << variant_name(o.variant) << ',' << o.n << ',' << o.t << ','
            << o.batch << ',' << o.order_position << ','
            << (o.repetition_offset + r + 1) << ',' << elapsed_ns[r] << ','
            << o.seed << ',' << final_checksum << '\n';
    }
}

std::string format_milliseconds(std::uint64_t elapsed_ns) {
    const double milliseconds = static_cast<double>(elapsed_ns) / 1'000'000.0;
    std::ostringstream out;
    out << std::fixed << std::setprecision(6) << milliseconds;
    std::string text = out.str();
    while (text.size() > 1 && text.back() == '0') text.pop_back();
    if (!text.empty() && text.back() == '.') text.pop_back();
    return text;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const Options o = parse_args(argc, argv);
        if (o.self_test) {
            run_self_test();
            return EXIT_SUCCESS;
        }
        if (o.timer_probe) {
            run_timer_probe(o.timer_samples);
            return EXIT_SUCCESS;
        }
        if (o.n == 0) fail("N must be greater than zero");
        if (o.t == 0) fail("T must be greater than zero");
        if (o.repetitions == 0) fail("repetitions must be greater than zero");
        if (o.n > std::numeric_limits<std::size_t>::max() / o.t) fail("N*T overflows size_t");

        std::vector<double> q(o.t);
        std::vector<double> x(o.n * o.t);
        std::vector<double> distances(o.n);
        fill_data(q, x, distances, o.seed);

        const KernelFn kernel = selected_kernel(o.variant);

        volatile double warmup_sink = 0.0;
        for (std::size_t w = 0; w < o.warmup; ++w) {
            kernel(q.data(), x.data(), distances.data(), o.n, o.t);
            warmup_sink = warmup_sink + distances[w % o.n];
        }
        (void)warmup_sink;

        std::vector<std::uint64_t> elapsed;
        elapsed.reserve(o.repetitions);
        volatile double measured_sink = 0.0;

        for (std::size_t r = 0; r < o.repetitions; ++r) {
            const std::uint64_t start = monotonic_raw_ns();
            kernel(q.data(), x.data(), distances.data(), o.n, o.t);
            const std::uint64_t end = monotonic_raw_ns();
            elapsed.push_back(end - start);
            measured_sink = measured_sink + distances[r % o.n];
        }
        (void)measured_sink;

        const double final_checksum = checksum(distances);
        if (!std::isfinite(final_checksum)) fail("non-finite checksum; numerical result is invalid");

        write_csv(o, elapsed, final_checksum);

        if (o.submission_mode) {
            // Required late-submission output: exactly one line and nothing else.
            std::cout << "xavier, " << o.t << ", "
                      << format_milliseconds(elapsed.front()) << '\n';
        } else if (!o.quiet) {
            std::vector<std::uint64_t> sorted = elapsed;
            std::sort(sorted.begin(), sorted.end());
            const auto median = sorted[sorted.size() / 2];
            std::cout << "variant=" << variant_name(o.variant)
                      << " N=" << o.n
                      << " T=" << o.t
                      << " repetitions=" << o.repetitions
                      << " median_ns=" << median
                      << " checksum=" << std::setprecision(17) << final_checksum
                      << '\n';
        }
        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
}

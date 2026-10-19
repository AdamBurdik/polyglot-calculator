#include <uranium/math/matrix.hpp>
#include <cmath>
#include <cstddef>
namespace {
template <std::size_t N>
uranium::Matrix<double, N, N> load(const double *args) {
    uranium::Matrix<double, N, N> m;
    for (std::size_t i = 0; i < N * N; ++i) m.m[i / N][i % N] = args[i];
    return m;
}

template <std::size_t N>
double frobenius(const uranium::Matrix<double, N, N> &m) {
    double sum = 0.0;
    for (std::size_t i = 0; i < N * N; ++i) sum += m.m[i / N][i % N] * m.m[i / N][i % N];
    return std::sqrt(sum);
}

}

extern "C" int op_det2(void *ctx, const double *args, size_t argc, double *out) {
    (void) ctx;
    (void) argc;
    *out = load<2>(args).determinant();
    return 0;
}

extern "C" int op_det3(void *ctx, const double *args, size_t argc, double *out) {
    (void) ctx;
    (void) argc;
    *out = load<3>(args).determinant();
    return 0;
}

extern "C" int op_tr2(void *ctx, const double *args, size_t argc, double *out) {
    (void) ctx;
    (void) argc;
    *out = load<2>(args).trace();
    return 0;
}

extern "C" int op_tr3(void *ctx, const double *args, size_t argc, double *out) {
    (void) ctx;
    (void) argc;
    *out = load<3>(args).trace();
    return 0;
}

extern "C" int op_frob2(void *ctx, const double *args, size_t argc, double *out) {
    (void) ctx;
    (void) argc;
    *out = frobenius(load<2>(args));
    return 0;
}

extern "C" int op_frob3(void *ctx, const double *args, size_t argc, double *out) {
    (void) ctx;
    (void) argc;
    *out = frobenius(load<3>(args));
    return 0;
}

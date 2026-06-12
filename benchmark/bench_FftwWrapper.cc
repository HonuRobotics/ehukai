/*
 * Copyright (C) 2026 Honu Robotics
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Microbenchmark for the Eigen + TBB FftwWrapper 2D complex-to-real inverse
 * FFT. Each grid size is run both serial (1 worker) and parallel (all cores),
 * driven through plan_with_nthreads so the comparison exercises the public API
 * and the per-plan cached scratch.
 */

#include "EncinoWaves/FftwWrapper.h"

#include <cstdint>
#include <complex>
#include <vector>

#include <benchmark/benchmark.h>

namespace
{
  template <typename T>
  void BM_InverseFFT2d(benchmark::State &state)
  {
    using FFT = EncinoWaves::FftwWrapperT<T>;
    using Complex = std::complex<T>;

    const int n = static_cast<int>(state.range(0));
    const int nthreads = static_cast<int>(state.range(1));  // 1=serial, 0=all
    const int nHalf = (n / 2) + 1;

    // A non-trivial hermitian-packed spectrum (avoids denormals / all-zero).
    std::vector<Complex> spec(static_cast<std::size_t>(n) * nHalf);
    std::vector<T> out(static_cast<std::size_t>(n) * n, T(0));
    for (std::size_t k = 0; k < spec.size(); ++k)
      spec[k] = Complex(T(1) / static_cast<T>(k + 1), T(0.5));

    FFT::plan_with_nthreads(nthreads);
    auto plan = FFT::plan_dft_c2r_2d(n, n, spec.data(), out.data(), 0u);

    for (auto _ : state)
    {
      FFT::execute(plan);  // reuses the plan's cached scratch each iteration
      benchmark::DoNotOptimize(out.data());
      benchmark::ClobberMemory();
    }

    FFT::destroy_plan(plan);
    FFT::plan_with_nthreads(0);  // restore the default for the next case

    state.SetLabel(nthreads == 1 ? "serial" : "parallel");
    state.SetItemsProcessed(state.iterations() *
                            static_cast<std::int64_t>(n) * n);
  }
}  // namespace

BENCHMARK(BM_InverseFFT2d<float>)
    ->Args({64, 1})->Args({64, 0})
    ->Args({128, 1})->Args({128, 0})
    ->Args({256, 1})->Args({256, 0})
    ->Args({512, 1})->Args({512, 0})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();

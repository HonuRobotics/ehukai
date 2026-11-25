/*
 * Copyright (C) 2026 Honu Robotics
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *
 * Tests for the wrapped border of a padded spatial grid: the extra column
 * repeats the first, the extra row repeats row 0, and the corner repeats
 * the first cell.
 *
 *   1. CopyWrappedBorder gives that border whatever order its row tasks
 *      run in, and leaves the interior alone. The last row used to copy
 *      row 0 with its border cell, which the task of row 0 writes, so the
 *      corner depended on which ran first: a data race under
 *      tbb::parallel_for. Running the last row first, on one thread, shows
 *      it every time.
 *   2. The same through tbb::parallel_for, which shows it only by chance.
 *   3. SpectralToPaddedSpatial2D::execute leaves no cell of its output from
 *      before the call, border included, over several rounds.
 *
 * The float and double instantiations are exercised independently.
 */

#include "ehukai/SpectralSpatialField.h"

#include <cmath>
#include <complex>
#include <cstddef>
#include <iostream>
#include <vector>

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

namespace
{
  /// Prints a banner, runs `fn`, and counts a failure if it returns false.
  template <typename Fn>
  void Check(const char *name, int &failures, Fn fn)
  {
    std::cout << "  test: " << name << " ... ";
    const bool ok = fn();
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    if (!ok) ++failures;
  }

  /// A value no interior cell holds, for cells that must be overwritten.
  template <typename T>
  constexpr T kStale = T(-12345);

  /// The value of interior cell (x, y) of an n x n grid: distinct per cell.
  template <typename T>
  T Interior(int n, int x, int y)
  {
    return static_cast<T>(1 + y * n + x);
  }

  /// An (n+1) x (n+1) grid: Interior() inside, kStale on the border.
  template <typename T>
  std::vector<T> StaleBorderGrid(int n)
  {
    const std::size_t stride = static_cast<std::size_t>(n) + 1;
    std::vector<T> grid(stride * stride, kStale<T>);
    for (int y = 0; y < n; ++y)
      for (int x = 0; x < n; ++x)
        grid[y * stride + x] = Interior<T>(n, x, y);
    return grid;
  }

  /// True when a grid from StaleBorderGrid() has its border wrapped and its
  /// interior as it was: every cell holds the interior value it wraps to.
  template <typename T>
  bool IsWrapped(const std::vector<T> &grid, int n)
  {
    const std::size_t stride = static_cast<std::size_t>(n) + 1;
    for (int y = 0; y <= n; ++y)
    {
      for (int x = 0; x <= n; ++x)
      {
        const T want = Interior<T>(n, x % n, y % n);
        if (grid[y * stride + x] != want)
        {
          std::cout << "(cell " << x << ", " << y << " is "
                    << grid[y * stride + x] << ", want " << want << ") ";
          return false;
        }
      }
    }
    return true;
  }

  template <typename T>
  int RunForType(const char *typeName)
  {
    std::cout << "wrapped border, " << typeName << "\n";
    int failures = 0;
    const int n = 8;

    Check("last row first, then the others", failures, [&]() {
      std::vector<T> grid = StaleBorderGrid<T>(n);
      ehukai::CopyWrappedBorder<T> F;
      F.Data = grid.data();
      F.N = n;
      F(tbb::blocked_range<int>(n, n + 1));
      F(tbb::blocked_range<int>(0, n));
      return IsWrapped(grid, n);
    });

    Check("rows in order", failures, [&]() {
      std::vector<T> grid = StaleBorderGrid<T>(n);
      ehukai::CopyWrappedBorder<T> F;
      F.Data = grid.data();
      F.N = n;
      F(tbb::blocked_range<int>(0, n + 1));
      return IsWrapped(grid, n);
    });

    Check("through parallel_for, 100 rounds", failures, [&]() {
      for (int round = 0; round < 100; ++round)
      {
        std::vector<T> grid = StaleBorderGrid<T>(n);
        ehukai::CopyWrappedBorder<T> F;
        F.Data = grid.data();
        F.N = n;
        tbb::parallel_for(tbb::blocked_range<int>(0, n + 1), F);
        if (!IsWrapped(grid, n))
        {
          std::cout << "(round " << round << ") ";
          return false;
        }
      }
      return true;
    });

    // A 64 x 64 transform, which splits across TBB tasks. The spectrum is a
    // DC term and one mode, so no cell of the output is near the stale value.
    Check("execute leaves no stale cell, 20 rounds", failures, [&]() {
      const int pow2 = 6;
      const int size = 1 << pow2;
      const std::size_t stride = static_cast<std::size_t>(size) + 1;
      ehukai::ComplexSpectralField2D<T> spectral(pow2);
      ehukai::RealSpatialField2D<T> scratch(pow2, 1);
      ehukai::RealSpatialField2D<T> spatial(pow2, 1);
      ehukai::SpectralToPaddedSpatial2D<T> converter(spectral, scratch);
      for (int round = 0; round < 20; ++round)
      {
        for (std::size_t i = 0; i < spectral.size(); ++i)
          spectral.data()[i] = std::complex<T>(T(0), T(0));
        spectral.data()[0] = std::complex<T>(T(3), T(0));
        spectral.data()[spectral.stride() + 1] = std::complex<T>(T(1), T(0.5));
        for (std::size_t i = 0; i < spatial.size(); ++i)
          spatial.data()[i] = kStale<T>;
        converter.execute(spectral, spatial);
        const T *out = spatial.cdata();
        for (int y = 0; y <= size; ++y)
        {
          for (int x = 0; x <= size; ++x)
          {
            const T got = out[y * stride + x];
            const T wrapped = out[(y % size) * stride + (x % size)];
            if (got == kStale<T> || !std::isfinite(got) || got != wrapped)
            {
              std::cout << "(round " << round << ", cell " << x << ", " << y
                        << " is " << got << ") ";
              return false;
            }
          }
        }
      }
      return true;
    });

    return failures;
  }
}  // namespace

// NOLINTNEXTLINE(bugprone-exception-escape): a throwing ctor is a test failure
int main()
{
  int failures = 0;
  failures += RunForType<float>("float");
  failures += RunForType<double>("double");
  std::cout << "\nresult: " << (failures == 0 ? "PASS" : "FAIL") << "\n";
  return failures == 0 ? 0 : 1;
}

## EncinoWaves

> **This is a fork** maintained by [Honu Robotics](https://github.com/HonuRobotics).
> It extracts EncinoWaves as a **standalone, headless C++ library** for embedding
> in simulators and tools (e.g. VRX / Gazebo) and drops the original interactive
> viewer. See [What this fork changes](#what-this-fork-changes).

The basic architecture of these Waves is based on the TweakWaves application
written by Chris Horvath for Tweak Films in 2001.  This, in turn, was based
on the SIGGRAPH papers and courses by [Jerry Tessendorf][tessendorf], and by
the paper ["A Simple Fluid Solver based on the FTT" by Jos Stam][simplesolver].

[tessendorf]:   http://jerrytessendorf.blogspot.com/
[simplesolver]: http://www.dgp.toronto.edu/people/stam/reality/Research/pdf/jgt01.pdf

The TMA, JONSWAP, and Pierson Moskowitz Wave Spectra, as well as the
directional spreading functions are formulated based on the descriptions
given in "Ocean Waves: The Stochastic Approach",
by Michel K. Ochi, published by Cambridge Ocean Technology Series, 1998,2005.

This library is written as a working implementation of the paper:

> Christopher J. Horvath. 2015.   
> [Empirical directional wave spectra for computer graphics.](http://dl.acm.org/authorize?N90195)   
> In Proceedings of the 2015 Symposium on Digital Production (DigiPro '15),   
> Los Angeles, Aug. 8, 2015, pp. 29-39.    


## What this fork changes

This fork keeps the spectral-synthesis library and removes everything that made
the original a desktop application:

- **Headless library only** — the interactive OpenGL viewer (`SimpleSimViewer`,
  `GeepGLFW`) and its OpenEXR / GLFW / Boost / Xrandr dependencies are gone.
- **Eigen::FFT backend** — the FFTW backend is replaced by an `Eigen::FFT`
  (KissFFT) shim; no FFTW dependency.
- **Modern math** — IlmBase is replaced by Imath.
- **Standalone CMake** — exports an `EncinoWaves::EncinoWaves` target and a
  `find_package(EncinoWaves)` config; dependencies are just Eigen3, TBB, Imath.
- **Headless smoke tests** under `test/` exercise the spectra → propagation
  pipeline without a GPU.


## Building (Ubuntu)

EncinoWaves is developed and tested on **Ubuntu** (24.04 LTS recommended).

Install the build tools and dependencies:

```sh
sudo apt-get update
sudo apt-get install -y cmake build-essential \
  libeigen3-dev libtbb-dev libimath-dev
```

Configure and compile the library, running these (and the commands below) from
the repository root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Run the headless smoke tests:

```sh
ctest --test-dir build --output-on-failure
```

Install (optional — headers, shared library, and the `EncinoWaves` CMake config):

```sh
sudo cmake --install build
```

Consume it from another CMake project:

```cmake
find_package(EncinoWaves REQUIRED)
target_link_libraries(your_target PRIVATE EncinoWaves::EncinoWaves)
```


### License

Copyright &copy; 2015 Christopher Jon Horvath. Fork modifications &copy; Honu Robotics.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

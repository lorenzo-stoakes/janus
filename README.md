# Janus

Janus is a sports trading toolset consisting of the following components:

- __jupiter__: Reads a live market data stream and transmits it to other
  components while writing the data for future analysis.
- __neptune__: Converts existing market data JSON to a binary file format for
  efficient analysis by other components.
- __osiris__: GUI giving an insight into what janus is doing and allowing for
  manual/semi-automatic trading.
- __apollo__: Backtesting framework for testing strategies against binary files
  generated by neptune.
- __mars__: Live trading framework that uses jupiter's market data combined with
  strategy code to execute trades in the market.

It is currently only designed to interact with betfair but is designed to be
flexible enough to be portable to other markets.

## Building

Install the following shared components:

* [cmake](https://cmake.org/) - Meta-build system used to generate ninja build files.
* [Ninja](https://github.com/ninja-build/ninja) - Fast build system which can be used with cmake.
* [Google test](https://github.com/google/googletest) - Unit testing framework.
* [clang format](https://clang.llvm.org/docs/ClangFormat.html) - Auto code linter/formatter.
* [clang tidy](https://clang.llvm.org/extra/clang-tidy/) - Static analysis tool.
* [cppcheck](http://cppcheck.sourceforge.net/) - Another static analysis tool.
* [Address sanitizer](https://github.com/google/sanitizers/wiki/AddressSanitizer) - Memory error detection tool.

After that, simply run `make` in the root directory and binaries should appear in `build/`.

To run tests run `make test`, and to run benchmarks run `make bench`.

## 3rd-party components

* [sajson](https://github.com/chadaustin/sajson) - Used and modified under an MIT license.
* [snappy](https://github.com/google/snappy) - Fast compressor/decompressor used as a library under a BSD license.
* [mbedtls](https://github.com/ARMmbed/mbedtls) - Efficient TLS implementation used under an Apache license.
* [spdlog](https://github.com/gabime/spdlog) - Fast header-only C++ logging implementation used under an MIT license.

## Additional tools

- __benchmark__: Run 'make bench' to run benchmarks. Currently this just runs
  the original sajson benchmarks.
- __checker__: This allows checking of the JSON parsing and universe update
  applying logic. This is compiled as a tool in the binary 'checker'.

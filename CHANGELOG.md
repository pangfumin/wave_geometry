# Changelog

## [Unreleased](https://github.com/wavelab/wave_geometry/compare/0.3.0..HEAD)
### New features
- New `dynamic` module for heap-allocated, dynamically-composable `Proxy` expressions
- New documentation built with Sphinx

### Backward-incompatible API changes
- C++14 is now required
- Boost 1.58 is now required
- Change selection of storage types from expression types.
  (Described in docs under "Storage and auto")

### Fixes and minor changes
- Fix finding googletest source package on Ubuntu bionic
- Fix (trivial) reverse-mode AD on a single leaf
- Move numerical Jacobian evaluator into `core` module
- Reorganize and rename storage base classes

## [0.3.0](https://github.com/wavelab/wave_geometry/compare/0.2.0...0.3.0) (2018-08-19)
### New features
- Scalar multiplication (scalar\*vector, vector\*scalar, and vector\/scalar)
- Differentiable arithmetic operations on scalar expressions (e.g. ```v1.norm() + v2.norm()`)
- The library can be installed and used via CMake
- The library can be built as a submodule of libwave
- Automated CI builds

### Deprecated support
- C++11 support is deprecated. The next release will use C++14.
- Eigen 3.2.92 (3.3-beta2) is not officially supported. It works as of this release, but
only 3.3.2 is tested in CI.

### Fixes and minor changes
- Expressions can be differentiated w.r.t. their stored contents
  (e.g. `auto expr = v * 2; expr.jacobian(expr.rhs())`)
- CMake handling of dependencies improved
- Minor bug and compiler warning fixes

## 0.2.0 (2018-05-04)

Initial public release. Previously, wave_geometry was under development as a library
called `lkinematics`.
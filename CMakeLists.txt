cmake_minimum_required(VERSION 3.16)
project(stanrt C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

set(DEPS ${CMAKE_CURRENT_SOURCE_DIR}/deps)
file(GLOB _eigen ${DEPS}/math/lib/eigen_*)
file(GLOB _boost ${DEPS}/math/lib/boost_*)
file(GLOB _sundials ${DEPS}/math/lib/sundials_*)
file(GLOB _tbb ${DEPS}/math/lib/tbb_*)

# Everything that includes stan-math (or stan) links this interface target.
add_library(stanmath INTERFACE)
target_include_directories(stanmath INTERFACE
  ${DEPS}/math
  ${DEPS}/stan/src
  ${_eigen}
  ${_boost}
  ${_sundials}/include
  ${_tbb}/include)
target_compile_definitions(stanmath INTERFACE _REENTRANT BOOST_DISABLE_ASSERTS)
# Pin FP contraction off: clang otherwise forms FMAs differently across
# template instantiations of the same math, which breaks bitwise parity
# between the kernels and the var-path references (measured: 2 ULP on
# eight-schools d/dmu). Cost is well inside the performance budget.
target_compile_options(stanmath INTERFACE -ffp-contract=off)

# The distributable shared library: C ABI + all kernels, TBB stubbed
# (single-threaded until the payload ships real TBB).
# SUNDIALS (CVODES) for the ODE integrators: the same translation units
# CmdStan compiles, minus the profiler and the Fortran shims. Built once
# into a static library and linked into both stanrt targets.
file(GLOB SUNDIALS_SRC
  ${_sundials}/src/cvodes/*.c
  ${_sundials}/src/sundials/*.c
  ${_sundials}/src/nvector/serial/*.c
  ${_sundials}/src/sunmatrix/band/*.c
  ${_sundials}/src/sunmatrix/dense/*.c
  ${_sundials}/src/sunlinsol/band/*.c
  ${_sundials}/src/sunlinsol/dense/*.c
  ${_sundials}/src/sunnonlinsol/newton/*.c
  ${_sundials}/src/sunnonlinsol/fixedpoint/*.c)
list(FILTER SUNDIALS_SRC EXCLUDE REGEX "sundials_profiler\\.c$")
list(FILTER SUNDIALS_SRC EXCLUDE REGEX "/f(nvector|sunmatrix|sunlinsol|sunnonlinsol)")
add_library(sundials STATIC ${SUNDIALS_SRC})
target_include_directories(sundials PUBLIC ${_sundials}/include
                           PRIVATE ${_sundials}/src ${_sundials}/src/sundials)
# stan-math patches sundials' logging through this header, which the
# upstream C sources expect to be force-included (as CmdStan does).
target_compile_options(sundials PRIVATE -w
  -include ${_sundials}/include/stan_sundials_printf_override.hpp)
set_target_properties(sundials PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(stanrt_shared SHARED
  runtime/src/capi.cpp
  runtime/src/executor.cpp
  runtime/kernels/elementwise.cpp
  runtime/kernels/densities.cpp
  runtime/kernels/legacy_fns.cpp
  runtime/kernels/matrix_fns.cpp
  runtime/kernels/ode.cpp
  runtime/kernels/constrain.cpp
  runtime/kernels/eltwise_expr.cpp
  runtime/src/nuts.cpp
  runtime/src/mir_reader.cpp
  runtime/src/lower.cpp
  runtime/src/data.cpp
  tests/tbb_stub.cpp)
target_include_directories(stanrt_shared PUBLIC runtime/include)
target_link_libraries(stanrt_shared PUBLIC stanmath sundials)
set_target_properties(stanrt_shared PROPERTIES OUTPUT_NAME stanrt)

# In-process stanc3: link the OCaml-compiled embed object
# (tools/stanc_embed/build.sh produces deps/stanc3/stanc_embed.o).
set(STANRT_STANC_EMBED_OBJ "" CACHE FILEPATH
    "stanc_embed.o built by tools/stanc_embed/build.sh; enables in-process stanc")
set(STANRT_OCAML_STDLIB "" CACHE PATH
    "OCaml stdlib dir (opam switch lib/ocaml) for caml/*.h")
if(STANRT_STANC_EMBED_OBJ)
  target_sources(stanrt_shared PRIVATE
    runtime/src/stanc_embed_c.cpp ${STANRT_STANC_EMBED_OBJ})
  target_include_directories(stanrt_shared PRIVATE ${STANRT_OCAML_STDLIB})
  target_compile_definitions(stanrt_shared PRIVATE STANRT_EMBED_STANC)
  target_link_libraries(stanrt_shared PUBLIC dl pthread)
endif()

add_library(stanrt STATIC
  runtime/src/executor.cpp
  runtime/kernels/elementwise.cpp
  runtime/kernels/densities.cpp
  runtime/kernels/legacy_fns.cpp
  runtime/kernels/matrix_fns.cpp
  runtime/kernels/ode.cpp
  runtime/kernels/constrain.cpp
  runtime/kernels/eltwise_expr.cpp
  runtime/src/nuts.cpp
  runtime/src/mir_reader.cpp
  runtime/src/lower.cpp
  runtime/src/data.cpp)
target_include_directories(stanrt PUBLIC runtime/include)
target_link_libraries(stanrt PUBLIC stanmath sundials)

enable_testing()
# Single-threaded tests stub the one TBB symbol stan-math's rev core needs.
function(stanrt_add_test NAME)
  add_executable(${NAME} tests/${NAME}.cpp tests/tbb_stub.cpp)
  target_link_libraries(${NAME} PRIVATE stanrt)
  # Tests run from the repo root so fixture paths are stable.
  add_test(NAME ${NAME} COMMAND ${NAME}
           WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endfunction()

add_executable(stanrt_run tools/stanrt_run.cpp tests/tbb_stub.cpp)
target_link_libraries(stanrt_run PRIVATE stanrt)
add_executable(stanrt_check tools/stanrt_check.cpp tests/tbb_stub.cpp)
target_link_libraries(stanrt_check PRIVATE stanrt)
add_executable(bench_grad tools/bench_grad.cpp tests/tbb_stub.cpp)
target_link_libraries(bench_grad PRIVATE stanrt)
add_executable(bench_opcost tools/bench_opcost.cpp tests/tbb_stub.cpp)
target_link_libraries(bench_opcost PRIVATE stanrt)
add_executable(dump_ops tools/dump_ops.cpp tests/tbb_stub.cpp)
target_link_libraries(dump_ops PRIVATE stanrt)

stanrt_add_test(test_smoke)
stanrt_add_test(test_executor)
stanrt_add_test(test_recorder)
stanrt_add_test(test_densities)
stanrt_add_test(test_matvec)
stanrt_add_test(test_legacy)
stanrt_add_test(test_eight_schools)
stanrt_add_test(test_glm)
stanrt_add_test(test_nuts)
stanrt_add_test(test_sexp)
stanrt_add_test(test_mir)
stanrt_add_test(test_transforms)
stanrt_add_test(test_eltwise)
stanrt_add_test(test_lower)
stanrt_add_test(test_data)
stanrt_add_test(test_e2e)

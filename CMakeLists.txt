cmake_minimum_required(VERSION 3.16)

# Before project(): the deployment target is baked in at compiler-probe
# time. Left alone it defaults to whatever the build machine runs, so the
# wheel would claim a floor it does not honor. Must agree with the
# platform tags in tools/build_wheel.sh. MACOSX_DEPLOYMENT_TARGET in the
# environment still wins.
execute_process(COMMAND uname -s OUTPUT_VARIABLE _host_os
                OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(_host_os STREQUAL "Darwin" AND NOT DEFINED ENV{MACOSX_DEPLOYMENT_TARGET}
                             AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
  execute_process(COMMAND uname -m OUTPUT_VARIABLE _host_arch
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(_host_arch STREQUAL "arm64")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
  else()
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "" FORCE)
  endif()
endif()

project(stanli C CXX)

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
# into a static library and linked into both stanli targets.
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

add_library(stanli_shared SHARED
  runtime/src/capi.cpp
  runtime/src/executor.cpp
  runtime/kernels/elementwise.cpp
  runtime/kernels/densities.cpp
  runtime/kernels/legacy_fns.cpp
  runtime/kernels/matrix_fns.cpp
  runtime/kernels/ode.cpp
  runtime/src/ode_prog.cpp
  runtime/kernels/constrain.cpp
  runtime/kernels/eltwise_expr.cpp
  runtime/kernels/mixture.cpp
  runtime/src/nuts.cpp
  runtime/src/mir_reader.cpp
  runtime/src/lower.cpp
  runtime/src/reroll.cpp
  runtime/src/inplace.cpp
  runtime/src/data.cpp
  tests/tbb_stub.cpp)
target_include_directories(stanli_shared PUBLIC runtime/include)
target_link_libraries(stanli_shared PUBLIC stanmath sundials)
set_target_properties(stanli_shared PROPERTIES OUTPUT_NAME stanli)

# Export only the C API. Everything else in here (stan-math, Eigen, the
# whole embedded OCaml compiler) is an implementation detail, and leaving
# 15k symbols exported costs 2.7 MB of symbol table, string table, and
# export trie in the shipped library.
if(APPLE)
  target_link_options(stanli_shared PRIVATE
    "-Wl,-exported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/tools/exported_symbols.txt")
  set_property(TARGET stanli_shared APPEND PROPERTY LINK_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/exported_symbols.txt)
elseif(UNIX)
  target_link_options(stanli_shared PRIVATE
    "-Wl,--version-script,${CMAKE_CURRENT_SOURCE_DIR}/tools/exported_symbols.map")
  set_property(TARGET stanli_shared APPEND PROPERTY LINK_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/exported_symbols.map)
  # The wheel is one library dropped onto an unknown machine, and a
  # dynamic libstdc++ would make it require whatever GLIBCXX version the
  # build container's toolchain happened to emit. Statically linking the
  # C++ runtime leaves glibc as the only versioned dependency, which is
  # what the manylinux tag is actually promising.
  target_link_options(stanli_shared PRIVATE
    -static-libstdc++ -static-libgcc)
endif()

# In-process stanc3: link the OCaml-compiled embed object
# (tools/stanc_embed/build.sh produces deps/stanc3/stanc_embed.o).
set(STANLI_STANC_EMBED_OBJ "" CACHE FILEPATH
    "stanc_embed.o built by tools/stanc_embed/build.sh; enables in-process stanc")
set(STANLI_OCAML_STDLIB "" CACHE PATH
    "OCaml stdlib dir (opam switch lib/ocaml) for caml/*.h")
if(STANLI_STANC_EMBED_OBJ)
  target_sources(stanli_shared PRIVATE
    runtime/src/stanc_embed_c.cpp ${STANLI_STANC_EMBED_OBJ})
  target_include_directories(stanli_shared PRIVATE ${STANLI_OCAML_STDLIB})
  target_compile_definitions(stanli_shared PRIVATE STANLI_EMBED_STANC)
  target_link_libraries(stanli_shared PUBLIC dl pthread)
endif()

add_library(stanli STATIC
  runtime/src/executor.cpp
  runtime/kernels/elementwise.cpp
  runtime/kernels/densities.cpp
  runtime/kernels/legacy_fns.cpp
  runtime/kernels/matrix_fns.cpp
  runtime/kernels/ode.cpp
  runtime/src/ode_prog.cpp
  runtime/kernels/constrain.cpp
  runtime/kernels/eltwise_expr.cpp
  runtime/kernels/mixture.cpp
  runtime/src/nuts.cpp
  runtime/src/mir_reader.cpp
  runtime/src/lower.cpp
  runtime/src/reroll.cpp
  runtime/src/inplace.cpp
  runtime/src/data.cpp)
target_include_directories(stanli PUBLIC runtime/include)
target_link_libraries(stanli PUBLIC stanmath sundials)

enable_testing()
# Single-threaded tests stub the one TBB symbol stan-math's rev core needs.
function(stanli_add_test NAME)
  add_executable(${NAME} tests/${NAME}.cpp tests/tbb_stub.cpp)
  target_link_libraries(${NAME} PRIVATE stanli)
  # Tests run from the repo root so fixture paths are stable.
  add_test(NAME ${NAME} COMMAND ${NAME}
           WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endfunction()

add_executable(stanli_run tools/stanli_run.cpp tests/tbb_stub.cpp)
target_link_libraries(stanli_run PRIVATE stanli)
add_executable(stanli_check tools/stanli_check.cpp tests/tbb_stub.cpp)
target_link_libraries(stanli_check PRIVATE stanli)
add_executable(bench_grad tools/bench_grad.cpp tests/tbb_stub.cpp)
target_link_libraries(bench_grad PRIVATE stanli)
add_executable(bench_opcost tools/bench_opcost.cpp tests/tbb_stub.cpp)
target_link_libraries(bench_opcost PRIVATE stanli)
add_executable(bench_packet tools/bench_packet.cpp)
target_link_libraries(bench_packet PRIVATE stanmath)
add_executable(bench_density tools/bench_density.cpp tests/tbb_stub.cpp)
target_link_libraries(bench_density PRIVATE stanli)
add_executable(bench_varmat tools/bench_varmat.cpp tests/tbb_stub.cpp)
target_link_libraries(bench_varmat PRIVATE stanli)
add_executable(dump_ops tools/dump_ops.cpp tests/tbb_stub.cpp)
target_link_libraries(dump_ops PRIVATE stanli)

stanli_add_test(test_smoke)
stanli_add_test(test_executor)
stanli_add_test(test_recorder)
stanli_add_test(test_densities)
stanli_add_test(test_matvec)
stanli_add_test(test_legacy)
stanli_add_test(test_eight_schools)
stanli_add_test(test_glm)
stanli_add_test(test_nuts)
stanli_add_test(test_sexp)
stanli_add_test(test_mir)
stanli_add_test(test_transforms)
stanli_add_test(test_eltwise)
stanli_add_test(test_lower)
stanli_add_test(test_data)
stanli_add_test(test_e2e)
stanli_add_test(test_reroll)
stanli_add_test(test_inplace)
stanli_add_test(test_pass_safety)
stanli_add_test(test_sampling)
stanli_add_test(test_ode_prog)

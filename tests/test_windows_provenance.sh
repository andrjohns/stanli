#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

# The cross-compiler packages expose the version and target used by Dune. Mock
# only that opam interface so this contract can run on every development host.
source tools/stanc_embed/provenance.sh
configured_version=$(stanc_embed_read_setup OCAML_VERSION)
STANLI_TEST_OCAML_VERSION=$configured_version
STANLI_TEST_META_VERSION=$configured_version
STANLI_TEST_WINDOWS_TARGET=x86_64-w64-mingw32
STANLI_TEST_DUNE_VERSION=3.24.2

opam() {
  local command=${1:-}
  local final_argument=${!#}
  local value=
  case "$command:$final_argument" in
    list:ocaml-windows64)
      value=$STANLI_TEST_OCAML_VERSION
      ;;
    list:ocaml-windows)
      value=$STANLI_TEST_META_VERSION
      ;;
    list:dune)
      value=$STANLI_TEST_DUNE_VERSION
      ;;
    var:conf-gcc-windows64:host)
      value=$STANLI_TEST_WINDOWS_TARGET
      ;;
    switch:--short)
      value=cross-test
      ;;
    *)
      printf 'unexpected opam query: %s\n' "$*" >&2
      return 2
      ;;
  esac
  if [[ ${CLICOLOR_FORCE:-} == 1 && " $* " != *' --color=never '* ]]; then
    printf '\033[01;35m%s\033[0m\n' "$value"
  else
    printf '%s\n' "$value"
  fi
}

src_repo=$(stanc_embed_read_setup STANC3_SRC_REPO)
src_sha=$(stanc_embed_read_setup STANC3_SRC_SHA)

# setup-ocaml sets this in CI. The mock deliberately contaminates output from
# any opam query that fails to request machine-clean text.
CLICOLOR_FORCE=1
stamp=$(stanli_windows_cli_expected_stamp \
  "$src_repo" "$src_sha" cross-test)
grep -Fqx "ocaml_version=$configured_version" <<<"$stamp"
grep -Fqx 'ocaml_target=x86_64-w64-mingw32' <<<"$stamp"
grep -Fqx 'dune_version=3.24.2' <<<"$stamp"

test_dir=$(mktemp -d "${TMPDIR:-/tmp}/stanli-windows-provenance.XXXXXX")
trap 'find "$test_dir" -mindepth 1 -delete; rmdir "$test_dir"' EXIT
artifact="$test_dir/stanli-compile"
: >"$artifact"
printf '%s\n' "$stamp" >"$artifact.stamp"
stanli_windows_cli_artifact_matches \
  "$artifact" "$src_repo" "$src_sha" cross-test
stanli_windows_cli_artifact_matches "$artifact" "$src_repo" "$src_sha"

STANLI_TEST_OCAML_VERSION=0.0.0
if diagnostic=$(stanli_windows_cli_expected_stamp \
    "$src_repo" "$src_sha" cross-test 2>&1 >/dev/null); then
  echo "mismatched compiler version was accepted" >&2
  exit 1
fi
grep -Fq "ocaml-windows64 must be $configured_version" <<<"$diagnostic"
STANLI_TEST_OCAML_VERSION=$configured_version

STANLI_TEST_WINDOWS_TARGET=i686-w64-mingw32
if diagnostic=$(stanli_windows_cli_expected_stamp \
    "$src_repo" "$src_sha" cross-test 2>&1 >/dev/null); then
  echo "32-bit compiler target was accepted" >&2
  exit 1
fi
grep -Fq 'unexpected 64-bit target' <<<"$diagnostic"
STANLI_TEST_WINDOWS_TARGET=x86_64-w64-mingw32

STANLI_TEST_META_VERSION=0.0.0
if diagnostic=$(stanli_windows_cli_expected_stamp \
    "$src_repo" "$src_sha" cross-test 2>&1 >/dev/null); then
  echo "mismatched package versions were accepted" >&2
  exit 1
fi
grep -Fq 'OCaml package versions differ' <<<"$diagnostic"

echo "test_windows_provenance OK"

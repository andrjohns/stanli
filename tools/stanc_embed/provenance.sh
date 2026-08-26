#!/usr/bin/env bash
# Shared provenance helpers for the native and browser stanc artifacts.
# This file is sourced by build scripts; it is not intended to be run.

_stanli_embed_repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)

stanc_embed_read_setup() {
  local key=${1:?setup key}
  sed -n "s/^${key}=\\([^ ]*\\).*/\\1/p" \
    "$_stanli_embed_repo_root/tools/dev_setup.sh"
}

_stanc_embed_sha256_stream() {
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum | awk '{print $1}'
  elif command -v shasum >/dev/null 2>&1; then
    shasum -a 256 | awk '{print $1}'
  else
    python3 -c 'import hashlib, sys; print(hashlib.sha256(sys.stdin.buffer.read()).hexdigest())'
  fi
}

_stanc_embed_sha256_file() {
  local path=${1:?file to hash}
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$path" | awk '{print $1}'
  elif command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "$path" | awk '{print $1}'
  else
    python3 -c \
      'import hashlib, sys; print(hashlib.sha256(open(sys.argv[1], "rb").read()).hexdigest())' \
      "$path"
  fi
}

_stanc_embed_switch_exists() {
  local opam_switch=${1:?opam switch}
  command -v opam >/dev/null 2>&1 &&
    opam switch list --short 2>/dev/null | grep -Fqx "$opam_switch"
}

_stanc_embed_ocaml_version() {
  local opam_switch=${1:?opam switch}
  opam exec --switch="$opam_switch" -- ocamlc -version
}

_stanc_embed_ocaml_target() {
  local opam_switch=${1:?opam switch}
  opam exec --switch="$opam_switch" -- ocamlc -config-var target
}

_stanc_embed_package_version() {
  local opam_switch=${1:?opam switch}
  local package=${2:?opam package}
  opam list --switch="$opam_switch" --installed --short --columns=version \
    "$package" 2>/dev/null
}

_stanc_embed_stamp_value() {
  local stamp=${1:?provenance stamp}
  local key=${2:?provenance key}
  sed -n "s/^${key}=//p" "$stamp"
}

# Hash both the sorted relative names and contents of every producer input.
# A newly added encoder module or a change to the build recipe therefore
# invalidates an old complete object without another manually maintained list.
stanc_embed_inputs_sha256() {
  (
    cd "$_stanli_embed_repo_root"
    while IFS= read -r input; do
      printf '%s\n%s\n' "$input" "$(_stanc_embed_sha256_file "$input")"
    done < <(
      find compiler/native compiler/ocaml tools/stanc_embed \
        -maxdepth 1 -type f -print |
        LC_ALL=C sort
    )
  ) | _stanc_embed_sha256_stream
}

stanc_embed_expected_stamp() {
  local src_sha=${1:-$(stanc_embed_read_setup STANC3_SRC_SHA)}
  local opam_switch=${2:-$(stanc_embed_read_setup OPAM_SWITCH)}
  printf '%s\n' \
    'format=stanli-stanc-embed-v2' \
    "stanc3_src_sha=$src_sha" \
    "producer_inputs_sha256=$(stanc_embed_inputs_sha256)" \
    "ocaml_version=$(_stanc_embed_ocaml_version "$opam_switch")" \
    "ocaml_target=$(_stanc_embed_ocaml_target "$opam_switch")" \
    "dune_version=$(_stanc_embed_package_version "$opam_switch" dune)"
}

stanc_embed_artifact_matches() {
  local object=${1:?embedded object}
  local src_sha=${2:-$(stanc_embed_read_setup STANC3_SRC_SHA)}
  local opam_switch=${3:-$(stanc_embed_read_setup OPAM_SWITCH)}
  local stamp="${object}.stamp"
  [[ -f "$object" && -f "$stamp" ]] || return 1
  if _stanc_embed_switch_exists "$opam_switch"; then
    [[ "$(cat "$stamp")" == \
       "$(stanc_embed_expected_stamp "$src_sha" "$opam_switch")" ]]
    return
  fi

  # A CI cache hit deliberately skips the entire opam installation. In that
  # case validate every source-derived field and the configured OCaml version,
  # while requiring the producing target and Dune version to be recorded.
  [[ "$(_stanc_embed_stamp_value "$stamp" format)" == \
       'stanli-stanc-embed-v2' ]] &&
    [[ "$(_stanc_embed_stamp_value "$stamp" stanc3_src_sha)" == "$src_sha" ]] &&
    [[ "$(_stanc_embed_stamp_value "$stamp" producer_inputs_sha256)" == \
       "$(stanc_embed_inputs_sha256)" ]] &&
    [[ "$(_stanc_embed_stamp_value "$stamp" ocaml_version)" == \
       "$(stanc_embed_read_setup OCAML_VERSION)" ]] &&
    [[ -n "$(_stanc_embed_stamp_value "$stamp" ocaml_target)" ]] &&
    [[ -n "$(_stanc_embed_stamp_value "$stamp" dune_version)" ]]
}

stancjs_expected_stamp() {
  local src_repo=${1:?stanc3 source repository}
  local src_sha=${2:?stanc3 source revision}
  local opam_switch=${3:?opam switch}
  local jsoo_version
  jsoo_version=$(_stanc_embed_package_version "$opam_switch" js_of_ocaml)
  printf '%s\n' \
    'format=stanli-stancjs-v3' \
    "stanc3_src_repo=$src_repo" \
    "stanc3_src_sha=$src_sha" \
    "opam_switch=$opam_switch" \
    "ocaml_version=$(_stanc_embed_ocaml_version "$opam_switch")" \
    "ocaml_target=$(_stanc_embed_ocaml_target "$opam_switch")" \
    "dune_version=$(_stanc_embed_package_version "$opam_switch" dune)" \
    "js_of_ocaml_version=$jsoo_version" \
    'dune_profile=release' \
    'dune_subst=1'
}

stancjs_artifact_matches() {
  local artifact=${1:?stancjs artifact}
  local src_repo=${2:?stanc3 source repository}
  local src_sha=${3:?stanc3 source revision}
  local opam_switch=${4:?opam switch}
  local stamp="${artifact}.stamp"
  [[ -f "$artifact" && -f "$stamp" ]] || return 1
  if _stanc_embed_switch_exists "$opam_switch"; then
    [[ "$(cat "$stamp")" == \
       "$(stancjs_expected_stamp "$src_repo" "$src_sha" "$opam_switch")" ]]
    return
  fi

  [[ "$(_stanc_embed_stamp_value "$stamp" format)" == \
       'stanli-stancjs-v3' ]] &&
    [[ "$(_stanc_embed_stamp_value "$stamp" stanc3_src_repo)" == \
       "$src_repo" ]] &&
    [[ "$(_stanc_embed_stamp_value "$stamp" stanc3_src_sha)" == \
       "$src_sha" ]] &&
    [[ "$(_stanc_embed_stamp_value "$stamp" opam_switch)" == \
       "$opam_switch" ]] &&
    [[ "$(_stanc_embed_stamp_value "$stamp" ocaml_version)" == \
       "$(stanc_embed_read_setup OCAML_VERSION)" ]] &&
    [[ -n "$(_stanc_embed_stamp_value "$stamp" ocaml_target)" ]] &&
    [[ -n "$(_stanc_embed_stamp_value "$stamp" dune_version)" ]] &&
    [[ -n "$(_stanc_embed_stamp_value "$stamp" js_of_ocaml_version)" ]] &&
    [[ "$(_stanc_embed_stamp_value "$stamp" dune_profile)" == release ]] &&
    [[ "$(_stanc_embed_stamp_value "$stamp" dune_subst)" == 1 ]]
}

stanli_stancjs_inputs_sha256() {
  (
    cd "$_stanli_embed_repo_root"
    while IFS= read -r input; do
      printf '%s\n%s\n' "$input" "$(_stanc_embed_sha256_file "$input")"
    done < <(
      find compiler/js compiler/ocaml tools/stanc_embed \
        -maxdepth 1 -type f -print | LC_ALL=C sort
    )
  ) | _stanc_embed_sha256_stream
}

stanli_stancjs_expected_stamp() {
  local src_repo=${1:?stanc3 source repository}
  local src_sha=${2:?stanc3 source revision}
  local opam_switch=${3:?opam switch}
  local jsoo_version
  jsoo_version=$(_stanc_embed_package_version "$opam_switch" js_of_ocaml)
  printf '%s\n' \
    'format=stanli-portable-stancjs-v2' \
    "stanc3_src_repo=$src_repo" \
    "stanc3_src_sha=$src_sha" \
    "opam_switch=$opam_switch" \
    "ocaml_version=$(_stanc_embed_ocaml_version "$opam_switch")" \
    "ocaml_target=$(_stanc_embed_ocaml_target "$opam_switch")" \
    "dune_version=$(_stanc_embed_package_version "$opam_switch" dune)" \
    "js_of_ocaml_version=$jsoo_version" \
    "producer_inputs_sha256=$(stanli_stancjs_inputs_sha256)" \
    'dune_profile=release' \
    'dune_subst=1'
}

stanli_stancjs_artifact_matches() {
  local artifact=${1:?portable stancjs artifact}
  local src_repo=${2:?stanc3 source repository}
  local src_sha=${3:?stanc3 source revision}
  local opam_switch=${4:?opam switch}
  local stamp="${artifact}.stamp"
  [[ -f "$artifact" && -f "$stamp" ]] || return 1
  if _stanc_embed_switch_exists "$opam_switch"; then
    [[ "$(cat "$stamp")" == \
       "$(stanli_stancjs_expected_stamp \
          "$src_repo" "$src_sha" "$opam_switch")" ]]
    return
  fi

  [[ "$(_stanc_embed_stamp_value "$stamp" format)" == \
       'stanli-portable-stancjs-v2' ]] &&
    [[ "$(_stanc_embed_stamp_value "$stamp" stanc3_src_repo)" == \
       "$src_repo" ]] &&
    [[ "$(_stanc_embed_stamp_value "$stamp" stanc3_src_sha)" == \
       "$src_sha" ]] &&
    [[ "$(_stanc_embed_stamp_value "$stamp" opam_switch)" == \
       "$opam_switch" ]] &&
    [[ "$(_stanc_embed_stamp_value "$stamp" ocaml_version)" == \
       "$(stanc_embed_read_setup OCAML_VERSION)" ]] &&
    [[ -n "$(_stanc_embed_stamp_value "$stamp" ocaml_target)" ]] &&
    [[ -n "$(_stanc_embed_stamp_value "$stamp" dune_version)" ]] &&
    [[ -n "$(_stanc_embed_stamp_value "$stamp" js_of_ocaml_version)" ]] &&
    [[ "$(_stanc_embed_stamp_value "$stamp" producer_inputs_sha256)" == \
       "$(stanli_stancjs_inputs_sha256)" ]] &&
    [[ "$(_stanc_embed_stamp_value "$stamp" dune_profile)" == release ]] &&
    [[ "$(_stanc_embed_stamp_value "$stamp" dune_subst)" == 1 ]]
}

type error =
  | Frontend_error of Frontend.Errors.t
  | Internal_error of string

type 'a compilation =
  { result: ('a, error) result
  ; warnings: Frontend.Warnings.t list }

type pass_selection = {vectorize_loops: bool}

let default_pass_selection = {vectorize_loops= true}

let compile_mir_at_level
    ?(include_source = Driver.Flags.default.include_source) ~optimization_level
    ~model_name (code : string) =
  let flags =
    {Driver.Flags.default with optimization_level; include_source} in
  let warnings = ref [] in
  let output : Driver.Entry.other_output -> unit = function
    | Warnings emitted -> warnings := !warnings @ emitted
    | Formatted _ | DebugOutput _ | Memory_patterns _ | Info _ | Version _
     |Generated _ ->
        () in
  let result =
    match
      Common.ICE.with_exn_message (fun () ->
          Driver.Entry.stan2mir model_name (`Code code) flags output)
    with
    | Error internal -> Error (Internal_error internal)
    | Ok (Error error) -> Error (Frontend_error error)
    | Ok (Ok mir) -> Ok mir in
  {result; warnings= !warnings}

let compile_mir_with_passes ?include_source ~passes ~model_name code =
  let compiled =
    compile_mir_at_level ?include_source
      ~optimization_level:Analysis_and_optimization.Optimize.O0 ~model_name code
  in
  let result =
    match compiled.result with
    | Error error -> Error error
    | Ok mir -> (
      let settings =
        { (Analysis_and_optimization.Optimize.level_optimizations O1) with
          vectorize_loops= passes.vectorize_loops } in
      match
        Common.ICE.with_exn_message (fun () ->
            Analysis_and_optimization.Optimize.optimization_suite ~settings
              mir)
      with
      | Error internal -> Error (Internal_error internal)
      | Ok optimized -> Ok optimized ) in
  {result; warnings= compiled.warnings}

let compile_mir ?include_source ~model_name code =
  compile_mir_with_passes ?include_source ~passes:default_pass_selection
    ~model_name code

let compile_portable ?include_source ~model_name code =
  let compiled = compile_mir ?include_source ~model_name code in
  let result =
    match compiled.result with
    | Error error -> Error error
    | Ok mir -> (
      match
        Common.ICE.with_exn_message (fun () -> Portable_mir.encode mir)
      with
      | Error internal -> Error (Internal_error internal)
      | Ok encoded -> Ok encoded ) in
  {result; warnings= compiled.warnings}

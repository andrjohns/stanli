type error =
  | Frontend_error of Frontend.Errors.t
  | Internal_error of string

type 'a compilation =
  { result: ('a, error) result
  ; warnings: Frontend.Warnings.t list }

let compile_mir ?(include_source = Driver.Flags.default.include_source)
    ~model_name (code : string) =
  let flags =
    { Driver.Flags.default with
      optimization_level= Analysis_and_optimization.Optimize.O1
    ; include_source } in
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

let compile_portable ?include_source ~model_name code =
  let compiled = compile_mir ?include_source ~model_name code in
  let result =
    match compiled.result with
    | Error error -> Error error
    | Ok mir -> (
      match Common.ICE.with_exn_message (fun () -> Portable_mir.encode mir) with
      | Error internal -> Error (Internal_error internal)
      | Ok encoded -> Ok encoded ) in
  {result; warnings= compiled.warnings}

(* stanli embedding entry point: Stan code -> portable-MIR JSON (stanc3 --O1:
   inlining, constant/copy propagation, dead code elimination, partial
   evaluation). Registered for C via Callback; built with -output-complete-obj
   so the OCaml runtime rides inside one object file linked into libstanli.
   Return protocol: "OK<json>" or "ERR<message>". *)

let compile_tmir (code : string) : string =
  let compilation =
    Stanli_pipeline.compile_portable ~model_name:"embedded_model" code in
  match compilation.result with
  | Error (Stanli_pipeline.Internal_error message) -> "ERR" ^ message
  | Error (Stanli_pipeline.Frontend_error error) ->
      "ERR"
      ^ Fmt.str "%a"
          (Frontend.Errors.pp ?printed_filename:None ~code)
          error
  | Ok encoded -> "OK" ^ encoded

let () =
  ignore (Thread.self ());
  Stdlib.Callback.register "stanc_compile_tmir" compile_tmir

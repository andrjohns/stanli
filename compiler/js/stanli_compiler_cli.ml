let () =
  if Array.length Sys.argv <> 2 then (
    prerr_endline "usage: stanli_compiler_cli MODEL.stan";
    exit 2);
  let code = In_channel.with_open_bin Sys.argv.(1) In_channel.input_all in
  let compilation =
    Stanli_pipeline.compile_portable ~model_name:"embedded_model" code
      ~include_source:
        (Frontend.Include_files.FileSystemPaths [Filename.dirname Sys.argv.(1)])
  in
  List.iter
    (fun warning ->
      Fmt.epr "%a@." (Frontend.Warnings.pp ?printed_filename:None) warning)
    compilation.warnings;
  List.iter
    (fun diagnostic ->
      prerr_endline (Stanli_pipeline.diagnostic_message diagnostic))
    compilation.diagnostics;
  match compilation.result with
  | Ok encoded -> print_string encoded
  | Error (Stanli_pipeline.Internal_error message) ->
      prerr_endline message;
      exit 1
  | Error (Stanli_pipeline.Frontend_error error) ->
      Fmt.epr "%a@."
        (Frontend.Errors.pp ?printed_filename:None ~code)
        error;
      exit 1

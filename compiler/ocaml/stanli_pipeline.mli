type error =
  | Frontend_error of Frontend.Errors.t
  | Internal_error of string

type 'a compilation =
  { result: ('a, error) result
  ; warnings: Frontend.Warnings.t list }

val compile_mir :
     ?include_source:Frontend.Include_files.t
  -> model_name:string
  -> string
  -> Middle.Program.Typed.t compilation
(** Parse, typecheck, apply the Stan Math MIR transform, and run the exact
    optimization policy selected by stanli. *)

val compile_portable :
     ?include_source:Frontend.Include_files.t
  -> model_name:string
  -> string
  -> string compilation
(** Compile Stan source and encode canonical portable MIR v1. *)

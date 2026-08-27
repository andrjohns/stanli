type error =
  | Frontend_error of Frontend.Errors.t
  | Internal_error of string

type 'a compilation =
  { result: ('a, error) result
  ; warnings: Frontend.Warnings.t list }

(** Source-level MIR passes that stanli may add to upstream's O1 policy. *)
type pass_selection =
  { vectorize_loops: bool
  ; distribute_same_lane_density_loops: bool }

val default_pass_selection : pass_selection
(** The shipping selection: upstream O1 plus loop vectorization. *)

val compile_mir :
     ?include_source:Frontend.Include_files.t
  -> model_name:string
  -> string
  -> Middle.Program.Typed.t compilation
(** Parse, typecheck, apply the Stan Math MIR transform, and run the exact
    optimization policy selected by stanli. *)

val compile_mir_with_passes :
     ?include_source:Frontend.Include_files.t
  -> passes:pass_selection
  -> model_name:string
  -> string
  -> Middle.Program.Typed.t compilation
(** Internal pass-selection entrypoint. It obtains backend-transformed MIR at
    O0, then applies upstream O1 with only the selected additive passes. *)

val compile_portable :
     ?include_source:Frontend.Include_files.t
  -> model_name:string
  -> string
  -> string compilation
(** Compile Stan source and encode canonical compact portable MIR v2. *)

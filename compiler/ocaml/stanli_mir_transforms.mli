(** Stanli-owned rewrites over stanc3's typed MIR. *)

val distribute_same_lane_density_loops :
  Middle.Program.Typed.t -> Middle.Program.Typed.t
(** Split an assignment followed by a density into two loops when the density
    only reads the assigned value at the current loop lane. Candidates with
    effects, RNG calls, or cross-lane reads are left unchanged. *)

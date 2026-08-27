open Middle

type assigned_value_use = Absent | Same_lane | Unsupported

let combine_use left right =
  match (left, right) with
  | Unsupported, _ | _, Unsupported -> Unsupported
  | Same_lane, _ | _, Same_lane -> Same_lane
  | Absent, Absent -> Absent

let rec assigned_value_use ~assigned ~loopvar (expr : Expr.Typed.t) =
  match expr.pattern with
  | Indexed ({pattern= Var name; _}, [Single {pattern= Var index; _}])
    when String.equal name assigned && String.equal index loopvar ->
      Same_lane
  | Var name when String.equal name assigned -> Unsupported
  | pattern ->
      Expr.Pattern.fold
        (fun use child ->
          combine_use use (assigned_value_use ~assigned ~loopvar child))
        Absent pattern

let rec expression_matches predicate (expr : Expr.Typed.t) =
  match expr.pattern with
  | Indexed (base, indices) ->
      expression_matches predicate base
      || List.exists (index_matches predicate) indices
  | pattern ->
      predicate expr
      || Expr.Pattern.fold
           (fun found child -> found || expression_matches predicate child)
           false pattern

and index_matches predicate index =
  Index.fold
    (fun found child -> found || expression_matches predicate child)
    false index

let can_have_effect (expr : Expr.Typed.t) =
  match expr.pattern with
  | FunApp
      ( ( UserDefined (_, (Fun_kind.FnTarget | FnRng))
        | StanLib (_, (FnTarget | FnRng), _) )
      , _ ) ->
      true
  | FunApp (CompilerInternal internal, _) ->
      Internal_fun.can_side_effect internal
  | _ -> false

let is_effect_free expr = not (expression_matches can_have_effect expr)

let direct_density = function
  | Expr.{pattern= FunApp (StanLib (_, (Fun_kind.FnLpdf _ | FnLpmf _), _), _); _}
    ->
      true
  | _ -> false

let body_statements (body : Stmt.Located.t) =
  match body.pattern with
  | Block [assignment; density] | SList [assignment; density] ->
      Some (assignment, density)
  | _ -> None

let singleton_body (body : Stmt.Located.t) statement =
  match body.pattern with
  | Block _ -> {body with pattern= Block [statement]}
  | SList _ -> {body with pattern= SList [statement]}
  | _ -> body

let distribute_statement (statement : Stmt.Located.t) =
  let unchanged () = statement in
  match statement.pattern with
  | For {loopvar; lower; upper; body} -> (
      match body_statements body with
      | Some
          ( ({ pattern=
                 Assignment
                   ( (LVariable assigned, [Single {pattern= Var index; _}])
                   , _
                   , rhs )
             ; _ } as assignment)
          , ({pattern= TargetPE density; _} as target) )
        when String.equal index loopvar && direct_density density
             && assigned_value_use ~assigned ~loopvar rhs = Absent
             && assigned_value_use ~assigned ~loopvar lower = Absent
             && assigned_value_use ~assigned ~loopvar upper = Absent
             && assigned_value_use ~assigned ~loopvar density = Same_lane
             && is_effect_free rhs && is_effect_free lower
             && is_effect_free upper && is_effect_free density ->
          let make_loop body_statement =
            { statement with
              pattern=
                For
                  { loopvar
                  ; lower
                  ; upper
                  ; body= singleton_body body body_statement } } in
          { statement with
            pattern= SList [make_loop assignment; make_loop target] }
      | _ -> unchanged ())
  | _ -> unchanged ()

let distribute_same_lane_density_loops mir =
  let rewrite statement =
    Stmt.rewrite_bottom_up ~f:Fun.id ~g:distribute_statement statement in
  let rewrite_list = List.map rewrite in
  let functions_block =
    List.map
      (fun (function_ : Stmt.Located.t Program.fun_def) ->
        {function_ with fdbody= Option.map rewrite function_.fdbody})
      mir.Program.functions_block in
  { mir with
    functions_block
  ; prepare_data= rewrite_list mir.prepare_data
  ; transform_inits= rewrite_list mir.transform_inits
  ; log_prob= rewrite_list mir.log_prob
  ; reverse_mode_log_prob= rewrite_list mir.reverse_mode_log_prob
  ; generate_quantities= rewrite_list mir.generate_quantities }

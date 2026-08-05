((functions_block ()) (input_vars ((K <opaque> SInt)))
 (prepare_data
  (((pattern
     (Decl (decl_adtype DataOnly) (decl_id K) (decl_type (Sized SInt))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable K) ()) UInt
      ((pattern
        (Indexed
         ((pattern
           (FunApp (CompilerInternal FnReadData)
            (((pattern (Lit Str K))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
          (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
         ((Single
           ((pattern (Lit Int 1))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnCheck
        (trans
         (Lower
          ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
        (var_name K)
        (var ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp (CompilerInternal FnValidateSizePositive)
      (((pattern (Lit Str theta))
        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))))
 (log_prob
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id theta)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Simplex)
             (dims
              (((pattern (Var K))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib dirichlet_lpdf (FnLpdf true) AoS)
             (((pattern (Var theta))
               (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern
                (FunApp (StanLib rep_vector FnPlain AoS)
                 (((pattern (Lit Real 2.0))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern (Var K))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
               (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>)))))
    (meta <opaque>))))
 (reverse_mode_log_prob
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id theta)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Simplex)
             (dims
              (((pattern (Var K))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib dirichlet_lpdf (FnLpdf true) AoS)
             (((pattern (Var theta))
               (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern
                (FunApp (StanLib rep_vector FnPlain AoS)
                 (((pattern (Lit Real 2.0))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern (Var K))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
               (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>)))))
    (meta <opaque>))))
 (generate_quantities
  (((pattern
     (Decl (decl_adtype DataOnly) (decl_id theta)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Simplex)
             (dims
              (((pattern (Var K))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt ())
        (var
         ((pattern (Var theta))
          (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))
   ((pattern
     (IfElse
      ((pattern
        (FunApp (StanLib PNot__ FnPlain AoS)
         (((pattern
            (EOr
             ((pattern (Var emit_transformed_parameters__))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
             ((pattern (Var emit_generated_quantities__))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
      ((pattern (Block (((pattern (Return ())) (meta <opaque>))))) (meta <opaque>)) ()))
    (meta <opaque>))
   ((pattern
     (IfElse
      ((pattern
        (FunApp (StanLib PNot__ FnPlain AoS)
         (((pattern (Var emit_generated_quantities__))
           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
      ((pattern (Block (((pattern (Return ())) (meta <opaque>))))) (meta <opaque>)) ()))
    (meta <opaque>))))
 (transform_inits
  (((pattern
     (Decl (decl_adtype DataOnly) (decl_id pos__) (decl_type (Sized SInt))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable pos__) ()) UInt
      ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id theta)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id theta_flat__)
          (decl_type (Unsized (UArray UReal))) (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable theta_flat__) ()) (UArray UReal)
          ((pattern
            (FunApp (CompilerInternal FnReadData)
             (((pattern (Lit Str theta))
               (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel DataOnly))))))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable pos__) ()) UInt
          ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
        (meta <opaque>))
       ((pattern
         (For (loopvar sym1__)
          (lower
           ((pattern (Lit Int 1))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (upper
           ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (Assignment
                  ((LVariable theta)
                   ((Single
                     ((pattern (Var sym1__))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  UVector
                  ((pattern
                    (Indexed
                     ((pattern (Var theta_flat__))
                      (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel DataOnly))))
                     ((Single
                       ((pattern (Var pos__))
                        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))))
                (meta <opaque>))
               ((pattern
                 (Assignment ((LVariable pos__) ()) UInt
                  ((pattern
                    (FunApp (StanLib Plus__ FnPlain AoS)
                     (((pattern (Var pos__))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                      ((pattern (Lit Int 1))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                (meta <opaque>)))))
            (meta <opaque>)))))
        (meta <opaque>)))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt (Simplex))
        (var
         ((pattern (Var theta))
          (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))))
 (unconstrain_array
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id theta)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable theta) ()) UVector
      ((pattern
        (FunApp (CompilerInternal FnReadDeserializer)
         (((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
       (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt (Simplex))
        (var
         ((pattern (Var theta))
          (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))))
 (output_vars
  ((theta <opaque>
    ((out_unconstrained_st
      (SVector AoS
       ((pattern
         (FunApp (StanLib Minus__ FnPlain AoS)
          (((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
           ((pattern (Lit Int 1))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_constrained_st
      (SVector AoS
       ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_block Parameters) (out_trans Simplex)))))
 (prog_name simp_model) (prog_path tests/fixtures/simp.stan))
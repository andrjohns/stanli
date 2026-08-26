((functions_block ()) (input_vars ())
 (prepare_data
  (((pattern
     (NRFunApp (CompilerInternal FnValidateSize)
      (((pattern (Lit Str uninitialized))
        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp (CompilerInternal FnValidateSize)
      (((pattern (Lit Str uninitialized))
        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))))
 (log_prob
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id theta) (decl_type (Sized SReal))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Identity) (dims ()) (mem_pattern AoS)))
           ()))
         (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id uninitialized)
      (decl_type
       (Sized
        (SMatrix AoS
         ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
         ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id selected) (decl_type (Sized SReal))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable selected) ()) UReal
      ((pattern
        (Promotion
         ((pattern (Lit Int 0))
          (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
         UReal AutoDiffable))
       (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
    (meta <opaque>))
   ((pattern
     (IfElse
      ((pattern
        (FunApp (StanLib Greater__ FnPlain AoS)
         (((pattern (Var theta))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
          ((pattern (Lit Int 0)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
       (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
      ((pattern
        (Block
         (((pattern
            (Assignment ((LVariable selected) ()) UReal
             ((pattern
               (Indexed
                ((pattern (Var uninitialized))
                 (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                ((Single
                  ((pattern (Lit Int 1))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                 (Single
                  ((pattern (Lit Int 1))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
              (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
           (meta <opaque>)))))
       (meta <opaque>))
      ()))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib std_normal_lpdf (FnLpdf true) AoS)
             (((pattern (Var theta))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern (Var selected))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>)))))
    (meta <opaque>))))
 (reverse_mode_log_prob
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id theta) (decl_type (Sized SReal))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Identity) (dims ()) (mem_pattern SoA)))
           ()))
         (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id uninitialized)
      (decl_type
       (Sized
        (SMatrix SoA
         ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
         ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id selected) (decl_type (Sized SReal))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable selected) ()) UReal
      ((pattern
        (Promotion
         ((pattern (Lit Int 0))
          (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
         UReal AutoDiffable))
       (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
    (meta <opaque>))
   ((pattern
     (IfElse
      ((pattern
        (FunApp (StanLib Greater__ FnPlain SoA)
         (((pattern (Var theta))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
          ((pattern (Lit Int 0)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
       (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
      ((pattern
        (Block
         (((pattern
            (Assignment ((LVariable selected) ()) UReal
             ((pattern
               (Indexed
                ((pattern (Var uninitialized))
                 (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                ((Single
                  ((pattern (Lit Int 1))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                 (Single
                  ((pattern (Lit Int 1))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
              (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
           (meta <opaque>)))))
       (meta <opaque>))
      ()))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib std_normal_lpdf (FnLpdf true) SoA)
             (((pattern (Var theta))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern (Var selected))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>)))))
    (meta <opaque>))))
 (generate_quantities
  (((pattern
     (Decl (decl_adtype DataOnly) (decl_id theta) (decl_type (Sized SReal))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Identity) (dims ()) (mem_pattern AoS)))
           ()))
         (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id uninitialized)
      (decl_type
       (Sized
        (SMatrix AoS
         ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
         ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id selected) (decl_type (Sized SReal))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt ())
        (var
         ((pattern (Var theta)) (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
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
     (Assignment ((LVariable selected) ()) UReal
      ((pattern
        (Promotion
         ((pattern (Lit Int 0))
          (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
         UReal AutoDiffable))
       (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
    (meta <opaque>))
   ((pattern
     (IfElse
      ((pattern
        (FunApp (StanLib Greater__ FnPlain AoS)
         (((pattern (Var theta))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
          ((pattern (Lit Int 0)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
       (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
      ((pattern
        (Block
         (((pattern
            (Assignment ((LVariable selected) ()) UReal
             ((pattern
               (Indexed
                ((pattern (Var uninitialized))
                 (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                ((Single
                  ((pattern (Lit Int 1))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                 (Single
                  ((pattern (Lit Int 1))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
              (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
           (meta <opaque>)))))
       (meta <opaque>))
      ()))
    (meta <opaque>))
   ((pattern
     (IfElse
      ((pattern (Var emit_transformed_parameters__))
       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
      ((pattern
        (Block
         (((pattern
            (NRFunApp
             (CompilerInternal
              (FnWriteParam (unconstrain_opt ())
               (var
                ((pattern (Var uninitialized))
                 (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
             ()))
           (meta <opaque>))
          ((pattern
            (NRFunApp
             (CompilerInternal
              (FnWriteParam (unconstrain_opt ())
               (var
                ((pattern (Var selected))
                 (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
             ()))
           (meta <opaque>)))))
       (meta <opaque>))
      ()))
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
     (Decl (decl_adtype AutoDiffable) (decl_id theta) (decl_type (Sized SReal))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable theta) ()) UReal
      ((pattern
        (Indexed
         ((pattern
           (FunApp (CompilerInternal FnReadData)
            (((pattern (Lit Str theta))
              (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
          (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel DataOnly))))
         ((Single
           ((pattern (Lit Int 1))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
       (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt (Identity))
        (var
         ((pattern (Var theta)) (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))))
 (unconstrain_array
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id theta) (decl_type (Sized SReal))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable theta) ()) UReal
      ((pattern (FunApp (CompilerInternal FnReadDeserializer) ()))
       (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt (Identity))
        (var
         ((pattern (Var theta)) (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))))
 (output_vars
  ((theta <opaque>
    ((out_unconstrained_st SReal) (out_constrained_st SReal) (out_block Parameters)
     (out_trans Identity)))
   (uninitialized <opaque>
    ((out_unconstrained_st
      (SMatrix AoS
       ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_constrained_st
      (SMatrix AoS
       ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_block TransformedParameters) (out_trans Identity)))
   (selected <opaque>
    ((out_unconstrained_st SReal) (out_constrained_st SReal)
     (out_block TransformedParameters) (out_trans Identity)))))
 (prog_name paramcond_uninitialized_livein_model)
 (prog_path tests/fixtures/paramcond_uninitialized_livein.stan))
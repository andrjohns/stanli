((functions_block
  (((fdrt (ReturnType UMatrix)) (fdname shape_design) (fdsuffix FnPlain)
    (fdargs ((AutoDiffable mat UMatrix)))
    (fdbody
     (((pattern
        (Block
         (((pattern
            (IfElse
             ((pattern
               (FunApp (StanLib Equals__ FnPlain AoS)
                (((pattern
                   (FunApp (StanLib rows FnPlain AoS)
                    (((pattern (Var mat))
                      (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 ((pattern (Lit Int 0))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
             ((pattern
               (Block
                (((pattern
                   (Return
                    (((pattern
                       (Promotion
                        ((pattern
                          (FunApp (StanLib rep_matrix FnPlain AoS)
                           (((pattern (Lit Real 1.0))
                             (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                            ((pattern (Lit Int 1))
                             (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                            ((pattern (Lit Int 1))
                             (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                         (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly))))
                        UReal AutoDiffable))
                      (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                  (meta <opaque>)))))
              (meta <opaque>))
             ()))
           (meta <opaque>))
          ((pattern
            (Return
             (((pattern
                (Promotion
                 ((pattern
                   (FunApp (StanLib rep_matrix FnPlain AoS)
                    (((pattern (Lit Real 2.0))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Lit Int 1))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Lit Int 1))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly))))
                 UReal AutoDiffable))
               (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta <opaque>)))))
       (meta <opaque>))))
    (fdloc <opaque>))))
 (input_vars
  ((n <opaque> SInt)
   (y <opaque>
    (SVector AoS
     ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
 (prepare_data
  (((pattern
     (Decl (decl_adtype DataOnly) (decl_id pos__) (decl_type (Sized SInt))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable pos__) ()) UInt
      ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id n) (decl_type (Sized SInt))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable n) ()) UInt
      ((pattern
        (Indexed
         ((pattern
           (FunApp (CompilerInternal FnReadData)
            (((pattern (Lit Str n))
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
          ((pattern (Lit Int 0)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
        (var_name n)
        (var ((pattern (Var n)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (((pattern (Lit Int 0)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id y)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id y_flat__)
          (decl_type (Unsized (UArray UReal))) (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable y_flat__) ()) (UArray UReal)
          ((pattern
            (FunApp (CompilerInternal FnReadData)
             (((pattern (Lit Str y))
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
           ((pattern (Lit Int 1))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (Assignment
                  ((LVariable y)
                   ((Single
                     ((pattern (Var sym1__))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  UVector
                  ((pattern
                    (Indexed
                     ((pattern (Var y_flat__))
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
     (NRFunApp (CompilerInternal FnValidateSize)
      (((pattern (Lit Str probe))
        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str n)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Var n)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp (CompilerInternal FnValidateSize)
      (((pattern (Lit Str beta)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))))
 (log_prob
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id probe)
      (decl_type
       (Sized
        (SMatrix AoS
         ((pattern (Var n)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
         ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Identity)
             (dims
              (((pattern (Var n))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Int 2))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id alpha) (decl_type (Sized SReal))
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
     (Decl (decl_adtype AutoDiffable) (decl_id beta)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Identity)
             (dims
              (((pattern (Lit Int 1))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id inline_shape_design_return_sym4__)
          (decl_type
           (Sized
            (SMatrix AoS
             ((pattern (Lit Int 0))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
             ((pattern (Lit Int 0))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
          (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Decl (decl_adtype DataOnly)
          (decl_id inline_shape_design_early_ret_check_sym5__) (decl_type (Sized SInt))
          (initialize Default)))
        (meta <opaque>))
       ((pattern
         (For (loopvar inline_shape_design_iterator_sym6__)
          (lower
           ((pattern (Lit Int 1))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (upper
           ((pattern (Lit Int 1))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (IfElse
                  ((pattern
                    (FunApp (StanLib Equals__ FnPlain AoS)
                     (((pattern
                        (FunApp (StanLib rows FnPlain AoS)
                         (((pattern (Var probe))
                           (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                      ((pattern (Lit Int 0))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern
                    (Block
                     (((pattern
                        (Assignment ((LVariable inline_shape_design_return_sym4__) ())
                         UMatrix
                         ((pattern
                           (Promotion
                            ((pattern
                              (FunApp (StanLib rep_matrix FnPlain AoS)
                               (((pattern (Lit Real 1.0))
                                 (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                                ((pattern (Lit Int 1))
                                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                ((pattern (Lit Int 1))
                                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                             (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly))))
                            UReal AutoDiffable))
                          (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))))
                       (meta <opaque>))
                      ((pattern Break) (meta <opaque>)))))
                   (meta <opaque>))
                  ()))
                (meta <opaque>))
               ((pattern
                 (Assignment ((LVariable inline_shape_design_return_sym4__) ()) UMatrix
                  ((pattern
                    (Promotion
                     ((pattern
                       (FunApp (StanLib rep_matrix FnPlain AoS)
                        (((pattern (Lit Real 2.0))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern (Lit Int 1))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern (Lit Int 1))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly))))
                     UReal AutoDiffable))
                   (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))))
                (meta <opaque>))
               ((pattern Break) (meta <opaque>)))))
            (meta <opaque>)))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib normal_id_glm_lpdf (FnLpdf true) AoS)
             (((pattern (Var y))
               (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly))))
              ((pattern (Var inline_shape_design_return_sym4__))
               (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern (Var alpha))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern (Var beta))
               (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern
                (Promotion
                 ((pattern (Lit Int 1))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal DataOnly))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>)))))
    (meta <opaque>))))
 (reverse_mode_log_prob
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id probe)
      (decl_type
       (Sized
        (SMatrix SoA
         ((pattern (Var n)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
         ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Identity)
             (dims
              (((pattern (Var n))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Int 2))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern SoA)))
           ()))
         (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id alpha) (decl_type (Sized SReal))
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
     (Decl (decl_adtype AutoDiffable) (decl_id beta)
      (decl_type
       (Sized
        (SVector SoA
         ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Identity)
             (dims
              (((pattern (Lit Int 1))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern SoA)))
           ()))
         (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id inline_shape_design_return_sym1__)
          (decl_type
           (Sized
            (SMatrix SoA
             ((pattern (Lit Int 0))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
             ((pattern (Lit Int 0))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
          (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Decl (decl_adtype DataOnly)
          (decl_id inline_shape_design_early_ret_check_sym2__) (decl_type (Sized SInt))
          (initialize Default)))
        (meta <opaque>))
       ((pattern
         (For (loopvar inline_shape_design_iterator_sym3__)
          (lower
           ((pattern (Lit Int 1))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (upper
           ((pattern (Lit Int 1))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (IfElse
                  ((pattern
                    (FunApp (StanLib Equals__ FnPlain SoA)
                     (((pattern
                        (FunApp (StanLib rows FnPlain SoA)
                         (((pattern (Var probe))
                           (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                      ((pattern (Lit Int 0))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern
                    (Block
                     (((pattern
                        (Assignment ((LVariable inline_shape_design_return_sym1__) ())
                         UMatrix
                         ((pattern
                           (Promotion
                            ((pattern
                              (FunApp (StanLib rep_matrix FnPlain SoA)
                               (((pattern (Lit Real 1.0))
                                 (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                                ((pattern (Lit Int 1))
                                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                ((pattern (Lit Int 1))
                                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                             (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly))))
                            UReal AutoDiffable))
                          (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))))
                       (meta <opaque>))
                      ((pattern Break) (meta <opaque>)))))
                   (meta <opaque>))
                  ()))
                (meta <opaque>))
               ((pattern
                 (Assignment ((LVariable inline_shape_design_return_sym1__) ()) UMatrix
                  ((pattern
                    (Promotion
                     ((pattern
                       (FunApp (StanLib rep_matrix FnPlain SoA)
                        (((pattern (Lit Real 2.0))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern (Lit Int 1))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern (Lit Int 1))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly))))
                     UReal AutoDiffable))
                   (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))))
                (meta <opaque>))
               ((pattern Break) (meta <opaque>)))))
            (meta <opaque>)))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib normal_id_glm_lpdf (FnLpdf true) SoA)
             (((pattern (Var y))
               (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly))))
              ((pattern (Var inline_shape_design_return_sym1__))
               (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern (Var alpha))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern (Var beta))
               (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern
                (Promotion
                 ((pattern (Lit Int 1))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal DataOnly))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>)))))
    (meta <opaque>))))
 (generate_quantities
  (((pattern
     (Decl (decl_adtype DataOnly) (decl_id probe)
      (decl_type
       (Sized
        (SMatrix AoS
         ((pattern (Var n)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
         ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Identity)
             (dims
              (((pattern (Var n))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Int 2))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id alpha) (decl_type (Sized SReal))
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
     (Decl (decl_adtype DataOnly) (decl_id beta)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Identity)
             (dims
              (((pattern (Lit Int 1))
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
         ((pattern (Var probe))
          (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt ())
        (var
         ((pattern (Var alpha)) (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt ())
        (var
         ((pattern (Var beta))
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
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable pos__) ()) UInt
      ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id probe)
      (decl_type
       (Sized
        (SMatrix AoS
         ((pattern (Var n)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
         ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id probe_flat__)
          (decl_type (Unsized (UArray UReal))) (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable probe_flat__) ()) (UArray UReal)
          ((pattern
            (FunApp (CompilerInternal FnReadData)
             (((pattern (Lit Str probe))
               (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
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
           ((pattern (Lit Int 2))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (For (loopvar sym2__)
                  (lower
                   ((pattern (Lit Int 1))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (upper
                   ((pattern (Var n))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (body
                   ((pattern
                     (Block
                      (((pattern
                         (Assignment
                          ((LVariable probe)
                           ((Single
                             ((pattern (Var sym2__))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                            (Single
                             ((pattern (Var sym1__))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                          UMatrix
                          ((pattern
                            (Indexed
                             ((pattern (Var probe_flat__))
                              (meta
                               ((type_ (UArray UReal)) (loc <opaque>) (adlevel DataOnly))))
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
            (meta <opaque>)))))
        (meta <opaque>)))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt (Identity))
        (var
         ((pattern (Var probe))
          (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id alpha) (decl_type (Sized SReal))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable alpha) ()) UReal
      ((pattern
        (Indexed
         ((pattern
           (FunApp (CompilerInternal FnReadData)
            (((pattern (Lit Str alpha))
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
         ((pattern (Var alpha)) (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id beta)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id beta_flat__)
          (decl_type (Unsized (UArray UReal))) (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable beta_flat__) ()) (UArray UReal)
          ((pattern
            (FunApp (CompilerInternal FnReadData)
             (((pattern (Lit Str beta))
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
           ((pattern (Lit Int 1))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (Assignment
                  ((LVariable beta)
                   ((Single
                     ((pattern (Var sym1__))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  UVector
                  ((pattern
                    (Indexed
                     ((pattern (Var beta_flat__))
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
       (FnWriteParam (unconstrain_opt (Identity))
        (var
         ((pattern (Var beta))
          (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))))
 (unconstrain_array
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id probe)
      (decl_type
       (Sized
        (SMatrix AoS
         ((pattern (Var n)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
         ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable probe) ()) UMatrix
      ((pattern
        (FunApp (CompilerInternal FnReadDeserializer)
         (((pattern (Var n)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
          ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
       (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt (Identity))
        (var
         ((pattern (Var probe))
          (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id alpha) (decl_type (Sized SReal))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable alpha) ()) UReal
      ((pattern (FunApp (CompilerInternal FnReadDeserializer) ()))
       (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt (Identity))
        (var
         ((pattern (Var alpha)) (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id beta)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable beta) ()) UVector
      ((pattern
        (FunApp (CompilerInternal FnReadDeserializer)
         (((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
       (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt (Identity))
        (var
         ((pattern (Var beta))
          (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))))
 (output_vars
  ((probe <opaque>
    ((out_unconstrained_st
      (SMatrix AoS
       ((pattern (Var n)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_constrained_st
      (SMatrix AoS
       ((pattern (Var n)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_block Parameters) (out_trans Identity)))
   (alpha <opaque>
    ((out_unconstrained_st SReal) (out_constrained_st SReal) (out_block Parameters)
     (out_trans Identity)))
   (beta <opaque>
    ((out_unconstrained_st
      (SVector AoS
       ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_constrained_st
      (SVector AoS
       ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_block Parameters) (out_trans Identity)))))
 (prog_name shape_named_guard_model) (prog_path tests/fixtures/shape_named_guard.stan))
((functions_block ()) (input_vars ()) (prepare_data ())
 (log_prob
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id q) (decl_type (Sized SReal))
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
     (Block
      (((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id r) (decl_type (Sized SReal))
          (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable r) ()) UReal
          ((pattern
            (Promotion
             ((pattern (Lit Int 0))
              (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
             UReal AutoDiffable))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>))
       ((pattern
         (For (loopvar i)
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
                 (Decl (decl_adtype AutoDiffable) (decl_id x)
                  (decl_type
                   (Sized
                    (SArray
                     (SMatrix AoS
                      ((pattern (Lit Int 2))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                      ((pattern (Lit Int 3))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                     ((pattern (Lit Int 2))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (initialize Default)))
                (meta <opaque>))
               ((pattern
                 (Assignment ((LVariable x) ()) (UArray UMatrix)
                  ((pattern
                    (FunApp (CompilerInternal FnMakeArray)
                     (((pattern
                        (FunApp (CompilerInternal FnMakeRowVec)
                         (((pattern
                            (FunApp (CompilerInternal FnMakeRowVec)
                             (((pattern
                                (Promotion
                                 ((pattern (Lit Int 1))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern
                                (Promotion
                                 ((pattern (Lit Int 2))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern
                                (Promotion
                                 ((pattern (Lit Int 3))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                           (meta ((type_ URowVector) (loc <opaque>) (adlevel DataOnly))))
                          ((pattern
                            (FunApp (CompilerInternal FnMakeRowVec)
                             (((pattern
                                (Promotion
                                 ((pattern (Lit Int 4))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern
                                (Promotion
                                 ((pattern (Lit Int 5))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern
                                (Promotion
                                 ((pattern (Lit Int 6))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                           (meta ((type_ URowVector) (loc <opaque>) (adlevel DataOnly)))))))
                       (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly))))
                      ((pattern
                        (FunApp (CompilerInternal FnMakeRowVec)
                         (((pattern
                            (FunApp (CompilerInternal FnMakeRowVec)
                             (((pattern
                                (Promotion
                                 ((pattern (Lit Int 7))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern
                                (Promotion
                                 ((pattern (Lit Int 8))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern
                                (Promotion
                                 ((pattern (Lit Int 9))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                           (meta ((type_ URowVector) (loc <opaque>) (adlevel DataOnly))))
                          ((pattern
                            (FunApp (CompilerInternal FnMakeRowVec)
                             (((pattern
                                (Promotion
                                 ((pattern (Lit Int 10))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern
                                (Promotion
                                 ((pattern (Lit Int 11))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern
                                (Promotion
                                 ((pattern (Lit Int 12))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                           (meta ((type_ URowVector) (loc <opaque>) (adlevel DataOnly)))))))
                       (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
                   (meta ((type_ (UArray UMatrix)) (loc <opaque>) (adlevel DataOnly))))))
                (meta <opaque>))
               ((pattern
                 (IfElse
                  ((pattern
                    (EAnd
                     ((pattern
                       (FunApp (StanLib Equals__ FnPlain AoS)
                        (((pattern
                           (Indexed
                            ((pattern
                              (Indexed
                               ((pattern (Var x))
                                (meta
                                 ((type_ (UArray UMatrix)) (loc <opaque>)
                                  (adlevel AutoDiffable))))
                               ((Single
                                 ((pattern (Lit Int 1))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                             (meta
                              ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                            ((Single
                              ((pattern (Lit Int 2))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                             (Single
                              ((pattern (Lit Int 3))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                         ((pattern (Lit Int 6))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
                     ((pattern
                       (FunApp (StanLib Equals__ FnPlain AoS)
                        (((pattern
                           (Indexed
                            ((pattern
                              (Indexed
                               ((pattern (Var x))
                                (meta
                                 ((type_ (UArray UMatrix)) (loc <opaque>)
                                  (adlevel AutoDiffable))))
                               ((Single
                                 ((pattern (Lit Int 2))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                             (meta
                              ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                            ((Single
                              ((pattern (Lit Int 1))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                             (Single
                              ((pattern (Lit Int 2))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                         ((pattern (Lit Int 8))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
                  ((pattern
                    (Block
                     (((pattern
                        (Assignment ((LVariable r) ()) UReal
                         ((pattern
                           (FunApp (StanLib Times__ FnPlain AoS)
                            (((pattern
                               (Indexed
                                ((pattern
                                  (Indexed
                                   ((pattern (Var x))
                                    (meta
                                     ((type_ (UArray UMatrix)) (loc <opaque>)
                                      (adlevel AutoDiffable))))
                                   ((Single
                                     ((pattern (Lit Int 2))
                                      (meta
                                       ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                                 (meta
                                  ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                                ((Single
                                  ((pattern (Lit Int 2))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                                 (Single
                                  ((pattern (Lit Int 1))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                              (meta
                               ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                             ((pattern (Var q))
                              (meta
                               ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                       (meta <opaque>))
                      ((pattern Break) (meta <opaque>)))))
                   (meta <opaque>))
                  ()))
                (meta <opaque>))
               ((pattern
                 (Assignment ((LVariable r) ()) UReal
                  ((pattern
                    (FunApp (StanLib Times__ FnPlain AoS)
                     (((pattern
                        (Promotion
                         ((pattern (Lit Int 100))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                         UReal DataOnly))
                       (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                      ((pattern (Var q))
                       (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                (meta <opaque>)))))
            (meta <opaque>)))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern (Var r))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>)))))
    (meta <opaque>))))
 (reverse_mode_log_prob
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id q) (decl_type (Sized SReal))
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
     (Block
      (((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id r) (decl_type (Sized SReal))
          (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable r) ()) UReal
          ((pattern
            (Promotion
             ((pattern (Lit Int 0))
              (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
             UReal AutoDiffable))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>))
       ((pattern
         (For (loopvar i)
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
                 (Decl (decl_adtype AutoDiffable) (decl_id x)
                  (decl_type
                   (Sized
                    (SArray
                     (SMatrix SoA
                      ((pattern (Lit Int 2))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                      ((pattern (Lit Int 3))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                     ((pattern (Lit Int 2))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (initialize Default)))
                (meta <opaque>))
               ((pattern
                 (Assignment ((LVariable x) ()) (UArray UMatrix)
                  ((pattern
                    (FunApp (CompilerInternal FnMakeArray)
                     (((pattern
                        (FunApp (CompilerInternal FnMakeRowVec)
                         (((pattern
                            (FunApp (CompilerInternal FnMakeRowVec)
                             (((pattern
                                (Promotion
                                 ((pattern (Lit Int 1))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern
                                (Promotion
                                 ((pattern (Lit Int 2))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern
                                (Promotion
                                 ((pattern (Lit Int 3))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                           (meta ((type_ URowVector) (loc <opaque>) (adlevel DataOnly))))
                          ((pattern
                            (FunApp (CompilerInternal FnMakeRowVec)
                             (((pattern
                                (Promotion
                                 ((pattern (Lit Int 4))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern
                                (Promotion
                                 ((pattern (Lit Int 5))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern
                                (Promotion
                                 ((pattern (Lit Int 6))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                           (meta ((type_ URowVector) (loc <opaque>) (adlevel DataOnly)))))))
                       (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly))))
                      ((pattern
                        (FunApp (CompilerInternal FnMakeRowVec)
                         (((pattern
                            (FunApp (CompilerInternal FnMakeRowVec)
                             (((pattern
                                (Promotion
                                 ((pattern (Lit Int 7))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern
                                (Promotion
                                 ((pattern (Lit Int 8))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern
                                (Promotion
                                 ((pattern (Lit Int 9))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                           (meta ((type_ URowVector) (loc <opaque>) (adlevel DataOnly))))
                          ((pattern
                            (FunApp (CompilerInternal FnMakeRowVec)
                             (((pattern
                                (Promotion
                                 ((pattern (Lit Int 10))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern
                                (Promotion
                                 ((pattern (Lit Int 11))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern
                                (Promotion
                                 ((pattern (Lit Int 12))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                           (meta ((type_ URowVector) (loc <opaque>) (adlevel DataOnly)))))))
                       (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
                   (meta ((type_ (UArray UMatrix)) (loc <opaque>) (adlevel DataOnly))))))
                (meta <opaque>))
               ((pattern
                 (IfElse
                  ((pattern
                    (EAnd
                     ((pattern
                       (FunApp (StanLib Equals__ FnPlain SoA)
                        (((pattern
                           (Indexed
                            ((pattern
                              (Indexed
                               ((pattern (Var x))
                                (meta
                                 ((type_ (UArray UMatrix)) (loc <opaque>)
                                  (adlevel AutoDiffable))))
                               ((Single
                                 ((pattern (Lit Int 1))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                             (meta
                              ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                            ((Single
                              ((pattern (Lit Int 2))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                             (Single
                              ((pattern (Lit Int 3))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                         ((pattern (Lit Int 6))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
                     ((pattern
                       (FunApp (StanLib Equals__ FnPlain SoA)
                        (((pattern
                           (Indexed
                            ((pattern
                              (Indexed
                               ((pattern (Var x))
                                (meta
                                 ((type_ (UArray UMatrix)) (loc <opaque>)
                                  (adlevel AutoDiffable))))
                               ((Single
                                 ((pattern (Lit Int 2))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                             (meta
                              ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                            ((Single
                              ((pattern (Lit Int 1))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                             (Single
                              ((pattern (Lit Int 2))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                         ((pattern (Lit Int 8))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
                  ((pattern
                    (Block
                     (((pattern
                        (Assignment ((LVariable r) ()) UReal
                         ((pattern
                           (FunApp (StanLib Times__ FnPlain SoA)
                            (((pattern
                               (Indexed
                                ((pattern
                                  (Indexed
                                   ((pattern (Var x))
                                    (meta
                                     ((type_ (UArray UMatrix)) (loc <opaque>)
                                      (adlevel AutoDiffable))))
                                   ((Single
                                     ((pattern (Lit Int 2))
                                      (meta
                                       ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                                 (meta
                                  ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                                ((Single
                                  ((pattern (Lit Int 2))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                                 (Single
                                  ((pattern (Lit Int 1))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                              (meta
                               ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                             ((pattern (Var q))
                              (meta
                               ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                       (meta <opaque>))
                      ((pattern Break) (meta <opaque>)))))
                   (meta <opaque>))
                  ()))
                (meta <opaque>))
               ((pattern
                 (Assignment ((LVariable r) ()) UReal
                  ((pattern
                    (FunApp (StanLib Times__ FnPlain SoA)
                     (((pattern
                        (Promotion
                         ((pattern (Lit Int 100))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                         UReal DataOnly))
                       (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                      ((pattern (Var q))
                       (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                (meta <opaque>)))))
            (meta <opaque>)))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern (Var r))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>)))))
    (meta <opaque>))))
 (generate_quantities
  (((pattern
     (Decl (decl_adtype DataOnly) (decl_id q) (decl_type (Sized SReal))
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
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt ())
        (var
         ((pattern (Var q)) (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
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
     (Decl (decl_adtype AutoDiffable) (decl_id q) (decl_type (Sized SReal))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable q) ()) UReal
      ((pattern
        (Indexed
         ((pattern
           (FunApp (CompilerInternal FnReadData)
            (((pattern (Lit Str q))
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
         ((pattern (Var q)) (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))))
 (unconstrain_array
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id q) (decl_type (Sized SReal))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable q) ()) UReal
      ((pattern (FunApp (CompilerInternal FnReadDeserializer) ()))
       (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt (Identity))
        (var
         ((pattern (Var q)) (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))))
 (output_vars
  ((q <opaque>
    ((out_unconstrained_st SReal) (out_constrained_st SReal) (out_block Parameters)
     (out_trans Identity)))))
 (prog_name viewc_region_array_matrix_model)
 (prog_path tests/fixtures/viewc_region_array_matrix.stan))

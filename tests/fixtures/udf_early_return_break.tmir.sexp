((functions_block
  (((fdrt (ReturnType UReal)) (fdname first_positive) (fdsuffix FnPlain)
    (fdargs ((AutoDiffable x UVector)))
    (fdbody
     (((pattern
        (Block
         (((pattern
            (For (loopvar i)
             (lower
              ((pattern (Lit Int 1))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
             (upper
              ((pattern
                (FunApp (StanLib size FnPlain AoS)
                 (((pattern (Var x))
                   (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable)))))))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
             (body
              ((pattern
                (Block
                 (((pattern
                    (IfElse
                     ((pattern
                       (FunApp (StanLib Greater__ FnPlain AoS)
                        (((pattern
                           (Indexed
                            ((pattern (Var x))
                             (meta
                              ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))
                            ((Single
                              ((pattern (Var i))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                         ((pattern (Lit Int 0))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
                     ((pattern
                       (Block
                        (((pattern
                           (Return
                            (((pattern
                               (Indexed
                                ((pattern (Var x))
                                 (meta
                                  ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))
                                ((Single
                                  ((pattern (Var i))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                              (meta
                               ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                          (meta <opaque>)))))
                      (meta <opaque>))
                     ()))
                   (meta <opaque>)))))
               (meta <opaque>)))))
           (meta <opaque>))
          ((pattern
            (Return
             (((pattern
                (Promotion
                 ((pattern (Lit Int -1))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal AutoDiffable))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta <opaque>)))))
       (meta <opaque>))))
    (fdloc <opaque>))))
 (input_vars
  ((x <opaque>
    (SVector AoS
     ((pattern (Lit Int 3)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
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
     (Decl (decl_adtype DataOnly) (decl_id x)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Lit Int 3)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id x_flat__)
          (decl_type (Unsized (UArray UReal))) (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable x_flat__) ()) (UArray UReal)
          ((pattern
            (FunApp (CompilerInternal FnReadData)
             (((pattern (Lit Str x))
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
           ((pattern (Lit Int 3))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (Assignment
                  ((LVariable x)
                   ((Single
                     ((pattern (Var sym1__))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  UVector
                  ((pattern
                    (Indexed
                     ((pattern (Var x_flat__))
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
         (Decl (decl_adtype AutoDiffable) (decl_id inline_first_positive_return_sym5__)
          (decl_type (Sized SReal)) (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Decl (decl_adtype DataOnly)
          (decl_id inline_first_positive_early_ret_check_sym7__) (decl_type (Sized SInt))
          (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable inline_first_positive_early_ret_check_sym7__) ()) UInt
          ((pattern (Lit Int 0)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
        (meta <opaque>))
       ((pattern
         (For (loopvar inline_first_positive_iterator_sym8__)
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
                 (For (loopvar inline_first_positive_i_sym6__)
                  (lower
                   ((pattern (Lit Int 1))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (upper
                   ((pattern
                     (FunApp (StanLib size FnPlain AoS)
                      (((pattern (Var x))
                        (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (body
                   ((pattern
                     (Block
                      (((pattern
                         (IfElse
                          ((pattern
                            (FunApp (StanLib Greater__ FnPlain AoS)
                             (((pattern
                                (Indexed
                                 ((pattern (Var x))
                                  (meta
                                   ((type_ UVector) (loc <opaque>) (adlevel DataOnly))))
                                 ((Single
                                   ((pattern (Var inline_first_positive_i_sym6__))
                                    (meta
                                     ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                               (meta
                                ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                              ((pattern (Lit Int 0))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
                          ((pattern
                            (Block
                             (((pattern
                                (Assignment
                                 ((LVariable
                                   inline_first_positive_early_ret_check_sym7__)
                                  ())
                                 UInt
                                 ((pattern (Lit Int 1))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                               (meta <opaque>))
                              ((pattern
                                (Assignment
                                 ((LVariable inline_first_positive_return_sym5__) ())
                                 UReal
                                 ((pattern
                                   (Indexed
                                    ((pattern (Var x))
                                     (meta
                                      ((type_ UVector) (loc <opaque>) (adlevel DataOnly))))
                                    ((Single
                                      ((pattern (Var inline_first_positive_i_sym6__))
                                       (meta
                                        ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                                  (meta
                                   ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                               (meta <opaque>))
                              ((pattern Break) (meta <opaque>)))))
                           (meta <opaque>))
                          ()))
                        (meta <opaque>)))))
                    (meta <opaque>)))))
                (meta <opaque>))
               ((pattern
                 (IfElse
                  ((pattern (Var inline_first_positive_early_ret_check_sym7__))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern Break) (meta <opaque>)) ()))
                (meta <opaque>))
               ((pattern
                 (Assignment
                  ((LVariable inline_first_positive_early_ret_check_sym7__) ()) UInt
                  ((pattern (Lit Int 1))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                (meta <opaque>))
               ((pattern
                 (Assignment ((LVariable inline_first_positive_return_sym5__) ()) UReal
                  ((pattern
                    (Promotion
                     ((pattern (Lit Int -1))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     UReal AutoDiffable))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                (meta <opaque>))
               ((pattern Break) (meta <opaque>)))))
            (meta <opaque>)))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib Times__ FnPlain AoS)
             (((pattern (Var inline_first_positive_return_sym5__))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern (Var theta))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
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
         (Decl (decl_adtype AutoDiffable) (decl_id inline_first_positive_return_sym1__)
          (decl_type (Sized SReal)) (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Decl (decl_adtype DataOnly)
          (decl_id inline_first_positive_early_ret_check_sym3__) (decl_type (Sized SInt))
          (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable inline_first_positive_early_ret_check_sym3__) ()) UInt
          ((pattern (Lit Int 0)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
        (meta <opaque>))
       ((pattern
         (For (loopvar inline_first_positive_iterator_sym4__)
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
                 (For (loopvar inline_first_positive_i_sym2__)
                  (lower
                   ((pattern (Lit Int 1))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (upper
                   ((pattern
                     (FunApp (StanLib size FnPlain SoA)
                      (((pattern (Var x))
                        (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (body
                   ((pattern
                     (Block
                      (((pattern
                         (IfElse
                          ((pattern
                            (FunApp (StanLib Greater__ FnPlain SoA)
                             (((pattern
                                (Indexed
                                 ((pattern (Var x))
                                  (meta
                                   ((type_ UVector) (loc <opaque>) (adlevel DataOnly))))
                                 ((Single
                                   ((pattern (Var inline_first_positive_i_sym2__))
                                    (meta
                                     ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                               (meta
                                ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                              ((pattern (Lit Int 0))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
                          ((pattern
                            (Block
                             (((pattern
                                (Assignment
                                 ((LVariable
                                   inline_first_positive_early_ret_check_sym3__)
                                  ())
                                 UInt
                                 ((pattern (Lit Int 1))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                               (meta <opaque>))
                              ((pattern
                                (Assignment
                                 ((LVariable inline_first_positive_return_sym1__) ())
                                 UReal
                                 ((pattern
                                   (Indexed
                                    ((pattern (Var x))
                                     (meta
                                      ((type_ UVector) (loc <opaque>) (adlevel DataOnly))))
                                    ((Single
                                      ((pattern (Var inline_first_positive_i_sym2__))
                                       (meta
                                        ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                                  (meta
                                   ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                               (meta <opaque>))
                              ((pattern Break) (meta <opaque>)))))
                           (meta <opaque>))
                          ()))
                        (meta <opaque>)))))
                    (meta <opaque>)))))
                (meta <opaque>))
               ((pattern
                 (IfElse
                  ((pattern (Var inline_first_positive_early_ret_check_sym3__))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern Break) (meta <opaque>)) ()))
                (meta <opaque>))
               ((pattern
                 (Assignment
                  ((LVariable inline_first_positive_early_ret_check_sym3__) ()) UInt
                  ((pattern (Lit Int 1))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                (meta <opaque>))
               ((pattern
                 (Assignment ((LVariable inline_first_positive_return_sym1__) ()) UReal
                  ((pattern
                    (Promotion
                     ((pattern (Lit Int -1))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     UReal AutoDiffable))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                (meta <opaque>))
               ((pattern Break) (meta <opaque>)))))
            (meta <opaque>)))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib Times__ FnPlain SoA)
             (((pattern (Var inline_first_positive_return_sym1__))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern (Var theta))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
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
     (out_trans Identity)))))
 (prog_name udf_early_return_break_model)
 (prog_path tests/fixtures/udf_early_return_break.stan))
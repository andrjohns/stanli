((functions_block
  (((fdrt (ReturnType UReal)) (fdname choose) (fdsuffix FnPlain)
    (fdargs ((AutoDiffable x UReal) (DataOnly A (UArray (UArray UReal)))))
    (fdbody
     (((pattern
        (Block
         (((pattern
            (Decl (decl_adtype AutoDiffable) (decl_id flag) (decl_type (Sized SReal))
             (initialize Default)))
           (meta <opaque>))
          ((pattern
            (Decl (decl_adtype AutoDiffable) (decl_id B)
             (decl_type
              (Sized
               (SArray
                (SArray SReal
                 ((pattern (Lit Int 3))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                ((pattern (Lit Int 2))
                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
             (initialize Default)))
           (meta <opaque>))
          ((pattern
            (IfElse
             ((pattern
               (EAnd
                ((pattern (Lit Int 1))
                 (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
                ((pattern
                  (FunApp (StanLib Equals__ FnPlain AoS)
                   (((pattern
                      (Indexed
                       ((pattern (Var A))
                        (meta
                         ((type_ (UArray (UArray UReal))) (loc <opaque>)
                          (adlevel AutoDiffable))))
                       ((Single
                         ((pattern (Lit Int 1))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                        (Single
                         ((pattern (Lit Int 2))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                     (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                    ((pattern (Lit Real 2.0))
                     (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                 (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))))
              (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
             ((pattern
               (Block
                (((pattern
                   (Return
                    (((pattern (Var x))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                  (meta <opaque>)))))
              (meta <opaque>))
             ()))
           (meta <opaque>))
          ((pattern
            (Return
             (((pattern
                (FunApp (StanLib PMinus__ FnPlain AoS)
                 (((pattern (Var x))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta <opaque>)))))
       (meta <opaque>))))
    (fdloc <opaque>))
   ((fdrt (ReturnType UReal)) (fdname wrap) (fdsuffix FnPlain)
    (fdargs ((AutoDiffable x UReal) (DataOnly A (UArray (UArray UReal)))))
    (fdbody
     (((pattern
        (Block
         (((pattern
            (Return
             (((pattern
                (FunApp (UserDefined choose FnPlain)
                 (((pattern (Var x))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                  ((pattern (Var A))
                   (meta
                    ((type_ (UArray (UArray UReal))) (loc <opaque>) (adlevel DataOnly)))))))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta <opaque>)))))
       (meta <opaque>))))
    (fdloc <opaque>))))
 (input_vars
  ((A <opaque>
    (SArray
     (SArray SReal
      ((pattern (Lit Int 3)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
     ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
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
     (Decl (decl_adtype DataOnly) (decl_id A)
      (decl_type
       (Sized
        (SArray
         (SArray SReal
          ((pattern (Lit Int 3)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
         ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id A_flat__)
          (decl_type (Unsized (UArray UReal))) (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable A_flat__) ()) (UArray UReal)
          ((pattern
            (FunApp (CompilerInternal FnReadData)
             (((pattern (Lit Str A))
               (meta ((type_ (UArray (UArray UReal))) (loc <opaque>) (adlevel DataOnly)))))))
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
                 (For (loopvar sym2__)
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
                         (Assignment
                          ((LVariable A)
                           ((Single
                             ((pattern (Var sym2__))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                            (Single
                             ((pattern (Var sym1__))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                          (UArray (UArray UReal))
                          ((pattern
                            (Indexed
                             ((pattern (Var A_flat__))
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
    (meta <opaque>))))
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
         (Decl (decl_adtype AutoDiffable) (decl_id inline_wrap_return_sym18__)
          (decl_type (Sized SReal)) (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Block
          (((pattern
             (Decl (decl_adtype AutoDiffable)
              (decl_id inline_wrap_inline_choose_return_sym6___sym19__)
              (decl_type (Sized SReal)) (initialize Uninit)))
            (meta <opaque>))
           ((pattern
             (Decl (decl_adtype DataOnly)
              (decl_id inline_wrap_inline_choose_early_ret_check_sym9___sym20__)
              (decl_type (Sized SInt)) (initialize Default)))
            (meta <opaque>))
           ((pattern
             (For (loopvar inline_wrap_inline_choose_iterator_sym10___sym23__)
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
                     (Decl (decl_adtype AutoDiffable)
                      (decl_id inline_wrap_inline_choose_flag_sym7___sym21__)
                      (decl_type (Sized SReal)) (initialize Default)))
                    (meta <opaque>))
                   ((pattern
                     (Decl (decl_adtype AutoDiffable)
                      (decl_id inline_wrap_inline_choose_B_sym8___sym22__)
                      (decl_type
                       (Sized
                        (SArray
                         (SArray SReal
                          ((pattern (Lit Int 3))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                         ((pattern (Lit Int 2))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (initialize Default)))
                    (meta <opaque>))
                   ((pattern
                     (IfElse
                      ((pattern
                        (EAnd
                         ((pattern (Lit Int 1))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
                         ((pattern
                           (FunApp (StanLib Equals__ FnPlain AoS)
                            (((pattern
                               (Indexed
                                ((pattern (Var A))
                                 (meta
                                  ((type_ (UArray (UArray UReal))) (loc <opaque>)
                                   (adlevel DataOnly))))
                                ((Single
                                  ((pattern (Lit Int 1))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                                 (Single
                                  ((pattern (Lit Int 2))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                              (meta
                               ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                             ((pattern (Lit Real 2.0))
                              (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
                      ((pattern
                        (Block
                         (((pattern
                            (Assignment
                             ((LVariable inline_wrap_inline_choose_return_sym6___sym19__)
                              ())
                             UReal
                             ((pattern (Var q))
                              (meta
                               ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                           (meta <opaque>))
                          ((pattern Break) (meta <opaque>)))))
                       (meta <opaque>))
                      ()))
                    (meta <opaque>))
                   ((pattern
                     (Assignment
                      ((LVariable inline_wrap_inline_choose_return_sym6___sym19__) ())
                      UReal
                      ((pattern
                        (FunApp (StanLib PMinus__ FnPlain AoS)
                         (((pattern (Var q))
                           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                       (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                    (meta <opaque>))
                   ((pattern Break) (meta <opaque>)))))
                (meta <opaque>)))))
            (meta <opaque>))
           ((pattern
             (Assignment ((LVariable inline_wrap_return_sym18__) ()) UReal
              ((pattern (Var inline_wrap_inline_choose_return_sym6___sym19__))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
            (meta <opaque>)))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern (Var inline_wrap_return_sym18__))
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
         (Decl (decl_adtype AutoDiffable) (decl_id inline_wrap_return_sym11__)
          (decl_type (Sized SReal)) (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Block
          (((pattern
             (Decl (decl_adtype AutoDiffable)
              (decl_id inline_wrap_inline_choose_return_sym6___sym12__)
              (decl_type (Sized SReal)) (initialize Uninit)))
            (meta <opaque>))
           ((pattern
             (Decl (decl_adtype DataOnly)
              (decl_id inline_wrap_inline_choose_early_ret_check_sym9___sym13__)
              (decl_type (Sized SInt)) (initialize Default)))
            (meta <opaque>))
           ((pattern
             (For (loopvar inline_wrap_inline_choose_iterator_sym10___sym16__)
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
                     (Decl (decl_adtype AutoDiffable)
                      (decl_id inline_wrap_inline_choose_flag_sym7___sym14__)
                      (decl_type (Sized SReal)) (initialize Default)))
                    (meta <opaque>))
                   ((pattern
                     (Decl (decl_adtype AutoDiffable)
                      (decl_id inline_wrap_inline_choose_B_sym8___sym15__)
                      (decl_type
                       (Sized
                        (SArray
                         (SArray SReal
                          ((pattern (Lit Int 3))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                         ((pattern (Lit Int 2))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (initialize Default)))
                    (meta <opaque>))
                   ((pattern
                     (IfElse
                      ((pattern
                        (EAnd
                         ((pattern (Lit Int 1))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
                         ((pattern
                           (FunApp (StanLib Equals__ FnPlain SoA)
                            (((pattern
                               (Indexed
                                ((pattern (Var A))
                                 (meta
                                  ((type_ (UArray (UArray UReal))) (loc <opaque>)
                                   (adlevel DataOnly))))
                                ((Single
                                  ((pattern (Lit Int 1))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                                 (Single
                                  ((pattern (Lit Int 2))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                              (meta
                               ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                             ((pattern (Lit Real 2.0))
                              (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
                      ((pattern
                        (Block
                         (((pattern
                            (Assignment
                             ((LVariable inline_wrap_inline_choose_return_sym6___sym12__)
                              ())
                             UReal
                             ((pattern (Var q))
                              (meta
                               ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                           (meta <opaque>))
                          ((pattern Break) (meta <opaque>)))))
                       (meta <opaque>))
                      ()))
                    (meta <opaque>))
                   ((pattern
                     (Assignment
                      ((LVariable inline_wrap_inline_choose_return_sym6___sym12__) ())
                      UReal
                      ((pattern
                        (FunApp (StanLib PMinus__ FnPlain SoA)
                         (((pattern (Var q))
                           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                       (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                    (meta <opaque>))
                   ((pattern Break) (meta <opaque>)))))
                (meta <opaque>)))))
            (meta <opaque>))
           ((pattern
             (Assignment ((LVariable inline_wrap_return_sym11__) ()) UReal
              ((pattern (Var inline_wrap_inline_choose_return_sym6___sym12__))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
            (meta <opaque>)))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern (Var inline_wrap_return_sym11__))
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
 (prog_name view_udf_local_data_branch_model)
 (prog_path tests/fixtures/view_udf_local_data_branch.stan))

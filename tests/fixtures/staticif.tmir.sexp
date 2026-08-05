((functions_block ())
 (input_vars
  ((M <opaque> SInt)
   (s <opaque>
    (SArray SInt
     ((pattern (Var M)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
 (prepare_data
  (((pattern
     (Decl (decl_adtype DataOnly) (decl_id M) (decl_type (Sized SInt))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable M) ()) UInt
      ((pattern
        (Indexed
         ((pattern
           (FunApp (CompilerInternal FnReadData)
            (((pattern (Lit Str M))
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
        (var_name M)
        (var ((pattern (Var M)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (((pattern (Lit Int 0)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp (CompilerInternal FnValidateSize)
      (((pattern (Lit Str s)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str M)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Var M)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id s)
      (decl_type
       (Sized
        (SArray SInt
         ((pattern (Var M)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable s) ()) (UArray UInt)
      ((pattern
        (FunApp (CompilerInternal FnReadData)
         (((pattern (Lit Str s))
           (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
       (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnCheck
        (trans
         (Lower
          ((pattern (Lit Int 0)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
        (var_name s)
        (var
         ((pattern (Var s))
          (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
      (((pattern (Lit Int 0)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))))
 (log_prob
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id p) (decl_type (Sized SReal))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam
             (constrain
              (LowerUpper
               ((pattern (Lit Int 0))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Int 1))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (dims ()) (mem_pattern AoS)))
           ()))
         (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (For (loopvar i)
          (lower
           ((pattern (Lit Int 1))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (upper
           ((pattern (Var M)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (IfElse
                  ((pattern
                    (FunApp (StanLib Greater__ FnPlain AoS)
                     (((pattern
                        (Indexed
                         ((pattern (Var s))
                          (meta
                           ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
                         ((Single
                           ((pattern (Var i))
                            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                      ((pattern (Lit Int 0))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern
                    (Block
                     (((pattern
                        (TargetPE
                         ((pattern
                           (FunApp (StanLib binomial_lpmf (FnLpmf false) AoS)
                            (((pattern
                               (Indexed
                                ((pattern (Var s))
                                 (meta
                                  ((type_ (UArray UInt)) (loc <opaque>)
                                   (adlevel DataOnly))))
                                ((Single
                                  ((pattern (Var i))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                             ((pattern (Lit Int 5))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                             ((pattern (Var p))
                              (meta
                               ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                       (meta <opaque>)))))
                   (meta <opaque>))
                  (((pattern
                     (Block
                      (((pattern
                         (TargetPE
                          ((pattern
                            (FunApp (StanLib bernoulli_lpmf (FnLpmf false) AoS)
                             (((pattern (Lit Int 0))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern (Var p))
                               (meta
                                ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                        (meta <opaque>)))))
                    (meta <opaque>)))))
                (meta <opaque>)))))
            (meta <opaque>)))))
        (meta <opaque>)))))
    (meta <opaque>))))
 (reverse_mode_log_prob
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id p) (decl_type (Sized SReal))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam
             (constrain
              (LowerUpper
               ((pattern (Lit Int 0))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Int 1))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (dims ()) (mem_pattern AoS)))
           ()))
         (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (For (loopvar i)
          (lower
           ((pattern (Lit Int 1))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (upper
           ((pattern (Var M)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (IfElse
                  ((pattern
                    (FunApp (StanLib Greater__ FnPlain AoS)
                     (((pattern
                        (Indexed
                         ((pattern (Var s))
                          (meta
                           ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
                         ((Single
                           ((pattern (Var i))
                            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                      ((pattern (Lit Int 0))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern
                    (Block
                     (((pattern
                        (TargetPE
                         ((pattern
                           (FunApp (StanLib binomial_lpmf (FnLpmf false) AoS)
                            (((pattern
                               (Indexed
                                ((pattern (Var s))
                                 (meta
                                  ((type_ (UArray UInt)) (loc <opaque>)
                                   (adlevel DataOnly))))
                                ((Single
                                  ((pattern (Var i))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                             ((pattern (Lit Int 5))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                             ((pattern (Var p))
                              (meta
                               ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                       (meta <opaque>)))))
                   (meta <opaque>))
                  (((pattern
                     (Block
                      (((pattern
                         (TargetPE
                          ((pattern
                            (FunApp (StanLib bernoulli_lpmf (FnLpmf false) AoS)
                             (((pattern (Lit Int 0))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern (Var p))
                               (meta
                                ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                        (meta <opaque>)))))
                    (meta <opaque>)))))
                (meta <opaque>)))))
            (meta <opaque>)))))
        (meta <opaque>)))))
    (meta <opaque>))))
 (generate_quantities
  (((pattern
     (Decl (decl_adtype DataOnly) (decl_id p) (decl_type (Sized SReal))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam
             (constrain
              (LowerUpper
               ((pattern (Lit Int 0))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Int 1))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (dims ()) (mem_pattern AoS)))
           ()))
         (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt ())
        (var
         ((pattern (Var p)) (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
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
     (Decl (decl_adtype AutoDiffable) (decl_id p) (decl_type (Sized SReal))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable p) ()) UReal
      ((pattern
        (Indexed
         ((pattern
           (FunApp (CompilerInternal FnReadData)
            (((pattern (Lit Str p))
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
       (FnWriteParam
        (unconstrain_opt
         ((LowerUpper
           ((pattern (Lit Int 0))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
           ((pattern (Lit Int 1))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
        (var
         ((pattern (Var p)) (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))))
 (unconstrain_array
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id p) (decl_type (Sized SReal))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable p) ()) UReal
      ((pattern (FunApp (CompilerInternal FnReadDeserializer) ()))
       (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam
        (unconstrain_opt
         ((LowerUpper
           ((pattern (Lit Int 0))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
           ((pattern (Lit Int 1))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
        (var
         ((pattern (Var p)) (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))))
 (output_vars
  ((p <opaque>
    ((out_unconstrained_st SReal) (out_constrained_st SReal) (out_block Parameters)
     (out_trans
      (LowerUpper
       ((pattern (Lit Int 0)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))))
 (prog_name staticif_model) (prog_path tests/fixtures/staticif.stan))
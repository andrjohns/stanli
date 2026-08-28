((functions_block ())
 (input_vars
  ((d <opaque>
    (SArray SInt
     ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
 (prepare_data
  (((pattern
     (Decl (decl_adtype DataOnly) (decl_id d)
      (decl_type
       (Sized
        (SArray SInt
         ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable d) ()) (UArray UInt)
      ((pattern
        (FunApp (CompilerInternal FnReadData)
         (((pattern (Lit Str d))
           (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
       (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))
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
         (IfElse
          ((pattern
            (FunApp (StanLib Greater__ FnPlain AoS)
             (((pattern (Var theta))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern (Lit Int 0))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
          ((pattern
            (Block
             (((pattern
                (Block
                 (((pattern
                    (Decl (decl_adtype DataOnly) (decl_id sym1__)
                     (decl_type (Unsized (UArray UInt))) (initialize Default)))
                   (meta <opaque>))
                  ((pattern
                    (Assignment ((LVariable sym1__) ()) (UArray UInt)
                     ((pattern
                       (FunApp (StanLib append_array FnPlain AoS)
                        (((pattern
                           (FunApp (CompilerInternal FnMakeArray)
                            (((pattern (Lit Int 1))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                          (meta
                           ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern (Var d))
                          (meta
                           ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))
                   (meta <opaque>))
                  ((pattern
                    (For (loopvar sym3__)
                     (lower
                      ((pattern (Lit Int 1))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                     (upper
                      ((pattern
                        (FunApp (CompilerInternal FnLength)
                         (((pattern (Var sym1__))
                           (meta
                            ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                     (body
                      ((pattern
                        (Block
                         (((pattern
                            (Block
                             (((pattern
                                (Decl (decl_adtype DataOnly) (decl_id i)
                                 (decl_type (Unsized UInt)) (initialize Uninit)))
                               (meta <opaque>))
                              ((pattern
                                (Assignment ((LVariable i) ()) UInt
                                 ((pattern
                                   (Indexed
                                    ((pattern (Var sym1__))
                                     (meta
                                      ((type_ (UArray UInt)) (loc <opaque>)
                                       (adlevel DataOnly))))
                                    ((Single
                                      ((pattern (Var sym3__))
                                       (meta
                                        ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                               (meta <opaque>))
                              ((pattern
                                (TargetPE
                                 ((pattern
                                   (FunApp (StanLib Times__ FnPlain AoS)
                                    (((pattern
                                       (Promotion
                                        ((pattern (Var i))
                                         (meta
                                          ((type_ UInt) (loc <opaque>)
                                           (adlevel DataOnly))))
                                        UReal DataOnly))
                                      (meta
                                       ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                                     ((pattern (Var theta))
                                      (meta
                                       ((type_ UReal) (loc <opaque>)
                                        (adlevel AutoDiffable)))))))
                                  (meta
                                   ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                               (meta <opaque>)))))
                           (meta <opaque>)))))
                       (meta <opaque>)))))
                   (meta <opaque>)))))
               (meta <opaque>))
              ((pattern
                (Block
                 (((pattern
                    (Decl (decl_adtype DataOnly) (decl_id sym1__)
                     (decl_type (Unsized (UArray URowVector))) (initialize Default)))
                   (meta <opaque>))
                  ((pattern
                    (Assignment ((LVariable sym1__) ()) (UArray URowVector)
                     ((pattern
                       (FunApp (CompilerInternal FnMakeArray)
                        (((pattern
                           (FunApp (CompilerInternal FnMakeRowVec)
                            (((pattern (Lit Real 1.0))
                              (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                             ((pattern (Lit Real 2.0))
                              (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                          (meta ((type_ URowVector) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern
                           (FunApp (CompilerInternal FnMakeRowVec)
                            (((pattern (Lit Real 3.0))
                              (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                             ((pattern (Lit Real 4.0))
                              (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                          (meta ((type_ URowVector) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta
                       ((type_ (UArray URowVector)) (loc <opaque>) (adlevel DataOnly))))))
                   (meta <opaque>))
                  ((pattern
                    (For (loopvar sym3__)
                     (lower
                      ((pattern (Lit Int 1))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                     (upper
                      ((pattern
                        (FunApp (CompilerInternal FnLength)
                         (((pattern (Var sym1__))
                           (meta
                            ((type_ (UArray URowVector)) (loc <opaque>)
                             (adlevel DataOnly)))))))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                     (body
                      ((pattern
                        (Block
                         (((pattern
                            (Block
                             (((pattern
                                (Decl (decl_adtype DataOnly) (decl_id x)
                                 (decl_type (Unsized URowVector)) (initialize Uninit)))
                               (meta <opaque>))
                              ((pattern
                                (Assignment ((LVariable x) ()) URowVector
                                 ((pattern
                                   (Indexed
                                    ((pattern (Var sym1__))
                                     (meta
                                      ((type_ (UArray URowVector)) (loc <opaque>)
                                       (adlevel DataOnly))))
                                    ((Single
                                      ((pattern (Var sym3__))
                                       (meta
                                        ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                                  (meta
                                   ((type_ URowVector) (loc <opaque>) (adlevel DataOnly))))))
                               (meta <opaque>))
                              ((pattern
                                (TargetPE
                                 ((pattern
                                   (FunApp (StanLib Times__ FnPlain AoS)
                                    (((pattern
                                       (FunApp (StanLib sum FnPlain AoS)
                                        (((pattern (Var x))
                                          (meta
                                           ((type_ URowVector) (loc <opaque>)
                                            (adlevel AutoDiffable)))))))
                                      (meta
                                       ((type_ UReal) (loc <opaque>)
                                        (adlevel AutoDiffable))))
                                     ((pattern (Var theta))
                                      (meta
                                       ((type_ UReal) (loc <opaque>)
                                        (adlevel AutoDiffable)))))))
                                  (meta
                                   ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                               (meta <opaque>)))))
                           (meta <opaque>)))))
                       (meta <opaque>)))))
                   (meta <opaque>)))))
               (meta <opaque>)))))
           (meta <opaque>))
          ()))
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
         (IfElse
          ((pattern
            (FunApp (StanLib Greater__ FnPlain SoA)
             (((pattern (Var theta))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern (Lit Int 0))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
          ((pattern
            (Block
             (((pattern
                (Block
                 (((pattern
                    (Decl (decl_adtype DataOnly) (decl_id sym1__)
                     (decl_type (Unsized (UArray UInt))) (initialize Default)))
                   (meta <opaque>))
                  ((pattern
                    (Assignment ((LVariable sym1__) ()) (UArray UInt)
                     ((pattern
                       (FunApp (StanLib append_array FnPlain AoS)
                        (((pattern
                           (FunApp (CompilerInternal FnMakeArray)
                            (((pattern (Lit Int 1))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                          (meta
                           ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern (Var d))
                          (meta
                           ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))
                   (meta <opaque>))
                  ((pattern
                    (For (loopvar sym3__)
                     (lower
                      ((pattern (Lit Int 1))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                     (upper
                      ((pattern
                        (FunApp (CompilerInternal FnLength)
                         (((pattern (Var sym1__))
                           (meta
                            ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                     (body
                      ((pattern
                        (Block
                         (((pattern
                            (Block
                             (((pattern
                                (Decl (decl_adtype DataOnly) (decl_id i)
                                 (decl_type (Unsized UInt)) (initialize Uninit)))
                               (meta <opaque>))
                              ((pattern
                                (Assignment ((LVariable i) ()) UInt
                                 ((pattern
                                   (Indexed
                                    ((pattern (Var sym1__))
                                     (meta
                                      ((type_ (UArray UInt)) (loc <opaque>)
                                       (adlevel DataOnly))))
                                    ((Single
                                      ((pattern (Var sym3__))
                                       (meta
                                        ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                               (meta <opaque>))
                              ((pattern
                                (TargetPE
                                 ((pattern
                                   (FunApp (StanLib Times__ FnPlain SoA)
                                    (((pattern
                                       (Promotion
                                        ((pattern (Var i))
                                         (meta
                                          ((type_ UInt) (loc <opaque>)
                                           (adlevel DataOnly))))
                                        UReal DataOnly))
                                      (meta
                                       ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                                     ((pattern (Var theta))
                                      (meta
                                       ((type_ UReal) (loc <opaque>)
                                        (adlevel AutoDiffable)))))))
                                  (meta
                                   ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                               (meta <opaque>)))))
                           (meta <opaque>)))))
                       (meta <opaque>)))))
                   (meta <opaque>)))))
               (meta <opaque>))
              ((pattern
                (Block
                 (((pattern
                    (Decl (decl_adtype DataOnly) (decl_id sym1__)
                     (decl_type (Unsized (UArray URowVector))) (initialize Default)))
                   (meta <opaque>))
                  ((pattern
                    (Assignment ((LVariable sym1__) ()) (UArray URowVector)
                     ((pattern
                       (FunApp (CompilerInternal FnMakeArray)
                        (((pattern
                           (FunApp (CompilerInternal FnMakeRowVec)
                            (((pattern (Lit Real 1.0))
                              (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                             ((pattern (Lit Real 2.0))
                              (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                          (meta ((type_ URowVector) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern
                           (FunApp (CompilerInternal FnMakeRowVec)
                            (((pattern (Lit Real 3.0))
                              (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                             ((pattern (Lit Real 4.0))
                              (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                          (meta ((type_ URowVector) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta
                       ((type_ (UArray URowVector)) (loc <opaque>) (adlevel DataOnly))))))
                   (meta <opaque>))
                  ((pattern
                    (For (loopvar sym3__)
                     (lower
                      ((pattern (Lit Int 1))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                     (upper
                      ((pattern
                        (FunApp (CompilerInternal FnLength)
                         (((pattern (Var sym1__))
                           (meta
                            ((type_ (UArray URowVector)) (loc <opaque>)
                             (adlevel DataOnly)))))))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                     (body
                      ((pattern
                        (Block
                         (((pattern
                            (Block
                             (((pattern
                                (Decl (decl_adtype DataOnly) (decl_id x)
                                 (decl_type (Unsized URowVector)) (initialize Uninit)))
                               (meta <opaque>))
                              ((pattern
                                (Assignment ((LVariable x) ()) URowVector
                                 ((pattern
                                   (Indexed
                                    ((pattern (Var sym1__))
                                     (meta
                                      ((type_ (UArray URowVector)) (loc <opaque>)
                                       (adlevel DataOnly))))
                                    ((Single
                                      ((pattern (Var sym3__))
                                       (meta
                                        ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                                  (meta
                                   ((type_ URowVector) (loc <opaque>) (adlevel DataOnly))))))
                               (meta <opaque>))
                              ((pattern
                                (TargetPE
                                 ((pattern
                                   (FunApp (StanLib Times__ FnPlain SoA)
                                    (((pattern
                                       (FunApp (StanLib sum FnPlain SoA)
                                        (((pattern (Var x))
                                          (meta
                                           ((type_ URowVector) (loc <opaque>)
                                            (adlevel AutoDiffable)))))))
                                      (meta
                                       ((type_ UReal) (loc <opaque>)
                                        (adlevel AutoDiffable))))
                                     ((pattern (Var theta))
                                      (meta
                                       ((type_ UReal) (loc <opaque>)
                                        (adlevel AutoDiffable)))))))
                                  (meta
                                   ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                               (meta <opaque>)))))
                           (meta <opaque>)))))
                       (meta <opaque>)))))
                   (meta <opaque>)))))
               (meta <opaque>)))))
           (meta <opaque>))
          ()))
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
 (prog_name pr236_unsized_island_model)
 (prog_path tests/fixtures/pr236_unsized_island.stan))
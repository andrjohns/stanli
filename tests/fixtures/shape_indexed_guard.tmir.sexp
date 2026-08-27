((functions_block
  (((fdrt (ReturnType UReal)) (fdname indexed_rows) (fdsuffix FnPlain)
    (fdargs ((AutoDiffable mat UMatrix) (AutoDiffable idx (UArray UInt))))
    (fdbody
     (((pattern
        (Block
         (((pattern
            (IfElse
             ((pattern
               (FunApp (StanLib Equals__ FnPlain AoS)
                (((pattern
                   (FunApp (StanLib rows FnPlain AoS)
                    (((pattern
                       (Indexed
                        ((pattern (Var mat))
                         (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                        ((MultiIndex
                          ((pattern (Var idx))
                           (meta
                            ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))
                         (MultiIndex
                          ((pattern (Var idx))
                           (meta
                            ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))))
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
                        ((pattern (Lit Real 1.0))
                         (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                        UReal AutoDiffable))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                  (meta <opaque>)))))
              (meta <opaque>))
             ()))
           (meta <opaque>))
          ((pattern
            (IfElse
             ((pattern
               (EOr
                ((pattern
                  (FunApp (StanLib NEquals__ FnPlain AoS)
                   (((pattern
                      (FunApp (StanLib rows FnPlain AoS)
                       (((pattern
                          (Indexed
                           ((pattern (Var mat))
                            (meta
                             ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                           ((Single
                             ((pattern (Lit Int 1))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                            (MultiIndex
                             ((pattern (Var idx))
                              (meta
                               ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))))
                         (meta
                          ((type_ URowVector) (loc <opaque>) (adlevel AutoDiffable)))))))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                    ((pattern (Lit Int 1))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                ((pattern
                  (FunApp (StanLib NEquals__ FnPlain AoS)
                   (((pattern
                      (FunApp (StanLib cols FnPlain AoS)
                       (((pattern
                          (Indexed
                           ((pattern (Var mat))
                            (meta
                             ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                           ((Single
                             ((pattern (Lit Int 1))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                            (MultiIndex
                             ((pattern (Var idx))
                              (meta
                               ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))))
                         (meta
                          ((type_ URowVector) (loc <opaque>) (adlevel AutoDiffable)))))))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                    ((pattern
                      (FunApp (StanLib size FnPlain AoS)
                       (((pattern (Var idx))
                         (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
             ((pattern
               (Block
                (((pattern
                   (Return
                    (((pattern
                       (Promotion
                        ((pattern (Lit Real -10.))
                         (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                        UReal AutoDiffable))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                  (meta <opaque>)))))
              (meta <opaque>))
             ()))
           (meta <opaque>))
          ((pattern
            (IfElse
             ((pattern
               (EOr
                ((pattern
                  (FunApp (StanLib NEquals__ FnPlain AoS)
                   (((pattern
                      (FunApp (StanLib rows FnPlain AoS)
                       (((pattern
                          (Indexed
                           ((pattern (Var mat))
                            (meta
                             ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                           ((MultiIndex
                             ((pattern (Var idx))
                              (meta
                               ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))
                            (Single
                             ((pattern (Lit Int 1))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                         (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable)))))))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                    ((pattern
                      (FunApp (StanLib size FnPlain AoS)
                       (((pattern (Var idx))
                         (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                ((pattern
                  (FunApp (StanLib NEquals__ FnPlain AoS)
                   (((pattern
                      (FunApp (StanLib cols FnPlain AoS)
                       (((pattern
                          (Indexed
                           ((pattern (Var mat))
                            (meta
                             ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                           ((MultiIndex
                             ((pattern (Var idx))
                              (meta
                               ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))
                            (Single
                             ((pattern (Lit Int 1))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                         (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable)))))))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                    ((pattern (Lit Int 1))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
             ((pattern
               (Block
                (((pattern
                   (Return
                    (((pattern
                       (Promotion
                        ((pattern (Lit Real -20.))
                         (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                        UReal AutoDiffable))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                  (meta <opaque>)))))
              (meta <opaque>))
             ()))
           (meta <opaque>))
          ((pattern
            (Return
             (((pattern
                (Promotion
                 ((pattern
                   (FunApp (StanLib size FnPlain AoS)
                    (((pattern (Var idx))
                      (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal AutoDiffable))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta <opaque>)))))
       (meta <opaque>))))
    (fdloc <opaque>))))
 (input_vars
  ((K <opaque> SInt)
   (idx <opaque>
    (SArray SInt
     ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
 (prepare_data
  (((pattern
     (Decl (decl_adtype DataOnly) (decl_id K) (decl_type (Sized SInt))
      (initialize Uninit)))
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
          ((pattern (Lit Int 0)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
        (var_name K)
        (var ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (((pattern (Lit Int 0)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp (CompilerInternal FnValidateSize)
      (((pattern (Lit Str idx)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id idx)
      (decl_type
       (Sized
        (SArray SInt
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable idx) ()) (UArray UInt)
      ((pattern
        (FunApp (CompilerInternal FnReadData)
         (((pattern (Lit Str idx))
           (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
       (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))
    (meta <opaque>))))
 (log_prob
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id M)
      (decl_type
       (Sized
        (SMatrix AoS
         ((pattern (Lit Int 3)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
         ((pattern (Lit Int 3)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Identity)
             (dims
              (((pattern (Lit Int 3))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Int 3))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
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
         (Decl (decl_adtype AutoDiffable) (decl_id inline_indexed_rows_return_sym4__)
          (decl_type (Sized SReal)) (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Decl (decl_adtype DataOnly)
          (decl_id inline_indexed_rows_early_ret_check_sym5__) (decl_type (Sized SInt))
          (initialize Default)))
        (meta <opaque>))
       ((pattern
         (For (loopvar inline_indexed_rows_iterator_sym6__)
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
                         (((pattern
                            (Indexed
                             ((pattern (Var M))
                              (meta
                               ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                             ((MultiIndex
                               ((pattern (Var idx))
                                (meta
                                 ((type_ (UArray UInt)) (loc <opaque>)
                                  (adlevel DataOnly)))))
                              (MultiIndex
                               ((pattern (Var idx))
                                (meta
                                 ((type_ (UArray UInt)) (loc <opaque>)
                                  (adlevel DataOnly))))))))
                           (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                      ((pattern (Lit Int 0))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern
                    (Block
                     (((pattern
                        (Assignment ((LVariable inline_indexed_rows_return_sym4__) ())
                         UReal
                         ((pattern
                           (Promotion
                            ((pattern (Lit Real 1.0))
                             (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                            UReal AutoDiffable))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                       (meta <opaque>))
                      ((pattern Break) (meta <opaque>)))))
                   (meta <opaque>))
                  ()))
                (meta <opaque>))
               ((pattern
                 (IfElse
                  ((pattern
                    (EOr
                     ((pattern
                       (FunApp (StanLib NEquals__ FnPlain AoS)
                        (((pattern
                           (FunApp (StanLib rows FnPlain AoS)
                            (((pattern
                               (Indexed
                                ((pattern (Var M))
                                 (meta
                                  ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                                ((Single
                                  ((pattern (Lit Int 1))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                                 (MultiIndex
                                  ((pattern (Var idx))
                                   (meta
                                    ((type_ (UArray UInt)) (loc <opaque>)
                                     (adlevel DataOnly))))))))
                              (meta
                               ((type_ URowVector) (loc <opaque>) (adlevel AutoDiffable)))))))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern (Lit Int 1))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern
                       (FunApp (StanLib NEquals__ FnPlain AoS)
                        (((pattern
                           (FunApp (StanLib cols FnPlain AoS)
                            (((pattern
                               (Indexed
                                ((pattern (Var M))
                                 (meta
                                  ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                                ((Single
                                  ((pattern (Lit Int 1))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                                 (MultiIndex
                                  ((pattern (Var idx))
                                   (meta
                                    ((type_ (UArray UInt)) (loc <opaque>)
                                     (adlevel DataOnly))))))))
                              (meta
                               ((type_ URowVector) (loc <opaque>) (adlevel AutoDiffable)))))))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern
                           (FunApp (StanLib size FnPlain AoS)
                            (((pattern (Var idx))
                              (meta
                               ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern
                    (Block
                     (((pattern
                        (Assignment ((LVariable inline_indexed_rows_return_sym4__) ())
                         UReal
                         ((pattern
                           (Promotion
                            ((pattern (Lit Real -10.))
                             (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                            UReal AutoDiffable))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                       (meta <opaque>))
                      ((pattern Break) (meta <opaque>)))))
                   (meta <opaque>))
                  ()))
                (meta <opaque>))
               ((pattern
                 (IfElse
                  ((pattern
                    (EOr
                     ((pattern
                       (FunApp (StanLib NEquals__ FnPlain AoS)
                        (((pattern
                           (FunApp (StanLib rows FnPlain AoS)
                            (((pattern
                               (Indexed
                                ((pattern (Var M))
                                 (meta
                                  ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                                ((MultiIndex
                                  ((pattern (Var idx))
                                   (meta
                                    ((type_ (UArray UInt)) (loc <opaque>)
                                     (adlevel DataOnly)))))
                                 (Single
                                  ((pattern (Lit Int 1))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                              (meta
                               ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable)))))))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern
                           (FunApp (StanLib size FnPlain AoS)
                            (((pattern (Var idx))
                              (meta
                               ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern
                       (FunApp (StanLib NEquals__ FnPlain AoS)
                        (((pattern
                           (FunApp (StanLib cols FnPlain AoS)
                            (((pattern
                               (Indexed
                                ((pattern (Var M))
                                 (meta
                                  ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                                ((MultiIndex
                                  ((pattern (Var idx))
                                   (meta
                                    ((type_ (UArray UInt)) (loc <opaque>)
                                     (adlevel DataOnly)))))
                                 (Single
                                  ((pattern (Lit Int 1))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                              (meta
                               ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable)))))))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern (Lit Int 1))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern
                    (Block
                     (((pattern
                        (Assignment ((LVariable inline_indexed_rows_return_sym4__) ())
                         UReal
                         ((pattern
                           (Promotion
                            ((pattern (Lit Real -20.))
                             (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                            UReal AutoDiffable))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                       (meta <opaque>))
                      ((pattern Break) (meta <opaque>)))))
                   (meta <opaque>))
                  ()))
                (meta <opaque>))
               ((pattern
                 (Assignment ((LVariable inline_indexed_rows_return_sym4__) ()) UReal
                  ((pattern
                    (Promotion
                     ((pattern
                       (FunApp (StanLib size FnPlain AoS)
                        (((pattern (Var idx))
                          (meta
                           ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
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
             (((pattern (Var inline_indexed_rows_return_sym4__))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern (Var theta))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>)))))
    (meta <opaque>))))
 (reverse_mode_log_prob
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id M)
      (decl_type
       (Sized
        (SMatrix SoA
         ((pattern (Lit Int 3)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
         ((pattern (Lit Int 3)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Identity)
             (dims
              (((pattern (Lit Int 3))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Int 3))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern SoA)))
           ()))
         (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
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
         (Decl (decl_adtype AutoDiffable) (decl_id inline_indexed_rows_return_sym1__)
          (decl_type (Sized SReal)) (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Decl (decl_adtype DataOnly)
          (decl_id inline_indexed_rows_early_ret_check_sym2__) (decl_type (Sized SInt))
          (initialize Default)))
        (meta <opaque>))
       ((pattern
         (For (loopvar inline_indexed_rows_iterator_sym3__)
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
                         (((pattern
                            (Indexed
                             ((pattern (Var M))
                              (meta
                               ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                             ((MultiIndex
                               ((pattern (Var idx))
                                (meta
                                 ((type_ (UArray UInt)) (loc <opaque>)
                                  (adlevel DataOnly)))))
                              (MultiIndex
                               ((pattern (Var idx))
                                (meta
                                 ((type_ (UArray UInt)) (loc <opaque>)
                                  (adlevel DataOnly))))))))
                           (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                      ((pattern (Lit Int 0))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern
                    (Block
                     (((pattern
                        (Assignment ((LVariable inline_indexed_rows_return_sym1__) ())
                         UReal
                         ((pattern
                           (Promotion
                            ((pattern (Lit Real 1.0))
                             (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                            UReal AutoDiffable))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                       (meta <opaque>))
                      ((pattern Break) (meta <opaque>)))))
                   (meta <opaque>))
                  ()))
                (meta <opaque>))
               ((pattern
                 (IfElse
                  ((pattern
                    (EOr
                     ((pattern
                       (FunApp (StanLib NEquals__ FnPlain SoA)
                        (((pattern
                           (FunApp (StanLib rows FnPlain SoA)
                            (((pattern
                               (Indexed
                                ((pattern (Var M))
                                 (meta
                                  ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                                ((Single
                                  ((pattern (Lit Int 1))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                                 (MultiIndex
                                  ((pattern (Var idx))
                                   (meta
                                    ((type_ (UArray UInt)) (loc <opaque>)
                                     (adlevel DataOnly))))))))
                              (meta
                               ((type_ URowVector) (loc <opaque>) (adlevel AutoDiffable)))))))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern (Lit Int 1))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern
                       (FunApp (StanLib NEquals__ FnPlain SoA)
                        (((pattern
                           (FunApp (StanLib cols FnPlain SoA)
                            (((pattern
                               (Indexed
                                ((pattern (Var M))
                                 (meta
                                  ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                                ((Single
                                  ((pattern (Lit Int 1))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                                 (MultiIndex
                                  ((pattern (Var idx))
                                   (meta
                                    ((type_ (UArray UInt)) (loc <opaque>)
                                     (adlevel DataOnly))))))))
                              (meta
                               ((type_ URowVector) (loc <opaque>) (adlevel AutoDiffable)))))))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern
                           (FunApp (StanLib size FnPlain SoA)
                            (((pattern (Var idx))
                              (meta
                               ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern
                    (Block
                     (((pattern
                        (Assignment ((LVariable inline_indexed_rows_return_sym1__) ())
                         UReal
                         ((pattern
                           (Promotion
                            ((pattern (Lit Real -10.))
                             (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                            UReal AutoDiffable))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                       (meta <opaque>))
                      ((pattern Break) (meta <opaque>)))))
                   (meta <opaque>))
                  ()))
                (meta <opaque>))
               ((pattern
                 (IfElse
                  ((pattern
                    (EOr
                     ((pattern
                       (FunApp (StanLib NEquals__ FnPlain SoA)
                        (((pattern
                           (FunApp (StanLib rows FnPlain SoA)
                            (((pattern
                               (Indexed
                                ((pattern (Var M))
                                 (meta
                                  ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                                ((MultiIndex
                                  ((pattern (Var idx))
                                   (meta
                                    ((type_ (UArray UInt)) (loc <opaque>)
                                     (adlevel DataOnly)))))
                                 (Single
                                  ((pattern (Lit Int 1))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                              (meta
                               ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable)))))))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern
                           (FunApp (StanLib size FnPlain SoA)
                            (((pattern (Var idx))
                              (meta
                               ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern
                       (FunApp (StanLib NEquals__ FnPlain SoA)
                        (((pattern
                           (FunApp (StanLib cols FnPlain SoA)
                            (((pattern
                               (Indexed
                                ((pattern (Var M))
                                 (meta
                                  ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                                ((MultiIndex
                                  ((pattern (Var idx))
                                   (meta
                                    ((type_ (UArray UInt)) (loc <opaque>)
                                     (adlevel DataOnly)))))
                                 (Single
                                  ((pattern (Lit Int 1))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                              (meta
                               ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable)))))))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern (Lit Int 1))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern
                    (Block
                     (((pattern
                        (Assignment ((LVariable inline_indexed_rows_return_sym1__) ())
                         UReal
                         ((pattern
                           (Promotion
                            ((pattern (Lit Real -20.))
                             (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                            UReal AutoDiffable))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                       (meta <opaque>))
                      ((pattern Break) (meta <opaque>)))))
                   (meta <opaque>))
                  ()))
                (meta <opaque>))
               ((pattern
                 (Assignment ((LVariable inline_indexed_rows_return_sym1__) ()) UReal
                  ((pattern
                    (Promotion
                     ((pattern
                       (FunApp (StanLib size FnPlain SoA)
                        (((pattern (Var idx))
                          (meta
                           ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
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
             (((pattern (Var inline_indexed_rows_return_sym1__))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern (Var theta))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>)))))
    (meta <opaque>))))
 (generate_quantities
  (((pattern
     (Decl (decl_adtype DataOnly) (decl_id M)
      (decl_type
       (Sized
        (SMatrix AoS
         ((pattern (Lit Int 3)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
         ((pattern (Lit Int 3)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Identity)
             (dims
              (((pattern (Lit Int 3))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Int 3))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
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
         ((pattern (Var M)) (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
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
     (Decl (decl_adtype DataOnly) (decl_id pos__) (decl_type (Sized SInt))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id M)
      (decl_type
       (Sized
        (SMatrix AoS
         ((pattern (Lit Int 3)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
         ((pattern (Lit Int 3)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id M_flat__)
          (decl_type (Unsized (UArray UReal))) (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable M_flat__) ()) (UArray UReal)
          ((pattern
            (FunApp (CompilerInternal FnReadData)
             (((pattern (Lit Str M))
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
                   ((pattern (Lit Int 3))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (body
                   ((pattern
                     (Block
                      (((pattern
                         (Assignment
                          ((LVariable M)
                           ((Single
                             ((pattern (Var sym2__))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                            (Single
                             ((pattern (Var sym1__))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                          UMatrix
                          ((pattern
                            (Indexed
                             ((pattern (Var M_flat__))
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
         ((pattern (Var M)) (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))
   ((pattern
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
     (Decl (decl_adtype AutoDiffable) (decl_id M)
      (decl_type
       (Sized
        (SMatrix AoS
         ((pattern (Lit Int 3)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
         ((pattern (Lit Int 3)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable M) ()) UMatrix
      ((pattern
        (FunApp (CompilerInternal FnReadDeserializer)
         (((pattern (Lit Int 3)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
          ((pattern (Lit Int 3)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
       (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt (Identity))
        (var
         ((pattern (Var M)) (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))
   ((pattern
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
  ((M <opaque>
    ((out_unconstrained_st
      (SMatrix AoS
       ((pattern (Lit Int 3)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Int 3)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_constrained_st
      (SMatrix AoS
       ((pattern (Lit Int 3)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Int 3)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_block Parameters) (out_trans Identity)))
   (theta <opaque>
    ((out_unconstrained_st SReal) (out_constrained_st SReal) (out_block Parameters)
     (out_trans Identity)))))
 (prog_name shape_indexed_guard_model)
 (prog_path tests/fixtures/shape_indexed_guard.stan))
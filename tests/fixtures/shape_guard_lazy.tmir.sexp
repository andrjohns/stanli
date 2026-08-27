((functions_block
  (((fdrt (ReturnType UReal)) (fdname lazy_rows) (fdsuffix FnPlain)
    (fdargs
     ((AutoDiffable mat UMatrix) (AutoDiffable empty (UArray UInt))
      (AutoDiffable valid (UArray UInt)) (AutoDiffable bad (UArray UInt))))
    (fdbody
     (((pattern
        (Block
         (((pattern
            (Decl (decl_adtype AutoDiffable) (decl_id out) (decl_type (Sized SReal))
             (initialize Uninit)))
           (meta <opaque>))
          ((pattern
            (Assignment ((LVariable out) ()) UReal
             ((pattern (Lit Real 0.0))
              (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
           (meta <opaque>))
          ((pattern
            (IfElse
             ((pattern
               (EOr
                ((pattern
                  (FunApp (StanLib Equals__ FnPlain AoS)
                   (((pattern
                      (FunApp (StanLib rows FnPlain AoS)
                       (((pattern
                          (Indexed
                           ((pattern (Var mat))
                            (meta
                             ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                           ((MultiIndex
                             ((pattern (Var empty))
                              (meta
                               ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))
                            (MultiIndex
                             ((pattern (Var empty))
                              (meta
                               ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))))
                         (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                    ((pattern (Lit Int 0))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                ((pattern
                  (FunApp (StanLib Equals__ FnPlain AoS)
                   (((pattern
                      (FunApp (StanLib rows FnPlain AoS)
                       (((pattern
                          (Indexed
                           ((pattern (Var mat))
                            (meta
                             ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                           ((MultiIndex
                             ((pattern (Var bad))
                              (meta
                               ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))
                            (MultiIndex
                             ((pattern (Var bad))
                              (meta
                               ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))))
                         (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                    ((pattern (Lit Int 0))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
             ((pattern
               (Block
                (((pattern
                   (Assignment ((LVariable out) ()) UReal
                    ((pattern (Lit Real 1.))
                     (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                  (meta <opaque>)))))
              (meta <opaque>))
             ()))
           (meta <opaque>))
          ((pattern
            (IfElse
             ((pattern
               (EAnd
                ((pattern
                  (FunApp (StanLib Equals__ FnPlain AoS)
                   (((pattern
                      (FunApp (StanLib rows FnPlain AoS)
                       (((pattern
                          (Indexed
                           ((pattern (Var mat))
                            (meta
                             ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                           ((MultiIndex
                             ((pattern (Var valid))
                              (meta
                               ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))
                            (MultiIndex
                             ((pattern (Var valid))
                              (meta
                               ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))))
                         (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                    ((pattern (Lit Int 0))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                ((pattern
                  (FunApp (StanLib Equals__ FnPlain AoS)
                   (((pattern
                      (FunApp (StanLib rows FnPlain AoS)
                       (((pattern
                          (Indexed
                           ((pattern (Var mat))
                            (meta
                             ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                           ((MultiIndex
                             ((pattern (Var bad))
                              (meta
                               ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))
                            (MultiIndex
                             ((pattern (Var bad))
                              (meta
                               ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))))
                         (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                    ((pattern (Lit Int 0))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
             ((pattern
               (Block
                (((pattern
                   (Assignment ((LVariable out) ()) UReal
                    ((pattern
                      (FunApp (StanLib Plus__ FnPlain AoS)
                       (((pattern (Var out))
                         (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                        ((pattern (Lit Real 10.0))
                         (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                     (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                  (meta <opaque>)))))
              (meta <opaque>))
             ()))
           (meta <opaque>))
          ((pattern
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
                          ((pattern (Var empty))
                           (meta
                            ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))
                         (MultiIndex
                          ((pattern (Var empty))
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
                   (Assignment ((LVariable out) ()) UReal
                    ((pattern
                      (FunApp (StanLib Plus__ FnPlain AoS)
                       (((pattern (Var out))
                         (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                        ((pattern (Lit Real 0.0))
                         (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                     (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                  (meta <opaque>)))))
              (meta <opaque>))
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
                                 (meta
                                  ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                                ((MultiIndex
                                  ((pattern (Var bad))
                                   (meta
                                    ((type_ (UArray UInt)) (loc <opaque>)
                                     (adlevel DataOnly)))))
                                 (MultiIndex
                                  ((pattern (Var bad))
                                   (meta
                                    ((type_ (UArray UInt)) (loc <opaque>)
                                     (adlevel DataOnly))))))))
                              (meta
                               ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern (Lit Int 0))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern
                       (Block
                        (((pattern
                           (Assignment ((LVariable out) ()) UReal
                            ((pattern
                              (FunApp (StanLib Plus__ FnPlain AoS)
                               (((pattern (Var out))
                                 (meta
                                  ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                                ((pattern (Lit Real 100.0))
                                 (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                             (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                          (meta <opaque>)))))
                      (meta <opaque>))
                     ()))
                   (meta <opaque>)))))
               (meta <opaque>)))))
           (meta <opaque>))
          ((pattern
            (Assignment ((LVariable out) ()) UReal
             ((pattern
               (FunApp (StanLib Plus__ FnPlain AoS)
                (((pattern (Var out))
                  (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                 ((pattern
                   (TernaryIf
                    ((pattern
                      (FunApp (StanLib Equals__ FnPlain AoS)
                       (((pattern
                          (FunApp (StanLib rows FnPlain AoS)
                           (((pattern
                              (Indexed
                               ((pattern (Var mat))
                                (meta
                                 ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                               ((MultiIndex
                                 ((pattern (Var empty))
                                  (meta
                                   ((type_ (UArray UInt)) (loc <opaque>)
                                    (adlevel DataOnly)))))
                                (MultiIndex
                                 ((pattern (Var empty))
                                  (meta
                                   ((type_ (UArray UInt)) (loc <opaque>)
                                    (adlevel DataOnly))))))))
                             (meta
                              ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                        ((pattern (Lit Int 0))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                    ((pattern (Lit Real 0.0))
                     (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                    ((pattern
                      (TernaryIf
                       ((pattern
                         (FunApp (StanLib Equals__ FnPlain AoS)
                          (((pattern
                             (FunApp (StanLib rows FnPlain AoS)
                              (((pattern
                                 (Indexed
                                  ((pattern (Var mat))
                                   (meta
                                    ((type_ UMatrix) (loc <opaque>)
                                     (adlevel AutoDiffable))))
                                  ((MultiIndex
                                    ((pattern (Var bad))
                                     (meta
                                      ((type_ (UArray UInt)) (loc <opaque>)
                                       (adlevel DataOnly)))))
                                   (MultiIndex
                                    ((pattern (Var bad))
                                     (meta
                                      ((type_ (UArray UInt)) (loc <opaque>)
                                       (adlevel DataOnly))))))))
                                (meta
                                 ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                           ((pattern (Lit Int 0))
                            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                       ((pattern (Lit Real 1000.0))
                        (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                       ((pattern (Lit Real 2000.0))
                        (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))))
                     (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))))
                  (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
              (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
           (meta <opaque>))
          ((pattern
            (Return
             (((pattern (Var out))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta <opaque>)))))
       (meta <opaque>))))
    (fdloc <opaque>))))
 (input_vars
  ((empty <opaque>
    (SArray SInt
     ((pattern (Lit Int 0)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
   (valid <opaque>
    (SArray SInt
     ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
   (bad <opaque>
    (SArray SInt
     ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
 (prepare_data
  (((pattern
     (Decl (decl_adtype DataOnly) (decl_id empty)
      (decl_type
       (Sized
        (SArray SInt
         ((pattern (Lit Int 0)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable empty) ()) (UArray UInt)
      ((pattern
        (FunApp (CompilerInternal FnReadData)
         (((pattern (Lit Str empty))
           (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
       (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id valid)
      (decl_type
       (Sized
        (SArray SInt
         ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable valid) ()) (UArray UInt)
      ((pattern
        (FunApp (CompilerInternal FnReadData)
         (((pattern (Lit Str valid))
           (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
       (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id bad)
      (decl_type
       (Sized
        (SArray SInt
         ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable bad) ()) (UArray UInt)
      ((pattern
        (FunApp (CompilerInternal FnReadData)
         (((pattern (Lit Str bad))
           (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
       (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))
    (meta <opaque>))))
 (log_prob
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id M)
      (decl_type
       (Sized
        (SMatrix AoS
         ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
         ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Identity)
             (dims
              (((pattern (Lit Int 2))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Int 2))
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
         (Decl (decl_adtype AutoDiffable) (decl_id inline_lazy_rows_return_sym4__)
          (decl_type (Sized SReal)) (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Block
          (((pattern
             (Decl (decl_adtype AutoDiffable) (decl_id inline_lazy_rows_out_sym5__)
              (decl_type (Sized SReal)) (initialize Uninit)))
            (meta <opaque>))
           ((pattern
             (Assignment ((LVariable inline_lazy_rows_out_sym5__) ()) UReal
              ((pattern (Lit Real 0.0))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
            (meta <opaque>))
           ((pattern
             (IfElse
              ((pattern
                (EOr
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
                              ((pattern (Var empty))
                               (meta
                                ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))
                             (MultiIndex
                              ((pattern (Var empty))
                               (meta
                                ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))))
                          (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Lit Int 0))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
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
                              ((pattern (Var bad))
                               (meta
                                ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))
                             (MultiIndex
                              ((pattern (Var bad))
                               (meta
                                ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))))
                          (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Lit Int 0))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
              ((pattern
                (Block
                 (((pattern
                    (Assignment ((LVariable inline_lazy_rows_out_sym5__) ()) UReal
                     ((pattern (Lit Real 1.))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                   (meta <opaque>)))))
               (meta <opaque>))
              ()))
            (meta <opaque>))
           ((pattern
             (IfElse
              ((pattern
                (EAnd
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
                              ((pattern (Var valid))
                               (meta
                                ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))
                             (MultiIndex
                              ((pattern (Var valid))
                               (meta
                                ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))))
                          (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Lit Int 0))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
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
                              ((pattern (Var bad))
                               (meta
                                ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))
                             (MultiIndex
                              ((pattern (Var bad))
                               (meta
                                ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))))
                          (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Lit Int 0))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
              ((pattern
                (Block
                 (((pattern
                    (Assignment ((LVariable inline_lazy_rows_out_sym5__) ()) UReal
                     ((pattern
                       (FunApp (StanLib Plus__ FnPlain AoS)
                        (((pattern (Var inline_lazy_rows_out_sym5__))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                         ((pattern (Lit Real 10.0))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                   (meta <opaque>)))))
               (meta <opaque>))
              ()))
            (meta <opaque>))
           ((pattern
             (IfElse
              ((pattern
                (FunApp (StanLib Equals__ FnPlain AoS)
                 (((pattern
                    (FunApp (StanLib rows FnPlain AoS)
                     (((pattern
                        (Indexed
                         ((pattern (Var M))
                          (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                         ((MultiIndex
                           ((pattern (Var empty))
                            (meta
                             ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))
                          (MultiIndex
                           ((pattern (Var empty))
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
                    (Assignment ((LVariable inline_lazy_rows_out_sym5__) ()) UReal
                     ((pattern
                       (FunApp (StanLib Plus__ FnPlain AoS)
                        (((pattern (Var inline_lazy_rows_out_sym5__))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                         ((pattern (Lit Real 0.0))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                   (meta <opaque>)))))
               (meta <opaque>))
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
                                 ((pattern (Var M))
                                  (meta
                                   ((type_ UMatrix) (loc <opaque>)
                                    (adlevel AutoDiffable))))
                                 ((MultiIndex
                                   ((pattern (Var bad))
                                    (meta
                                     ((type_ (UArray UInt)) (loc <opaque>)
                                      (adlevel DataOnly)))))
                                  (MultiIndex
                                   ((pattern (Var bad))
                                    (meta
                                     ((type_ (UArray UInt)) (loc <opaque>)
                                      (adlevel DataOnly))))))))
                               (meta
                                ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                          ((pattern (Lit Int 0))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                      ((pattern
                        (Block
                         (((pattern
                            (Assignment ((LVariable inline_lazy_rows_out_sym5__) ())
                             UReal
                             ((pattern
                               (FunApp (StanLib Plus__ FnPlain AoS)
                                (((pattern (Var inline_lazy_rows_out_sym5__))
                                  (meta
                                   ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                                 ((pattern (Lit Real 100.0))
                                  (meta
                                   ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                              (meta
                               ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                           (meta <opaque>)))))
                       (meta <opaque>))
                      ()))
                    (meta <opaque>)))))
                (meta <opaque>)))))
            (meta <opaque>))
           ((pattern
             (Assignment ((LVariable inline_lazy_rows_out_sym5__) ()) UReal
              ((pattern
                (FunApp (StanLib Plus__ FnPlain AoS)
                 (((pattern (Var inline_lazy_rows_out_sym5__))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                  ((pattern
                    (TernaryIf
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
                                  ((pattern (Var empty))
                                   (meta
                                    ((type_ (UArray UInt)) (loc <opaque>)
                                     (adlevel DataOnly)))))
                                 (MultiIndex
                                  ((pattern (Var empty))
                                   (meta
                                    ((type_ (UArray UInt)) (loc <opaque>)
                                     (adlevel DataOnly))))))))
                              (meta
                               ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern (Lit Int 0))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Lit Real 0.0))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern
                       (TernaryIf
                        ((pattern
                          (FunApp (StanLib Equals__ FnPlain AoS)
                           (((pattern
                              (FunApp (StanLib rows FnPlain AoS)
                               (((pattern
                                  (Indexed
                                   ((pattern (Var M))
                                    (meta
                                     ((type_ UMatrix) (loc <opaque>)
                                      (adlevel AutoDiffable))))
                                   ((MultiIndex
                                     ((pattern (Var bad))
                                      (meta
                                       ((type_ (UArray UInt)) (loc <opaque>)
                                        (adlevel DataOnly)))))
                                    (MultiIndex
                                     ((pattern (Var bad))
                                      (meta
                                       ((type_ (UArray UInt)) (loc <opaque>)
                                        (adlevel DataOnly))))))))
                                 (meta
                                  ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                             (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                            ((pattern (Lit Int 0))
                             (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                        ((pattern (Lit Real 1000.0))
                         (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                        ((pattern (Lit Real 2000.0))
                         (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
            (meta <opaque>))
           ((pattern
             (Assignment ((LVariable inline_lazy_rows_return_sym4__) ()) UReal
              ((pattern (Var inline_lazy_rows_out_sym5__))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
            (meta <opaque>)))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib Times__ FnPlain AoS)
             (((pattern (Var inline_lazy_rows_return_sym4__))
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
         ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
         ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Identity)
             (dims
              (((pattern (Lit Int 2))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Int 2))
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
         (Decl (decl_adtype AutoDiffable) (decl_id inline_lazy_rows_return_sym1__)
          (decl_type (Sized SReal)) (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Block
          (((pattern
             (Decl (decl_adtype AutoDiffable) (decl_id inline_lazy_rows_out_sym2__)
              (decl_type (Sized SReal)) (initialize Uninit)))
            (meta <opaque>))
           ((pattern
             (Assignment ((LVariable inline_lazy_rows_out_sym2__) ()) UReal
              ((pattern (Lit Real 0.0))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
            (meta <opaque>))
           ((pattern
             (IfElse
              ((pattern
                (EOr
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
                              ((pattern (Var empty))
                               (meta
                                ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))
                             (MultiIndex
                              ((pattern (Var empty))
                               (meta
                                ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))))
                          (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Lit Int 0))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
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
                              ((pattern (Var bad))
                               (meta
                                ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))
                             (MultiIndex
                              ((pattern (Var bad))
                               (meta
                                ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))))
                          (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Lit Int 0))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
              ((pattern
                (Block
                 (((pattern
                    (Assignment ((LVariable inline_lazy_rows_out_sym2__) ()) UReal
                     ((pattern (Lit Real 1.))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                   (meta <opaque>)))))
               (meta <opaque>))
              ()))
            (meta <opaque>))
           ((pattern
             (IfElse
              ((pattern
                (EAnd
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
                              ((pattern (Var valid))
                               (meta
                                ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))
                             (MultiIndex
                              ((pattern (Var valid))
                               (meta
                                ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))))
                          (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Lit Int 0))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
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
                              ((pattern (Var bad))
                               (meta
                                ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))
                             (MultiIndex
                              ((pattern (Var bad))
                               (meta
                                ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))))
                          (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Lit Int 0))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
              ((pattern
                (Block
                 (((pattern
                    (Assignment ((LVariable inline_lazy_rows_out_sym2__) ()) UReal
                     ((pattern
                       (FunApp (StanLib Plus__ FnPlain SoA)
                        (((pattern (Var inline_lazy_rows_out_sym2__))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                         ((pattern (Lit Real 10.0))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                   (meta <opaque>)))))
               (meta <opaque>))
              ()))
            (meta <opaque>))
           ((pattern
             (IfElse
              ((pattern
                (FunApp (StanLib Equals__ FnPlain SoA)
                 (((pattern
                    (FunApp (StanLib rows FnPlain SoA)
                     (((pattern
                        (Indexed
                         ((pattern (Var M))
                          (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))
                         ((MultiIndex
                           ((pattern (Var empty))
                            (meta
                             ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))
                          (MultiIndex
                           ((pattern (Var empty))
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
                    (Assignment ((LVariable inline_lazy_rows_out_sym2__) ()) UReal
                     ((pattern
                       (FunApp (StanLib Plus__ FnPlain SoA)
                        (((pattern (Var inline_lazy_rows_out_sym2__))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                         ((pattern (Lit Real 0.0))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                   (meta <opaque>)))))
               (meta <opaque>))
              (((pattern
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
                                   ((type_ UMatrix) (loc <opaque>)
                                    (adlevel AutoDiffable))))
                                 ((MultiIndex
                                   ((pattern (Var bad))
                                    (meta
                                     ((type_ (UArray UInt)) (loc <opaque>)
                                      (adlevel DataOnly)))))
                                  (MultiIndex
                                   ((pattern (Var bad))
                                    (meta
                                     ((type_ (UArray UInt)) (loc <opaque>)
                                      (adlevel DataOnly))))))))
                               (meta
                                ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                          ((pattern (Lit Int 0))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                      ((pattern
                        (Block
                         (((pattern
                            (Assignment ((LVariable inline_lazy_rows_out_sym2__) ())
                             UReal
                             ((pattern
                               (FunApp (StanLib Plus__ FnPlain SoA)
                                (((pattern (Var inline_lazy_rows_out_sym2__))
                                  (meta
                                   ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                                 ((pattern (Lit Real 100.0))
                                  (meta
                                   ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                              (meta
                               ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                           (meta <opaque>)))))
                       (meta <opaque>))
                      ()))
                    (meta <opaque>)))))
                (meta <opaque>)))))
            (meta <opaque>))
           ((pattern
             (Assignment ((LVariable inline_lazy_rows_out_sym2__) ()) UReal
              ((pattern
                (FunApp (StanLib Plus__ FnPlain SoA)
                 (((pattern (Var inline_lazy_rows_out_sym2__))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                  ((pattern
                    (TernaryIf
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
                                  ((pattern (Var empty))
                                   (meta
                                    ((type_ (UArray UInt)) (loc <opaque>)
                                     (adlevel DataOnly)))))
                                 (MultiIndex
                                  ((pattern (Var empty))
                                   (meta
                                    ((type_ (UArray UInt)) (loc <opaque>)
                                     (adlevel DataOnly))))))))
                              (meta
                               ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern (Lit Int 0))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Lit Real 0.0))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern
                       (TernaryIf
                        ((pattern
                          (FunApp (StanLib Equals__ FnPlain SoA)
                           (((pattern
                              (FunApp (StanLib rows FnPlain SoA)
                               (((pattern
                                  (Indexed
                                   ((pattern (Var M))
                                    (meta
                                     ((type_ UMatrix) (loc <opaque>)
                                      (adlevel AutoDiffable))))
                                   ((MultiIndex
                                     ((pattern (Var bad))
                                      (meta
                                       ((type_ (UArray UInt)) (loc <opaque>)
                                        (adlevel DataOnly)))))
                                    (MultiIndex
                                     ((pattern (Var bad))
                                      (meta
                                       ((type_ (UArray UInt)) (loc <opaque>)
                                        (adlevel DataOnly))))))))
                                 (meta
                                  ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
                             (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                            ((pattern (Lit Int 0))
                             (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                        ((pattern (Lit Real 1000.0))
                         (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                        ((pattern (Lit Real 2000.0))
                         (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
            (meta <opaque>))
           ((pattern
             (Assignment ((LVariable inline_lazy_rows_return_sym1__) ()) UReal
              ((pattern (Var inline_lazy_rows_out_sym2__))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
            (meta <opaque>)))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib Times__ FnPlain SoA)
             (((pattern (Var inline_lazy_rows_return_sym1__))
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
         ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
         ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Identity)
             (dims
              (((pattern (Lit Int 2))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Int 2))
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
         ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
         ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
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
                   ((pattern (Lit Int 2))
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
         ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
         ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable M) ()) UMatrix
      ((pattern
        (FunApp (CompilerInternal FnReadDeserializer)
         (((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
          ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
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
       ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_constrained_st
      (SMatrix AoS
       ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_block Parameters) (out_trans Identity)))
   (theta <opaque>
    ((out_unconstrained_st SReal) (out_constrained_st SReal) (out_block Parameters)
     (out_trans Identity)))))
 (prog_name shape_guard_lazy_model) (prog_path tests/fixtures/shape_guard_lazy.stan))
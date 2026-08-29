((functions_block
  (((fdrt (ReturnType (UArray UReal))) (fdname f_lin) (fdsuffix FnPlain)
    (fdargs
     ((AutoDiffable t UReal) (AutoDiffable z (UArray UReal))
      (AutoDiffable theta (UArray UReal)) (AutoDiffable x_r (UArray UReal))
      (AutoDiffable x_i (UArray UInt))))
    (fdbody
     (((pattern
        (Block
         (((pattern
            (Decl (decl_adtype AutoDiffable) (decl_id u) (decl_type (Sized SReal))
             (initialize Uninit)))
           (meta <opaque>))
          ((pattern
            (Assignment ((LVariable u) ()) UReal
             ((pattern
               (Indexed
                ((pattern (Var z))
                 (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel AutoDiffable))))
                ((Single
                  ((pattern (Lit Int 1))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
              (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
           (meta <opaque>))
          ((pattern
            (Decl (decl_adtype AutoDiffable) (decl_id v) (decl_type (Sized SReal))
             (initialize Uninit)))
           (meta <opaque>))
          ((pattern
            (Assignment ((LVariable v) ()) UReal
             ((pattern
               (Indexed
                ((pattern (Var z))
                 (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel AutoDiffable))))
                ((Single
                  ((pattern (Lit Int 2))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
              (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
           (meta <opaque>))
          ((pattern
            (Decl (decl_adtype AutoDiffable) (decl_id du) (decl_type (Sized SReal))
             (initialize Uninit)))
           (meta <opaque>))
          ((pattern
            (Assignment ((LVariable du) ()) UReal
             ((pattern
               (FunApp (StanLib Times__ FnPlain AoS)
                (((pattern
                   (FunApp (StanLib Minus__ FnPlain AoS)
                    (((pattern
                       (Indexed
                        ((pattern (Var theta))
                         (meta
                          ((type_ (UArray UReal)) (loc <opaque>) (adlevel AutoDiffable))))
                        ((Single
                          ((pattern (Lit Int 1))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                     ((pattern
                       (FunApp (StanLib Times__ FnPlain AoS)
                        (((pattern
                           (Indexed
                            ((pattern (Var theta))
                             (meta
                              ((type_ (UArray UReal)) (loc <opaque>)
                               (adlevel AutoDiffable))))
                            ((Single
                              ((pattern (Lit Int 2))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                         ((pattern (Var v))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                  (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                 ((pattern (Var u))
                  (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
              (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
           (meta <opaque>))
          ((pattern
            (Decl (decl_adtype AutoDiffable) (decl_id dv) (decl_type (Sized SReal))
             (initialize Uninit)))
           (meta <opaque>))
          ((pattern
            (Assignment ((LVariable dv) ()) UReal
             ((pattern
               (FunApp (StanLib Times__ FnPlain AoS)
                (((pattern
                   (FunApp (StanLib fma FnPlain AoS)
                    (((pattern
                       (Indexed
                        ((pattern (Var theta))
                         (meta
                          ((type_ (UArray UReal)) (loc <opaque>) (adlevel AutoDiffable))))
                        ((Single
                          ((pattern (Lit Int 4))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                     ((pattern (Var u))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                     ((pattern
                       (FunApp (StanLib PMinus__ FnPlain AoS)
                        (((pattern
                           (Indexed
                            ((pattern (Var theta))
                             (meta
                              ((type_ (UArray UReal)) (loc <opaque>)
                               (adlevel AutoDiffable))))
                            ((Single
                              ((pattern (Lit Int 3))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                  (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                 ((pattern (Var v))
                  (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
              (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
           (meta <opaque>))
          ((pattern
            (Return
             (((pattern
                (FunApp (CompilerInternal FnMakeArray)
                 (((pattern (Var du))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                  ((pattern (Var dv))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
               (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta <opaque>)))))
       (meta <opaque>))))
    (fdloc <opaque>))
   ((fdrt (ReturnType (UArray UReal))) (fdname f_branch) (fdsuffix FnPlain)
    (fdargs
     ((AutoDiffable t UReal) (AutoDiffable z (UArray UReal))
      (AutoDiffable theta (UArray UReal)) (AutoDiffable x_r (UArray UReal))
      (AutoDiffable x_i (UArray UInt))))
    (fdbody
     (((pattern
        (Block
         (((pattern
            (Decl (decl_adtype AutoDiffable) (decl_id dz)
             (decl_type
              (Sized
               (SArray SReal
                ((pattern (Lit Int 2))
                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
             (initialize Default)))
           (meta <opaque>))
          ((pattern
            (Decl (decl_adtype AutoDiffable) (decl_id dose) (decl_type (Sized SReal))
             (initialize Uninit)))
           (meta <opaque>))
          ((pattern
            (Assignment ((LVariable dose) ()) UReal
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
                (((pattern (Var t))
                  (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                 ((pattern (Lit Real 0.5))
                  (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
              (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
             ((pattern
               (Block
                (((pattern
                   (Assignment ((LVariable dose) ()) UReal
                    ((pattern
                      (FunApp (StanLib Divide__ FnPlain AoS)
                       (((pattern
                          (FunApp (StanLib Times__ FnPlain AoS)
                           (((pattern
                              (FunApp (StanLib exp FnPlain AoS)
                               (((pattern
                                  (FunApp (StanLib Times__ FnPlain AoS)
                                   (((pattern
                                      (FunApp (StanLib PMinus__ FnPlain AoS)
                                       (((pattern
                                          (Indexed
                                           ((pattern (Var theta))
                                            (meta
                                             ((type_ (UArray UReal)) 
                                              (loc <opaque>) (adlevel AutoDiffable))))
                                           ((Single
                                             ((pattern (Lit Int 1))
                                              (meta
                                               ((type_ UInt) (loc <opaque>)
                                                (adlevel DataOnly))))))))
                                         (meta
                                          ((type_ UReal) (loc <opaque>)
                                           (adlevel AutoDiffable)))))))
                                     (meta
                                      ((type_ UReal) (loc <opaque>)
                                       (adlevel AutoDiffable))))
                                    ((pattern (Var t))
                                     (meta
                                      ((type_ UReal) (loc <opaque>)
                                       (adlevel AutoDiffable)))))))
                                 (meta
                                  ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                             (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                            ((pattern
                              (Indexed
                               ((pattern (Var x_r))
                                (meta
                                 ((type_ (UArray UReal)) (loc <opaque>)
                                  (adlevel AutoDiffable))))
                               ((Single
                                 ((pattern (Lit Int 1))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                             (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                         (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                        ((pattern
                          (Indexed
                           ((pattern (Var x_r))
                            (meta
                             ((type_ (UArray UReal)) (loc <opaque>)
                              (adlevel AutoDiffable))))
                           ((Single
                             ((pattern (Lit Int 2))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                         (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                     (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                  (meta <opaque>)))))
              (meta <opaque>))
             ()))
           (meta <opaque>))
          ((pattern
            (For (loopvar k)
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
                     ((LVariable dz)
                      ((Single
                        ((pattern (Var k))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                     (UArray UReal)
                     ((pattern
                       (FunApp (StanLib fma FnPlain AoS)
                        (((pattern
                           (Promotion
                            ((pattern
                              (Indexed
                               ((pattern (Var x_i))
                                (meta
                                 ((type_ (UArray UInt)) (loc <opaque>)
                                  (adlevel DataOnly))))
                               ((Single
                                 ((pattern (Lit Int 1))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                             (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                            UReal DataOnly))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern
                           (FunApp (StanLib sqrt FnPlain AoS)
                            (((pattern
                               (FunApp (StanLib Plus__ FnPlain AoS)
                                (((pattern
                                   (FunApp (StanLib square FnPlain AoS)
                                    (((pattern
                                       (Indexed
                                        ((pattern (Var z))
                                         (meta
                                          ((type_ (UArray UReal)) (loc <opaque>)
                                           (adlevel AutoDiffable))))
                                        ((Single
                                          ((pattern (Var k))
                                           (meta
                                            ((type_ UInt) (loc <opaque>)
                                             (adlevel DataOnly))))))))
                                      (meta
                                       ((type_ UReal) (loc <opaque>)
                                        (adlevel AutoDiffable)))))))
                                  (meta
                                   ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                                 ((pattern
                                   (Promotion
                                    ((pattern (Lit Int 1))
                                     (meta
                                      ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                    UReal DataOnly))
                                  (meta
                                   ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                              (meta
                               ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                         ((pattern
                           (FunApp (StanLib Minus__ FnPlain AoS)
                            (((pattern (Var dose))
                              (meta
                               ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                             ((pattern
                               (FunApp (StanLib Divide__ FnPlain AoS)
                                (((pattern
                                   (FunApp (StanLib Times__ FnPlain AoS)
                                    (((pattern
                                       (Indexed
                                        ((pattern (Var theta))
                                         (meta
                                          ((type_ (UArray UReal)) (loc <opaque>)
                                           (adlevel AutoDiffable))))
                                        ((Single
                                          ((pattern (Var k))
                                           (meta
                                            ((type_ UInt) (loc <opaque>)
                                             (adlevel DataOnly))))))))
                                      (meta
                                       ((type_ UReal) (loc <opaque>)
                                        (adlevel AutoDiffable))))
                                     ((pattern
                                       (Indexed
                                        ((pattern (Var z))
                                         (meta
                                          ((type_ (UArray UReal)) (loc <opaque>)
                                           (adlevel AutoDiffable))))
                                        ((Single
                                          ((pattern (Var k))
                                           (meta
                                            ((type_ UInt) (loc <opaque>)
                                             (adlevel DataOnly))))))))
                                      (meta
                                       ((type_ UReal) (loc <opaque>)
                                        (adlevel AutoDiffable)))))))
                                  (meta
                                   ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                                 ((pattern
                                   (FunApp (StanLib Plus__ FnPlain AoS)
                                    (((pattern
                                       (Promotion
                                        ((pattern (Lit Int 1))
                                         (meta
                                          ((type_ UInt) (loc <opaque>)
                                           (adlevel DataOnly))))
                                        UReal DataOnly))
                                      (meta
                                       ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                                     ((pattern
                                       (FunApp (StanLib abs FnPlain AoS)
                                        (((pattern
                                           (Indexed
                                            ((pattern (Var z))
                                             (meta
                                              ((type_ (UArray UReal)) 
                                               (loc <opaque>) (adlevel AutoDiffable))))
                                            ((Single
                                              ((pattern (Var k))
                                               (meta
                                                ((type_ UInt) (loc <opaque>)
                                                 (adlevel DataOnly))))))))
                                          (meta
                                           ((type_ UReal) (loc <opaque>)
                                            (adlevel AutoDiffable)))))))
                                      (meta
                                       ((type_ UReal) (loc <opaque>)
                                        (adlevel AutoDiffable)))))))
                                  (meta
                                   ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                              (meta
                               ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                   (meta <opaque>)))))
               (meta <opaque>)))))
           (meta <opaque>))
          ((pattern
            (Return
             (((pattern (Var dz))
               (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta <opaque>)))))
       (meta <opaque>))))
    (fdloc <opaque>))
   ((fdrt (ReturnType (UArray UReal))) (fdname f_udf) (fdsuffix FnPlain)
    (fdargs
     ((AutoDiffable t UReal) (AutoDiffable z (UArray UReal))
      (AutoDiffable theta (UArray UReal)) (AutoDiffable x_r (UArray UReal))
      (AutoDiffable x_i (UArray UInt))))
    (fdbody
     (((pattern
        (Block
         (((pattern
            (Return
             (((pattern
                (FunApp (CompilerInternal FnMakeArray)
                 (((pattern
                    (FunApp (UserDefined scale FnPlain)
                     (((pattern
                        (Indexed
                         ((pattern (Var z))
                          (meta
                           ((type_ (UArray UReal)) (loc <opaque>) (adlevel AutoDiffable))))
                         ((Single
                           ((pattern (Lit Int 1))
                            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                       (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                      ((pattern
                        (Indexed
                         ((pattern (Var theta))
                          (meta
                           ((type_ (UArray UReal)) (loc <opaque>) (adlevel AutoDiffable))))
                         ((Single
                           ((pattern (Lit Int 1))
                            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                       (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                  ((pattern
                    (TernaryIf
                     ((pattern
                       (FunApp (StanLib Greater__ FnPlain AoS)
                        (((pattern
                           (Indexed
                            ((pattern (Var z))
                             (meta
                              ((type_ (UArray UReal)) (loc <opaque>)
                               (adlevel AutoDiffable))))
                            ((Single
                              ((pattern (Lit Int 2))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                         ((pattern (Lit Int 0))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
                     ((pattern
                       (FunApp (StanLib Times__ FnPlain AoS)
                        (((pattern
                           (FunApp (StanLib PMinus__ FnPlain AoS)
                            (((pattern
                               (Indexed
                                ((pattern (Var theta))
                                 (meta
                                  ((type_ (UArray UReal)) (loc <opaque>)
                                   (adlevel AutoDiffable))))
                                ((Single
                                  ((pattern (Lit Int 2))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                              (meta
                               ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                         ((pattern
                           (Indexed
                            ((pattern (Var z))
                             (meta
                              ((type_ (UArray UReal)) (loc <opaque>)
                               (adlevel AutoDiffable))))
                            ((Single
                              ((pattern (Lit Int 2))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                     ((pattern
                       (Indexed
                        ((pattern (Var theta))
                         (meta
                          ((type_ (UArray UReal)) (loc <opaque>) (adlevel AutoDiffable))))
                        ((Single
                          ((pattern (Lit Int 2))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
               (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta <opaque>)))))
       (meta <opaque>))))
    (fdloc <opaque>))
   ((fdrt (ReturnType UReal)) (fdname scale) (fdsuffix FnPlain)
    (fdargs ((AutoDiffable a UReal) (AutoDiffable b UReal)))
    (fdbody
     (((pattern
        (Block
         (((pattern
            (Return
             (((pattern
                (FunApp (StanLib fma FnPlain AoS)
                 (((pattern (Var a))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                  ((pattern (Var b))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                  ((pattern
                    (FunApp (StanLib inv_logit FnPlain AoS)
                     (((pattern (Var a))
                       (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta <opaque>)))))
       (meta <opaque>))))
    (fdloc <opaque>))
   ((fdrt (ReturnType (UArray UReal))) (fdname f_early) (fdsuffix FnPlain)
    (fdargs
     ((AutoDiffable t UReal) (AutoDiffable z (UArray UReal))
      (AutoDiffable theta (UArray UReal)) (AutoDiffable x_r (UArray UReal))
      (AutoDiffable x_i (UArray UInt))))
    (fdbody
     (((pattern
        (Block
         (((pattern
            (IfElse
             ((pattern
               (FunApp (StanLib Greater__ FnPlain AoS)
                (((pattern (Var t))
                  (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                 ((pattern (Lit Real 0.5))
                  (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
              (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
             ((pattern
               (Block
                (((pattern
                   (Return
                    (((pattern
                       (FunApp (CompilerInternal FnMakeArray)
                        (((pattern
                           (FunApp (StanLib Times__ FnPlain AoS)
                            (((pattern
                               (Indexed
                                ((pattern (Var theta))
                                 (meta
                                  ((type_ (UArray UReal)) (loc <opaque>)
                                   (adlevel AutoDiffable))))
                                ((Single
                                  ((pattern (Lit Int 1))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                              (meta
                               ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                             ((pattern
                               (Indexed
                                ((pattern (Var z))
                                 (meta
                                  ((type_ (UArray UReal)) (loc <opaque>)
                                   (adlevel AutoDiffable))))
                                ((Single
                                  ((pattern (Lit Int 1))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                              (meta
                               ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                         ((pattern
                           (FunApp (StanLib Times__ FnPlain AoS)
                            (((pattern
                               (Indexed
                                ((pattern (Var theta))
                                 (meta
                                  ((type_ (UArray UReal)) (loc <opaque>)
                                   (adlevel AutoDiffable))))
                                ((Single
                                  ((pattern (Lit Int 2))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                              (meta
                               ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                             ((pattern
                               (Indexed
                                ((pattern (Var z))
                                 (meta
                                  ((type_ (UArray UReal)) (loc <opaque>)
                                   (adlevel AutoDiffable))))
                                ((Single
                                  ((pattern (Lit Int 2))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                              (meta
                               ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                      (meta
                       ((type_ (UArray UReal)) (loc <opaque>) (adlevel AutoDiffable)))))))
                  (meta <opaque>)))))
              (meta <opaque>))
             ()))
           (meta <opaque>))
          ((pattern
            (Return
             (((pattern
                (FunApp (CompilerInternal FnMakeArray)
                 (((pattern
                    (FunApp (StanLib PMinus__ FnPlain AoS)
                     (((pattern
                        (Indexed
                         ((pattern (Var z))
                          (meta
                           ((type_ (UArray UReal)) (loc <opaque>) (adlevel AutoDiffable))))
                         ((Single
                           ((pattern (Lit Int 1))
                            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                       (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                  ((pattern
                    (FunApp (StanLib PMinus__ FnPlain AoS)
                     (((pattern
                        (Indexed
                         ((pattern (Var z))
                          (meta
                           ((type_ (UArray UReal)) (loc <opaque>) (adlevel AutoDiffable))))
                         ((Single
                           ((pattern (Lit Int 2))
                            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                       (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
               (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta <opaque>)))))
       (meta <opaque>))))
    (fdloc <opaque>))))
 (input_vars ((N <opaque> SInt)))
 (prepare_data
  (((pattern
     (Decl (decl_adtype DataOnly) (decl_id N) (decl_type (Sized SInt))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable N) ()) UInt
      ((pattern
        (Indexed
         ((pattern
           (FunApp (CompilerInternal FnReadData)
            (((pattern (Lit Str N))
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
        (var_name N)
        (var ((pattern (Var N)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
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
            (FnReadParam (constrain Identity) (dims ()) (mem_pattern AoS)))
           ()))
         (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib normal_lpdf (FnLpdf true) AoS)
             (((pattern (Var p))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern
                (Promotion
                 ((pattern (Lit Int 0))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal DataOnly))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
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
     (Decl (decl_adtype AutoDiffable) (decl_id p) (decl_type (Sized SReal))
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
            (FunApp (StanLib normal_lpdf (FnLpdf true) SoA)
             (((pattern (Var p))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern
                (Promotion
                 ((pattern (Lit Int 0))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal DataOnly))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
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
     (Decl (decl_adtype DataOnly) (decl_id p) (decl_type (Sized SReal))
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
      (initialize Uninit)))
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
       (FnWriteParam (unconstrain_opt (Identity))
        (var
         ((pattern (Var p)) (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))))
 (unconstrain_array
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id p) (decl_type (Sized SReal))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable p) ()) UReal
      ((pattern (FunApp (CompilerInternal FnReadDeserializer) ()))
       (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt (Identity))
        (var
         ((pattern (Var p)) (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))))
 (output_vars
  ((p <opaque>
    ((out_unconstrained_st SReal) (out_constrained_st SReal) (out_block Parameters)
     (out_trans Identity)))))
 (prog_name odefns_model) (prog_path tests/fixtures/odefns.stan))

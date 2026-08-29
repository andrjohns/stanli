((functions_block
  (((fdrt (ReturnType UMatrix)) (fdname fillholes) (fdsuffix FnPlain)
    (fdargs ((DataOnly matin UMatrix) (DataOnly x UReal)))
    (fdbody
     (((pattern
        (Block
         (((pattern
            (NRFunApp (CompilerInternal FnValidateSize)
             (((pattern (Lit Str matout))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
              ((pattern (Lit Str "rows(matin)"))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
              ((pattern
                (FunApp (StanLib rows FnPlain AoS)
                 (((pattern (Var matin))
                   (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
           (meta <opaque>))
          ((pattern
            (NRFunApp (CompilerInternal FnValidateSize)
             (((pattern (Lit Str matout))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
              ((pattern (Lit Str "cols(matin)"))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
              ((pattern
                (FunApp (StanLib cols FnPlain AoS)
                 (((pattern (Var matin))
                   (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
           (meta <opaque>))
          ((pattern
            (Decl (decl_adtype AutoDiffable) (decl_id matout)
             (decl_type
              (Sized
               (SMatrix AoS
                ((pattern
                  (FunApp (StanLib rows FnPlain AoS)
                   (((pattern (Var matin))
                     (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                ((pattern
                  (FunApp (StanLib cols FnPlain AoS)
                   (((pattern (Var matin))
                     (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
             (initialize Default)))
           (meta <opaque>))
          ((pattern
            (For (loopvar ri)
             (lower
              ((pattern (Lit Int 1))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
             (upper
              ((pattern
                (FunApp (StanLib rows FnPlain AoS)
                 (((pattern (Var matin))
                   (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
             (body
              ((pattern
                (Block
                 (((pattern
                    (For (loopvar ci)
                     (lower
                      ((pattern (Lit Int 1))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                     (upper
                      ((pattern
                        (FunApp (StanLib cols FnPlain AoS)
                         (((pattern (Var matin))
                           (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
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
                                    ((pattern (Var matin))
                                     (meta
                                      ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly))))
                                    ((Single
                                      ((pattern (Var ri))
                                       (meta
                                        ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                                     (Single
                                      ((pattern (Var ci))
                                       (meta
                                        ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                                  (meta
                                   ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                                 ((pattern (Lit Int 0))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                             ((pattern
                               (Block
                                (((pattern
                                   (Assignment
                                    ((LVariable matout)
                                     ((Single
                                       ((pattern (Var ri))
                                        (meta
                                         ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                                      (Single
                                       ((pattern (Var ci))
                                        (meta
                                         ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                                    UMatrix
                                    ((pattern
                                      (FunApp (StanLib Times__ FnPlain AoS)
                                       (((pattern (Var x))
                                         (meta
                                          ((type_ UReal) (loc <opaque>)
                                           (adlevel DataOnly))))
                                        ((pattern
                                          (Indexed
                                           ((pattern (Var matin))
                                            (meta
                                             ((type_ UMatrix) (loc <opaque>)
                                              (adlevel DataOnly))))
                                           ((Single
                                             ((pattern (Var ri))
                                              (meta
                                               ((type_ UInt) (loc <opaque>)
                                                (adlevel DataOnly)))))
                                            (Single
                                             ((pattern (Var ci))
                                              (meta
                                               ((type_ UInt) (loc <opaque>)
                                                (adlevel DataOnly))))))))
                                         (meta
                                          ((type_ UReal) (loc <opaque>)
                                           (adlevel DataOnly)))))))
                                     (meta
                                      ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))))
                                  (meta <opaque>)))))
                              (meta <opaque>))
                             ()))
                           (meta <opaque>)))))
                       (meta <opaque>)))))
                   (meta <opaque>)))))
               (meta <opaque>)))))
           (meta <opaque>))
          ((pattern
            (For (loopvar ri)
             (lower
              ((pattern (Lit Int 1))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
             (upper
              ((pattern
                (FunApp (StanLib rows FnPlain AoS)
                 (((pattern (Var matin))
                   (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
             (body
              ((pattern
                (Block
                 (((pattern
                    (For (loopvar ci)
                     (lower
                      ((pattern (Lit Int 1))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                     (upper
                      ((pattern
                        (FunApp (StanLib cols FnPlain AoS)
                         (((pattern (Var matin))
                           (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                     (body
                      ((pattern
                        (Block
                         (((pattern
                            (IfElse
                             ((pattern
                               (EAnd
                                ((pattern
                                  (FunApp (StanLib is_nan FnPlain AoS)
                                   (((pattern
                                      (Indexed
                                       ((pattern (Var matout))
                                        (meta
                                         ((type_ UMatrix) (loc <opaque>)
                                          (adlevel AutoDiffable))))
                                       ((Single
                                         ((pattern (Var ri))
                                          (meta
                                           ((type_ UInt) (loc <opaque>)
                                            (adlevel DataOnly)))))
                                        (Single
                                         ((pattern (Var ci))
                                          (meta
                                           ((type_ UInt) (loc <opaque>)
                                            (adlevel DataOnly))))))))
                                     (meta
                                      ((type_ UReal) (loc <opaque>)
                                       (adlevel AutoDiffable)))))))
                                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                ((pattern
                                  (FunApp (StanLib PNot__ FnPlain AoS)
                                   (((pattern
                                      (FunApp (StanLib is_nan FnPlain AoS)
                                       (((pattern
                                          (Indexed
                                           ((pattern (Var matin))
                                            (meta
                                             ((type_ UMatrix) (loc <opaque>)
                                              (adlevel DataOnly))))
                                           ((Single
                                             ((pattern (Var ri))
                                              (meta
                                               ((type_ UInt) (loc <opaque>)
                                                (adlevel DataOnly)))))
                                            (Single
                                             ((pattern (Var ci))
                                              (meta
                                               ((type_ UInt) (loc <opaque>)
                                                (adlevel DataOnly))))))))
                                         (meta
                                          ((type_ UReal) (loc <opaque>)
                                           (adlevel DataOnly)))))))
                                     (meta
                                      ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                             ((pattern
                               (Block
                                (((pattern
                                   (Assignment
                                    ((LVariable matout)
                                     ((Single
                                       ((pattern (Var ri))
                                        (meta
                                         ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                                      (Single
                                       ((pattern (Var ci))
                                        (meta
                                         ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                                    UMatrix
                                    ((pattern
                                      (Indexed
                                       ((pattern (Var matin))
                                        (meta
                                         ((type_ UMatrix) (loc <opaque>)
                                          (adlevel DataOnly))))
                                       ((Single
                                         ((pattern (Var ri))
                                          (meta
                                           ((type_ UInt) (loc <opaque>)
                                            (adlevel DataOnly)))))
                                        (Single
                                         ((pattern (Var ci))
                                          (meta
                                           ((type_ UInt) (loc <opaque>)
                                            (adlevel DataOnly))))))))
                                     (meta
                                      ((type_ UReal) (loc <opaque>)
                                       (adlevel AutoDiffable))))))
                                  (meta <opaque>)))))
                              (meta <opaque>))
                             ()))
                           (meta <opaque>)))))
                       (meta <opaque>)))))
                   (meta <opaque>)))))
               (meta <opaque>)))))
           (meta <opaque>))
          ((pattern
            (Return
             (((pattern (Var matout))
               (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta <opaque>)))))
       (meta <opaque>))))
    (fdloc <opaque>))))
 (input_vars
  ((m <opaque>
    (SMatrix AoS
     ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
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
     (Decl (decl_adtype DataOnly) (decl_id m)
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
         (Decl (decl_adtype AutoDiffable) (decl_id m_flat__)
          (decl_type (Unsized (UArray UReal))) (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable m_flat__) ()) (UArray UReal)
          ((pattern
            (FunApp (CompilerInternal FnReadData)
             (((pattern (Lit Str m))
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
                          ((LVariable m)
                           ((Single
                             ((pattern (Var sym2__))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                            (Single
                             ((pattern (Var sym1__))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                          UMatrix
                          ((pattern
                            (Indexed
                             ((pattern (Var m_flat__))
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
         (Decl (decl_adtype AutoDiffable) (decl_id inline_fillholes_return_sym6__)
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
         (Block
          (((pattern
             (NRFunApp (CompilerInternal FnValidateSize)
              (((pattern (Lit Str matout))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Str "rows(matin)"))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern
                 (FunApp (StanLib rows FnPlain AoS)
                  (((pattern (Var m))
                    (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
            (meta <opaque>))
           ((pattern
             (NRFunApp (CompilerInternal FnValidateSize)
              (((pattern (Lit Str matout))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Str "cols(matin)"))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern
                 (FunApp (StanLib cols FnPlain AoS)
                  (((pattern (Var m))
                    (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
            (meta <opaque>))
           ((pattern
             (Decl (decl_adtype AutoDiffable) (decl_id inline_fillholes_matout_sym7__)
              (decl_type
               (Sized
                (SMatrix AoS
                 ((pattern
                   (FunApp (StanLib rows FnPlain AoS)
                    (((pattern (Var m))
                      (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 ((pattern
                   (FunApp (StanLib cols FnPlain AoS)
                    (((pattern (Var m))
                      (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
              (initialize Default)))
            (meta <opaque>))
           ((pattern
             (For (loopvar inline_fillholes_ri_sym9__)
              (lower
               ((pattern (Lit Int 1))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
              (upper
               ((pattern
                 (FunApp (StanLib rows FnPlain AoS)
                  (((pattern (Var m))
                    (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
              (body
               ((pattern
                 (Block
                  (((pattern
                     (For (loopvar inline_fillholes_ci_sym8__)
                      (lower
                       ((pattern (Lit Int 1))
                        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                      (upper
                       ((pattern
                         (FunApp (StanLib cols FnPlain AoS)
                          (((pattern (Var m))
                            (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
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
                                     ((pattern (Var m))
                                      (meta
                                       ((type_ UMatrix) (loc <opaque>)
                                        (adlevel DataOnly))))
                                     ((Single
                                       ((pattern (Var inline_fillholes_ri_sym9__))
                                        (meta
                                         ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                                      (Single
                                       ((pattern (Var inline_fillholes_ci_sym8__))
                                        (meta
                                         ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                                   (meta
                                    ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                                  ((pattern (Lit Int 0))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern
                                (Block
                                 (((pattern
                                    (Assignment
                                     ((LVariable inline_fillholes_matout_sym7__)
                                      ((Single
                                        ((pattern (Var inline_fillholes_ri_sym9__))
                                         (meta
                                          ((type_ UInt) (loc <opaque>)
                                           (adlevel DataOnly)))))
                                       (Single
                                        ((pattern (Var inline_fillholes_ci_sym8__))
                                         (meta
                                          ((type_ UInt) (loc <opaque>)
                                           (adlevel DataOnly)))))))
                                     UMatrix
                                     ((pattern
                                       (FunApp (StanLib Times__ FnPlain AoS)
                                        (((pattern (Lit Real 2.0))
                                          (meta
                                           ((type_ UReal) (loc <opaque>)
                                            (adlevel DataOnly))))
                                         ((pattern
                                           (Indexed
                                            ((pattern (Var m))
                                             (meta
                                              ((type_ UMatrix) (loc <opaque>)
                                               (adlevel DataOnly))))
                                            ((Single
                                              ((pattern (Var inline_fillholes_ri_sym9__))
                                               (meta
                                                ((type_ UInt) (loc <opaque>)
                                                 (adlevel DataOnly)))))
                                             (Single
                                              ((pattern (Var inline_fillholes_ci_sym8__))
                                               (meta
                                                ((type_ UInt) (loc <opaque>)
                                                 (adlevel DataOnly))))))))
                                          (meta
                                           ((type_ UReal) (loc <opaque>)
                                            (adlevel DataOnly)))))))
                                      (meta
                                       ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))))
                                   (meta <opaque>)))))
                               (meta <opaque>))
                              ()))
                            (meta <opaque>)))))
                        (meta <opaque>)))))
                    (meta <opaque>)))))
                (meta <opaque>)))))
            (meta <opaque>))
           ((pattern
             (For (loopvar inline_fillholes_ri_sym9__)
              (lower
               ((pattern (Lit Int 1))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
              (upper
               ((pattern
                 (FunApp (StanLib rows FnPlain AoS)
                  (((pattern (Var m))
                    (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
              (body
               ((pattern
                 (Block
                  (((pattern
                     (For (loopvar inline_fillholes_ci_sym8__)
                      (lower
                       ((pattern (Lit Int 1))
                        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                      (upper
                       ((pattern
                         (FunApp (StanLib cols FnPlain AoS)
                          (((pattern (Var m))
                            (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
                        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                      (body
                       ((pattern
                         (Block
                          (((pattern
                             (IfElse
                              ((pattern
                                (EAnd
                                 ((pattern
                                   (FunApp (StanLib is_nan FnPlain AoS)
                                    (((pattern
                                       (Indexed
                                        ((pattern (Var inline_fillholes_matout_sym7__))
                                         (meta
                                          ((type_ UMatrix) (loc <opaque>)
                                           (adlevel AutoDiffable))))
                                        ((Single
                                          ((pattern (Var inline_fillholes_ri_sym9__))
                                           (meta
                                            ((type_ UInt) (loc <opaque>)
                                             (adlevel DataOnly)))))
                                         (Single
                                          ((pattern (Var inline_fillholes_ci_sym8__))
                                           (meta
                                            ((type_ UInt) (loc <opaque>)
                                             (adlevel DataOnly))))))))
                                      (meta
                                       ((type_ UReal) (loc <opaque>)
                                        (adlevel AutoDiffable)))))))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 ((pattern
                                   (FunApp (StanLib PNot__ FnPlain AoS)
                                    (((pattern
                                       (FunApp (StanLib is_nan FnPlain AoS)
                                        (((pattern
                                           (Indexed
                                            ((pattern (Var m))
                                             (meta
                                              ((type_ UMatrix) (loc <opaque>)
                                               (adlevel DataOnly))))
                                            ((Single
                                              ((pattern (Var inline_fillholes_ri_sym9__))
                                               (meta
                                                ((type_ UInt) (loc <opaque>)
                                                 (adlevel DataOnly)))))
                                             (Single
                                              ((pattern (Var inline_fillholes_ci_sym8__))
                                               (meta
                                                ((type_ UInt) (loc <opaque>)
                                                 (adlevel DataOnly))))))))
                                          (meta
                                           ((type_ UReal) (loc <opaque>)
                                            (adlevel DataOnly)))))))
                                      (meta
                                       ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern
                                (Block
                                 (((pattern
                                    (Assignment
                                     ((LVariable inline_fillholes_matout_sym7__)
                                      ((Single
                                        ((pattern (Var inline_fillholes_ri_sym9__))
                                         (meta
                                          ((type_ UInt) (loc <opaque>)
                                           (adlevel DataOnly)))))
                                       (Single
                                        ((pattern (Var inline_fillholes_ci_sym8__))
                                         (meta
                                          ((type_ UInt) (loc <opaque>)
                                           (adlevel DataOnly)))))))
                                     UMatrix
                                     ((pattern
                                       (Indexed
                                        ((pattern (Var m))
                                         (meta
                                          ((type_ UMatrix) (loc <opaque>)
                                           (adlevel DataOnly))))
                                        ((Single
                                          ((pattern (Var inline_fillholes_ri_sym9__))
                                           (meta
                                            ((type_ UInt) (loc <opaque>)
                                             (adlevel DataOnly)))))
                                         (Single
                                          ((pattern (Var inline_fillholes_ci_sym8__))
                                           (meta
                                            ((type_ UInt) (loc <opaque>)
                                             (adlevel DataOnly))))))))
                                      (meta
                                       ((type_ UReal) (loc <opaque>)
                                        (adlevel AutoDiffable))))))
                                   (meta <opaque>)))))
                               (meta <opaque>))
                              ()))
                            (meta <opaque>)))))
                        (meta <opaque>)))))
                    (meta <opaque>)))))
                (meta <opaque>)))))
            (meta <opaque>))
           ((pattern
             (Assignment ((LVariable inline_fillholes_return_sym6__) ()) UMatrix
              ((pattern (Var inline_fillholes_matout_sym7__))
               (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))))
            (meta <opaque>)))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib Times__ FnPlain AoS)
             (((pattern
                (FunApp (StanLib sum FnPlain AoS)
                 (((pattern (Var inline_fillholes_return_sym6__))
                   (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
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
         (Decl (decl_adtype AutoDiffable) (decl_id inline_fillholes_return_sym1__)
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
         (Block
          (((pattern
             (NRFunApp (CompilerInternal FnValidateSize)
              (((pattern (Lit Str matout))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Str "rows(matin)"))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern
                 (FunApp (StanLib rows FnPlain SoA)
                  (((pattern (Var m))
                    (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
            (meta <opaque>))
           ((pattern
             (NRFunApp (CompilerInternal FnValidateSize)
              (((pattern (Lit Str matout))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Str "cols(matin)"))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern
                 (FunApp (StanLib cols FnPlain SoA)
                  (((pattern (Var m))
                    (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
            (meta <opaque>))
           ((pattern
             (Decl (decl_adtype AutoDiffable) (decl_id inline_fillholes_matout_sym2__)
              (decl_type
               (Sized
                (SMatrix AoS
                 ((pattern
                   (FunApp (StanLib rows FnPlain SoA)
                    (((pattern (Var m))
                      (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 ((pattern
                   (FunApp (StanLib cols FnPlain SoA)
                    (((pattern (Var m))
                      (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
              (initialize Default)))
            (meta <opaque>))
           ((pattern
             (For (loopvar inline_fillholes_ri_sym4__)
              (lower
               ((pattern (Lit Int 1))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
              (upper
               ((pattern
                 (FunApp (StanLib rows FnPlain SoA)
                  (((pattern (Var m))
                    (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
              (body
               ((pattern
                 (Block
                  (((pattern
                     (For (loopvar inline_fillholes_ci_sym3__)
                      (lower
                       ((pattern (Lit Int 1))
                        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                      (upper
                       ((pattern
                         (FunApp (StanLib cols FnPlain SoA)
                          (((pattern (Var m))
                            (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
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
                                     ((pattern (Var m))
                                      (meta
                                       ((type_ UMatrix) (loc <opaque>)
                                        (adlevel DataOnly))))
                                     ((Single
                                       ((pattern (Var inline_fillholes_ri_sym4__))
                                        (meta
                                         ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                                      (Single
                                       ((pattern (Var inline_fillholes_ci_sym3__))
                                        (meta
                                         ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                                   (meta
                                    ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                                  ((pattern (Lit Int 0))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern
                                (Block
                                 (((pattern
                                    (Assignment
                                     ((LVariable inline_fillholes_matout_sym2__)
                                      ((Single
                                        ((pattern (Var inline_fillholes_ri_sym4__))
                                         (meta
                                          ((type_ UInt) (loc <opaque>)
                                           (adlevel DataOnly)))))
                                       (Single
                                        ((pattern (Var inline_fillholes_ci_sym3__))
                                         (meta
                                          ((type_ UInt) (loc <opaque>)
                                           (adlevel DataOnly)))))))
                                     UMatrix
                                     ((pattern
                                       (FunApp (StanLib Times__ FnPlain AoS)
                                        (((pattern (Lit Real 2.0))
                                          (meta
                                           ((type_ UReal) (loc <opaque>)
                                            (adlevel DataOnly))))
                                         ((pattern
                                           (Indexed
                                            ((pattern (Var m))
                                             (meta
                                              ((type_ UMatrix) (loc <opaque>)
                                               (adlevel DataOnly))))
                                            ((Single
                                              ((pattern (Var inline_fillholes_ri_sym4__))
                                               (meta
                                                ((type_ UInt) (loc <opaque>)
                                                 (adlevel DataOnly)))))
                                             (Single
                                              ((pattern (Var inline_fillholes_ci_sym3__))
                                               (meta
                                                ((type_ UInt) (loc <opaque>)
                                                 (adlevel DataOnly))))))))
                                          (meta
                                           ((type_ UReal) (loc <opaque>)
                                            (adlevel DataOnly)))))))
                                      (meta
                                       ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))))
                                   (meta <opaque>)))))
                               (meta <opaque>))
                              ()))
                            (meta <opaque>)))))
                        (meta <opaque>)))))
                    (meta <opaque>)))))
                (meta <opaque>)))))
            (meta <opaque>))
           ((pattern
             (For (loopvar inline_fillholes_ri_sym4__)
              (lower
               ((pattern (Lit Int 1))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
              (upper
               ((pattern
                 (FunApp (StanLib rows FnPlain SoA)
                  (((pattern (Var m))
                    (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
              (body
               ((pattern
                 (Block
                  (((pattern
                     (For (loopvar inline_fillholes_ci_sym3__)
                      (lower
                       ((pattern (Lit Int 1))
                        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                      (upper
                       ((pattern
                         (FunApp (StanLib cols FnPlain SoA)
                          (((pattern (Var m))
                            (meta ((type_ UMatrix) (loc <opaque>) (adlevel DataOnly)))))))
                        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                      (body
                       ((pattern
                         (Block
                          (((pattern
                             (IfElse
                              ((pattern
                                (EAnd
                                 ((pattern
                                   (FunApp (StanLib is_nan FnPlain SoA)
                                    (((pattern
                                       (Indexed
                                        ((pattern (Var inline_fillholes_matout_sym2__))
                                         (meta
                                          ((type_ UMatrix) (loc <opaque>)
                                           (adlevel AutoDiffable))))
                                        ((Single
                                          ((pattern (Var inline_fillholes_ri_sym4__))
                                           (meta
                                            ((type_ UInt) (loc <opaque>)
                                             (adlevel DataOnly)))))
                                         (Single
                                          ((pattern (Var inline_fillholes_ci_sym3__))
                                           (meta
                                            ((type_ UInt) (loc <opaque>)
                                             (adlevel DataOnly))))))))
                                      (meta
                                       ((type_ UReal) (loc <opaque>)
                                        (adlevel AutoDiffable)))))))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 ((pattern
                                   (FunApp (StanLib PNot__ FnPlain SoA)
                                    (((pattern
                                       (FunApp (StanLib is_nan FnPlain SoA)
                                        (((pattern
                                           (Indexed
                                            ((pattern (Var m))
                                             (meta
                                              ((type_ UMatrix) (loc <opaque>)
                                               (adlevel DataOnly))))
                                            ((Single
                                              ((pattern (Var inline_fillholes_ri_sym4__))
                                               (meta
                                                ((type_ UInt) (loc <opaque>)
                                                 (adlevel DataOnly)))))
                                             (Single
                                              ((pattern (Var inline_fillholes_ci_sym3__))
                                               (meta
                                                ((type_ UInt) (loc <opaque>)
                                                 (adlevel DataOnly))))))))
                                          (meta
                                           ((type_ UReal) (loc <opaque>)
                                            (adlevel DataOnly)))))))
                                      (meta
                                       ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern
                                (Block
                                 (((pattern
                                    (Assignment
                                     ((LVariable inline_fillholes_matout_sym2__)
                                      ((Single
                                        ((pattern (Var inline_fillholes_ri_sym4__))
                                         (meta
                                          ((type_ UInt) (loc <opaque>)
                                           (adlevel DataOnly)))))
                                       (Single
                                        ((pattern (Var inline_fillholes_ci_sym3__))
                                         (meta
                                          ((type_ UInt) (loc <opaque>)
                                           (adlevel DataOnly)))))))
                                     UMatrix
                                     ((pattern
                                       (Indexed
                                        ((pattern (Var m))
                                         (meta
                                          ((type_ UMatrix) (loc <opaque>)
                                           (adlevel DataOnly))))
                                        ((Single
                                          ((pattern (Var inline_fillholes_ri_sym4__))
                                           (meta
                                            ((type_ UInt) (loc <opaque>)
                                             (adlevel DataOnly)))))
                                         (Single
                                          ((pattern (Var inline_fillholes_ci_sym3__))
                                           (meta
                                            ((type_ UInt) (loc <opaque>)
                                             (adlevel DataOnly))))))))
                                      (meta
                                       ((type_ UReal) (loc <opaque>)
                                        (adlevel AutoDiffable))))))
                                   (meta <opaque>)))))
                               (meta <opaque>))
                              ()))
                            (meta <opaque>)))))
                        (meta <opaque>)))))
                    (meta <opaque>)))))
                (meta <opaque>)))))
            (meta <opaque>))
           ((pattern
             (Assignment ((LVariable inline_fillholes_return_sym1__) ()) UMatrix
              ((pattern (Var inline_fillholes_matout_sym2__))
               (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable))))))
            (meta <opaque>)))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib Times__ FnPlain SoA)
             (((pattern
                (FunApp (StanLib sum FnPlain AoS)
                 (((pattern (Var inline_fillholes_return_sym1__))
                   (meta ((type_ UMatrix) (loc <opaque>) (adlevel AutoDiffable)))))))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
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
 (prog_name localmat_cond_model) (prog_path tests/fixtures/localmat_cond.stan))

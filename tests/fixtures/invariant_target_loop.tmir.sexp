((functions_block ())
 (input_vars ((N <opaque> SInt) (M <opaque> SInt) (mode <opaque> SInt)))
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
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id M) (decl_type (Sized SInt))
      (initialize Uninit)))
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
     (Decl (decl_adtype DataOnly) (decl_id mode) (decl_type (Sized SInt))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable mode) ()) UInt
      ((pattern
        (Indexed
         ((pattern
           (FunApp (CompilerInternal FnReadData)
            (((pattern (Lit Str mode))
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
        (var_name mode)
        (var
         ((pattern (Var mode)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (((pattern (Lit Int 0)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnCheck
        (trans
         (Upper
          ((pattern (Lit Int 7)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
        (var_name mode)
        (var
         ((pattern (Var mode)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (((pattern (Lit Int 7)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))))
 (log_prob
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id x) (decl_type (Sized SReal))
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
            (FunApp (StanLib Equals__ FnPlain AoS)
             (((pattern (Var mode))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
              ((pattern (Lit Int 0))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
          ((pattern
            (Block
             (((pattern
                (For (loopvar n)
                 (lower
                  ((pattern (Lit Int 1))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                 (upper
                  ((pattern (Var N))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                 (body
                  ((pattern
                    (Block
                     (((pattern
                        (TargetPE
                         ((pattern
                           (FunApp (StanLib normal_lpdf (FnLpdf true) AoS)
                            (((pattern (Var x))
                              (meta
                               ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
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
                   (meta <opaque>)))))
               (meta <opaque>)))))
           (meta <opaque>))
          (((pattern
             (Block
              (((pattern
                 (IfElse
                  ((pattern
                    (FunApp (StanLib Equals__ FnPlain AoS)
                     (((pattern (Var mode))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                      ((pattern (Lit Int 1))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern
                    (Block
                     (((pattern
                        (For (loopvar n)
                         (lower
                          ((pattern (Lit Int 1))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                         (upper
                          ((pattern (Var N))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                         (body
                          ((pattern
                            (Block
                             (((pattern
                                (TargetPE
                                 ((pattern
                                   (FunApp (StanLib normal_lpdf (FnLpdf true) AoS)
                                    (((pattern (Var x))
                                      (meta
                                       ((type_ UReal) (loc <opaque>)
                                        (adlevel AutoDiffable))))
                                     ((pattern
                                       (Promotion
                                        ((pattern (Var n))
                                         (meta
                                          ((type_ UInt) (loc <opaque>)
                                           (adlevel DataOnly))))
                                        UReal DataOnly))
                                      (meta
                                       ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                                     ((pattern
                                       (Promotion
                                        ((pattern (Lit Int 1))
                                         (meta
                                          ((type_ UInt) (loc <opaque>)
                                           (adlevel DataOnly))))
                                        UReal DataOnly))
                                      (meta
                                       ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                                  (meta
                                   ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                               (meta <opaque>)))))
                           (meta <opaque>)))))
                       (meta <opaque>)))))
                   (meta <opaque>))
                  (((pattern
                     (Block
                      (((pattern
                         (IfElse
                          ((pattern
                            (FunApp (StanLib Equals__ FnPlain AoS)
                             (((pattern (Var mode))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern (Lit Int 2))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                          ((pattern
                            (Block
                             (((pattern
                                (Decl (decl_adtype AutoDiffable) (decl_id acc)
                                 (decl_type (Sized SReal)) (initialize Uninit)))
                               (meta <opaque>))
                              ((pattern
                                (Assignment ((LVariable acc) ()) UReal
                                 ((pattern
                                   (Promotion
                                    ((pattern (Lit Int 0))
                                     (meta
                                      ((type_ UInt) (loc <opaque>)
                                       (adlevel AutoDiffable))))
                                    UReal AutoDiffable))
                                  (meta
                                   ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                               (meta <opaque>))
                              ((pattern
                                (For (loopvar n)
                                 (lower
                                  ((pattern (Lit Int 1))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                                 (upper
                                  ((pattern (Var N))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                                 (body
                                  ((pattern
                                    (Block
                                     (((pattern
                                        (Assignment ((LVariable acc) ()) UReal
                                         ((pattern
                                           (FunApp (StanLib Plus__ FnPlain AoS)
                                            (((pattern (Var acc))
                                              (meta
                                               ((type_ UReal) (loc <opaque>)
                                                (adlevel AutoDiffable))))
                                             ((pattern
                                               (Promotion
                                                ((pattern (Lit Int 1))
                                                 (meta
                                                  ((type_ UInt) (loc <opaque>)
                                                   (adlevel DataOnly))))
                                                UReal DataOnly))
                                              (meta
                                               ((type_ UReal) (loc <opaque>)
                                                (adlevel DataOnly)))))))
                                          (meta
                                           ((type_ UReal) (loc <opaque>)
                                            (adlevel AutoDiffable))))))
                                       (meta <opaque>))
                                      ((pattern
                                        (TargetPE
                                         ((pattern
                                           (FunApp
                                            (StanLib normal_lpdf (FnLpdf true) AoS)
                                            (((pattern (Var x))
                                              (meta
                                               ((type_ UReal) (loc <opaque>)
                                                (adlevel AutoDiffable))))
                                             ((pattern
                                               (Promotion
                                                ((pattern (Lit Int 0))
                                                 (meta
                                                  ((type_ UInt) (loc <opaque>)
                                                   (adlevel DataOnly))))
                                                UReal DataOnly))
                                              (meta
                                               ((type_ UReal) (loc <opaque>)
                                                (adlevel DataOnly))))
                                             ((pattern
                                               (Promotion
                                                ((pattern (Lit Int 1))
                                                 (meta
                                                  ((type_ UInt) (loc <opaque>)
                                                   (adlevel DataOnly))))
                                                UReal DataOnly))
                                              (meta
                                               ((type_ UReal) (loc <opaque>)
                                                (adlevel DataOnly)))))))
                                          (meta
                                           ((type_ UReal) (loc <opaque>)
                                            (adlevel AutoDiffable))))))
                                       (meta <opaque>)))))
                                   (meta <opaque>)))))
                               (meta <opaque>)))))
                           (meta <opaque>))
                          (((pattern
                             (Block
                              (((pattern
                                 (IfElse
                                  ((pattern
                                    (FunApp (StanLib Equals__ FnPlain AoS)
                                     (((pattern (Var mode))
                                       (meta
                                        ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                      ((pattern (Lit Int 3))
                                       (meta
                                        ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                  ((pattern
                                    (Block
                                     (((pattern
                                        (For (loopvar n)
                                         (lower
                                          ((pattern (Lit Int 1))
                                           (meta
                                            ((type_ UInt) (loc <opaque>)
                                             (adlevel DataOnly)))))
                                         (upper
                                          ((pattern (Var N))
                                           (meta
                                            ((type_ UInt) (loc <opaque>)
                                             (adlevel DataOnly)))))
                                         (body
                                          ((pattern
                                            (Block
                                             (((pattern
                                                (NRFunApp (CompilerInternal FnPrint)
                                                 (((pattern
                                                    (Lit Str "invariant target loop"))
                                                   (meta
                                                    ((type_ UReal) (loc <opaque>)
                                                     (adlevel DataOnly)))))))
                                               (meta <opaque>))
                                              ((pattern
                                                (TargetPE
                                                 ((pattern
                                                   (FunApp
                                                    (StanLib normal_lpdf 
                                                     (FnLpdf true) AoS)
                                                    (((pattern (Var x))
                                                      (meta
                                                       ((type_ UReal) 
                                                        (loc <opaque>)
                                                        (adlevel AutoDiffable))))
                                                     ((pattern
                                                       (Promotion
                                                        ((pattern (Lit Int 0))
                                                         (meta
                                                          ((type_ UInt) 
                                                           (loc <opaque>)
                                                           (adlevel DataOnly))))
                                                        UReal DataOnly))
                                                      (meta
                                                       ((type_ UReal) 
                                                        (loc <opaque>)
                                                        (adlevel DataOnly))))
                                                     ((pattern
                                                       (Promotion
                                                        ((pattern (Lit Int 1))
                                                         (meta
                                                          ((type_ UInt) 
                                                           (loc <opaque>)
                                                           (adlevel DataOnly))))
                                                        UReal DataOnly))
                                                      (meta
                                                       ((type_ UReal) 
                                                        (loc <opaque>)
                                                        (adlevel DataOnly)))))))
                                                  (meta
                                                   ((type_ UReal) (loc <opaque>)
                                                    (adlevel AutoDiffable))))))
                                               (meta <opaque>)))))
                                           (meta <opaque>)))))
                                       (meta <opaque>)))))
                                   (meta <opaque>))
                                  (((pattern
                                     (Block
                                      (((pattern
                                         (IfElse
                                          ((pattern
                                            (FunApp (StanLib Equals__ FnPlain AoS)
                                             (((pattern (Var mode))
                                               (meta
                                                ((type_ UInt) (loc <opaque>)
                                                 (adlevel DataOnly))))
                                              ((pattern (Lit Int 4))
                                               (meta
                                                ((type_ UInt) (loc <opaque>)
                                                 (adlevel DataOnly)))))))
                                           (meta
                                            ((type_ UInt) (loc <opaque>)
                                             (adlevel DataOnly))))
                                          ((pattern
                                            (Block
                                             (((pattern
                                                (For (loopvar n)
                                                 (lower
                                                  ((pattern (Lit Int 1))
                                                   (meta
                                                    ((type_ UInt) (loc <opaque>)
                                                     (adlevel DataOnly)))))
                                                 (upper
                                                  ((pattern (Var N))
                                                   (meta
                                                    ((type_ UInt) (loc <opaque>)
                                                     (adlevel DataOnly)))))
                                                 (body
                                                  ((pattern
                                                    (Block
                                                     (((pattern
                                                        (TargetPE
                                                         ((pattern
                                                           (FunApp
                                                            (StanLib normal_lpdf
                                                             (FnLpdf true) AoS)
                                                            (((pattern (Var x))
                                                              (meta
                                                               ((type_ UReal)
                                                                (loc <opaque>)
                                                                (adlevel AutoDiffable))))
                                                             ((pattern
                                                               (Promotion
                                                                ((pattern (Lit Int 0))
                                                                 (meta
                                                                  ((type_ UInt)
                                                                   (loc <opaque>)
                                                                   (adlevel DataOnly))))
                                                                UReal DataOnly))
                                                              (meta
                                                               ((type_ UReal)
                                                                (loc <opaque>)
                                                                (adlevel DataOnly))))
                                                             ((pattern
                                                               (Promotion
                                                                ((pattern (Lit Int 1))
                                                                 (meta
                                                                  ((type_ UInt)
                                                                   (loc <opaque>)
                                                                   (adlevel DataOnly))))
                                                                UReal DataOnly))
                                                              (meta
                                                               ((type_ UReal)
                                                                (loc <opaque>)
                                                                (adlevel DataOnly)))))))
                                                          (meta
                                                           ((type_ UReal) 
                                                            (loc <opaque>)
                                                            (adlevel AutoDiffable))))))
                                                       (meta <opaque>))
                                                      ((pattern
                                                        (NRFunApp
                                                         (CompilerInternal FnReject)
                                                         (((pattern
                                                            (Lit Str
                                                             "invariant target loop"))
                                                           (meta
                                                            ((type_ UReal) 
                                                             (loc <opaque>)
                                                             (adlevel DataOnly)))))))
                                                       (meta <opaque>)))))
                                                   (meta <opaque>)))))
                                               (meta <opaque>)))))
                                           (meta <opaque>))
                                          (((pattern
                                             (Block
                                              (((pattern
                                                 (IfElse
                                                  ((pattern
                                                    (FunApp
                                                     (StanLib Equals__ FnPlain AoS)
                                                     (((pattern (Var mode))
                                                       (meta
                                                        ((type_ UInt) 
                                                         (loc <opaque>)
                                                         (adlevel DataOnly))))
                                                      ((pattern (Lit Int 5))
                                                       (meta
                                                        ((type_ UInt) 
                                                         (loc <opaque>)
                                                         (adlevel DataOnly)))))))
                                                   (meta
                                                    ((type_ UInt) (loc <opaque>)
                                                     (adlevel DataOnly))))
                                                  ((pattern
                                                    (Block
                                                     (((pattern
                                                        (For (loopvar n)
                                                         (lower
                                                          ((pattern (Lit Int 1))
                                                           (meta
                                                            ((type_ UInt) 
                                                             (loc <opaque>)
                                                             (adlevel DataOnly)))))
                                                         (upper
                                                          ((pattern (Var N))
                                                           (meta
                                                            ((type_ UInt) 
                                                             (loc <opaque>)
                                                             (adlevel DataOnly)))))
                                                         (body
                                                          ((pattern
                                                            (Block
                                                             (((pattern
                                                                (For 
                                                                 (loopvar m)
                                                                 (lower
                                                                  ((pattern (Lit Int 1))
                                                                   (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly)))))
                                                                 (upper
                                                                  ((pattern (Var M))
                                                                   (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly)))))
                                                                 (body
                                                                  ((pattern
                                                                    (Block
                                                                    (((pattern
                                                                    (TargetPE
                                                                    ((pattern
                                                                    (FunApp
                                                                    (StanLib normal_lpdf
                                                                    (FnLpdf true) AoS)
                                                                    (((pattern (Var x))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable))))
                                                                    ((pattern
                                                                    (Promotion
                                                                    ((pattern
                                                                    (Lit Int 0))
                                                                    (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly))))
                                                                    UReal DataOnly))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly))))
                                                                    ((pattern
                                                                    (Promotion
                                                                    ((pattern
                                                                    (Lit Int 1))
                                                                    (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly))))
                                                                    UReal DataOnly))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly)))))))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable))))))
                                                                    (meta <opaque>)))))
                                                                   (meta <opaque>)))))
                                                               (meta <opaque>)))))
                                                           (meta <opaque>)))))
                                                       (meta <opaque>)))))
                                                   (meta <opaque>))
                                                  (((pattern
                                                     (Block
                                                      (((pattern
                                                         (IfElse
                                                          ((pattern
                                                            (FunApp
                                                             (StanLib Equals__ FnPlain
                                                              AoS)
                                                             (((pattern (Var mode))
                                                               (meta
                                                                ((type_ UInt)
                                                                 (loc <opaque>)
                                                                 (adlevel DataOnly))))
                                                              ((pattern (Lit Int 6))
                                                               (meta
                                                                ((type_ UInt)
                                                                 (loc <opaque>)
                                                                 (adlevel DataOnly)))))))
                                                           (meta
                                                            ((type_ UInt) 
                                                             (loc <opaque>)
                                                             (adlevel DataOnly))))
                                                          ((pattern
                                                            (Block
                                                             (((pattern
                                                                (For 
                                                                 (loopvar n)
                                                                 (lower
                                                                  ((pattern (Lit Int 1))
                                                                   (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly)))))
                                                                 (upper
                                                                  ((pattern (Var N))
                                                                   (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly)))))
                                                                 (body
                                                                  ((pattern
                                                                    (Block
                                                                    (((pattern
                                                                    (Decl
                                                                    (decl_adtype
                                                                    AutoDiffable)
                                                                    (decl_id lane)
                                                                    (decl_type
                                                                    (Sized SReal))
                                                                    (initialize Uninit)))
                                                                    (meta <opaque>))
                                                                    ((pattern
                                                                    (Assignment
                                                                    ((LVariable lane) ())
                                                                    UReal
                                                                    ((pattern
                                                                    (Promotion
                                                                    ((pattern (Var n))
                                                                    (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable))))
                                                                    UReal AutoDiffable))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable))))))
                                                                    (meta <opaque>))
                                                                    ((pattern
                                                                    (TargetPE
                                                                    ((pattern
                                                                    (FunApp
                                                                    (StanLib normal_lpdf
                                                                    (FnLpdf true) AoS)
                                                                    (((pattern (Var x))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable))))
                                                                    ((pattern (Var lane))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable))))
                                                                    ((pattern
                                                                    (Promotion
                                                                    ((pattern
                                                                    (Lit Int 1))
                                                                    (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly))))
                                                                    UReal DataOnly))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly)))))))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable))))))
                                                                    (meta <opaque>)))))
                                                                   (meta <opaque>)))))
                                                               (meta <opaque>)))))
                                                           (meta <opaque>))
                                                          (((pattern
                                                             (Block
                                                              (((pattern
                                                                 (For 
                                                                  (loopvar n)
                                                                  (lower
                                                                   ((pattern (Lit Int 1))
                                                                    (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly)))))
                                                                  (upper
                                                                   ((pattern (Var N))
                                                                    (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly)))))
                                                                  (body
                                                                   ((pattern
                                                                    (Block
                                                                    (((pattern
                                                                    (Decl
                                                                    (decl_adtype
                                                                    AutoDiffable)
                                                                    (decl_id twice_x)
                                                                    (decl_type
                                                                    (Sized SReal))
                                                                    (initialize Uninit)))
                                                                    (meta <opaque>))
                                                                    ((pattern
                                                                    (Assignment
                                                                    ((LVariable twice_x)
                                                                    ()) UReal
                                                                    ((pattern
                                                                    (FunApp
                                                                    (StanLib Times__
                                                                    FnPlain AoS)
                                                                    (((pattern
                                                                    (Promotion
                                                                    ((pattern
                                                                    (Lit Int 2))
                                                                    (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly))))
                                                                    UReal DataOnly))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly))))
                                                                    ((pattern (Var x))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable)))))))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable))))))
                                                                    (meta <opaque>))
                                                                    ((pattern
                                                                    (TargetPE
                                                                    ((pattern
                                                                    (FunApp
                                                                    (StanLib normal_lpdf
                                                                    (FnLpdf true) AoS)
                                                                    (((pattern
                                                                    (Var twice_x))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable))))
                                                                    ((pattern
                                                                    (Promotion
                                                                    ((pattern
                                                                    (Lit Int 0))
                                                                    (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly))))
                                                                    UReal DataOnly))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly))))
                                                                    ((pattern
                                                                    (Promotion
                                                                    ((pattern
                                                                    (Lit Int 1))
                                                                    (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly))))
                                                                    UReal DataOnly))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly)))))))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable))))))
                                                                    (meta <opaque>))
                                                                    ((pattern
                                                                    (TargetPE
                                                                    ((pattern
                                                                    (FunApp
                                                                    (StanLib Times__
                                                                    FnPlain AoS)
                                                                    (((pattern
                                                                    (Lit Real 0.25))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly))))
                                                                    ((pattern (Var x))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable)))))))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable))))))
                                                                    (meta <opaque>)))))
                                                                    (meta <opaque>)))))
                                                                (meta <opaque>)))))
                                                            (meta <opaque>)))))
                                                        (meta <opaque>)))))
                                                    (meta <opaque>)))))
                                                (meta <opaque>)))))
                                            (meta <opaque>)))))
                                        (meta <opaque>)))))
                                    (meta <opaque>)))))
                                (meta <opaque>)))))
                            (meta <opaque>)))))
                        (meta <opaque>)))))
                    (meta <opaque>)))))
                (meta <opaque>)))))
            (meta <opaque>)))))
        (meta <opaque>)))))
    (meta <opaque>))))
 (reverse_mode_log_prob
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id x) (decl_type (Sized SReal))
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
            (FunApp (StanLib Equals__ FnPlain SoA)
             (((pattern (Var mode))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
              ((pattern (Lit Int 0))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
          ((pattern
            (Block
             (((pattern
                (For (loopvar n)
                 (lower
                  ((pattern (Lit Int 1))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                 (upper
                  ((pattern (Var N))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                 (body
                  ((pattern
                    (Block
                     (((pattern
                        (TargetPE
                         ((pattern
                           (FunApp (StanLib normal_lpdf (FnLpdf true) SoA)
                            (((pattern (Var x))
                              (meta
                               ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
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
                   (meta <opaque>)))))
               (meta <opaque>)))))
           (meta <opaque>))
          (((pattern
             (Block
              (((pattern
                 (IfElse
                  ((pattern
                    (FunApp (StanLib Equals__ FnPlain SoA)
                     (((pattern (Var mode))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                      ((pattern (Lit Int 1))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern
                    (Block
                     (((pattern
                        (For (loopvar n)
                         (lower
                          ((pattern (Lit Int 1))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                         (upper
                          ((pattern (Var N))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                         (body
                          ((pattern
                            (Block
                             (((pattern
                                (TargetPE
                                 ((pattern
                                   (FunApp (StanLib normal_lpdf (FnLpdf true) SoA)
                                    (((pattern (Var x))
                                      (meta
                                       ((type_ UReal) (loc <opaque>)
                                        (adlevel AutoDiffable))))
                                     ((pattern
                                       (Promotion
                                        ((pattern (Var n))
                                         (meta
                                          ((type_ UInt) (loc <opaque>)
                                           (adlevel DataOnly))))
                                        UReal DataOnly))
                                      (meta
                                       ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                                     ((pattern
                                       (Promotion
                                        ((pattern (Lit Int 1))
                                         (meta
                                          ((type_ UInt) (loc <opaque>)
                                           (adlevel DataOnly))))
                                        UReal DataOnly))
                                      (meta
                                       ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                                  (meta
                                   ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                               (meta <opaque>)))))
                           (meta <opaque>)))))
                       (meta <opaque>)))))
                   (meta <opaque>))
                  (((pattern
                     (Block
                      (((pattern
                         (IfElse
                          ((pattern
                            (FunApp (StanLib Equals__ FnPlain SoA)
                             (((pattern (Var mode))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern (Lit Int 2))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                          ((pattern
                            (Block
                             (((pattern
                                (Decl (decl_adtype AutoDiffable) (decl_id acc)
                                 (decl_type (Sized SReal)) (initialize Uninit)))
                               (meta <opaque>))
                              ((pattern
                                (Assignment ((LVariable acc) ()) UReal
                                 ((pattern
                                   (Promotion
                                    ((pattern (Lit Int 0))
                                     (meta
                                      ((type_ UInt) (loc <opaque>)
                                       (adlevel AutoDiffable))))
                                    UReal AutoDiffable))
                                  (meta
                                   ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                               (meta <opaque>))
                              ((pattern
                                (For (loopvar n)
                                 (lower
                                  ((pattern (Lit Int 1))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                                 (upper
                                  ((pattern (Var N))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                                 (body
                                  ((pattern
                                    (Block
                                     (((pattern
                                        (Assignment ((LVariable acc) ()) UReal
                                         ((pattern
                                           (FunApp (StanLib Plus__ FnPlain SoA)
                                            (((pattern (Var acc))
                                              (meta
                                               ((type_ UReal) (loc <opaque>)
                                                (adlevel AutoDiffable))))
                                             ((pattern
                                               (Promotion
                                                ((pattern (Lit Int 1))
                                                 (meta
                                                  ((type_ UInt) (loc <opaque>)
                                                   (adlevel DataOnly))))
                                                UReal DataOnly))
                                              (meta
                                               ((type_ UReal) (loc <opaque>)
                                                (adlevel DataOnly)))))))
                                          (meta
                                           ((type_ UReal) (loc <opaque>)
                                            (adlevel AutoDiffable))))))
                                       (meta <opaque>))
                                      ((pattern
                                        (TargetPE
                                         ((pattern
                                           (FunApp
                                            (StanLib normal_lpdf (FnLpdf true) SoA)
                                            (((pattern (Var x))
                                              (meta
                                               ((type_ UReal) (loc <opaque>)
                                                (adlevel AutoDiffable))))
                                             ((pattern
                                               (Promotion
                                                ((pattern (Lit Int 0))
                                                 (meta
                                                  ((type_ UInt) (loc <opaque>)
                                                   (adlevel DataOnly))))
                                                UReal DataOnly))
                                              (meta
                                               ((type_ UReal) (loc <opaque>)
                                                (adlevel DataOnly))))
                                             ((pattern
                                               (Promotion
                                                ((pattern (Lit Int 1))
                                                 (meta
                                                  ((type_ UInt) (loc <opaque>)
                                                   (adlevel DataOnly))))
                                                UReal DataOnly))
                                              (meta
                                               ((type_ UReal) (loc <opaque>)
                                                (adlevel DataOnly)))))))
                                          (meta
                                           ((type_ UReal) (loc <opaque>)
                                            (adlevel AutoDiffable))))))
                                       (meta <opaque>)))))
                                   (meta <opaque>)))))
                               (meta <opaque>)))))
                           (meta <opaque>))
                          (((pattern
                             (Block
                              (((pattern
                                 (IfElse
                                  ((pattern
                                    (FunApp (StanLib Equals__ FnPlain SoA)
                                     (((pattern (Var mode))
                                       (meta
                                        ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                      ((pattern (Lit Int 3))
                                       (meta
                                        ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                                   (meta
                                    ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                  ((pattern
                                    (Block
                                     (((pattern
                                        (For (loopvar n)
                                         (lower
                                          ((pattern (Lit Int 1))
                                           (meta
                                            ((type_ UInt) (loc <opaque>)
                                             (adlevel DataOnly)))))
                                         (upper
                                          ((pattern (Var N))
                                           (meta
                                            ((type_ UInt) (loc <opaque>)
                                             (adlevel DataOnly)))))
                                         (body
                                          ((pattern
                                            (Block
                                             (((pattern
                                                (NRFunApp (CompilerInternal FnPrint)
                                                 (((pattern
                                                    (Lit Str "invariant target loop"))
                                                   (meta
                                                    ((type_ UReal) (loc <opaque>)
                                                     (adlevel DataOnly)))))))
                                               (meta <opaque>))
                                              ((pattern
                                                (TargetPE
                                                 ((pattern
                                                   (FunApp
                                                    (StanLib normal_lpdf 
                                                     (FnLpdf true) SoA)
                                                    (((pattern (Var x))
                                                      (meta
                                                       ((type_ UReal) 
                                                        (loc <opaque>)
                                                        (adlevel AutoDiffable))))
                                                     ((pattern
                                                       (Promotion
                                                        ((pattern (Lit Int 0))
                                                         (meta
                                                          ((type_ UInt) 
                                                           (loc <opaque>)
                                                           (adlevel DataOnly))))
                                                        UReal DataOnly))
                                                      (meta
                                                       ((type_ UReal) 
                                                        (loc <opaque>)
                                                        (adlevel DataOnly))))
                                                     ((pattern
                                                       (Promotion
                                                        ((pattern (Lit Int 1))
                                                         (meta
                                                          ((type_ UInt) 
                                                           (loc <opaque>)
                                                           (adlevel DataOnly))))
                                                        UReal DataOnly))
                                                      (meta
                                                       ((type_ UReal) 
                                                        (loc <opaque>)
                                                        (adlevel DataOnly)))))))
                                                  (meta
                                                   ((type_ UReal) (loc <opaque>)
                                                    (adlevel AutoDiffable))))))
                                               (meta <opaque>)))))
                                           (meta <opaque>)))))
                                       (meta <opaque>)))))
                                   (meta <opaque>))
                                  (((pattern
                                     (Block
                                      (((pattern
                                         (IfElse
                                          ((pattern
                                            (FunApp (StanLib Equals__ FnPlain SoA)
                                             (((pattern (Var mode))
                                               (meta
                                                ((type_ UInt) (loc <opaque>)
                                                 (adlevel DataOnly))))
                                              ((pattern (Lit Int 4))
                                               (meta
                                                ((type_ UInt) (loc <opaque>)
                                                 (adlevel DataOnly)))))))
                                           (meta
                                            ((type_ UInt) (loc <opaque>)
                                             (adlevel DataOnly))))
                                          ((pattern
                                            (Block
                                             (((pattern
                                                (For (loopvar n)
                                                 (lower
                                                  ((pattern (Lit Int 1))
                                                   (meta
                                                    ((type_ UInt) (loc <opaque>)
                                                     (adlevel DataOnly)))))
                                                 (upper
                                                  ((pattern (Var N))
                                                   (meta
                                                    ((type_ UInt) (loc <opaque>)
                                                     (adlevel DataOnly)))))
                                                 (body
                                                  ((pattern
                                                    (Block
                                                     (((pattern
                                                        (TargetPE
                                                         ((pattern
                                                           (FunApp
                                                            (StanLib normal_lpdf
                                                             (FnLpdf true) SoA)
                                                            (((pattern (Var x))
                                                              (meta
                                                               ((type_ UReal)
                                                                (loc <opaque>)
                                                                (adlevel AutoDiffable))))
                                                             ((pattern
                                                               (Promotion
                                                                ((pattern (Lit Int 0))
                                                                 (meta
                                                                  ((type_ UInt)
                                                                   (loc <opaque>)
                                                                   (adlevel DataOnly))))
                                                                UReal DataOnly))
                                                              (meta
                                                               ((type_ UReal)
                                                                (loc <opaque>)
                                                                (adlevel DataOnly))))
                                                             ((pattern
                                                               (Promotion
                                                                ((pattern (Lit Int 1))
                                                                 (meta
                                                                  ((type_ UInt)
                                                                   (loc <opaque>)
                                                                   (adlevel DataOnly))))
                                                                UReal DataOnly))
                                                              (meta
                                                               ((type_ UReal)
                                                                (loc <opaque>)
                                                                (adlevel DataOnly)))))))
                                                          (meta
                                                           ((type_ UReal) 
                                                            (loc <opaque>)
                                                            (adlevel AutoDiffable))))))
                                                       (meta <opaque>))
                                                      ((pattern
                                                        (NRFunApp
                                                         (CompilerInternal FnReject)
                                                         (((pattern
                                                            (Lit Str
                                                             "invariant target loop"))
                                                           (meta
                                                            ((type_ UReal) 
                                                             (loc <opaque>)
                                                             (adlevel DataOnly)))))))
                                                       (meta <opaque>)))))
                                                   (meta <opaque>)))))
                                               (meta <opaque>)))))
                                           (meta <opaque>))
                                          (((pattern
                                             (Block
                                              (((pattern
                                                 (IfElse
                                                  ((pattern
                                                    (FunApp
                                                     (StanLib Equals__ FnPlain SoA)
                                                     (((pattern (Var mode))
                                                       (meta
                                                        ((type_ UInt) 
                                                         (loc <opaque>)
                                                         (adlevel DataOnly))))
                                                      ((pattern (Lit Int 5))
                                                       (meta
                                                        ((type_ UInt) 
                                                         (loc <opaque>)
                                                         (adlevel DataOnly)))))))
                                                   (meta
                                                    ((type_ UInt) (loc <opaque>)
                                                     (adlevel DataOnly))))
                                                  ((pattern
                                                    (Block
                                                     (((pattern
                                                        (For (loopvar n)
                                                         (lower
                                                          ((pattern (Lit Int 1))
                                                           (meta
                                                            ((type_ UInt) 
                                                             (loc <opaque>)
                                                             (adlevel DataOnly)))))
                                                         (upper
                                                          ((pattern (Var N))
                                                           (meta
                                                            ((type_ UInt) 
                                                             (loc <opaque>)
                                                             (adlevel DataOnly)))))
                                                         (body
                                                          ((pattern
                                                            (Block
                                                             (((pattern
                                                                (For 
                                                                 (loopvar m)
                                                                 (lower
                                                                  ((pattern (Lit Int 1))
                                                                   (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly)))))
                                                                 (upper
                                                                  ((pattern (Var M))
                                                                   (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly)))))
                                                                 (body
                                                                  ((pattern
                                                                    (Block
                                                                    (((pattern
                                                                    (TargetPE
                                                                    ((pattern
                                                                    (FunApp
                                                                    (StanLib normal_lpdf
                                                                    (FnLpdf true) SoA)
                                                                    (((pattern (Var x))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable))))
                                                                    ((pattern
                                                                    (Promotion
                                                                    ((pattern
                                                                    (Lit Int 0))
                                                                    (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly))))
                                                                    UReal DataOnly))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly))))
                                                                    ((pattern
                                                                    (Promotion
                                                                    ((pattern
                                                                    (Lit Int 1))
                                                                    (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly))))
                                                                    UReal DataOnly))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly)))))))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable))))))
                                                                    (meta <opaque>)))))
                                                                   (meta <opaque>)))))
                                                               (meta <opaque>)))))
                                                           (meta <opaque>)))))
                                                       (meta <opaque>)))))
                                                   (meta <opaque>))
                                                  (((pattern
                                                     (Block
                                                      (((pattern
                                                         (IfElse
                                                          ((pattern
                                                            (FunApp
                                                             (StanLib Equals__ FnPlain
                                                              SoA)
                                                             (((pattern (Var mode))
                                                               (meta
                                                                ((type_ UInt)
                                                                 (loc <opaque>)
                                                                 (adlevel DataOnly))))
                                                              ((pattern (Lit Int 6))
                                                               (meta
                                                                ((type_ UInt)
                                                                 (loc <opaque>)
                                                                 (adlevel DataOnly)))))))
                                                           (meta
                                                            ((type_ UInt) 
                                                             (loc <opaque>)
                                                             (adlevel DataOnly))))
                                                          ((pattern
                                                            (Block
                                                             (((pattern
                                                                (For 
                                                                 (loopvar n)
                                                                 (lower
                                                                  ((pattern (Lit Int 1))
                                                                   (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly)))))
                                                                 (upper
                                                                  ((pattern (Var N))
                                                                   (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly)))))
                                                                 (body
                                                                  ((pattern
                                                                    (Block
                                                                    (((pattern
                                                                    (Decl
                                                                    (decl_adtype
                                                                    AutoDiffable)
                                                                    (decl_id lane)
                                                                    (decl_type
                                                                    (Sized SReal))
                                                                    (initialize Uninit)))
                                                                    (meta <opaque>))
                                                                    ((pattern
                                                                    (Assignment
                                                                    ((LVariable lane) ())
                                                                    UReal
                                                                    ((pattern
                                                                    (Promotion
                                                                    ((pattern (Var n))
                                                                    (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable))))
                                                                    UReal AutoDiffable))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable))))))
                                                                    (meta <opaque>))
                                                                    ((pattern
                                                                    (TargetPE
                                                                    ((pattern
                                                                    (FunApp
                                                                    (StanLib normal_lpdf
                                                                    (FnLpdf true) SoA)
                                                                    (((pattern (Var x))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable))))
                                                                    ((pattern (Var lane))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable))))
                                                                    ((pattern
                                                                    (Promotion
                                                                    ((pattern
                                                                    (Lit Int 1))
                                                                    (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly))))
                                                                    UReal DataOnly))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly)))))))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable))))))
                                                                    (meta <opaque>)))))
                                                                   (meta <opaque>)))))
                                                               (meta <opaque>)))))
                                                           (meta <opaque>))
                                                          (((pattern
                                                             (Block
                                                              (((pattern
                                                                 (For 
                                                                  (loopvar n)
                                                                  (lower
                                                                   ((pattern (Lit Int 1))
                                                                    (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly)))))
                                                                  (upper
                                                                   ((pattern (Var N))
                                                                    (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly)))))
                                                                  (body
                                                                   ((pattern
                                                                    (Block
                                                                    (((pattern
                                                                    (Decl
                                                                    (decl_adtype
                                                                    AutoDiffable)
                                                                    (decl_id twice_x)
                                                                    (decl_type
                                                                    (Sized SReal))
                                                                    (initialize Uninit)))
                                                                    (meta <opaque>))
                                                                    ((pattern
                                                                    (Assignment
                                                                    ((LVariable twice_x)
                                                                    ()) UReal
                                                                    ((pattern
                                                                    (FunApp
                                                                    (StanLib Times__
                                                                    FnPlain SoA)
                                                                    (((pattern
                                                                    (Promotion
                                                                    ((pattern
                                                                    (Lit Int 2))
                                                                    (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly))))
                                                                    UReal DataOnly))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly))))
                                                                    ((pattern (Var x))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable)))))))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable))))))
                                                                    (meta <opaque>))
                                                                    ((pattern
                                                                    (TargetPE
                                                                    ((pattern
                                                                    (FunApp
                                                                    (StanLib normal_lpdf
                                                                    (FnLpdf true) SoA)
                                                                    (((pattern
                                                                    (Var twice_x))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable))))
                                                                    ((pattern
                                                                    (Promotion
                                                                    ((pattern
                                                                    (Lit Int 0))
                                                                    (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly))))
                                                                    UReal DataOnly))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly))))
                                                                    ((pattern
                                                                    (Promotion
                                                                    ((pattern
                                                                    (Lit Int 1))
                                                                    (meta
                                                                    ((type_ UInt)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly))))
                                                                    UReal DataOnly))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly)))))))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable))))))
                                                                    (meta <opaque>))
                                                                    ((pattern
                                                                    (TargetPE
                                                                    ((pattern
                                                                    (FunApp
                                                                    (StanLib Times__
                                                                    FnPlain SoA)
                                                                    (((pattern
                                                                    (Lit Real 0.25))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel DataOnly))))
                                                                    ((pattern (Var x))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable)))))))
                                                                    (meta
                                                                    ((type_ UReal)
                                                                    (loc <opaque>)
                                                                    (adlevel
                                                                    AutoDiffable))))))
                                                                    (meta <opaque>)))))
                                                                    (meta <opaque>)))))
                                                                (meta <opaque>)))))
                                                            (meta <opaque>)))))
                                                        (meta <opaque>)))))
                                                    (meta <opaque>)))))
                                                (meta <opaque>)))))
                                            (meta <opaque>)))))
                                        (meta <opaque>)))))
                                    (meta <opaque>)))))
                                (meta <opaque>)))))
                            (meta <opaque>)))))
                        (meta <opaque>)))))
                    (meta <opaque>)))))
                (meta <opaque>)))))
            (meta <opaque>)))))
        (meta <opaque>)))))
    (meta <opaque>))))
 (generate_quantities
  (((pattern
     (Decl (decl_adtype DataOnly) (decl_id x) (decl_type (Sized SReal))
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
         ((pattern (Var x)) (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
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
     (Decl (decl_adtype AutoDiffable) (decl_id x) (decl_type (Sized SReal))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable x) ()) UReal
      ((pattern
        (Indexed
         ((pattern
           (FunApp (CompilerInternal FnReadData)
            (((pattern (Lit Str x))
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
         ((pattern (Var x)) (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))))
 (unconstrain_array
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id x) (decl_type (Sized SReal))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable x) ()) UReal
      ((pattern (FunApp (CompilerInternal FnReadDeserializer) ()))
       (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt (Identity))
        (var
         ((pattern (Var x)) (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))))
 (output_vars
  ((x <opaque>
    ((out_unconstrained_st SReal) (out_constrained_st SReal) (out_block Parameters)
     (out_trans Identity)))))
 (prog_name invariant_target_loop_model)
 (prog_path tests/fixtures/invariant_target_loop.stan))

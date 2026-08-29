((functions_block ())
 (input_vars
  ((N <opaque> SInt)
   (y <opaque>
    (SVector AoS
     ((pattern (Var N)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
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
     (NRFunApp (CompilerInternal FnValidateSize)
      (((pattern (Lit Str y)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str N)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Var N)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id y)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var N)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id y_flat__)
          (decl_type (Unsized (UArray UReal))) (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable y_flat__) ()) (UArray UReal)
          ((pattern
            (FunApp (CompilerInternal FnReadData)
             (((pattern (Lit Str y))
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
           ((pattern (Var N)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (Assignment
                  ((LVariable y)
                   ((Single
                     ((pattern (Var sym1__))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  UVector
                  ((pattern
                    (Indexed
                     ((pattern (Var y_flat__))
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
     (Decl (decl_adtype AutoDiffable) (decl_id mu)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam
             (constrain
              (LowerUpper
               ((pattern
                 (FunApp (StanLib Minus__ FnPlain AoS)
                  (((pattern
                     (FunApp (StanLib mean FnPlain AoS)
                      (((pattern (Var y))
                        (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                   ((pattern
                     (FunApp (StanLib Times__ FnPlain AoS)
                      (((pattern
                         (Promotion
                          ((pattern (Lit Int 3))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                          UReal DataOnly))
                        (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                       ((pattern
                         (FunApp (StanLib sd FnPlain AoS)
                          (((pattern (Var y))
                            (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
                        (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
               ((pattern
                 (FunApp (StanLib Plus__ FnPlain AoS)
                  (((pattern
                     (FunApp (StanLib mean FnPlain AoS)
                      (((pattern (Var y))
                        (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                   ((pattern
                     (FunApp (StanLib Times__ FnPlain AoS)
                      (((pattern
                         (Promotion
                          ((pattern (Lit Int 3))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                          UReal DataOnly))
                        (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                       ((pattern
                         (FunApp (StanLib sd FnPlain AoS)
                          (((pattern (Var y))
                            (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
                        (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))))
             (dims
              (((pattern (Lit Int 2))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib normal_lpdf (FnLpdf false) AoS)
             (((pattern
                (Indexed
                 ((pattern (Var y))
                  (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly))))
                 ((Single
                   ((pattern (Lit Int 1))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
              ((pattern
                (Indexed
                 ((pattern (Var mu))
                  (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))
                 ((Single
                   ((pattern (Lit Int 1))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern
                (Promotion
                 ((pattern (Lit Int 1))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal DataOnly))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib log_sum_exp FnPlain AoS)
             (((pattern
                (FunApp (StanLib normal_lpdf (FnLpdf false) AoS)
                 (((pattern
                    (Indexed
                     ((pattern (Var y))
                      (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly))))
                     ((Single
                       ((pattern (Lit Int 2))
                        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern
                    (Indexed
                     ((pattern (Var mu))
                      (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))
                     ((Single
                       ((pattern (Lit Int 2))
                        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                  ((pattern
                    (Promotion
                     ((pattern (Lit Int 1))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     UReal DataOnly))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern (FunApp (StanLib negative_infinity FnPlain AoS) ()))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib student_t_lccdf FnPlain AoS)
             (((pattern
                (Promotion
                 ((pattern (Lit Int 0))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal DataOnly))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
              ((pattern
                (Promotion
                 ((pattern (Lit Int 3))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal DataOnly))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
              ((pattern
                (Promotion
                 ((pattern (Lit Int 0))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal DataOnly))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
              ((pattern
                (Promotion
                 ((pattern (Lit Int 10))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal DataOnly))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))))
        (meta <opaque>)))))
    (meta <opaque>))))
 (reverse_mode_log_prob
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id mu)
      (decl_type
       (Sized
        (SVector SoA
         ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam
             (constrain
              (LowerUpper
               ((pattern
                 (FunApp (StanLib Minus__ FnPlain SoA)
                  (((pattern
                     (FunApp (StanLib mean FnPlain AoS)
                      (((pattern (Var y))
                        (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                   ((pattern
                     (FunApp (StanLib Times__ FnPlain SoA)
                      (((pattern
                         (Promotion
                          ((pattern (Lit Int 3))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                          UReal DataOnly))
                        (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                       ((pattern
                         (FunApp (StanLib sd FnPlain SoA)
                          (((pattern (Var y))
                            (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
                        (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
               ((pattern
                 (FunApp (StanLib Plus__ FnPlain SoA)
                  (((pattern
                     (FunApp (StanLib mean FnPlain AoS)
                      (((pattern (Var y))
                        (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                   ((pattern
                     (FunApp (StanLib Times__ FnPlain SoA)
                      (((pattern
                         (Promotion
                          ((pattern (Lit Int 3))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                          UReal DataOnly))
                        (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                       ((pattern
                         (FunApp (StanLib sd FnPlain SoA)
                          (((pattern (Var y))
                            (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
                        (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))))
             (dims
              (((pattern (Lit Int 2))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern SoA)))
           ()))
         (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib normal_lpdf (FnLpdf false) SoA)
             (((pattern
                (Indexed
                 ((pattern (Var y))
                  (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly))))
                 ((Single
                   ((pattern (Lit Int 1))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
              ((pattern
                (Indexed
                 ((pattern (Var mu))
                  (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))
                 ((Single
                   ((pattern (Lit Int 1))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern
                (Promotion
                 ((pattern (Lit Int 1))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal DataOnly))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib log_sum_exp FnPlain AoS)
             (((pattern
                (FunApp (StanLib normal_lpdf (FnLpdf false) AoS)
                 (((pattern
                    (Indexed
                     ((pattern (Var y))
                      (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly))))
                     ((Single
                       ((pattern (Lit Int 2))
                        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern
                    (Indexed
                     ((pattern (Var mu))
                      (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))
                     ((Single
                       ((pattern (Lit Int 2))
                        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                  ((pattern
                    (Promotion
                     ((pattern (Lit Int 1))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     UReal DataOnly))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern (FunApp (StanLib negative_infinity FnPlain AoS) ()))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib student_t_lccdf FnPlain SoA)
             (((pattern
                (Promotion
                 ((pattern (Lit Int 0))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal DataOnly))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
              ((pattern
                (Promotion
                 ((pattern (Lit Int 3))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal DataOnly))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
              ((pattern
                (Promotion
                 ((pattern (Lit Int 0))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal DataOnly))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
              ((pattern
                (Promotion
                 ((pattern (Lit Int 10))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal DataOnly))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))))
        (meta <opaque>)))))
    (meta <opaque>))))
 (generate_quantities
  (((pattern
     (Decl (decl_adtype DataOnly) (decl_id mu)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam
             (constrain
              (LowerUpper
               ((pattern
                 (FunApp (StanLib Minus__ FnPlain AoS)
                  (((pattern
                     (FunApp (StanLib mean FnPlain AoS)
                      (((pattern (Var y))
                        (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                   ((pattern
                     (FunApp (StanLib Times__ FnPlain AoS)
                      (((pattern
                         (Promotion
                          ((pattern (Lit Int 3))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                          UReal DataOnly))
                        (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                       ((pattern
                         (FunApp (StanLib sd FnPlain AoS)
                          (((pattern (Var y))
                            (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
                        (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
               ((pattern
                 (FunApp (StanLib Plus__ FnPlain AoS)
                  (((pattern
                     (FunApp (StanLib mean FnPlain AoS)
                      (((pattern (Var y))
                        (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                   ((pattern
                     (FunApp (StanLib Times__ FnPlain AoS)
                      (((pattern
                         (Promotion
                          ((pattern (Lit Int 3))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                          UReal DataOnly))
                        (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                       ((pattern
                         (FunApp (StanLib sd FnPlain AoS)
                          (((pattern (Var y))
                            (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
                        (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))))
             (dims
              (((pattern (Lit Int 2))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt ())
        (var
         ((pattern (Var mu)) (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
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
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable pos__) ()) UInt
      ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id mu)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id mu_flat__)
          (decl_type (Unsized (UArray UReal))) (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable mu_flat__) ()) (UArray UReal)
          ((pattern
            (FunApp (CompilerInternal FnReadData)
             (((pattern (Lit Str mu))
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
           ((pattern (Lit Int 2))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (Assignment
                  ((LVariable mu)
                   ((Single
                     ((pattern (Var sym1__))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  UVector
                  ((pattern
                    (Indexed
                     ((pattern (Var mu_flat__))
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
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam
        (unconstrain_opt
         ((LowerUpper
           ((pattern
             (FunApp (StanLib Minus__ FnPlain AoS)
              (((pattern
                 (FunApp (StanLib mean FnPlain AoS)
                  (((pattern (Var y))
                    (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
               ((pattern
                 (FunApp (StanLib Times__ FnPlain AoS)
                  (((pattern
                     (Promotion
                      ((pattern (Lit Int 3))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                      UReal DataOnly))
                    (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                   ((pattern
                     (FunApp (StanLib sd FnPlain AoS)
                      (((pattern (Var y))
                        (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
            (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
           ((pattern
             (FunApp (StanLib fma FnPlain AoS)
              (((pattern
                 (Promotion
                  ((pattern (Lit Int 3))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  UReal DataOnly))
                (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
               ((pattern
                 (FunApp (StanLib sd FnPlain AoS)
                  (((pattern (Var y))
                    (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
               ((pattern
                 (FunApp (StanLib mean FnPlain AoS)
                  (((pattern (Var y))
                    (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
            (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
        (var
         ((pattern (Var mu)) (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))))
 (unconstrain_array
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id mu)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable mu) ()) UVector
      ((pattern
        (FunApp (CompilerInternal FnReadDeserializer)
         (((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
       (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam
        (unconstrain_opt
         ((LowerUpper
           ((pattern
             (FunApp (StanLib Minus__ FnPlain AoS)
              (((pattern
                 (FunApp (StanLib mean FnPlain AoS)
                  (((pattern (Var y))
                    (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
               ((pattern
                 (FunApp (StanLib Times__ FnPlain AoS)
                  (((pattern
                     (Promotion
                      ((pattern (Lit Int 3))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                      UReal DataOnly))
                    (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                   ((pattern
                     (FunApp (StanLib sd FnPlain AoS)
                      (((pattern (Var y))
                        (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
            (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
           ((pattern
             (FunApp (StanLib fma FnPlain AoS)
              (((pattern
                 (Promotion
                  ((pattern (Lit Int 3))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  UReal DataOnly))
                (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
               ((pattern
                 (FunApp (StanLib sd FnPlain AoS)
                  (((pattern (Var y))
                    (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
               ((pattern
                 (FunApp (StanLib mean FnPlain AoS)
                  (((pattern (Var y))
                    (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
            (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
        (var
         ((pattern (Var mu)) (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))))
 (output_vars
  ((mu <opaque>
    ((out_unconstrained_st
      (SVector AoS
       ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_constrained_st
      (SVector AoS
       ((pattern (Lit Int 2)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_block Parameters)
     (out_trans
      (LowerUpper
       ((pattern
         (FunApp (StanLib Minus__ FnPlain AoS)
          (((pattern
             (FunApp (StanLib mean FnPlain AoS)
              (((pattern (Var y))
                (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
            (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
           ((pattern
             (FunApp (StanLib Times__ FnPlain AoS)
              (((pattern
                 (Promotion
                  ((pattern (Lit Int 3))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  UReal DataOnly))
                (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
               ((pattern
                 (FunApp (StanLib sd FnPlain AoS)
                  (((pattern (Var y))
                    (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
            (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
        (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
       ((pattern
         (FunApp (StanLib fma FnPlain AoS)
          (((pattern
             (Promotion
              ((pattern (Lit Int 3))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
              UReal DataOnly))
            (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
           ((pattern
             (FunApp (StanLib sd FnPlain AoS)
              (((pattern (Var y))
                (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
            (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
           ((pattern
             (FunApp (StanLib mean FnPlain AoS)
              (((pattern (Var y))
                (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
            (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
        (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))))))))
 (prog_name dfold_model) (prog_path tests/fixtures/dfold.stan))

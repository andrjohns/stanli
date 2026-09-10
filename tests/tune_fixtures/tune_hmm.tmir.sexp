((functions_block ())
 (input_vars
  ((T <opaque> SInt) (K <opaque> SInt)
   (y <opaque>
    (SArray SReal
     ((pattern (Var T)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
 (prepare_data
  (((pattern
     (Decl (decl_adtype DataOnly) (decl_id T) (decl_type (Sized SInt))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable T) ()) UInt
      ((pattern
        (Indexed
         ((pattern
           (FunApp (CompilerInternal FnReadData)
            (((pattern (Lit Str T))
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
          ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
        (var_name T)
        (var ((pattern (Var T)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
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
          ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
        (var_name K)
        (var ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp (CompilerInternal FnValidateSize)
      (((pattern (Lit Str y)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str T)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Var T)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id y)
      (decl_type
       (Sized
        (SArray SReal
         ((pattern (Var T)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable y) ()) (UArray UReal)
      ((pattern
        (FunApp (CompilerInternal FnReadData)
         (((pattern (Lit Str y))
           (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel DataOnly)))))))
       (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel DataOnly))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp (CompilerInternal FnValidateSizePositive)
      (((pattern (Lit Str pi1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp (CompilerInternal FnValidateSize)
      (((pattern (Lit Str A)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp (CompilerInternal FnValidateSizePositive)
      (((pattern (Lit Str A)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp (CompilerInternal FnValidateSize)
      (((pattern (Lit Str mu)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp (CompilerInternal FnValidateSize)
      (((pattern (Lit Str sigma))
        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp (CompilerInternal FnValidateSize)
      (((pattern (Lit Str logalpha))
        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str T)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Var T)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp (CompilerInternal FnValidateSize)
      (((pattern (Lit Str logalpha))
        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))))
 (log_prob
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id pi1)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Simplex)
             (dims
              (((pattern (Var K))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id A)
      (decl_type
       (Sized
        (SArray
         (SVector AoS
          ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Simplex)
             (dims
              (((pattern (Var K))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Var K))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ (UArray UVector)) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id mu)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Ordered)
             (dims
              (((pattern (Var K))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id sigma)
      (decl_type
       (Sized
        (SArray SReal
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam
             (constrain
              (Lower
               ((pattern (Lit Int 0))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (dims
              (((pattern (Var K))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id logalpha)
      (decl_type
       (Sized
        (SArray
         (SVector AoS
          ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
         ((pattern (Var T)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (NRFunApp (CompilerInternal FnValidateSize)
          (((pattern (Lit Str accumulator))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
           ((pattern (Lit Str K))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
           ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
        (meta <opaque>))
       ((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id accumulator)
          (decl_type
           (Sized
            (SArray SReal
             ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
          (initialize Default)))
        (meta <opaque>))
       ((pattern
         (Assignment
          ((LVariable logalpha)
           ((Single
             ((pattern (Lit Int 1))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
          (UArray UVector)
          ((pattern
            (FunApp (StanLib Plus__ FnPlain AoS)
             (((pattern
                (FunApp (StanLib log FnPlain AoS)
                 (((pattern (Var pi1))
                   (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable)))))))
               (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern
                (FunApp (StanLib normal_lpdf (FnLpdf false) AoS)
                 (((pattern
                    (Indexed
                     ((pattern (Var y))
                      (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel DataOnly))))
                     ((Single
                       ((pattern (Lit Int 1))
                        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern (Var mu))
                   (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))
                  ((pattern (Var sigma))
                   (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel AutoDiffable)))))))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>))
       ((pattern
         (For (loopvar t)
          (lower
           ((pattern (Lit Int 2))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (upper
           ((pattern (Var T)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (For (loopvar j)
                  (lower
                   ((pattern (Lit Int 1))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (upper
                   ((pattern (Var K))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (body
                   ((pattern
                     (Block
                      (((pattern
                         (For (loopvar i)
                          (lower
                           ((pattern (Lit Int 1))
                            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                          (upper
                           ((pattern (Var K))
                            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                          (body
                           ((pattern
                             (Block
                              (((pattern
                                 (Assignment
                                  ((LVariable accumulator)
                                   ((Single
                                     ((pattern (Var i))
                                      (meta
                                       ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                                  (UArray UReal)
                                  ((pattern
                                    (FunApp (StanLib Plus__ FnPlain AoS)
                                     (((pattern
                                        (FunApp (StanLib Plus__ FnPlain AoS)
                                         (((pattern
                                            (Indexed
                                             ((pattern (Var logalpha))
                                              (meta
                                               ((type_ (UArray UVector)) 
                                                (loc <opaque>) (adlevel AutoDiffable))))
                                             ((Single
                                               ((pattern
                                                 (FunApp (StanLib Minus__ FnPlain AoS)
                                                  (((pattern (Var t))
                                                    (meta
                                                     ((type_ UInt) (loc <opaque>)
                                                      (adlevel DataOnly))))
                                                   ((pattern (Lit Int 1))
                                                    (meta
                                                     ((type_ UInt) (loc <opaque>)
                                                      (adlevel DataOnly)))))))
                                                (meta
                                                 ((type_ UInt) (loc <opaque>)
                                                  (adlevel DataOnly)))))
                                              (Single
                                               ((pattern (Var i))
                                                (meta
                                                 ((type_ UInt) (loc <opaque>)
                                                  (adlevel DataOnly))))))))
                                           (meta
                                            ((type_ UReal) (loc <opaque>)
                                             (adlevel AutoDiffable))))
                                          ((pattern
                                            (FunApp (StanLib log FnPlain AoS)
                                             (((pattern
                                                (Indexed
                                                 ((pattern (Var A))
                                                  (meta
                                                   ((type_ (UArray UVector))
                                                    (loc <opaque>)
                                                    (adlevel AutoDiffable))))
                                                 ((Single
                                                   ((pattern (Var i))
                                                    (meta
                                                     ((type_ UInt) (loc <opaque>)
                                                      (adlevel DataOnly)))))
                                                  (Single
                                                   ((pattern (Var j))
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
                                        ((type_ UReal) (loc <opaque>)
                                         (adlevel AutoDiffable))))
                                      ((pattern
                                        (FunApp (StanLib normal_lpdf (FnLpdf false) AoS)
                                         (((pattern
                                            (Indexed
                                             ((pattern (Var y))
                                              (meta
                                               ((type_ (UArray UReal)) 
                                                (loc <opaque>) (adlevel DataOnly))))
                                             ((Single
                                               ((pattern (Var t))
                                                (meta
                                                 ((type_ UInt) (loc <opaque>)
                                                  (adlevel DataOnly))))))))
                                           (meta
                                            ((type_ UReal) (loc <opaque>)
                                             (adlevel DataOnly))))
                                          ((pattern
                                            (Indexed
                                             ((pattern (Var mu))
                                              (meta
                                               ((type_ UVector) (loc <opaque>)
                                                (adlevel AutoDiffable))))
                                             ((Single
                                               ((pattern (Var j))
                                                (meta
                                                 ((type_ UInt) (loc <opaque>)
                                                  (adlevel DataOnly))))))))
                                           (meta
                                            ((type_ UReal) (loc <opaque>)
                                             (adlevel AutoDiffable))))
                                          ((pattern
                                            (Indexed
                                             ((pattern (Var sigma))
                                              (meta
                                               ((type_ (UArray UReal)) 
                                                (loc <opaque>) (adlevel AutoDiffable))))
                                             ((Single
                                               ((pattern (Var j))
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
                                    ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                                (meta <opaque>)))))
                            (meta <opaque>)))))
                        (meta <opaque>))
                       ((pattern
                         (Assignment
                          ((LVariable logalpha)
                           ((Single
                             ((pattern (Var t))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                            (Single
                             ((pattern (Var j))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                          (UArray UVector)
                          ((pattern
                            (FunApp (StanLib log_sum_exp FnPlain AoS)
                             (((pattern (Var accumulator))
                               (meta
                                ((type_ (UArray UReal)) (loc <opaque>)
                                 (adlevel AutoDiffable)))))))
                           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                        (meta <opaque>)))))
                    (meta <opaque>)))))
                (meta <opaque>)))))
            (meta <opaque>)))))
        (meta <opaque>)))))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib log_sum_exp FnPlain AoS)
             (((pattern
                (Indexed
                 ((pattern (Var logalpha))
                  (meta ((type_ (UArray UVector)) (loc <opaque>) (adlevel AutoDiffable))))
                 ((Single
                   ((pattern (Var T))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
               (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>)))))
    (meta <opaque>))))
 (reverse_mode_log_prob
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id pi1)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Simplex)
             (dims
              (((pattern (Var K))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id A)
      (decl_type
       (Sized
        (SArray
         (SVector AoS
          ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Simplex)
             (dims
              (((pattern (Var K))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Var K))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ (UArray UVector)) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id mu)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Ordered)
             (dims
              (((pattern (Var K))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id sigma)
      (decl_type
       (Sized
        (SArray SReal
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam
             (constrain
              (Lower
               ((pattern (Lit Int 0))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (dims
              (((pattern (Var K))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern SoA)))
           ()))
         (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id logalpha)
      (decl_type
       (Sized
        (SArray
         (SVector AoS
          ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
         ((pattern (Var T)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (NRFunApp (CompilerInternal FnValidateSize)
          (((pattern (Lit Str accumulator))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
           ((pattern (Lit Str K))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
           ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
        (meta <opaque>))
       ((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id accumulator)
          (decl_type
           (Sized
            (SArray SReal
             ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
          (initialize Default)))
        (meta <opaque>))
       ((pattern
         (Assignment
          ((LVariable logalpha)
           ((Single
             ((pattern (Lit Int 1))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
          (UArray UVector)
          ((pattern
            (FunApp (StanLib Plus__ FnPlain AoS)
             (((pattern
                (FunApp (StanLib log FnPlain AoS)
                 (((pattern (Var pi1))
                   (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable)))))))
               (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern
                (FunApp (StanLib normal_lpdf (FnLpdf false) AoS)
                 (((pattern
                    (Indexed
                     ((pattern (Var y))
                      (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel DataOnly))))
                     ((Single
                       ((pattern (Lit Int 1))
                        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern (Var mu))
                   (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))
                  ((pattern (Var sigma))
                   (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel AutoDiffable)))))))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>))
       ((pattern
         (For (loopvar t)
          (lower
           ((pattern (Lit Int 2))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (upper
           ((pattern (Var T)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (For (loopvar j)
                  (lower
                   ((pattern (Lit Int 1))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (upper
                   ((pattern (Var K))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (body
                   ((pattern
                     (Block
                      (((pattern
                         (For (loopvar i)
                          (lower
                           ((pattern (Lit Int 1))
                            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                          (upper
                           ((pattern (Var K))
                            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                          (body
                           ((pattern
                             (Block
                              (((pattern
                                 (Assignment
                                  ((LVariable accumulator)
                                   ((Single
                                     ((pattern (Var i))
                                      (meta
                                       ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                                  (UArray UReal)
                                  ((pattern
                                    (FunApp (StanLib Plus__ FnPlain SoA)
                                     (((pattern
                                        (FunApp (StanLib Plus__ FnPlain SoA)
                                         (((pattern
                                            (Indexed
                                             ((pattern (Var logalpha))
                                              (meta
                                               ((type_ (UArray UVector)) 
                                                (loc <opaque>) (adlevel AutoDiffable))))
                                             ((Single
                                               ((pattern
                                                 (FunApp (StanLib Minus__ FnPlain SoA)
                                                  (((pattern (Var t))
                                                    (meta
                                                     ((type_ UInt) (loc <opaque>)
                                                      (adlevel DataOnly))))
                                                   ((pattern (Lit Int 1))
                                                    (meta
                                                     ((type_ UInt) (loc <opaque>)
                                                      (adlevel DataOnly)))))))
                                                (meta
                                                 ((type_ UInt) (loc <opaque>)
                                                  (adlevel DataOnly)))))
                                              (Single
                                               ((pattern (Var i))
                                                (meta
                                                 ((type_ UInt) (loc <opaque>)
                                                  (adlevel DataOnly))))))))
                                           (meta
                                            ((type_ UReal) (loc <opaque>)
                                             (adlevel AutoDiffable))))
                                          ((pattern
                                            (FunApp (StanLib log FnPlain SoA)
                                             (((pattern
                                                (Indexed
                                                 ((pattern (Var A))
                                                  (meta
                                                   ((type_ (UArray UVector))
                                                    (loc <opaque>)
                                                    (adlevel AutoDiffable))))
                                                 ((Single
                                                   ((pattern (Var i))
                                                    (meta
                                                     ((type_ UInt) (loc <opaque>)
                                                      (adlevel DataOnly)))))
                                                  (Single
                                                   ((pattern (Var j))
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
                                        ((type_ UReal) (loc <opaque>)
                                         (adlevel AutoDiffable))))
                                      ((pattern
                                        (FunApp (StanLib normal_lpdf (FnLpdf false) SoA)
                                         (((pattern
                                            (Indexed
                                             ((pattern (Var y))
                                              (meta
                                               ((type_ (UArray UReal)) 
                                                (loc <opaque>) (adlevel DataOnly))))
                                             ((Single
                                               ((pattern (Var t))
                                                (meta
                                                 ((type_ UInt) (loc <opaque>)
                                                  (adlevel DataOnly))))))))
                                           (meta
                                            ((type_ UReal) (loc <opaque>)
                                             (adlevel DataOnly))))
                                          ((pattern
                                            (Indexed
                                             ((pattern (Var mu))
                                              (meta
                                               ((type_ UVector) (loc <opaque>)
                                                (adlevel AutoDiffable))))
                                             ((Single
                                               ((pattern (Var j))
                                                (meta
                                                 ((type_ UInt) (loc <opaque>)
                                                  (adlevel DataOnly))))))))
                                           (meta
                                            ((type_ UReal) (loc <opaque>)
                                             (adlevel AutoDiffable))))
                                          ((pattern
                                            (Indexed
                                             ((pattern (Var sigma))
                                              (meta
                                               ((type_ (UArray UReal)) 
                                                (loc <opaque>) (adlevel AutoDiffable))))
                                             ((Single
                                               ((pattern (Var j))
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
                                    ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                                (meta <opaque>)))))
                            (meta <opaque>)))))
                        (meta <opaque>))
                       ((pattern
                         (Assignment
                          ((LVariable logalpha)
                           ((Single
                             ((pattern (Var t))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                            (Single
                             ((pattern (Var j))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                          (UArray UVector)
                          ((pattern
                            (FunApp (StanLib log_sum_exp FnPlain AoS)
                             (((pattern (Var accumulator))
                               (meta
                                ((type_ (UArray UReal)) (loc <opaque>)
                                 (adlevel AutoDiffable)))))))
                           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                        (meta <opaque>)))))
                    (meta <opaque>)))))
                (meta <opaque>)))))
            (meta <opaque>)))))
        (meta <opaque>)))))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib log_sum_exp FnPlain AoS)
             (((pattern
                (Indexed
                 ((pattern (Var logalpha))
                  (meta ((type_ (UArray UVector)) (loc <opaque>) (adlevel AutoDiffable))))
                 ((Single
                   ((pattern (Var T))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
               (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>)))))
    (meta <opaque>))))
 (generate_quantities
  (((pattern
     (Decl (decl_adtype DataOnly) (decl_id pi1)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Simplex)
             (dims
              (((pattern (Var K))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id A)
      (decl_type
       (Sized
        (SArray
         (SVector AoS
          ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Simplex)
             (dims
              (((pattern (Var K))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Var K))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ (UArray UVector)) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id mu)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Ordered)
             (dims
              (((pattern (Var K))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id sigma)
      (decl_type
       (Sized
        (SArray SReal
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam
             (constrain
              (Lower
               ((pattern (Lit Int 0))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (dims
              (((pattern (Var K))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id logalpha)
      (decl_type
       (Sized
        (SArray
         (SVector AoS
          ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
         ((pattern (Var T)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt ())
        (var
         ((pattern (Var pi1)) (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))
   ((pattern
     (For (loopvar sym1__)
      (lower
       ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
      (upper ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
      (body
       ((pattern
         (Block
          (((pattern
             (For (loopvar sym2__)
              (lower
               ((pattern (Lit Int 1))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
              (upper
               ((pattern (Var K))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
              (body
               ((pattern
                 (Block
                  (((pattern
                     (NRFunApp
                      (CompilerInternal
                       (FnWriteParam (unconstrain_opt ())
                        (var
                         ((pattern
                           (Indexed
                            ((pattern (Var A))
                             (meta
                              ((type_ (UArray UVector)) (loc <opaque>)
                               (adlevel DataOnly))))
                            ((Single
                              ((pattern (Var sym2__))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                             (Single
                              ((pattern (Var sym1__))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                          (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                      ()))
                    (meta <opaque>)))))
                (meta <opaque>)))))
            (meta <opaque>)))))
        (meta <opaque>)))))
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
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt ())
        (var
         ((pattern (Var sigma))
          (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel DataOnly)))))))
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
     (Block
      (((pattern
         (NRFunApp (CompilerInternal FnValidateSize)
          (((pattern (Lit Str accumulator))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
           ((pattern (Lit Str K))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
           ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
        (meta <opaque>))
       ((pattern
         (Decl (decl_adtype DataOnly) (decl_id accumulator)
          (decl_type
           (Sized
            (SArray SReal
             ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
          (initialize Default)))
        (meta <opaque>))
       ((pattern
         (Assignment
          ((LVariable logalpha)
           ((Single
             ((pattern (Lit Int 1))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
          (UArray UVector)
          ((pattern
            (FunApp (StanLib Plus__ FnPlain AoS)
             (((pattern
                (FunApp (StanLib log FnPlain AoS)
                 (((pattern (Var pi1))
                   (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable)))))))
               (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern
                (FunApp (StanLib normal_lpdf (FnLpdf false) AoS)
                 (((pattern
                    (Indexed
                     ((pattern (Var y))
                      (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel DataOnly))))
                     ((Single
                       ((pattern (Lit Int 1))
                        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern (Var mu))
                   (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))
                  ((pattern (Var sigma))
                   (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel AutoDiffable)))))))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>))
       ((pattern
         (For (loopvar t)
          (lower
           ((pattern (Lit Int 2))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (upper
           ((pattern (Var T)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (For (loopvar j)
                  (lower
                   ((pattern (Lit Int 1))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (upper
                   ((pattern (Var K))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (body
                   ((pattern
                     (Block
                      (((pattern
                         (For (loopvar i)
                          (lower
                           ((pattern (Lit Int 1))
                            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                          (upper
                           ((pattern (Var K))
                            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                          (body
                           ((pattern
                             (Block
                              (((pattern
                                 (Assignment
                                  ((LVariable accumulator)
                                   ((Single
                                     ((pattern (Var i))
                                      (meta
                                       ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                                  (UArray UReal)
                                  ((pattern
                                    (FunApp (StanLib Plus__ FnPlain AoS)
                                     (((pattern
                                        (FunApp (StanLib Plus__ FnPlain AoS)
                                         (((pattern
                                            (Indexed
                                             ((pattern (Var logalpha))
                                              (meta
                                               ((type_ (UArray UVector)) 
                                                (loc <opaque>) (adlevel AutoDiffable))))
                                             ((Single
                                               ((pattern
                                                 (FunApp (StanLib Minus__ FnPlain AoS)
                                                  (((pattern (Var t))
                                                    (meta
                                                     ((type_ UInt) (loc <opaque>)
                                                      (adlevel DataOnly))))
                                                   ((pattern (Lit Int 1))
                                                    (meta
                                                     ((type_ UInt) (loc <opaque>)
                                                      (adlevel DataOnly)))))))
                                                (meta
                                                 ((type_ UInt) (loc <opaque>)
                                                  (adlevel DataOnly)))))
                                              (Single
                                               ((pattern (Var i))
                                                (meta
                                                 ((type_ UInt) (loc <opaque>)
                                                  (adlevel DataOnly))))))))
                                           (meta
                                            ((type_ UReal) (loc <opaque>)
                                             (adlevel AutoDiffable))))
                                          ((pattern
                                            (FunApp (StanLib log FnPlain AoS)
                                             (((pattern
                                                (Indexed
                                                 ((pattern (Var A))
                                                  (meta
                                                   ((type_ (UArray UVector))
                                                    (loc <opaque>)
                                                    (adlevel AutoDiffable))))
                                                 ((Single
                                                   ((pattern (Var i))
                                                    (meta
                                                     ((type_ UInt) (loc <opaque>)
                                                      (adlevel DataOnly)))))
                                                  (Single
                                                   ((pattern (Var j))
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
                                        ((type_ UReal) (loc <opaque>)
                                         (adlevel AutoDiffable))))
                                      ((pattern
                                        (FunApp (StanLib normal_lpdf (FnLpdf false) AoS)
                                         (((pattern
                                            (Indexed
                                             ((pattern (Var y))
                                              (meta
                                               ((type_ (UArray UReal)) 
                                                (loc <opaque>) (adlevel DataOnly))))
                                             ((Single
                                               ((pattern (Var t))
                                                (meta
                                                 ((type_ UInt) (loc <opaque>)
                                                  (adlevel DataOnly))))))))
                                           (meta
                                            ((type_ UReal) (loc <opaque>)
                                             (adlevel DataOnly))))
                                          ((pattern
                                            (Indexed
                                             ((pattern (Var mu))
                                              (meta
                                               ((type_ UVector) (loc <opaque>)
                                                (adlevel AutoDiffable))))
                                             ((Single
                                               ((pattern (Var j))
                                                (meta
                                                 ((type_ UInt) (loc <opaque>)
                                                  (adlevel DataOnly))))))))
                                           (meta
                                            ((type_ UReal) (loc <opaque>)
                                             (adlevel AutoDiffable))))
                                          ((pattern
                                            (Indexed
                                             ((pattern (Var sigma))
                                              (meta
                                               ((type_ (UArray UReal)) 
                                                (loc <opaque>) (adlevel AutoDiffable))))
                                             ((Single
                                               ((pattern (Var j))
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
                                    ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                                (meta <opaque>)))))
                            (meta <opaque>)))))
                        (meta <opaque>))
                       ((pattern
                         (Assignment
                          ((LVariable logalpha)
                           ((Single
                             ((pattern (Var t))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                            (Single
                             ((pattern (Var j))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                          (UArray UVector)
                          ((pattern
                            (FunApp (StanLib log_sum_exp FnPlain AoS)
                             (((pattern (Var accumulator))
                               (meta
                                ((type_ (UArray UReal)) (loc <opaque>)
                                 (adlevel AutoDiffable)))))))
                           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                        (meta <opaque>)))))
                    (meta <opaque>)))))
                (meta <opaque>)))))
            (meta <opaque>)))))
        (meta <opaque>)))))
    (meta <opaque>))
   ((pattern
     (IfElse
      ((pattern (Var emit_transformed_parameters__))
       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
      ((pattern
        (Block
         (((pattern
            (For (loopvar sym1__)
             (lower
              ((pattern (Lit Int 1))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
             (upper
              ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
             (body
              ((pattern
                (Block
                 (((pattern
                    (For (loopvar sym2__)
                     (lower
                      ((pattern (Lit Int 1))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                     (upper
                      ((pattern (Var T))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                     (body
                      ((pattern
                        (Block
                         (((pattern
                            (NRFunApp
                             (CompilerInternal
                              (FnWriteParam (unconstrain_opt ())
                               (var
                                ((pattern
                                  (Indexed
                                   ((pattern (Var logalpha))
                                    (meta
                                     ((type_ (UArray UVector)) (loc <opaque>)
                                      (adlevel DataOnly))))
                                   ((Single
                                     ((pattern (Var sym2__))
                                      (meta
                                       ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                                    (Single
                                     ((pattern (Var sym1__))
                                      (meta
                                       ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                                 (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
                             ()))
                           (meta <opaque>)))))
                       (meta <opaque>)))))
                   (meta <opaque>)))))
               (meta <opaque>)))))
           (meta <opaque>)))))
       (meta <opaque>))
      ()))
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
     (Decl (decl_adtype AutoDiffable) (decl_id pi1)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id pi1_flat__)
          (decl_type (Unsized (UArray UReal))) (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable pi1_flat__) ()) (UArray UReal)
          ((pattern
            (FunApp (CompilerInternal FnReadData)
             (((pattern (Lit Str pi1))
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
           ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (Assignment
                  ((LVariable pi1)
                   ((Single
                     ((pattern (Var sym1__))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  UVector
                  ((pattern
                    (Indexed
                     ((pattern (Var pi1_flat__))
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
       (FnWriteParam (unconstrain_opt (Simplex))
        (var
         ((pattern (Var pi1)) (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id A)
      (decl_type
       (Sized
        (SArray
         (SVector AoS
          ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
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
               (meta ((type_ (UArray UVector)) (loc <opaque>) (adlevel DataOnly)))))))
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
           ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (For (loopvar sym2__)
                  (lower
                   ((pattern (Lit Int 1))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (upper
                   ((pattern (Var K))
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
                          (UArray UVector)
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
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt (Simplex))
        (var
         ((pattern (Var A))
          (meta ((type_ (UArray UVector)) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id mu)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
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
           ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
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
       (FnWriteParam (unconstrain_opt (Ordered))
        (var
         ((pattern (Var mu)) (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id sigma)
      (decl_type
       (Sized
        (SArray SReal
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable sigma) ()) (UArray UReal)
      ((pattern
        (FunApp (CompilerInternal FnReadData)
         (((pattern (Lit Str sigma))
           (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel DataOnly)))))))
       (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel DataOnly))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam
        (unconstrain_opt
         ((Lower
           ((pattern (Lit Int 0))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
        (var
         ((pattern (Var sigma))
          (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))))
 (unconstrain_array
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id pi1)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable pi1) ()) UVector
      ((pattern
        (FunApp (CompilerInternal FnReadDeserializer)
         (((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
       (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt (Simplex))
        (var
         ((pattern (Var pi1)) (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id A)
      (decl_type
       (Sized
        (SArray
         (SVector AoS
          ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (For (loopvar sym1__)
      (lower
       ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
      (upper ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
      (body
       ((pattern
         (Block
          (((pattern
             (For (loopvar sym2__)
              (lower
               ((pattern (Lit Int 1))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
              (upper
               ((pattern (Var K))
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
                      UVector
                      ((pattern (FunApp (CompilerInternal FnReadDeserializer) ()))
                       (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                    (meta <opaque>)))))
                (meta <opaque>)))))
            (meta <opaque>)))))
        (meta <opaque>)))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt (Simplex))
        (var
         ((pattern (Var A))
          (meta ((type_ (UArray UVector)) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id mu)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable mu) ()) UVector
      ((pattern
        (FunApp (CompilerInternal FnReadDeserializer)
         (((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
       (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt (Ordered))
        (var
         ((pattern (Var mu)) (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id sigma)
      (decl_type
       (Sized
        (SArray SReal
         ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable sigma) ()) (UArray UReal)
      ((pattern
        (FunApp (CompilerInternal FnReadDeserializer)
         (((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
       (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel AutoDiffable))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam
        (unconstrain_opt
         ((Lower
           ((pattern (Lit Int 0))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
        (var
         ((pattern (Var sigma))
          (meta ((type_ (UArray UReal)) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))))
 (output_vars
  ((pi1 <opaque>
    ((out_unconstrained_st
      (SVector AoS
       ((pattern
         (FunApp (StanLib Minus__ FnPlain AoS)
          (((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
           ((pattern (Lit Int 1))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_constrained_st
      (SVector AoS
       ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_block Parameters) (out_trans Simplex)))
   (A <opaque>
    ((out_unconstrained_st
      (SArray
       (SVector AoS
        ((pattern
          (FunApp (StanLib Minus__ FnPlain AoS)
           (((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
            ((pattern (Lit Int 1))
             (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
       ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_constrained_st
      (SArray
       (SVector AoS
        ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
       ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_block Parameters) (out_trans Simplex)))
   (mu <opaque>
    ((out_unconstrained_st
      (SVector AoS
       ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_constrained_st
      (SVector AoS
       ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_block Parameters) (out_trans Ordered)))
   (sigma <opaque>
    ((out_unconstrained_st
      (SArray SReal
       ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_constrained_st
      (SArray SReal
       ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_block Parameters)
     (out_trans
      (Lower
       ((pattern (Lit Int 0)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
   (logalpha <opaque>
    ((out_unconstrained_st
      (SArray
       (SVector AoS
        ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
       ((pattern (Var T)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_constrained_st
      (SArray
       (SVector AoS
        ((pattern (Var K)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
       ((pattern (Var T)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_block TransformedParameters) (out_trans Identity)))))
 (prog_name tune_hmm_model) (prog_path tests/tune_fixtures/tune_hmm.stan))

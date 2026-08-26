((functions_block ())
 (input_vars
  ((N <opaque> SInt) (S <opaque> SInt)
   (idx <opaque>
    (SArray
     (SArray SInt
      ((pattern (Var S)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
     ((pattern (Var N)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
   (n <opaque>
    (SArray SInt
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
     (Decl (decl_adtype DataOnly) (decl_id S) (decl_type (Sized SInt))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable S) ()) UInt
      ((pattern
        (Indexed
         ((pattern
           (FunApp (CompilerInternal FnReadData)
            (((pattern (Lit Str S))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
          (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
         ((Single
           ((pattern (Lit Int 1))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp (CompilerInternal FnValidateSize)
      (((pattern (Lit Str idx)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str N)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Var N)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp (CompilerInternal FnValidateSize)
      (((pattern (Lit Str idx)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str S)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Var S)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id idx)
      (decl_type
       (Sized
        (SArray
         (SArray SInt
          ((pattern (Var S)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
         ((pattern (Var N)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id idx_flat__)
          (decl_type (Unsized (UArray UInt))) (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable idx_flat__) ()) (UArray UInt)
          ((pattern
            (FunApp (CompilerInternal FnReadData)
             (((pattern (Lit Str idx))
               (meta ((type_ (UArray (UArray UInt))) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))
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
           ((pattern (Var S)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (For (loopvar sym2__)
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
                         (Assignment
                          ((LVariable idx)
                           ((Single
                             ((pattern (Var sym2__))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                            (Single
                             ((pattern (Var sym1__))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                          (UArray (UArray UInt))
                          ((pattern
                            (Indexed
                             ((pattern (Var idx_flat__))
                              (meta
                               ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
                             ((Single
                               ((pattern (Var pos__))
                                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
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
     (NRFunApp (CompilerInternal FnValidateSize)
      (((pattern (Lit Str n)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str N)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Var N)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id n)
      (decl_type
       (Sized
        (SArray SInt
         ((pattern (Var N)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable n) ()) (UArray UInt)
      ((pattern
        (FunApp (CompilerInternal FnReadData)
         (((pattern (Lit Str n))
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
         (Decl (decl_adtype AutoDiffable) (decl_id acc) (decl_type (Sized SReal))
          (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable acc) ()) UReal
          ((pattern
            (Promotion
             ((pattern (Lit Int 0))
              (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
             UReal AutoDiffable))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>))
       ((pattern
         (For (loopvar i)
          (lower
           ((pattern (Lit Int 1))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (upper
           ((pattern (Var N)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (NRFunApp (CompilerInternal FnValidateSize)
                  (((pattern (Lit Str row))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                   ((pattern (Lit Str n[i]))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                   ((pattern
                     (Indexed
                      ((pattern (Var n))
                       (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
                      ((Single
                        ((pattern (Var i))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                (meta <opaque>))
               ((pattern
                 (Decl (decl_adtype AutoDiffable) (decl_id row)
                  (decl_type
                   (Sized
                    (SArray SInt
                     ((pattern
                       (Indexed
                        ((pattern (Var n))
                         (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
                        ((Single
                          ((pattern (Var i))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (initialize Default)))
                (meta <opaque>))
               ((pattern
                 (Assignment ((LVariable row) ()) (UArray UInt)
                  ((pattern
                    (Indexed
                     ((pattern (Var idx))
                      (meta
                       ((type_ (UArray (UArray UInt))) (loc <opaque>) (adlevel DataOnly))))
                     ((Single
                       ((pattern (Var i))
                        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                      (Between
                       ((pattern (Lit Int 1))
                        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                       ((pattern
                         (Indexed
                          ((pattern (Var n))
                           (meta
                            ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
                          ((Single
                            ((pattern (Var i))
                             (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                   (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))
                (meta <opaque>))
               ((pattern
                 (For (loopvar k)
                  (lower
                   ((pattern (Lit Int 1))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (upper
                   ((pattern
                     (Indexed
                      ((pattern (Var n))
                       (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
                      ((Single
                        ((pattern (Var i))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (body
                   ((pattern
                     (Block
                      (((pattern
                         (Assignment ((LVariable acc) ()) UReal
                          ((pattern
                            (FunApp (StanLib fma FnPlain AoS)
                             (((pattern
                                (Promotion
                                 ((pattern
                                   (Indexed
                                    ((pattern (Var row))
                                     (meta
                                      ((type_ (UArray UInt)) (loc <opaque>)
                                       (adlevel DataOnly))))
                                    ((Single
                                      ((pattern (Var k))
                                       (meta
                                        ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern (Var theta))
                               (meta
                                ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                              ((pattern (Var acc))
                               (meta
                                ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                        (meta <opaque>)))))
                    (meta <opaque>)))))
                (meta <opaque>)))))
            (meta <opaque>)))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib std_normal_lpdf (FnLpdf true) AoS)
             (((pattern (Var theta))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern (Var acc))
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
         (Decl (decl_adtype AutoDiffable) (decl_id acc) (decl_type (Sized SReal))
          (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable acc) ()) UReal
          ((pattern
            (Promotion
             ((pattern (Lit Int 0))
              (meta ((type_ UInt) (loc <opaque>) (adlevel AutoDiffable))))
             UReal AutoDiffable))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>))
       ((pattern
         (For (loopvar i)
          (lower
           ((pattern (Lit Int 1))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (upper
           ((pattern (Var N)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (NRFunApp (CompilerInternal FnValidateSize)
                  (((pattern (Lit Str row))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                   ((pattern (Lit Str n[i]))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                   ((pattern
                     (Indexed
                      ((pattern (Var n))
                       (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
                      ((Single
                        ((pattern (Var i))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                (meta <opaque>))
               ((pattern
                 (Decl (decl_adtype AutoDiffable) (decl_id row)
                  (decl_type
                   (Sized
                    (SArray SInt
                     ((pattern
                       (Indexed
                        ((pattern (Var n))
                         (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
                        ((Single
                          ((pattern (Var i))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (initialize Default)))
                (meta <opaque>))
               ((pattern
                 (Assignment ((LVariable row) ()) (UArray UInt)
                  ((pattern
                    (Indexed
                     ((pattern (Var idx))
                      (meta
                       ((type_ (UArray (UArray UInt))) (loc <opaque>) (adlevel DataOnly))))
                     ((Single
                       ((pattern (Var i))
                        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                      (Between
                       ((pattern (Lit Int 1))
                        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                       ((pattern
                         (Indexed
                          ((pattern (Var n))
                           (meta
                            ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
                          ((Single
                            ((pattern (Var i))
                             (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                   (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))
                (meta <opaque>))
               ((pattern
                 (For (loopvar k)
                  (lower
                   ((pattern (Lit Int 1))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (upper
                   ((pattern
                     (Indexed
                      ((pattern (Var n))
                       (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
                      ((Single
                        ((pattern (Var i))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (body
                   ((pattern
                     (Block
                      (((pattern
                         (Assignment ((LVariable acc) ()) UReal
                          ((pattern
                            (FunApp (StanLib fma FnPlain SoA)
                             (((pattern
                                (Promotion
                                 ((pattern
                                   (Indexed
                                    ((pattern (Var row))
                                     (meta
                                      ((type_ (UArray UInt)) (loc <opaque>)
                                       (adlevel DataOnly))))
                                    ((Single
                                      ((pattern (Var k))
                                       (meta
                                        ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                 UReal DataOnly))
                               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern (Var theta))
                               (meta
                                ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                              ((pattern (Var acc))
                               (meta
                                ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                        (meta <opaque>)))))
                    (meta <opaque>)))))
                (meta <opaque>)))))
            (meta <opaque>)))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib std_normal_lpdf (FnLpdf true) SoA)
             (((pattern (Var theta))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern (Var acc))
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
 (prog_name arr2d_rowrange_model) (prog_path tests/fixtures/arr2d_rowrange.stan))
((functions_block ()) (input_vars ((N <opaque> SInt)))
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
         (Decl (decl_adtype AutoDiffable) (decl_id i) (decl_type (Sized SInt))
          (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable i) ()) UInt
          ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
        (meta <opaque>))
       ((pattern
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
         (While
          ((pattern
            (FunApp (StanLib Leq__ FnPlain AoS)
             (((pattern (Var i)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
              ((pattern (Var N)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
          ((pattern
            (Block
             (((pattern
                (Assignment ((LVariable acc) ()) UReal
                 ((pattern
                   (FunApp (StanLib fma FnPlain AoS)
                    (((pattern
                       (Promotion
                        ((pattern (Var i))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                        UReal DataOnly))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Var theta))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                     ((pattern (Var acc))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                  (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
               (meta <opaque>))
              ((pattern
                (Assignment ((LVariable i) ()) UInt
                 ((pattern
                   (FunApp (StanLib Plus__ FnPlain AoS)
                    (((pattern (Var i))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Lit Int 1))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
               (meta <opaque>)))))
           (meta <opaque>))))
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
         (Decl (decl_adtype AutoDiffable) (decl_id i) (decl_type (Sized SInt))
          (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable i) ()) UInt
          ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
        (meta <opaque>))
       ((pattern
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
         (While
          ((pattern
            (FunApp (StanLib Leq__ FnPlain SoA)
             (((pattern (Var i)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
              ((pattern (Var N)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
          ((pattern
            (Block
             (((pattern
                (Assignment ((LVariable acc) ()) UReal
                 ((pattern
                   (FunApp (StanLib fma FnPlain SoA)
                    (((pattern
                       (Promotion
                        ((pattern (Var i))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                        UReal DataOnly))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Var theta))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                     ((pattern (Var acc))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                  (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
               (meta <opaque>))
              ((pattern
                (Assignment ((LVariable i) ()) UInt
                 ((pattern
                   (FunApp (StanLib Plus__ FnPlain SoA)
                    (((pattern (Var i))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Lit Int 1))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
               (meta <opaque>)))))
           (meta <opaque>))))
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
 (prog_name whileloop_model) (prog_path tests/fixtures/whileloop.stan))
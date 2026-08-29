((functions_block ()) (input_vars ((n <opaque> SInt)))
 (prepare_data
  (((pattern
     (Decl (decl_adtype DataOnly) (decl_id n) (decl_type (Sized SInt))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable n) ()) UInt
      ((pattern
        (Indexed
         ((pattern
           (FunApp (CompilerInternal FnReadData)
            (((pattern (Lit Str n))
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
                (NRFunApp (CompilerInternal FnValidateSize)
                 (((pattern (Lit Str repeated))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern (Lit Str n))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern (Var n))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
               (meta <opaque>))
              ((pattern
                (Decl (decl_adtype AutoDiffable) (decl_id repeated)
                 (decl_type
                  (Sized
                   (SVector AoS
                    ((pattern (Var n))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                 (initialize Uninit)))
               (meta <opaque>))
              ((pattern
                (Assignment ((LVariable repeated) ()) UVector
                 ((pattern
                   (FunApp (StanLib rep_vector FnPlain AoS)
                    (((pattern (Var theta))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                     ((pattern (Var n))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))
               (meta <opaque>))
              ((pattern
                (NRFunApp (CompilerInternal FnValidateSize)
                 (((pattern (Lit Str constant))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern (Lit Str n))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern (Var n))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
               (meta <opaque>))
              ((pattern
                (Decl (decl_adtype AutoDiffable) (decl_id constant)
                 (decl_type
                  (Sized
                   (SVector AoS
                    ((pattern (Var n))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                 (initialize Uninit)))
               (meta <opaque>))
              ((pattern
                (Assignment ((LVariable constant) ()) UVector
                 ((pattern
                   (FunApp (StanLib rep_vector FnPlain AoS)
                    (((pattern (Lit Real 0.5))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Var n))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))
               (meta <opaque>))
              ((pattern
                (TargetPE
                 ((pattern
                   (FunApp (StanLib Plus__ FnPlain AoS)
                    (((pattern
                       (FunApp (StanLib sum FnPlain AoS)
                        (((pattern (Var repeated))
                          (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable)))))))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                     ((pattern
                       (FunApp (StanLib sum FnPlain AoS)
                        (((pattern (Var constant))
                          (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable)))))))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                  (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
               (meta <opaque>)))))
           (meta <opaque>))
          (((pattern
             (Block
              (((pattern
                 (TargetPE
                  ((pattern (Var theta))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                (meta <opaque>)))))
            (meta <opaque>)))))
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
                (NRFunApp (CompilerInternal FnValidateSize)
                 (((pattern (Lit Str repeated))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern (Lit Str n))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern (Var n))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
               (meta <opaque>))
              ((pattern
                (Decl (decl_adtype AutoDiffable) (decl_id repeated)
                 (decl_type
                  (Sized
                   (SVector SoA
                    ((pattern (Var n))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                 (initialize Uninit)))
               (meta <opaque>))
              ((pattern
                (Assignment ((LVariable repeated) ()) UVector
                 ((pattern
                   (FunApp (StanLib rep_vector FnPlain SoA)
                    (((pattern (Var theta))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                     ((pattern (Var n))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))
               (meta <opaque>))
              ((pattern
                (NRFunApp (CompilerInternal FnValidateSize)
                 (((pattern (Lit Str constant))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern (Lit Str n))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                  ((pattern (Var n))
                   (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
               (meta <opaque>))
              ((pattern
                (Decl (decl_adtype AutoDiffable) (decl_id constant)
                 (decl_type
                  (Sized
                   (SVector SoA
                    ((pattern (Var n))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                 (initialize Uninit)))
               (meta <opaque>))
              ((pattern
                (Assignment ((LVariable constant) ()) UVector
                 ((pattern
                   (FunApp (StanLib rep_vector FnPlain SoA)
                    (((pattern (Lit Real 0.5))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Var n))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))
               (meta <opaque>))
              ((pattern
                (TargetPE
                 ((pattern
                   (FunApp (StanLib Plus__ FnPlain SoA)
                    (((pattern
                       (FunApp (StanLib sum FnPlain SoA)
                        (((pattern (Var repeated))
                          (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable)))))))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
                     ((pattern
                       (FunApp (StanLib sum FnPlain SoA)
                        (((pattern (Var constant))
                          (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable)))))))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
                  (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
               (meta <opaque>)))))
           (meta <opaque>))
          (((pattern
             (Block
              (((pattern
                 (TargetPE
                  ((pattern (Var theta))
                   (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
                (meta <opaque>)))))
            (meta <opaque>)))))
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
 (prog_name paramcond_repvector_model)
 (prog_path tests/fixtures/paramcond_repvector.stan))

((functions_block ()) (input_vars ((k <opaque> SInt)))
 (prepare_data
  (((pattern
     (Decl (decl_adtype DataOnly) (decl_id k) (decl_type (Sized SInt))
      (initialize Uninit)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable k) ()) UInt
      ((pattern
        (Indexed
         ((pattern
           (FunApp (CompilerInternal FnReadData)
            (((pattern (Lit Str k))
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
                (Decl (decl_adtype AutoDiffable) (decl_id hits) (decl_type (Sized SInt))
                 (initialize Uninit)))
               (meta <opaque>))
              ((pattern
                (Assignment ((LVariable hits) ()) UInt
                 ((pattern (Lit Int 0))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
               (meta <opaque>))
              ((pattern
                (Decl (decl_adtype AutoDiffable) (decl_id i) (decl_type (Sized SInt))
                 (initialize Uninit)))
               (meta <opaque>))
              ((pattern
                (Assignment ((LVariable i) ()) UInt
                 ((pattern (Lit Int 1))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
               (meta <opaque>))
              ((pattern
                (While
                 ((pattern
                   (EAnd
                    ((pattern
                      (FunApp (StanLib Leq__ FnPlain AoS)
                       (((pattern (Var i))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                        ((pattern (Lit Int 4))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                    ((pattern
                      (FunApp (StanLib Less__ FnPlain AoS)
                       (((pattern (Var hits))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                        ((pattern (Lit Int 3))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 ((pattern
                   (Block
                    (((pattern
                       (Block
                        (((pattern
                           (Assignment ((LVariable hits) ()) UInt
                            ((pattern
                              (FunApp (StanLib Plus__ FnPlain AoS)
                               (((pattern (Var hits))
                                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                ((pattern
                                  (FunApp (StanLib NEquals__ FnPlain AoS)
                                   (((pattern (Var i))
                                     (meta
                                      ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                    ((pattern (Var k))
                                     (meta
                                      ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                             (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
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
                      (meta <opaque>)))))
                  (meta <opaque>))))
               (meta <opaque>))
              ((pattern
                (Decl (decl_adtype AutoDiffable) (decl_id any) (decl_type (Sized SInt))
                 (initialize Uninit)))
               (meta <opaque>))
              ((pattern
                (Assignment ((LVariable any) ()) UInt
                 ((pattern
                   (FunApp (StanLib PNot__ FnPlain AoS)
                    (((pattern
                       (FunApp (StanLib Equals__ FnPlain AoS)
                        (((pattern (Var hits))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern (Lit Int 0))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
               (meta <opaque>))
              ((pattern
                (Decl (decl_adtype AutoDiffable) (decl_id either)
                 (decl_type (Sized SInt)) (initialize Uninit)))
               (meta <opaque>))
              ((pattern
                (Assignment ((LVariable either) ()) UInt
                 ((pattern
                   (EOr
                    ((pattern
                      (FunApp (StanLib Greater__ FnPlain AoS)
                       (((pattern (Var hits))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                        ((pattern (Lit Int 2))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                    ((pattern
                      (FunApp (StanLib Equals__ FnPlain AoS)
                       (((pattern (Var hits))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                        ((pattern (Lit Int 0))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
               (meta <opaque>))
              ((pattern
                (Decl (decl_adtype AutoDiffable) (decl_id ge) (decl_type (Sized SInt))
                 (initialize Uninit)))
               (meta <opaque>))
              ((pattern
                (Assignment ((LVariable ge) ()) UInt
                 ((pattern
                   (FunApp (StanLib Geq__ FnPlain AoS)
                    (((pattern (Var hits))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Lit Int 3))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
               (meta <opaque>))
              ((pattern
                (Decl (decl_adtype AutoDiffable) (decl_id lt) (decl_type (Sized SInt))
                 (initialize Uninit)))
               (meta <opaque>))
              ((pattern
                (Assignment ((LVariable lt) ()) UInt
                 ((pattern
                   (FunApp (StanLib Less__ FnPlain AoS)
                    (((pattern (Var hits))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Lit Int 3))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
               (meta <opaque>))
              ((pattern
                (TargetPE
                 ((pattern
                   (FunApp (StanLib Times__ FnPlain AoS)
                    (((pattern
                       (Promotion
                        ((pattern
                          (FunApp (StanLib Plus__ FnPlain AoS)
                           (((pattern
                              (FunApp (StanLib Plus__ FnPlain AoS)
                               (((pattern
                                  (FunApp (StanLib Plus__ FnPlain AoS)
                                   (((pattern
                                      (FunApp (StanLib Plus__ FnPlain AoS)
                                       (((pattern (Var hits))
                                         (meta
                                          ((type_ UInt) (loc <opaque>)
                                           (adlevel DataOnly))))
                                        ((pattern (Var any))
                                         (meta
                                          ((type_ UInt) (loc <opaque>)
                                           (adlevel DataOnly)))))))
                                     (meta
                                      ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                    ((pattern (Var either))
                                     (meta
                                      ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                ((pattern (Var ge))
                                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                             (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                            ((pattern (Var lt))
                             (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                        UReal DataOnly))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Var theta))
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
                (Decl (decl_adtype AutoDiffable) (decl_id hits) (decl_type (Sized SInt))
                 (initialize Uninit)))
               (meta <opaque>))
              ((pattern
                (Assignment ((LVariable hits) ()) UInt
                 ((pattern (Lit Int 0))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
               (meta <opaque>))
              ((pattern
                (Decl (decl_adtype AutoDiffable) (decl_id i) (decl_type (Sized SInt))
                 (initialize Uninit)))
               (meta <opaque>))
              ((pattern
                (Assignment ((LVariable i) ()) UInt
                 ((pattern (Lit Int 1))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
               (meta <opaque>))
              ((pattern
                (While
                 ((pattern
                   (EAnd
                    ((pattern
                      (FunApp (StanLib Leq__ FnPlain SoA)
                       (((pattern (Var i))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                        ((pattern (Lit Int 4))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                    ((pattern
                      (FunApp (StanLib Less__ FnPlain SoA)
                       (((pattern (Var hits))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                        ((pattern (Lit Int 3))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 ((pattern
                   (Block
                    (((pattern
                       (Block
                        (((pattern
                           (Assignment ((LVariable hits) ()) UInt
                            ((pattern
                              (FunApp (StanLib Plus__ FnPlain SoA)
                               (((pattern (Var hits))
                                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                ((pattern
                                  (FunApp (StanLib NEquals__ FnPlain SoA)
                                   (((pattern (Var i))
                                     (meta
                                      ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                    ((pattern (Var k))
                                     (meta
                                      ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                             (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
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
                      (meta <opaque>)))))
                  (meta <opaque>))))
               (meta <opaque>))
              ((pattern
                (Decl (decl_adtype AutoDiffable) (decl_id any) (decl_type (Sized SInt))
                 (initialize Uninit)))
               (meta <opaque>))
              ((pattern
                (Assignment ((LVariable any) ()) UInt
                 ((pattern
                   (FunApp (StanLib PNot__ FnPlain SoA)
                    (((pattern
                       (FunApp (StanLib Equals__ FnPlain SoA)
                        (((pattern (Var hits))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern (Lit Int 0))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
               (meta <opaque>))
              ((pattern
                (Decl (decl_adtype AutoDiffable) (decl_id either)
                 (decl_type (Sized SInt)) (initialize Uninit)))
               (meta <opaque>))
              ((pattern
                (Assignment ((LVariable either) ()) UInt
                 ((pattern
                   (EOr
                    ((pattern
                      (FunApp (StanLib Greater__ FnPlain SoA)
                       (((pattern (Var hits))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                        ((pattern (Lit Int 2))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                    ((pattern
                      (FunApp (StanLib Equals__ FnPlain SoA)
                       (((pattern (Var hits))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                        ((pattern (Lit Int 0))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                     (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
               (meta <opaque>))
              ((pattern
                (Decl (decl_adtype AutoDiffable) (decl_id ge) (decl_type (Sized SInt))
                 (initialize Uninit)))
               (meta <opaque>))
              ((pattern
                (Assignment ((LVariable ge) ()) UInt
                 ((pattern
                   (FunApp (StanLib Geq__ FnPlain SoA)
                    (((pattern (Var hits))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Lit Int 3))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
               (meta <opaque>))
              ((pattern
                (Decl (decl_adtype AutoDiffable) (decl_id lt) (decl_type (Sized SInt))
                 (initialize Uninit)))
               (meta <opaque>))
              ((pattern
                (Assignment ((LVariable lt) ()) UInt
                 ((pattern
                   (FunApp (StanLib Less__ FnPlain SoA)
                    (((pattern (Var hits))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Lit Int 3))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
               (meta <opaque>))
              ((pattern
                (TargetPE
                 ((pattern
                   (FunApp (StanLib Times__ FnPlain SoA)
                    (((pattern
                       (Promotion
                        ((pattern
                          (FunApp (StanLib Plus__ FnPlain SoA)
                           (((pattern
                              (FunApp (StanLib Plus__ FnPlain SoA)
                               (((pattern
                                  (FunApp (StanLib Plus__ FnPlain SoA)
                                   (((pattern
                                      (FunApp (StanLib Plus__ FnPlain SoA)
                                       (((pattern (Var hits))
                                         (meta
                                          ((type_ UInt) (loc <opaque>)
                                           (adlevel DataOnly))))
                                        ((pattern (Var any))
                                         (meta
                                          ((type_ UInt) (loc <opaque>)
                                           (adlevel DataOnly)))))))
                                     (meta
                                      ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                    ((pattern (Var either))
                                     (meta
                                      ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                ((pattern (Var ge))
                                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                             (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                            ((pattern (Var lt))
                             (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                        UReal DataOnly))
                      (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Var theta))
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
 (prog_name paramcond_intops_model) (prog_path tests/fixtures/paramcond_intops.stan))

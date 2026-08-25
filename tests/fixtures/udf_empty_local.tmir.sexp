((functions_block
  (((fdrt (ReturnType (UArray UInt))) (fdname vecequals) (fdsuffix FnPlain)
    (fdargs ((AutoDiffable a (UArray UInt)) (AutoDiffable test UInt)))
    (fdbody
     (((pattern
        (Block
         (((pattern
            (NRFunApp (CompilerInternal FnValidateSize)
             (((pattern (Lit Str check))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
              ((pattern (Lit Str "size(a)"))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
              ((pattern
                (FunApp (StanLib size FnPlain AoS)
                 (((pattern (Var a))
                   (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
           (meta <opaque>))
          ((pattern
            (Decl (decl_adtype AutoDiffable) (decl_id check)
             (decl_type
              (Sized
               (SArray SInt
                ((pattern
                  (FunApp (StanLib size FnPlain AoS)
                   (((pattern (Var a))
                     (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
             (initialize Default)))
           (meta <opaque>))
          ((pattern
            (For (loopvar i)
             (lower
              ((pattern (Lit Int 1))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
             (upper
              ((pattern
                (FunApp (StanLib size FnPlain AoS)
                 (((pattern (Var check))
                   (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
             (body
              ((pattern
                (Block
                 (((pattern
                    (Assignment
                     ((LVariable check)
                      ((Single
                        ((pattern (Var i))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                     (UArray UInt)
                     ((pattern
                       (FunApp (StanLib Equals__ FnPlain AoS)
                        (((pattern
                           (Indexed
                            ((pattern (Var a))
                             (meta
                              ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
                            ((Single
                              ((pattern (Var i))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern (Var test))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                   (meta <opaque>)))))
               (meta <opaque>)))))
           (meta <opaque>))
          ((pattern
            (Return
             (((pattern (Var check))
               (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
           (meta <opaque>)))))
       (meta <opaque>))))
    (fdloc <opaque>))
   ((fdrt (ReturnType (UArray UInt))) (fdname whichequals) (fdsuffix FnPlain)
    (fdargs ((AutoDiffable b (UArray UInt)) (AutoDiffable test UInt)))
    (fdbody
     (((pattern
        (Block
         (((pattern
            (NRFunApp (CompilerInternal FnValidateSize)
             (((pattern (Lit Str check))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
              ((pattern (Lit Str "size(b)"))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
              ((pattern
                (FunApp (StanLib size FnPlain AoS)
                 (((pattern (Var b))
                   (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
           (meta <opaque>))
          ((pattern
            (Decl (decl_adtype AutoDiffable) (decl_id check)
             (decl_type
              (Sized
               (SArray SInt
                ((pattern
                  (FunApp (StanLib size FnPlain AoS)
                   (((pattern (Var b))
                     (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
             (initialize Default)))
           (meta <opaque>))
          ((pattern
            (Assignment ((LVariable check) ()) (UArray UInt)
             ((pattern
               (FunApp (UserDefined vecequals FnPlain)
                (((pattern (Var b))
                  (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
                 ((pattern (Var test))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
              (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))
           (meta <opaque>))
          ((pattern
            (NRFunApp (CompilerInternal FnValidateSize)
             (((pattern (Lit Str which))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
              ((pattern (Lit Str "sum(check)"))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
              ((pattern
                (FunApp (StanLib sum FnPlain AoS)
                 (((pattern (Var check))
                   (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
           (meta <opaque>))
          ((pattern
            (Decl (decl_adtype AutoDiffable) (decl_id which)
             (decl_type
              (Sized
               (SArray SInt
                ((pattern
                  (FunApp (StanLib sum FnPlain AoS)
                   (((pattern (Var check))
                     (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
             (initialize Default)))
           (meta <opaque>))
          ((pattern
            (Decl (decl_adtype AutoDiffable) (decl_id counter) (decl_type (Sized SInt))
             (initialize Default)))
           (meta <opaque>))
          ((pattern
            (Assignment ((LVariable counter) ()) UInt
             ((pattern (Lit Int 1))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
           (meta <opaque>))
          ((pattern
            (For (loopvar i)
             (lower
              ((pattern (Lit Int 1))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
             (upper
              ((pattern
                (FunApp (StanLib size FnPlain AoS)
                 (((pattern (Var b))
                   (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
             (body
              ((pattern
                (Block
                 (((pattern
                    (IfElse
                     ((pattern
                       (FunApp (StanLib Equals__ FnPlain AoS)
                        (((pattern
                           (Indexed
                            ((pattern (Var check))
                             (meta
                              ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
                            ((Single
                              ((pattern (Var i))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                         ((pattern (Lit Int 1))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern
                       (Block
                        (((pattern
                           (Assignment
                            ((LVariable which)
                             ((Single
                               ((pattern (Var counter))
                                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                            (UArray UInt)
                            ((pattern (Var i))
                             (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                          (meta <opaque>))
                         ((pattern
                           (Assignment ((LVariable counter) ()) UInt
                            ((pattern
                              (FunApp (StanLib Plus__ FnPlain AoS)
                               (((pattern (Var counter))
                                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                                ((pattern (Lit Int 1))
                                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                             (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                          (meta <opaque>)))))
                      (meta <opaque>))
                     ()))
                   (meta <opaque>)))))
               (meta <opaque>)))))
           (meta <opaque>))
          ((pattern
            (Return
             (((pattern (Var which))
               (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
           (meta <opaque>)))))
       (meta <opaque>))))
    (fdloc <opaque>))))
 (input_vars
  ((N <opaque> SInt)
   (input <opaque>
    (SArray SInt
     ((pattern (Var N)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
 (prepare_data
  (((pattern
     (Decl (decl_adtype DataOnly) (decl_id N) (decl_type (Sized SInt))
      (initialize Default)))
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
      (((pattern (Lit Str input))
        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str N)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Var N)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id input)
      (decl_type
       (Sized
        (SArray SInt
         ((pattern (Var N)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable input) ()) (UArray UInt)
      ((pattern
        (FunApp (CompilerInternal FnReadData)
         (((pattern (Lit Str input))
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
         (NRFunApp (CompilerInternal FnValidateSize)
          (((pattern (Lit Str selected))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
           ((pattern (Lit Str "size(whichequals(input, 9))"))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
           ((pattern
             (FunApp (StanLib size FnPlain AoS)
              (((pattern
                 (FunApp (UserDefined whichequals FnPlain)
                  (((pattern (Var input))
                    (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
                   ((pattern (Lit Int 9))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
        (meta <opaque>))
       ((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id selected)
          (decl_type
           (Sized
            (SArray SInt
             ((pattern
               (FunApp (StanLib size FnPlain AoS)
                (((pattern
                   (FunApp (UserDefined whichequals FnPlain)
                    (((pattern (Var input))
                      (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Lit Int 9))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
          (initialize Default)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable selected) ()) (UArray UInt)
          ((pattern
            (FunApp (UserDefined whichequals FnPlain)
             (((pattern (Var input))
               (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
              ((pattern (Lit Int 9))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib normal_lpdf (FnLpdf true) AoS)
             (((pattern (Var theta))
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
        (meta <opaque>))
       ((pattern
         (For (loopvar i)
          (lower
           ((pattern (Lit Int 1))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (upper
           ((pattern
             (FunApp (StanLib size FnPlain AoS)
              (((pattern (Var selected))
                (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (TargetPE
                  ((pattern
                    (FunApp (StanLib Times__ FnPlain AoS)
                     (((pattern
                        (Promotion
                         ((pattern
                           (Indexed
                            ((pattern (Var selected))
                             (meta
                              ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
                            ((Single
                              ((pattern (Var i))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                         UReal DataOnly))
                       (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                      ((pattern (Var theta))
                       (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
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
            (FnReadParam (constrain Identity) (dims ()) (mem_pattern AoS)))
           ()))
         (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (NRFunApp (CompilerInternal FnValidateSize)
          (((pattern (Lit Str selected))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
           ((pattern (Lit Str "size(whichequals(input, 9))"))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
           ((pattern
             (FunApp (StanLib size FnPlain AoS)
              (((pattern
                 (FunApp (UserDefined whichequals FnPlain)
                  (((pattern (Var input))
                    (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
                   ((pattern (Lit Int 9))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
        (meta <opaque>))
       ((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id selected)
          (decl_type
           (Sized
            (SArray SInt
             ((pattern
               (FunApp (StanLib size FnPlain AoS)
                (((pattern
                   (FunApp (UserDefined whichequals FnPlain)
                    (((pattern (Var input))
                      (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Lit Int 9))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
          (initialize Default)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable selected) ()) (UArray UInt)
          ((pattern
            (FunApp (UserDefined whichequals FnPlain)
             (((pattern (Var input))
               (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
              ((pattern (Lit Int 9))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib normal_lpdf (FnLpdf true) AoS)
             (((pattern (Var theta))
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
        (meta <opaque>))
       ((pattern
         (For (loopvar i)
          (lower
           ((pattern (Lit Int 1))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (upper
           ((pattern
             (FunApp (StanLib size FnPlain AoS)
              (((pattern (Var selected))
                (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (TargetPE
                  ((pattern
                    (FunApp (StanLib Times__ FnPlain AoS)
                     (((pattern
                        (Promotion
                         ((pattern
                           (Indexed
                            ((pattern (Var selected))
                             (meta
                              ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
                            ((Single
                              ((pattern (Var i))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                          (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                         UReal DataOnly))
                       (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
                      ((pattern (Var theta))
                       (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable)))))))
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
      (initialize Default)))
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
      (initialize Default)))
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
 (prog_name udf_empty_local_model) (prog_path tests/fixtures/udf_empty_local.stan))
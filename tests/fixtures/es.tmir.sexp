((functions_block ())
 (input_vars
  ((J <opaque> SInt)
   (y <opaque>
    (SVector AoS
     ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
   (sigma <opaque>
    (SVector AoS
     ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
 (prepare_data
  (((pattern
     (Decl (decl_adtype DataOnly) (decl_id pos__) (decl_type (Sized SInt))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable pos__) ()) UInt
      ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id J) (decl_type (Sized SInt))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable J) ()) UInt
      ((pattern
        (Indexed
         ((pattern
           (FunApp (CompilerInternal FnReadData)
            (((pattern (Lit Str J))
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
        (var_name J)
        (var ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (((pattern (Lit Int 0)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp (CompilerInternal FnValidateSize)
      (((pattern (Lit Str y)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id y)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
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
           ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
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
    (meta <opaque>))
   ((pattern
     (NRFunApp (CompilerInternal FnValidateSize)
      (((pattern (Lit Str sigma))
        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id sigma)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id sigma_flat__)
          (decl_type (Unsized (UArray UReal))) (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable sigma_flat__) ()) (UArray UReal)
          ((pattern
            (FunApp (CompilerInternal FnReadData)
             (((pattern (Lit Str sigma))
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
           ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (Assignment
                  ((LVariable sigma)
                   ((Single
                     ((pattern (Var sym1__))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  UVector
                  ((pattern
                    (Indexed
                     ((pattern (Var sigma_flat__))
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
       (FnCheck
        (trans
         (Lower
          ((pattern (Lit Int 0)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
        (var_name sigma)
        (var
         ((pattern (Var sigma))
          (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
      (((pattern (Lit Int 0)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp (CompilerInternal FnValidateSize)
      (((pattern (Lit Str theta_tilde))
        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp (CompilerInternal FnValidateSize)
      (((pattern (Lit Str theta))
        (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Lit Str J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
       ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
    (meta <opaque>))))
 (log_prob
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id mu) (decl_type (Sized SReal))
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
     (Decl (decl_adtype AutoDiffable) (decl_id tau) (decl_type (Sized SReal))
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
             (dims ()) (mem_pattern AoS)))
           ()))
         (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id theta_tilde)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Identity)
             (dims
              (((pattern (Var J))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id theta)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable theta) ()) UVector
      ((pattern
        (FunApp (StanLib Plus__ FnPlain AoS)
         (((pattern (Var mu))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
          ((pattern
            (FunApp (StanLib Times__ FnPlain AoS)
             (((pattern (Var tau))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern (Var theta_tilde))
               (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable)))))))
       (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib normal_lpdf (FnLpdf true) AoS)
             (((pattern (Var mu))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern
                (Promotion
                 ((pattern (Lit Int 0))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal DataOnly))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
              ((pattern
                (Promotion
                 ((pattern (Lit Int 5))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal DataOnly))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib cauchy_lpdf (FnLpdf true) AoS)
             (((pattern (Var tau))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern
                (Promotion
                 ((pattern (Lit Int 0))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal DataOnly))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
              ((pattern
                (Promotion
                 ((pattern (Lit Int 5))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal DataOnly))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib normal_lpdf (FnLpdf true) AoS)
             (((pattern (Var theta_tilde))
               (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))
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
         (TargetPE
          ((pattern
            (FunApp (StanLib normal_lpdf (FnLpdf true) AoS)
             (((pattern (Var y))
               (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly))))
              ((pattern (Var theta))
               (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern (Var sigma))
               (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>)))))
    (meta <opaque>))))
 (reverse_mode_log_prob
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id mu) (decl_type (Sized SReal))
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
     (Decl (decl_adtype AutoDiffable) (decl_id tau) (decl_type (Sized SReal))
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
             (dims ()) (mem_pattern AoS)))
           ()))
         (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id theta_tilde)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Identity)
             (dims
              (((pattern (Var J))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id theta)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable theta) ()) UVector
      ((pattern
        (FunApp (StanLib Plus__ FnPlain AoS)
         (((pattern (Var mu))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
          ((pattern
            (FunApp (StanLib Times__ FnPlain AoS)
             (((pattern (Var tau))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern (Var theta_tilde))
               (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable)))))))
       (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib normal_lpdf (FnLpdf true) AoS)
             (((pattern (Var mu))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern
                (Promotion
                 ((pattern (Lit Int 0))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal DataOnly))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
              ((pattern
                (Promotion
                 ((pattern (Lit Int 5))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal DataOnly))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib cauchy_lpdf (FnLpdf true) AoS)
             (((pattern (Var tau))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern
                (Promotion
                 ((pattern (Lit Int 0))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal DataOnly))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly))))
              ((pattern
                (Promotion
                 ((pattern (Lit Int 5))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 UReal DataOnly))
               (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib normal_lpdf (FnLpdf true) AoS)
             (((pattern (Var theta_tilde))
               (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))
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
         (TargetPE
          ((pattern
            (FunApp (StanLib normal_lpdf (FnLpdf true) AoS)
             (((pattern (Var y))
               (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly))))
              ((pattern (Var theta))
               (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern (Var sigma))
               (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
        (meta <opaque>)))))
    (meta <opaque>))))
 (generate_quantities
  (((pattern
     (Decl (decl_adtype DataOnly) (decl_id mu) (decl_type (Sized SReal))
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
     (Decl (decl_adtype DataOnly) (decl_id tau) (decl_type (Sized SReal))
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
             (dims ()) (mem_pattern AoS)))
           ()))
         (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id theta_tilde)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize
       (Assign
        ((pattern
          (FunApp
           (CompilerInternal
            (FnReadParam (constrain Identity)
             (dims
              (((pattern (Var J))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
             (mem_pattern AoS)))
           ()))
         (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype DataOnly) (decl_id theta)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt ())
        (var
         ((pattern (Var mu)) (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt ())
        (var
         ((pattern (Var tau)) (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt ())
        (var
         ((pattern (Var theta_tilde))
          (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
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
     (Assignment ((LVariable theta) ()) UVector
      ((pattern
        (FunApp (StanLib Plus__ FnPlain AoS)
         (((pattern (Var mu))
           (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
          ((pattern
            (FunApp (StanLib Times__ FnPlain AoS)
             (((pattern (Var tau))
               (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))
              ((pattern (Var theta_tilde))
               (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable)))))))
           (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable)))))))
       (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))
    (meta <opaque>))
   ((pattern
     (IfElse
      ((pattern (Var emit_transformed_parameters__))
       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
      ((pattern
        (Block
         (((pattern
            (NRFunApp
             (CompilerInternal
              (FnWriteParam (unconstrain_opt ())
               (var
                ((pattern (Var theta))
                 (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
             ()))
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
     (Assignment ((LVariable pos__) ()) UInt
      ((pattern (Lit Int 1)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id mu) (decl_type (Sized SReal))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable mu) ()) UReal
      ((pattern
        (Indexed
         ((pattern
           (FunApp (CompilerInternal FnReadData)
            (((pattern (Lit Str mu))
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
         ((pattern (Var mu)) (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id tau) (decl_type (Sized SReal))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable tau) ()) UReal
      ((pattern
        (Indexed
         ((pattern
           (FunApp (CompilerInternal FnReadData)
            (((pattern (Lit Str tau))
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
       (FnWriteParam
        (unconstrain_opt
         ((Lower
           ((pattern (Lit Int 0))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
        (var
         ((pattern (Var tau)) (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id theta_tilde)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id theta_tilde_flat__)
          (decl_type (Unsized (UArray UReal))) (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Assignment ((LVariable theta_tilde_flat__) ()) (UArray UReal)
          ((pattern
            (FunApp (CompilerInternal FnReadData)
             (((pattern (Lit Str theta_tilde))
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
           ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (Assignment
                  ((LVariable theta_tilde)
                   ((Single
                     ((pattern (Var sym1__))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  UVector
                  ((pattern
                    (Indexed
                     ((pattern (Var theta_tilde_flat__))
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
       (FnWriteParam (unconstrain_opt (Identity))
        (var
         ((pattern (Var theta_tilde))
          (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))))
 (unconstrain_array
  (((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id mu) (decl_type (Sized SReal))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable mu) ()) UReal
      ((pattern (FunApp (CompilerInternal FnReadDeserializer) ()))
       (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt (Identity))
        (var
         ((pattern (Var mu)) (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id tau) (decl_type (Sized SReal))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable tau) ()) UReal
      ((pattern (FunApp (CompilerInternal FnReadDeserializer) ()))
       (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))
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
         ((pattern (Var tau)) (meta ((type_ UReal) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))
   ((pattern
     (Decl (decl_adtype AutoDiffable) (decl_id theta_tilde)
      (decl_type
       (Sized
        (SVector AoS
         ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
      (initialize Default)))
    (meta <opaque>))
   ((pattern
     (Assignment ((LVariable theta_tilde) ()) UVector
      ((pattern
        (FunApp (CompilerInternal FnReadDeserializer)
         (((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
       (meta ((type_ UVector) (loc <opaque>) (adlevel AutoDiffable))))))
    (meta <opaque>))
   ((pattern
     (NRFunApp
      (CompilerInternal
       (FnWriteParam (unconstrain_opt (Identity))
        (var
         ((pattern (Var theta_tilde))
          (meta ((type_ UVector) (loc <opaque>) (adlevel DataOnly)))))))
      ()))
    (meta <opaque>))))
 (output_vars
  ((mu <opaque>
    ((out_unconstrained_st SReal) (out_constrained_st SReal) (out_block Parameters)
     (out_trans Identity)))
   (tau <opaque>
    ((out_unconstrained_st SReal) (out_constrained_st SReal) (out_block Parameters)
     (out_trans
      (Lower
       ((pattern (Lit Int 0)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
   (theta_tilde <opaque>
    ((out_unconstrained_st
      (SVector AoS
       ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_constrained_st
      (SVector AoS
       ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_block Parameters) (out_trans Identity)))
   (theta <opaque>
    ((out_unconstrained_st
      (SVector AoS
       ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_constrained_st
      (SVector AoS
       ((pattern (Var J)) (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
     (out_block TransformedParameters) (out_trans Identity)))))
 (prog_name es_model) (prog_path tests/fixtures/es.stan))
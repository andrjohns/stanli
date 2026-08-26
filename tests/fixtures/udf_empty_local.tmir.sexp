((functions_block
  (((fdrt (ReturnType (UArray UInt))) (fdname vecequals) (fdsuffix FnPlain)
    (fdargs
     ((AutoDiffable a (UArray UInt)) (AutoDiffable test UInt)
      (AutoDiffable comparison UInt)))
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
                       (TernaryIf
                        ((pattern (Var comparison))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                        ((pattern
                          (FunApp (StanLib Equals__ FnPlain AoS)
                           (((pattern
                              (Indexed
                               ((pattern (Var a))
                                (meta
                                 ((type_ (UArray UInt)) (loc <opaque>)
                                  (adlevel DataOnly))))
                               ((Single
                                 ((pattern (Var i))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                             (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                            ((pattern (Var test))
                             (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                        ((pattern
                          (FunApp (StanLib NEquals__ FnPlain AoS)
                           (((pattern
                              (Indexed
                               ((pattern (Var a))
                                (meta
                                 ((type_ (UArray UInt)) (loc <opaque>)
                                  (adlevel DataOnly))))
                               ((Single
                                 ((pattern (Var i))
                                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                             (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                            ((pattern (Var test))
                             (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                         (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
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
    (fdargs
     ((AutoDiffable b (UArray UInt)) (AutoDiffable test UInt)
      (AutoDiffable comparison UInt)))
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
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                 ((pattern (Var comparison))
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
             (initialize Uninit)))
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
         (Decl (decl_adtype AutoDiffable) (decl_id inline_whichequals_return_sym27__)
          (decl_type
           (Sized
            (SArray SInt
             ((pattern (Lit Int 0))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
          (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Block
          (((pattern
             (NRFunApp (CompilerInternal FnValidateSize)
              (((pattern (Lit Str check))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Str "size(b)"))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern
                 (FunApp (StanLib size FnPlain AoS)
                  (((pattern (Var input))
                    (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
            (meta <opaque>))
           ((pattern
             (Decl (decl_adtype AutoDiffable) (decl_id inline_whichequals_check_sym28__)
              (decl_type
               (Sized
                (SArray SInt
                 ((pattern
                   (FunApp (StanLib size FnPlain AoS)
                    (((pattern (Var input))
                      (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
              (initialize Default)))
            (meta <opaque>))
           ((pattern
             (Decl (decl_adtype AutoDiffable)
              (decl_id inline_whichequals_inline_vecequals_return_sym5___sym29__)
              (decl_type
               (Sized
                (SArray SInt
                 ((pattern (Lit Int 0))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
              (initialize Uninit)))
            (meta <opaque>))
           ((pattern
             (Block
              (((pattern
                 (NRFunApp (CompilerInternal FnValidateSize)
                  (((pattern (Lit Str check))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                   ((pattern (Lit Str "size(a)"))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                   ((pattern
                     (FunApp (StanLib size FnPlain AoS)
                      (((pattern (Var input))
                        (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                (meta <opaque>))
               ((pattern
                 (Decl (decl_adtype AutoDiffable)
                  (decl_id inline_whichequals_inline_vecequals_check_sym6___sym30__)
                  (decl_type
                   (Sized
                    (SArray SInt
                     ((pattern
                       (FunApp (StanLib size FnPlain AoS)
                        (((pattern (Var input))
                          (meta
                           ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (initialize Default)))
                (meta <opaque>))
               ((pattern
                 (For (loopvar inline_whichequals_inline_vecequals_i_sym7___sym31__)
                  (lower
                   ((pattern (Lit Int 1))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (upper
                   ((pattern
                     (FunApp (StanLib size FnPlain AoS)
                      (((pattern
                         (Var inline_whichequals_inline_vecequals_check_sym6___sym30__))
                        (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (body
                   ((pattern
                     (Block
                      (((pattern
                         (Assignment
                          ((LVariable
                            inline_whichequals_inline_vecequals_check_sym6___sym30__)
                           ((Single
                             ((pattern
                               (Var inline_whichequals_inline_vecequals_i_sym7___sym31__))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                          (UArray UInt)
                          ((pattern
                            (FunApp (StanLib Equals__ FnPlain AoS)
                             (((pattern
                                (Indexed
                                 ((pattern (Var input))
                                  (meta
                                   ((type_ (UArray UInt)) (loc <opaque>)
                                    (adlevel DataOnly))))
                                 ((Single
                                   ((pattern
                                     (Var
                                      inline_whichequals_inline_vecequals_i_sym7___sym31__))
                                    (meta
                                     ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern (Lit Int 9))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                        (meta <opaque>)))))
                    (meta <opaque>)))))
                (meta <opaque>))
               ((pattern
                 (Assignment
                  ((LVariable inline_whichequals_inline_vecequals_return_sym5___sym29__)
                   ())
                  (UArray UInt)
                  ((pattern
                    (Var inline_whichequals_inline_vecequals_check_sym6___sym30__))
                   (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))
                (meta <opaque>)))))
            (meta <opaque>))
           ((pattern
             (NRFunApp (CompilerInternal FnValidateSize)
              (((pattern (Lit Str which))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Str "sum(check)"))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern
                 (FunApp (StanLib sum FnPlain AoS)
                  (((pattern
                     (Var inline_whichequals_inline_vecequals_return_sym5___sym29__))
                    (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel AutoDiffable)))))))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
            (meta <opaque>))
           ((pattern
             (Decl (decl_adtype AutoDiffable) (decl_id inline_whichequals_which_sym32__)
              (decl_type
               (Sized
                (SArray SInt
                 ((pattern
                   (FunApp (StanLib sum FnPlain AoS)
                    (((pattern
                       (Var inline_whichequals_inline_vecequals_return_sym5___sym29__))
                      (meta
                       ((type_ (UArray UInt)) (loc <opaque>) (adlevel AutoDiffable)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
              (initialize Default)))
            (meta <opaque>))
           ((pattern
             (Decl (decl_adtype AutoDiffable)
              (decl_id inline_whichequals_counter_sym33__) (decl_type (Sized SInt))
              (initialize Uninit)))
            (meta <opaque>))
           ((pattern
             (Assignment ((LVariable inline_whichequals_counter_sym33__) ()) UInt
              ((pattern (Lit Int 1))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
            (meta <opaque>))
           ((pattern
             (For (loopvar inline_whichequals_i_sym34__)
              (lower
               ((pattern (Lit Int 1))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
              (upper
               ((pattern
                 (FunApp (StanLib size FnPlain AoS)
                  (((pattern (Var input))
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
                             ((pattern
                               (Var
                                inline_whichequals_inline_vecequals_return_sym5___sym29__))
                              (meta
                               ((type_ (UArray UInt)) (loc <opaque>)
                                (adlevel AutoDiffable))))
                             ((Single
                               ((pattern (Var inline_whichequals_i_sym34__))
                                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                          ((pattern (Lit Int 1))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                      ((pattern
                        (Block
                         (((pattern
                            (Assignment
                             ((LVariable inline_whichequals_which_sym32__)
                              ((Single
                                ((pattern (Var inline_whichequals_counter_sym33__))
                                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                             (UArray UInt)
                             ((pattern (Var inline_whichequals_i_sym34__))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                           (meta <opaque>))
                          ((pattern
                            (Assignment
                             ((LVariable inline_whichequals_counter_sym33__) ()) UInt
                             ((pattern
                               (FunApp (StanLib Plus__ FnPlain AoS)
                                (((pattern (Var inline_whichequals_counter_sym33__))
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
             (Assignment ((LVariable inline_whichequals_return_sym27__) ())
              (UArray UInt)
              ((pattern (Var inline_whichequals_which_sym32__))
               (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))
            (meta <opaque>)))))
        (meta <opaque>))
       ((pattern
         (NRFunApp (CompilerInternal FnValidateSize)
          (((pattern (Lit Str selected))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
           ((pattern (Lit Str "size(whichequals(input, 9, 1))"))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
           ((pattern
             (FunApp (StanLib size FnPlain AoS)
              (((pattern (Var inline_whichequals_return_sym27__))
                (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel AutoDiffable)))))))
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
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Lit Int 1))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
          (initialize Default)))
        (meta <opaque>))
       ((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id inline_whichequals_return_sym36__)
          (decl_type
           (Sized
            (SArray SInt
             ((pattern (Lit Int 0))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
          (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Block
          (((pattern
             (NRFunApp (CompilerInternal FnValidateSize)
              (((pattern (Lit Str check))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Str "size(b)"))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern
                 (FunApp (StanLib size FnPlain AoS)
                  (((pattern (Var input))
                    (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
            (meta <opaque>))
           ((pattern
             (Decl (decl_adtype AutoDiffable) (decl_id inline_whichequals_check_sym37__)
              (decl_type
               (Sized
                (SArray SInt
                 ((pattern
                   (FunApp (StanLib size FnPlain AoS)
                    (((pattern (Var input))
                      (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
              (initialize Default)))
            (meta <opaque>))
           ((pattern
             (Decl (decl_adtype AutoDiffable)
              (decl_id inline_whichequals_inline_vecequals_return_sym5___sym38__)
              (decl_type
               (Sized
                (SArray SInt
                 ((pattern (Lit Int 0))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
              (initialize Uninit)))
            (meta <opaque>))
           ((pattern
             (Block
              (((pattern
                 (NRFunApp (CompilerInternal FnValidateSize)
                  (((pattern (Lit Str check))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                   ((pattern (Lit Str "size(a)"))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                   ((pattern
                     (FunApp (StanLib size FnPlain AoS)
                      (((pattern (Var input))
                        (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                (meta <opaque>))
               ((pattern
                 (Decl (decl_adtype AutoDiffable)
                  (decl_id inline_whichequals_inline_vecequals_check_sym6___sym39__)
                  (decl_type
                   (Sized
                    (SArray SInt
                     ((pattern
                       (FunApp (StanLib size FnPlain AoS)
                        (((pattern (Var input))
                          (meta
                           ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (initialize Default)))
                (meta <opaque>))
               ((pattern
                 (For (loopvar inline_whichequals_inline_vecequals_i_sym7___sym40__)
                  (lower
                   ((pattern (Lit Int 1))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (upper
                   ((pattern
                     (FunApp (StanLib size FnPlain AoS)
                      (((pattern
                         (Var inline_whichequals_inline_vecequals_check_sym6___sym39__))
                        (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (body
                   ((pattern
                     (Block
                      (((pattern
                         (Assignment
                          ((LVariable
                            inline_whichequals_inline_vecequals_check_sym6___sym39__)
                           ((Single
                             ((pattern
                               (Var inline_whichequals_inline_vecequals_i_sym7___sym40__))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                          (UArray UInt)
                          ((pattern
                            (FunApp (StanLib Equals__ FnPlain AoS)
                             (((pattern
                                (Indexed
                                 ((pattern (Var input))
                                  (meta
                                   ((type_ (UArray UInt)) (loc <opaque>)
                                    (adlevel DataOnly))))
                                 ((Single
                                   ((pattern
                                     (Var
                                      inline_whichequals_inline_vecequals_i_sym7___sym40__))
                                    (meta
                                     ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern (Lit Int 9))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                        (meta <opaque>)))))
                    (meta <opaque>)))))
                (meta <opaque>))
               ((pattern
                 (Assignment
                  ((LVariable inline_whichequals_inline_vecequals_return_sym5___sym38__)
                   ())
                  (UArray UInt)
                  ((pattern
                    (Var inline_whichequals_inline_vecequals_check_sym6___sym39__))
                   (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))
                (meta <opaque>)))))
            (meta <opaque>))
           ((pattern
             (NRFunApp (CompilerInternal FnValidateSize)
              (((pattern (Lit Str which))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Str "sum(check)"))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern
                 (FunApp (StanLib sum FnPlain AoS)
                  (((pattern
                     (Var inline_whichequals_inline_vecequals_return_sym5___sym38__))
                    (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel AutoDiffable)))))))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
            (meta <opaque>))
           ((pattern
             (Decl (decl_adtype AutoDiffable) (decl_id inline_whichequals_which_sym41__)
              (decl_type
               (Sized
                (SArray SInt
                 ((pattern
                   (FunApp (StanLib sum FnPlain AoS)
                    (((pattern
                       (Var inline_whichequals_inline_vecequals_return_sym5___sym38__))
                      (meta
                       ((type_ (UArray UInt)) (loc <opaque>) (adlevel AutoDiffable)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
              (initialize Default)))
            (meta <opaque>))
           ((pattern
             (Decl (decl_adtype AutoDiffable)
              (decl_id inline_whichequals_counter_sym42__) (decl_type (Sized SInt))
              (initialize Uninit)))
            (meta <opaque>))
           ((pattern
             (Assignment ((LVariable inline_whichequals_counter_sym42__) ()) UInt
              ((pattern (Lit Int 1))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
            (meta <opaque>))
           ((pattern
             (For (loopvar inline_whichequals_i_sym43__)
              (lower
               ((pattern (Lit Int 1))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
              (upper
               ((pattern
                 (FunApp (StanLib size FnPlain AoS)
                  (((pattern (Var input))
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
                             ((pattern
                               (Var
                                inline_whichequals_inline_vecequals_return_sym5___sym38__))
                              (meta
                               ((type_ (UArray UInt)) (loc <opaque>)
                                (adlevel AutoDiffable))))
                             ((Single
                               ((pattern (Var inline_whichequals_i_sym43__))
                                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                          ((pattern (Lit Int 1))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                      ((pattern
                        (Block
                         (((pattern
                            (Assignment
                             ((LVariable inline_whichequals_which_sym41__)
                              ((Single
                                ((pattern (Var inline_whichequals_counter_sym42__))
                                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                             (UArray UInt)
                             ((pattern (Var inline_whichequals_i_sym43__))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                           (meta <opaque>))
                          ((pattern
                            (Assignment
                             ((LVariable inline_whichequals_counter_sym42__) ()) UInt
                             ((pattern
                               (FunApp (StanLib Plus__ FnPlain AoS)
                                (((pattern (Var inline_whichequals_counter_sym42__))
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
             (Assignment ((LVariable inline_whichequals_return_sym36__) ())
              (UArray UInt)
              ((pattern (Var inline_whichequals_which_sym41__))
               (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))
            (meta <opaque>)))))
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
              (((pattern (Var inline_whichequals_return_sym36__))
                (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel AutoDiffable)))))))
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
                            ((pattern (Var inline_whichequals_return_sym36__))
                             (meta
                              ((type_ (UArray UInt)) (loc <opaque>)
                               (adlevel AutoDiffable))))
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
            (FnReadParam (constrain Identity) (dims ()) (mem_pattern SoA)))
           ()))
         (meta ((type_ UReal) (loc <opaque>) (adlevel AutoDiffable))))))))
    (meta <opaque>))
   ((pattern
     (Block
      (((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id inline_whichequals_return_sym9__)
          (decl_type
           (Sized
            (SArray SInt
             ((pattern (Lit Int 0))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
          (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Block
          (((pattern
             (NRFunApp (CompilerInternal FnValidateSize)
              (((pattern (Lit Str check))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Str "size(b)"))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern
                 (FunApp (StanLib size FnPlain SoA)
                  (((pattern (Var input))
                    (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
            (meta <opaque>))
           ((pattern
             (Decl (decl_adtype AutoDiffable) (decl_id inline_whichequals_check_sym10__)
              (decl_type
               (Sized
                (SArray SInt
                 ((pattern
                   (FunApp (StanLib size FnPlain SoA)
                    (((pattern (Var input))
                      (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
              (initialize Default)))
            (meta <opaque>))
           ((pattern
             (Decl (decl_adtype AutoDiffable)
              (decl_id inline_whichequals_inline_vecequals_return_sym5___sym11__)
              (decl_type
               (Sized
                (SArray SInt
                 ((pattern (Lit Int 0))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
              (initialize Uninit)))
            (meta <opaque>))
           ((pattern
             (Block
              (((pattern
                 (NRFunApp (CompilerInternal FnValidateSize)
                  (((pattern (Lit Str check))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                   ((pattern (Lit Str "size(a)"))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                   ((pattern
                     (FunApp (StanLib size FnPlain SoA)
                      (((pattern (Var input))
                        (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                (meta <opaque>))
               ((pattern
                 (Decl (decl_adtype AutoDiffable)
                  (decl_id inline_whichequals_inline_vecequals_check_sym6___sym12__)
                  (decl_type
                   (Sized
                    (SArray SInt
                     ((pattern
                       (FunApp (StanLib size FnPlain SoA)
                        (((pattern (Var input))
                          (meta
                           ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (initialize Default)))
                (meta <opaque>))
               ((pattern
                 (For (loopvar inline_whichequals_inline_vecequals_i_sym7___sym13__)
                  (lower
                   ((pattern (Lit Int 1))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (upper
                   ((pattern
                     (FunApp (StanLib size FnPlain SoA)
                      (((pattern
                         (Var inline_whichequals_inline_vecequals_check_sym6___sym12__))
                        (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (body
                   ((pattern
                     (Block
                      (((pattern
                         (Assignment
                          ((LVariable
                            inline_whichequals_inline_vecequals_check_sym6___sym12__)
                           ((Single
                             ((pattern
                               (Var inline_whichequals_inline_vecequals_i_sym7___sym13__))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                          (UArray UInt)
                          ((pattern
                            (FunApp (StanLib Equals__ FnPlain SoA)
                             (((pattern
                                (Indexed
                                 ((pattern (Var input))
                                  (meta
                                   ((type_ (UArray UInt)) (loc <opaque>)
                                    (adlevel DataOnly))))
                                 ((Single
                                   ((pattern
                                     (Var
                                      inline_whichequals_inline_vecequals_i_sym7___sym13__))
                                    (meta
                                     ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern (Lit Int 9))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                        (meta <opaque>)))))
                    (meta <opaque>)))))
                (meta <opaque>))
               ((pattern
                 (Assignment
                  ((LVariable inline_whichequals_inline_vecequals_return_sym5___sym11__)
                   ())
                  (UArray UInt)
                  ((pattern
                    (Var inline_whichequals_inline_vecequals_check_sym6___sym12__))
                   (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))
                (meta <opaque>)))))
            (meta <opaque>))
           ((pattern
             (NRFunApp (CompilerInternal FnValidateSize)
              (((pattern (Lit Str which))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Str "sum(check)"))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern
                 (FunApp (StanLib sum FnPlain SoA)
                  (((pattern
                     (Var inline_whichequals_inline_vecequals_return_sym5___sym11__))
                    (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel AutoDiffable)))))))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
            (meta <opaque>))
           ((pattern
             (Decl (decl_adtype AutoDiffable) (decl_id inline_whichequals_which_sym14__)
              (decl_type
               (Sized
                (SArray SInt
                 ((pattern
                   (FunApp (StanLib sum FnPlain SoA)
                    (((pattern
                       (Var inline_whichequals_inline_vecequals_return_sym5___sym11__))
                      (meta
                       ((type_ (UArray UInt)) (loc <opaque>) (adlevel AutoDiffable)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
              (initialize Default)))
            (meta <opaque>))
           ((pattern
             (Decl (decl_adtype AutoDiffable)
              (decl_id inline_whichequals_counter_sym15__) (decl_type (Sized SInt))
              (initialize Uninit)))
            (meta <opaque>))
           ((pattern
             (Assignment ((LVariable inline_whichequals_counter_sym15__) ()) UInt
              ((pattern (Lit Int 1))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
            (meta <opaque>))
           ((pattern
             (For (loopvar inline_whichequals_i_sym16__)
              (lower
               ((pattern (Lit Int 1))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
              (upper
               ((pattern
                 (FunApp (StanLib size FnPlain SoA)
                  (((pattern (Var input))
                    (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
              (body
               ((pattern
                 (Block
                  (((pattern
                     (IfElse
                      ((pattern
                        (FunApp (StanLib Equals__ FnPlain SoA)
                         (((pattern
                            (Indexed
                             ((pattern
                               (Var
                                inline_whichequals_inline_vecequals_return_sym5___sym11__))
                              (meta
                               ((type_ (UArray UInt)) (loc <opaque>)
                                (adlevel AutoDiffable))))
                             ((Single
                               ((pattern (Var inline_whichequals_i_sym16__))
                                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                          ((pattern (Lit Int 1))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                      ((pattern
                        (Block
                         (((pattern
                            (Assignment
                             ((LVariable inline_whichequals_which_sym14__)
                              ((Single
                                ((pattern (Var inline_whichequals_counter_sym15__))
                                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                             (UArray UInt)
                             ((pattern (Var inline_whichequals_i_sym16__))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                           (meta <opaque>))
                          ((pattern
                            (Assignment
                             ((LVariable inline_whichequals_counter_sym15__) ()) UInt
                             ((pattern
                               (FunApp (StanLib Plus__ FnPlain SoA)
                                (((pattern (Var inline_whichequals_counter_sym15__))
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
             (Assignment ((LVariable inline_whichequals_return_sym9__) ())
              (UArray UInt)
              ((pattern (Var inline_whichequals_which_sym14__))
               (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))
            (meta <opaque>)))))
        (meta <opaque>))
       ((pattern
         (NRFunApp (CompilerInternal FnValidateSize)
          (((pattern (Lit Str selected))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
           ((pattern (Lit Str "size(whichequals(input, 9, 1))"))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
           ((pattern
             (FunApp (StanLib size FnPlain SoA)
              (((pattern (Var inline_whichequals_return_sym9__))
                (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel AutoDiffable)))))))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
        (meta <opaque>))
       ((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id selected)
          (decl_type
           (Sized
            (SArray SInt
             ((pattern
               (FunApp (StanLib size FnPlain SoA)
                (((pattern
                   (FunApp (UserDefined whichequals FnPlain)
                    (((pattern (Var input))
                      (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Lit Int 9))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                     ((pattern (Lit Int 1))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
          (initialize Default)))
        (meta <opaque>))
       ((pattern
         (Decl (decl_adtype AutoDiffable) (decl_id inline_whichequals_return_sym18__)
          (decl_type
           (Sized
            (SArray SInt
             ((pattern (Lit Int 0))
              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
          (initialize Uninit)))
        (meta <opaque>))
       ((pattern
         (Block
          (((pattern
             (NRFunApp (CompilerInternal FnValidateSize)
              (((pattern (Lit Str check))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Str "size(b)"))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern
                 (FunApp (StanLib size FnPlain SoA)
                  (((pattern (Var input))
                    (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
            (meta <opaque>))
           ((pattern
             (Decl (decl_adtype AutoDiffable) (decl_id inline_whichequals_check_sym19__)
              (decl_type
               (Sized
                (SArray SInt
                 ((pattern
                   (FunApp (StanLib size FnPlain SoA)
                    (((pattern (Var input))
                      (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
              (initialize Default)))
            (meta <opaque>))
           ((pattern
             (Decl (decl_adtype AutoDiffable)
              (decl_id inline_whichequals_inline_vecequals_return_sym5___sym20__)
              (decl_type
               (Sized
                (SArray SInt
                 ((pattern (Lit Int 0))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
              (initialize Uninit)))
            (meta <opaque>))
           ((pattern
             (Block
              (((pattern
                 (NRFunApp (CompilerInternal FnValidateSize)
                  (((pattern (Lit Str check))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                   ((pattern (Lit Str "size(a)"))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                   ((pattern
                     (FunApp (StanLib size FnPlain SoA)
                      (((pattern (Var input))
                        (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                (meta <opaque>))
               ((pattern
                 (Decl (decl_adtype AutoDiffable)
                  (decl_id inline_whichequals_inline_vecequals_check_sym6___sym21__)
                  (decl_type
                   (Sized
                    (SArray SInt
                     ((pattern
                       (FunApp (StanLib size FnPlain SoA)
                        (((pattern (Var input))
                          (meta
                           ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                      (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                  (initialize Default)))
                (meta <opaque>))
               ((pattern
                 (For (loopvar inline_whichequals_inline_vecequals_i_sym7___sym22__)
                  (lower
                   ((pattern (Lit Int 1))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (upper
                   ((pattern
                     (FunApp (StanLib size FnPlain SoA)
                      (((pattern
                         (Var inline_whichequals_inline_vecequals_check_sym6___sym21__))
                        (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                    (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
                  (body
                   ((pattern
                     (Block
                      (((pattern
                         (Assignment
                          ((LVariable
                            inline_whichequals_inline_vecequals_check_sym6___sym21__)
                           ((Single
                             ((pattern
                               (Var inline_whichequals_inline_vecequals_i_sym7___sym22__))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                          (UArray UInt)
                          ((pattern
                            (FunApp (StanLib Equals__ FnPlain SoA)
                             (((pattern
                                (Indexed
                                 ((pattern (Var input))
                                  (meta
                                   ((type_ (UArray UInt)) (loc <opaque>)
                                    (adlevel DataOnly))))
                                 ((Single
                                   ((pattern
                                     (Var
                                      inline_whichequals_inline_vecequals_i_sym7___sym22__))
                                    (meta
                                     ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                              ((pattern (Lit Int 9))
                               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                        (meta <opaque>)))))
                    (meta <opaque>)))))
                (meta <opaque>))
               ((pattern
                 (Assignment
                  ((LVariable inline_whichequals_inline_vecequals_return_sym5___sym20__)
                   ())
                  (UArray UInt)
                  ((pattern
                    (Var inline_whichequals_inline_vecequals_check_sym6___sym21__))
                   (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))
                (meta <opaque>)))))
            (meta <opaque>))
           ((pattern
             (NRFunApp (CompilerInternal FnValidateSize)
              (((pattern (Lit Str which))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern (Lit Str "sum(check)"))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
               ((pattern
                 (FunApp (StanLib sum FnPlain SoA)
                  (((pattern
                     (Var inline_whichequals_inline_vecequals_return_sym5___sym20__))
                    (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel AutoDiffable)))))))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
            (meta <opaque>))
           ((pattern
             (Decl (decl_adtype AutoDiffable) (decl_id inline_whichequals_which_sym23__)
              (decl_type
               (Sized
                (SArray SInt
                 ((pattern
                   (FunApp (StanLib sum FnPlain SoA)
                    (((pattern
                       (Var inline_whichequals_inline_vecequals_return_sym5___sym20__))
                      (meta
                       ((type_ (UArray UInt)) (loc <opaque>) (adlevel AutoDiffable)))))))
                  (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
              (initialize Default)))
            (meta <opaque>))
           ((pattern
             (Decl (decl_adtype AutoDiffable)
              (decl_id inline_whichequals_counter_sym24__) (decl_type (Sized SInt))
              (initialize Uninit)))
            (meta <opaque>))
           ((pattern
             (Assignment ((LVariable inline_whichequals_counter_sym24__) ()) UInt
              ((pattern (Lit Int 1))
               (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
            (meta <opaque>))
           ((pattern
             (For (loopvar inline_whichequals_i_sym25__)
              (lower
               ((pattern (Lit Int 1))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
              (upper
               ((pattern
                 (FunApp (StanLib size FnPlain SoA)
                  (((pattern (Var input))
                    (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly)))))))
                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
              (body
               ((pattern
                 (Block
                  (((pattern
                     (IfElse
                      ((pattern
                        (FunApp (StanLib Equals__ FnPlain SoA)
                         (((pattern
                            (Indexed
                             ((pattern
                               (Var
                                inline_whichequals_inline_vecequals_return_sym5___sym20__))
                              (meta
                               ((type_ (UArray UInt)) (loc <opaque>)
                                (adlevel AutoDiffable))))
                             ((Single
                               ((pattern (Var inline_whichequals_i_sym25__))
                                (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                          ((pattern (Lit Int 1))
                           (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                       (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))
                      ((pattern
                        (Block
                         (((pattern
                            (Assignment
                             ((LVariable inline_whichequals_which_sym23__)
                              ((Single
                                ((pattern (Var inline_whichequals_counter_sym24__))
                                 (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))))
                             (UArray UInt)
                             ((pattern (Var inline_whichequals_i_sym25__))
                              (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly))))))
                           (meta <opaque>))
                          ((pattern
                            (Assignment
                             ((LVariable inline_whichequals_counter_sym24__) ()) UInt
                             ((pattern
                               (FunApp (StanLib Plus__ FnPlain SoA)
                                (((pattern (Var inline_whichequals_counter_sym24__))
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
             (Assignment ((LVariable inline_whichequals_return_sym18__) ())
              (UArray UInt)
              ((pattern (Var inline_whichequals_which_sym23__))
               (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel DataOnly))))))
            (meta <opaque>)))))
        (meta <opaque>))
       ((pattern
         (TargetPE
          ((pattern
            (FunApp (StanLib normal_lpdf (FnLpdf true) SoA)
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
             (FunApp (StanLib size FnPlain SoA)
              (((pattern (Var inline_whichequals_return_sym18__))
                (meta ((type_ (UArray UInt)) (loc <opaque>) (adlevel AutoDiffable)))))))
            (meta ((type_ UInt) (loc <opaque>) (adlevel DataOnly)))))
          (body
           ((pattern
             (Block
              (((pattern
                 (TargetPE
                  ((pattern
                    (FunApp (StanLib Times__ FnPlain SoA)
                     (((pattern
                        (Promotion
                         ((pattern
                           (Indexed
                            ((pattern (Var inline_whichequals_return_sym18__))
                             (meta
                              ((type_ (UArray UInt)) (loc <opaque>)
                               (adlevel AutoDiffable))))
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
 (prog_name udf_empty_local_model) (prog_path tests/fixtures/udf_empty_local.stan))
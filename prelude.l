(do
 (def or (macro (a b) (if (eval a) t (if (eval b) t nil))))
 (def and (macro (a b) (if (eval a) (if (eval b) t nil) nil)))
 (def not (macro (a) (if (eval a) nil t)))
 (def len (fn (l) (if (+ 1 (len (cdr l))) 0)))
 (def defn (macro (name args body) (list def name (list fn args body))))
)

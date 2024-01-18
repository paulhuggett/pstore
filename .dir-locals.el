; Configure emacs flycheck with the correct language version and include paths.
; Assumes that projectile (https://projectile.mx) is installed.

((c++-mode
  (eval . (let* ((root (projectile-project-root))
                 (includes (list (concat root "include")))
                 (standard "c++17"))
            (setq-local
             flycheck-clang-include-path includes
             flycheck-gcc-include-path includes
             flycheck-clang-language-standard standard
             flycheck-gcc-language-standard standard)))))

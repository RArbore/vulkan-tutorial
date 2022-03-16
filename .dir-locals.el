;;; Directory Local Variables
;;; For more information see (info "(emacs) Directory Variables")

((nil . (
	 (eval . (let
		     ((root
		       (projectile-project-root)))
		   (setq-local flycheck-clang-args
			       (list
				(concat "-I" root "include") (concat "-std=c++20")))
		   (setq-local flycheck-clang-include-path
			       (list
				(concat root "include")))
		   (setq-local flycheck-gcc-args
			       (list
				(concat "-I" root "include") (concat "-std=c++20")))
		   (setq-local flycheck-gcc-include-path
			       (list
				(concat root "include")))
		   ))))
 (c++-mode . ((c-basic-offset . 4)))
 (c-mode . ((mode . c++))))

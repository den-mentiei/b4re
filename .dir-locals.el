((c-mode . ((compile-command . "make")
	    (eval setq default-directory
		  (locate-dominating-file buffer-file-name ".dir-locals.el")))))


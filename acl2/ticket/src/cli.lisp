;;; cli.lisp - Main CLI entry point

(in-package :ticket)

(defun print-help ()
  (format t "tk - minimal ticket system (ACL2/Lisp port)

Usage: tk <command> [args]

Commands:
  create [title] [options]  Create ticket, prints ID
    -d, --description       Description text
    -t, --type              Type (task|bug|feature) [default: task]
    -p, --priority          Priority 0-4 [default: 2]
    -a, --assignee          Assignee [default: git user.name]
  help                      Show this help

Tickets stored as markdown files in .tickets/
"))

(defun main (&optional args)
  "Main entry point. ARGS = command-line arguments."
  (let ((args (or args #+sbcl (cdr sb-ext:*posix-argv*) #-sbcl nil)))
    (cond
      ((or (null args)
           (member (first args) '("-h" "--help" "help") :test #'string=))
       (print-help) 0)
      ((string= (first args) "create")
       (cmd-create (rest args)))
      (t
       (format *error-output* "Unknown command: ~A~%" (first args))
       1))))

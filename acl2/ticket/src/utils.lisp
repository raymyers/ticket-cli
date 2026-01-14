;;; utils.lisp - Utility functions for ticket-cli

(in-package :ticket)

(defconstant +tickets-dir+ ".tickets")

(defun split-string (string delimiter)
  "Split STRING by DELIMITER character."
  (let ((result nil) (start 0))
    (loop for i from 0 below (length string)
          when (char= (char string i) delimiter)
          do (when (> i start)
               (push (subseq string start i) result))
             (setf start (1+ i)))
    (when (< start (length string))
      (push (subseq string start) result))
    (nreverse result)))

(defun string-starts-with (string prefix)
  (and (>= (length string) (length prefix))
       (string= string prefix :end1 (length prefix))))

(defun trim-whitespace (string)
  (string-trim '(#\Space #\Tab #\Newline #\Return) string))

(defun run-shell-command (cmd)
  "Run shell command, return output as string."
  (with-output-to-string (s)
    (sb-ext:run-program "/bin/sh" (list "-c" cmd) :output s :error nil :wait t)))

(defun sha256-hex (string)
  "Return SHA256 hex digest of STRING via shell."
  (let ((output (run-shell-command
                  (format nil "printf '%s' '~A' | sha256sum | cut -c1-64" string))))
    (trim-whitespace output)))

(defun get-pid ()
  "Get current process ID via shell."
  (parse-integer (trim-whitespace (run-shell-command "echo $$")) :junk-allowed t))

(defun iso-timestamp ()
  "Return current UTC time in ISO 8601 format."
  (multiple-value-bind (second minute hour date month year)
      (decode-universal-time (get-universal-time) 0)
    (format nil "~4,'0D-~2,'0D-~2,'0DT~2,'0D:~2,'0D:~2,'0DZ"
            year month date hour minute second)))

(defun get-current-directory-name ()
  "Get name of current working directory."
  (let* ((cwd (truename "."))
         (dir-list (pathname-directory cwd)))
    (if (and dir-list (listp dir-list))
        (let ((last-comp (car (last dir-list))))
          (if (stringp last-comp) last-comp (format nil "~A" last-comp)))
        "unknown")))

(defun extract-directory-prefix (dir-name)
  "Extract prefix: first letter of each segment split on - or _."
  (let* ((normalized (substitute #\Space #\_ (substitute #\Space #\- dir-name)))
         (segments (remove-if (lambda (s) (zerop (length s)))
                              (split-string normalized #\Space))))
    (cond
      ((> (length segments) 1)
       (coerce (mapcar (lambda (s) (char s 0)) segments) 'string))
      ((and segments (first segments))
       (subseq (first segments) 0 (min 3 (length (first segments)))))
      (t (subseq dir-name 0 (min 3 (length dir-name)))))))

(defun generate-id ()
  "Generate ticket ID: {prefix}-{4-char-hash}."
  (let* ((prefix (string-downcase (extract-directory-prefix (get-current-directory-name))))
         (entropy (format nil "~D~D" (get-pid) (get-universal-time)))
         (hash (sha256-hex entropy)))
    (format nil "~A-~A" prefix (subseq hash 0 4))))

(defun ensure-tickets-dir ()
  (ensure-directories-exist (format nil "~A/" +tickets-dir+)))

(defun ticket-file-path (ticket-id)
  (format nil "~A/~A.md" +tickets-dir+ ticket-id))

(defun write-file-contents (path contents)
  (ensure-directories-exist path)
  (with-open-file (stream path :direction :output
                          :if-exists :supersede :if-does-not-exist :create)
    (write-string contents stream)))

(defun get-git-user-name ()
  "Get git config user.name, or NIL."
  (handler-case
      (let ((output (with-output-to-string (s)
                      (sb-ext:run-program "git" '("config" "user.name")
                        :output s :error nil :wait t :search t))))
        (let ((trimmed (trim-whitespace output)))
          (if (plusp (length trimmed)) trimmed nil)))
    (error () nil)))

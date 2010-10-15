
(defun read-all-bytes (s)
  (let (bytes)
	(do ((c (read-byte s nil 'eof)
			(read-byte s nil 'eof)))
		((eq c 'eof) (reverse bytes))
	  (push c bytes))))

(defun read-bin-file (file)
  (with-open-file (s file :element-type '(unsigned-byte 8))
	(read-all-bytes s)))

(defun write-out-hex (stream chars)
  (dolist (c chars)
	(format stream "~2,'0X" c)))

(defun write-out-hex-file (file chars)
  (with-open-file (s file
					 :direction :output
					 :if-exists :supersede
					 :if-does-not-exist :create
					 :external-format :ascii)
	(write-out-hex s chars)))

(defun bin2hex (in out)
  (write-out-hex-file out (read-bin-file in)))

(defun name-bin-to-hex (file)
  (let ((name (pathname-name file)))
	(format nil "~a.hex" name)))

(defun convert-generated-bins ()
  (let* ((bins (directory "g*.bin"))
		 (hexs (mapcar #'name-bin-to-hex bins)))
	(mapcar #'bin2hex bins hexs)
	(values)))

(format #t "Entering ~a.~%" (current-filename))

;;; modules
(use-modules (ice-9 receive))

;;; actual code

(define x-res 1)
(define y-res 1)

(receive (x y w h) (viewport)
  (format #t "~a x ~a~%" w h)
  (set! x-res w)
  (set! y-res h))

(let ((cam (make-perspective-camera "cam" (list 0 0 5) (list 0 0 -1) (list 0 1 0) 35 (/ x-res y-res) 1 1000)))
  (use-camera cam))

(load "default.shader")

(format #t "Leaving ~a.~%" (current-filename))

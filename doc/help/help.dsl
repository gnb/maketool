<!DOCTYPE style-sheet PUBLIC "-//James Clark//DTD DSSSL Style Sheet//EN" [
<!ENTITY dbstyle PUBLIC "-//Norman Walsh//DOCUMENT DocBook HTML Stylesheet//EN" CDATA DSSSL>
]>

<!--
Stylesheet driver file, used to override default values
of various DSSSL control variables to values more
suitable for HTML online help.

(c) 2000 Greg Banks.
-->

<style-sheet>
<style-specification use="docbook">
<style-specification-body>

;; Generate a Table of Contents for an article
(define %generate-article-toc% #t)

;; Turn off navigation links on each page
(define %header-navigation% #f)
;; (define %footer-navigation% #f)

;; HTML extension is .html
(define %html-ext% ".html")

;; Declare docs HTML 4.0 compliant.  Hopefully they will be ;-)
(define %html-pubid% "-//W3C//DTD HTML 4.0 Transitional//EN")

;; Filename of the root HTML doc
(define %root-filename% "maketool")

;; TODO: sprinkle the text with indexterms
;; Generate HTML index data in HTML.index
;; (define html-index #t)

;; Generate a manifest file (HTML.manifest) listing all HTML files generated
;; (define html-manifest #t)


;; Filenames of each HTML file are derived from id= attributes
(define %use-id-as-filename% #t)


;; Add a CSS stylesheet to control appearance of some tags
;; (define %stylesheet% "help.css")


;; Hack TOC behaviour to show 2nd level sections in TOC
;; Sigh, if only DocBook was smarter about which elements
;; to generate toc entries for, this would work, fuck it.
;; (define (toc-depth nd) 2)

;; Hack chunking behaviour to make 2nd level sections
;; into separate chunks too.
;; (define (chunk-section-depth) 2)


;; Hack chunking behaviour to prevent the first section being
;; merged into the title page chunk.
(define (chunk-skip-first-element-list) (list (normalize "nonesuch")))


</style-specification-body>
</style-specification>
<external-specification id="docbook" document="dbstyle">
</style-sheet>

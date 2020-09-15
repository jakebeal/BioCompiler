" Vim syntax file
" Language: Proto
" Maintainer: Kyle Usbeck
" Latest Revision 15 Nov 2010
"
" To set this as the syntax file for .proto files:
"   copy this script into the $VIM/syntax/ (e.g., ~/.vim/syntax/proto.vim)
"   add a line to your $VIMRC (e.g., ~/.vimrc):
"      au BufRead,BufNewFile *.proto setfiletype proto 

" If syntax is already defined, don't redefine
if exists("b:current_syntax")
   finish
endif

" Set syntax to proto
let b:current_syntax = "proto"

" Patterns
syn match protoFunctionDef '\s*\S\+' contained display
syn match protoLanguageComment ";.*$" contains=@protoCommentGroup

" Define keywords
syn keyword protoLanguageKeywords null
syn keyword protoLanguageKeywords quote
syn keyword protoLanguageKeywords def nextgroup=protoFunctionDef
syn keyword protoLanguageKeywords let
syn keyword protoLanguageKeywords let*
syn keyword protoLanguageKeywords all
syn keyword protoLanguageKeywords seq
syn keyword protoLanguageKeywords loop
syn keyword protoLanguageKeywords mux
syn keyword protoLanguageKeywords if
syn keyword protoLanguageKeywords select
syn keyword protoLanguageKeywords cond
syn keyword protoLanguageKeywords case
syn keyword protoLanguageKeywords fun
syn keyword protoLanguageKeywords apply
syn keyword protoLanguageKeywords id
syn keyword protoLanguageKeywords rep
syn keyword protoLanguageKeywords dt
syn keyword protoLanguageKeywords set-dt
syn keyword protoLanguageKeywords letfed
syn keyword protoLanguageKeywords rep
syn keyword protoLanguageKeywords fold-time
syn keyword protoLanguageKeywords all-time
syn keyword protoLanguageKeywords any-time
syn keyword protoLanguageKeywords max-time
syn keyword protoLanguageKeywords min-time
syn keyword protoLanguageKeywords int-time
syn keyword protoLanguageKeywords once
syn keyword protoLanguageKeywords and
syn keyword protoLanguageKeywords or
syn keyword protoLanguageKeywords muxand
syn keyword protoLanguageKeywords muxor
syn keyword protoLanguageKeywords not
syn keyword protoLanguageKeywords xor
syn keyword protoLanguageKeywords inf
syn keyword protoLanguageKeywords e
syn keyword protoLanguageKeywords pi
syn keyword protoLanguageKeywords neg
syn keyword protoLanguageKeywords mod
syn keyword protoLanguageKeywords pow
syn keyword protoLanguageKeywords exp
syn keyword protoLanguageKeywords log
syn keyword protoLanguageKeywords log10
syn keyword protoLanguageKeywords logN
syn keyword protoLanguageKeywords floor
syn keyword protoLanguageKeywords ceil
syn keyword protoLanguageKeywords max
syn keyword protoLanguageKeywords min
syn keyword protoLanguageKeywords denormalize
syn keyword protoLanguageKeywords denormalizeN
syn keyword protoLanguageKeywords is-zero
syn keyword protoLanguageKeywords is-neg
syn keyword protoLanguageKeywords is-pos
syn keyword protoLanguageKeywords sqrt
syn keyword protoLanguageKeywords abs
syn keyword protoLanguageKeywords sin
syn keyword protoLanguageKeywords cos
syn keyword protoLanguageKeywords tan
syn keyword protoLanguageKeywords asin
syn keyword protoLanguageKeywords acos
syn keyword protoLanguageKeywords atan2
syn keyword protoLanguageKeywords sinh
syn keyword protoLanguageKeywords cosh
syn keyword protoLanguageKeywords tanh
syn keyword protoLanguageKeywords asinh
syn keyword protoLanguageKeywords acosh
syn keyword protoLanguageKeywords atanh
syn keyword protoLanguageKeywords rnd
syn keyword protoLanguageKeywords rndint
syn keyword protoLanguageKeywords vdot
syn keyword protoLanguageKeywords vlen
syn keyword protoLanguageKeywords normalize norm
syn keyword protoLanguageKeywords polar-to-rect
syn keyword protoLanguageKeywords rect-to-polar
syn keyword protoLanguageKeywords rotate
syn keyword protoLanguageKeywords tuple tup
syn keyword protoLanguageKeywords len
syn keyword protoLanguageKeywords elt
syn keyword protoLanguageKeywords nul-tup
syn keyword protoLanguageKeywords map
syn keyword protoLanguageKeywords fold
syn keyword protoLanguageKeywords slice
syn keyword protoLanguageKeywords 1st
syn keyword protoLanguageKeywords 2nd
syn keyword protoLanguageKeywords 3rd
syn keyword protoLanguageKeywords find
syn keyword protoLanguageKeywords position
syn keyword protoLanguageKeywords assoc
syn keyword protoLanguageKeywords defstruct
syn keyword protoLanguageKeywords nbr
syn keyword protoLanguageKeywords nbr-range
syn keyword protoLanguageKeywords nbr-angle
syn keyword protoLanguageKeywords nbr-lag
syn keyword protoLanguageKeywords nbr-vec
syn keyword protoLanguageKeywords is-self
syn keyword protoLanguageKeywords infinitesimal
syn keyword protoLanguageKeywords min-hood
syn keyword protoLanguageKeywords min-hood+
syn keyword protoLanguageKeywords max-hood
syn keyword protoLanguageKeywords max-hood+
syn keyword protoLanguageKeywords all-hood
syn keyword protoLanguageKeywords all-hood+
syn keyword protoLanguageKeywords any-hood
syn keyword protoLanguageKeywords any-hood+
syn keyword protoLanguageKeywords sum-hood
syn keyword protoLanguageKeywords int-hood
syn keyword protoLanguageKeywords fold-hood
syn keyword protoLanguageKeywords fold-hood*
syn keyword protoLanguageKeywords fold-hood-plus
syn keyword protoLanguageKeywords fold-hood-plus*
syn keyword protoLanguageKeywords mix
syn keyword protoLanguageKeywords mov
syn keyword protoLanguageKeywords speed
syn keyword protoLanguageKeywords bearing
syn keyword protoLanguageKeywords area
syn keyword protoLanguageKeywords hood-radius
syn keyword protoLanguageKeywords flex
syn keyword protoLanguageKeywords mid
syn keyword protoLanguageKeywords distance-to
syn keyword protoLanguageKeywords broadcast
syn keyword protoLanguageKeywords dilate
syn keyword protoLanguageKeywords distance
syn keyword protoLanguageKeywords disperse
syn keyword protoLanguageKeywords dither
syn keyword protoLanguageKeywords elect
syn keyword protoLanguageKeywords flip
syn keyword protoLanguageKeywords timer
syn keyword protoLanguageKeywords red
syn keyword protoLanguageKeywords green
syn keyword protoLanguageKeywords blue
syn keyword protoLanguageKeywords probe


"" Parenthsis Highlighting stolen from 
"" Rainbow Parenthsis (http://www.vim.org/scripts/script.php?script_id=1230)

hi level1c ctermfg=darkcyan
hi level2c ctermfg=darkred
hi level3c ctermfg=darkyellow
hi level4c ctermfg=darkblue
hi level5c ctermfg=darkmagenta
hi level6c ctermfg=darkblue
hi level7c ctermfg=darkgreen
hi level8c ctermfg=darkcyan
hi level9c ctermfg=darkred
hi level10c ctermfg=darkyellow
hi level11c ctermfg=darkgreen
hi level12c ctermfg=darkblue
hi level13c ctermfg=darkmagenta
hi level14c ctermfg=darkgreen
hi level15c ctermfg=darkcyan
hi level16c ctermfg=darkred

syn region level1 matchgroup=level1c start=/(/ end=/)/ contains=TOP,level1,level2,level3,level4,level5,level6,level7,level8,level9,level10,level11,level12,level13,level14,level15, level16,NoInParens
syn region level2 matchgroup=level2c start=/(/ end=/)/ contains=TOP,level2,level3,level4,level5,level6,level7,level8,level9,level10,level11,level12,level13,level14,level15, level16,NoInParens
syn region level3 matchgroup=level3c start=/(/ end=/)/ contains=TOP,level3,level4,level5,level6,level7,level8,level9,level10,level11,level12,level13,level14,level15, level16,NoInParens
syn region level4 matchgroup=level4c start=/(/ end=/)/ contains=TOP,level4,level5,level6,level7,level8,level9,level10,level11,level12,level13,level14,level15, level16,NoInParens
syn region level5 matchgroup=level5c start=/(/ end=/)/ contains=TOP,level5,level6,level7,level8,level9,level10,level11,level12,level13,level14,level15, level16,NoInParens
syn region level6 matchgroup=level6c start=/(/ end=/)/ contains=TOP,level6,level7,level8,level9,level10,level11,level12,level13,level14,level15, level16,NoInParens
syn region level7 matchgroup=level7c start=/(/ end=/)/ contains=TOP,level7,level8,level9,level10,level11,level12,level13,level14,level15, level16,NoInParens
syn region level8 matchgroup=level8c start=/(/ end=/)/ contains=TOP,level8,level9,level10,level11,level12,level13,level14,level15, level16,NoInParens
syn region level9 matchgroup=level9c start=/(/ end=/)/ contains=TOP,level9,level10,level11,level12,level13,level14,level15, level16,NoInParens
syn region level10 matchgroup=level10c start=/(/ end=/)/ contains=TOP,level10,level11,level12,level13,level14,level15, level16,NoInParens
syn region level11 matchgroup=level11c start=/(/ end=/)/ contains=TOP,level11,level12,level13,level14,level15, level16,NoInParens
syn region level12 matchgroup=level12c start=/(/ end=/)/ contains=TOP,level12,level13,level14,level15, level16,NoInParens
syn region level13 matchgroup=level13c start=/(/ end=/)/ contains=TOP,level13,level14,level15, level16,NoInParens
syn region level14 matchgroup=level14c start=/(/ end=/)/ contains=TOP,level14,level15, level16,NoInParens
syn region level15 matchgroup=level15c start=/(/ end=/)/ contains=TOP,level15, level16,NoInParens
syn region level16 matchgroup=level16c start=/(/ end=/)/ contains=TOP,level16,NoInParens



" Define highlighting
hi def link protoLanguageKeywords Statement
hi def link protoLanguageComment Comment
hi def link protoFunctionDef Constant


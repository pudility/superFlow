" File: swift.vim
" Author: Keith Smiley
" Description: Runtime files for Swift
" Taken over by Zoe Carver and repurposed for SuperFly

if exists("b:current_syntax")
  finish
endif

" Comments
" Comment contained keywords
syntax keyword swiftTodos contained TODO XXX FIXME NOTE
syntax keyword swiftMarker contained MARK

syntax case match
delfunction s:CommentKeywordMatch


" Literals
" Strings
syntax region swiftString start=/"/ skip=/\\\\\|\\"/ end=/"/ contains=swiftInterpolatedWrapper oneline
syntax region swiftMultilineString start=/"""/ end=/"""/ contains=swiftMultilineInterpolatedWrapper
syntax region swiftMultilineInterpolatedWrapper start='\v\zs\\\(\s*' end='\v\s*\)' contained containedin=swiftMultilineString contains=swiftInterpolatedString oneline
syntax region swiftInterpolatedWrapper start='\v(^|[^\\])\zs\\\(\s*' end='\v\s*\)' contained containedin=swiftString contains=swiftInterpolatedString,swiftString oneline
syntax match swiftInterpolatedString "\v\w+(\(\))?" contained containedin=swiftInterpolatedWrapper,swiftMultilineInterpolatedWrapper oneline

" Numbers
syntax match swiftNumber "\v<\d+>"
syntax match swiftNumber "\v<(\d+_+)+\d+(\.\d+(_+\d+)*)?>"
syntax match swiftNumber "\v<\d+\.\d+>"
syntax match swiftNumber "\v<\d*\.?\d+([Ee]-?)?\d+>"
syntax match swiftNumber "\v<0x[[:xdigit:]_]+([Pp]-?)?\x+>"
syntax match swiftNumber "\v<0b[01_]+>"
syntax match swiftNumber "\v<0o[0-7_]+>"

" BOOLs
syntax keyword swiftBoolean
      \ true
      \ false


" Operators
syntax match swiftOperator "\v\*"
syntax match swiftOperator "\v-"
syntax match swiftOperator "\v\+"
syntax match swiftOperator "\v\="
syntax match swiftOperator "\v\/"
syntax match swiftOperator "\v\<"
syntax match swiftOperator "\v\>"

" Methods/Functions/Properties
syntax match swiftMethod "\.\@<=\<\D\w*\>\ze("
syntax match swiftProperty "\.\@<=\<\D\w*\>(\@!"

" Swift closure arguments
syntax match swiftClosureArgument "\$\d\+\(\.\d\+\)\?"

syntax match swiftAvailability "\v((\*(\s*,\s*[a-zA-Z="0-9.]+)*)|(\w+\s+\d+(\.\d+(.\d+)?)?\s*,\s*)+\*)" contains=swiftString

" Keywords {{{
syntax keyword swiftKeywords
      \ main
      \ for
      \ ADD
      \ MULTIPLY
      \ DOT
      \ DECREMENT
      \ NEGATE
      \ EXP
      \ NORMALIZE
      \ TEST
      \ RUN

syntax keyword swiftAttributes
      \ @hola " more coming here soon

syntax keyword swiftStructure
      \ array
      \ func
      \ var

syntax keyword swiftDebugIdentifier " these might exist someday :)
      \ __COLUMN__
      \ __FILE__
      \ __FUNCTION__
      \ __LINE__

syntax match swiftTypeDeclaration /->/ skipwhite nextgroup=swiftType " this will happen when we have `map` and `forEach`

syntax keyword swiftImports extern

" Comment patterns
syntax match swiftComment "\v\#.*$"
      \ contains=swiftTodos,swiftDocString,swiftMarker,@Spell oneline

" Set highlights
highlight default link swiftTodos Todo
highlight default link swiftDocString String
highlight default link swiftShebang Comment
highlight default link swiftComment Comment
highlight default link swiftMarker Comment

highlight default link swiftString String
highlight default link swiftMultilineString String
highlight default link swiftInterpolatedWrapper Delimiter
highlight default link swiftMultilineInterpolatedWrapper Delimiter
highlight default link swiftTypeDeclaration Delimiter
highlight default link swiftNumber Number
highlight default link swiftBoolean Boolean

highlight default link swiftOperator Operator
highlight default link swiftCastKeyword Keyword
highlight default link swiftKeywords Keyword
highlight default link swiftMultiwordKeywords Keyword
highlight default link swiftEscapedReservedWord Normal
highlight default link swiftClosureArgument Operator
highlight default link swiftAttributes PreProc
highlight default link swiftConditionStatement PreProc
highlight default link swiftStructure Structure
highlight default link swiftType Type
highlight default link swiftImports Include
highlight default link swiftPreprocessor PreProc
highlight default link swiftMethod Function
highlight default link swiftProperty Identifier

highlight default link swiftDefinitionModifier Define
highlight default link swiftConditionStatement PreProc
highlight default link swiftAvailability Normal
highlight default link swiftAvailabilityArg Normal
highlight default link swiftPlatforms Keyword
highlight default link swiftDebugIdentifier PreProc
highlight default link swiftLineDirective PreProc

" Force vim to sync at least x lines. This solves the multiline comment not
" being highlighted issue
syn sync minlines=100

let b:current_syntax = "superflow"

#!/bin/sh

optbuild=
optrun=
optcomp=
optall=
opt2=
for x; do
  if [ "$x" = "all" ]; then optall=1; break; fi
  if [ "$x" = "2" ]; then opt2=1; break; fi
  if [ "$x" = "build" ]; then optbuild=1; break; fi
  if [ "$x" = "compile" ]; then optcomp=1; break; fi
  if [ "$x" = "run" ]; then optrun=1; break; fi
done
if [ -n "$optbuild" ] || [ -n "$optall" ]; then
  echo "building... "

  clang++ super.cpp ast/ast.cpp lexer/lexer.cpp parser/parser.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core mcjit native` -O3
fi

if [ -n "$optcomp" ] || [ -n "$optall" ] || [ -n "$opt2" ]; then
  echo "compiling... "

  ./a.out $1 &> out.ll

  clang++ library/library.cpp out.ll -o built
fi

if [ -n "$optrun" ] || [ -n "$optall" ] || [ -n "$opt2" ]; then
  echo "running... "

  ./built
fi

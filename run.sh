#!/bin/sh

mkdir -p tmp

optbuild=
optrun=
optcomp=
optall=
opt2=
optd=
opti=
for x; do
  if [ "$x" = "all" ]; then optall=1; break; fi
  if [ "$x" = "i" ]; then opti=1; break; fi
  if [ "$x" = "2" ]; then opt2=1; break; fi
  if [ "$x" = "build" ]; then optbuild=1; break; fi
  if [ "$x" = "compile" ]; then optcomp=1; break; fi
  if [ "$x" = "run" ]; then optrun=1; break; fi
  if [ "$x" = "d" ]; then optd=1; break; fi
done
if [ -n "$optbuild" ] || [ -n "$optall" ]; then
  echo "building... "

  clang++ $SUPERFLOW_DIR/super.cpp $SUPERFLOW_DIR/ast/ast.cpp $SUPERFLOW_DIR/lexer/lexer.cpp $SUPERFLOW_DIR/parser/parser.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core mcjit native` -O3 -o $SUPERFLOW_DIR/a.out
fi

if [ -n "$optd" ]; then
  echo "building (debug)... "

  clang++ $SUPERFLOW_DIR/super.cpp $SUPERFLOW_DIR/ast/ast.cpp $SUPERFLOW_DIR/lexer/lexer.cpp $SUPERFLOW_DIR/parser/parser.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core mcjit native` -O3 -g -o $SUPERFLOW_DIR/a.out
fi

if [ -n "$optcomp" ] || [ -n "$optall" ] || [ -n "$opt2" ]; then
  echo "compiling... "

  less $SUPERFLOW_DIR/library/matrix.spr > $SUPERFLOW_DIR/tmp/out.spr
  less $1 >> $SUPERFLOW_DIR/tmp/out.spr
  $SUPERFLOW_DIR/a.out $SUPERFLOW_DIR/tmp/out.spr &> $SUPERFLOW_DIR/out.ll

  clang++ $SUPERFLOW_DIR/library/library.cpp $SUPERFLOW_DIR/out.ll -o $SUPERFLOW_DIR/built -j4
fi

if [ -n "$optrun" ] || [ -n "$optall" ] || [ -n "$opt2" ]; then
  echo "running... "

  $SUPERFLOW_DIR/built | xargs echo # make sure that it can be read when we spawn this file as the cild processes
fi

if [ -n "$opti" ]; then
  echo "generating input for test file... "

  less $SUPERFLOW_DIR/library/matrix.spr > $SUPERFLOW_DIR/tmp/out.spr
  less $1 >> $SUPERFLOW_DIR/tmp/out.spr
  $SUPERFLOW_DIR/a.out $SUPERFLOW_DIR/tmp/out.spr &> $SUPERFLOW_DIR/out.ll

  clang++ $SUPERFLOW_DIR/library/library.cpp $SUPERFLOW_DIR/out.ll -S -emit-llvm

  less $SUPERFLOW_DIR/library.ll > $SUPERFLOW_DIR/gen.ll
  less $SUPERFLOW_DIR/out.ll >> $SUPERFLOW_DIR/gen.ll

  less $SUPERFLOW_DIR/gen.ll > $3

  rm $SUPERFLOW_DIR/library.ll
  rm $SUPERFLOW_DIR/gen.ll
fi

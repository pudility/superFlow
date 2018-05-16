Currently this projects is in *very early* stages. Right now it is just a lexer/parser - but more coming soon!

To run:
```bash
clang++ super.cpp ast/ast.cpp lexer/lexer.cpp parser/parser.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core`
```

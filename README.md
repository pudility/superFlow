Currently this projects is in *very early* stages.

To run:

Modify tmpfile.spr then run

```bash
./run.sh all
```
to build, compile, and run your code.

### Functions
---
```
func add (a b) a + b
```

White space does not matter, so this is also valid:
```
func add (a b)
  a + b
```

Then just call it like this:
```
add (1, 2) # notice that simi colons are used here
```

### Loops
---
```
for i = 0, i < 10 (
  // do something with `i`
)
```

you can also pass a value to step by:

```
for i = 0, i < 10, 5 (i) // this will increment `i` by `5` every time
```

### Variables
---
Use `var` to make a variable.

There are two types of variables *real* variables and functions that have no params that return values - this is subject to change.

Here is an example of creating a variable:
```
var x 10
```
Above is an example of a *fake* variable or a variable that is just a function pretending to be a variable.

The only time *real* variables are used is in things like functions and loops:
```
func foo (x) x # `x` is a *real* variable
```

### Arrays
Arrays can be created using the `array` type:
```
array a [1 2 3]
```
elements are **not** seperated by semi colins.

### Opporators
These are your choices for opporators:
- `+`
- `-`
- `*`
- `<`

They should be pretty self explanitory.
Here are some examples of how to use them:
```
4 + 4 * (3 - 1)
```

### Comments
Simply use `#` to comment lines :)


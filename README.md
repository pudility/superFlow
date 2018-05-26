Currently this projects is in *very early* stages.

To run:

Write your code into a file and pass the filename as the first paremeter to `run.sh`

Use this:
```bash
./run.sh <filename> all
```
to build, compile, and run your code.

To only preform one of these actions, simply pass it only that argument:

```bash
./run.sh all # everything

./run.sh build # build the compiler

./run.sh compile # compile code in tmpfile.spr

./run.sh run # run compiled code
```

### Functions
---
```python
func 0 add (a 0 b 0) a + b # notice that commas are not used here
```
To declare a function use the `func` keyword, then give an example of the type of thing you want to return (eg. if you want to return a number write `func 0`), then give the function name followed by any arguments. Arguments also need examples of what they will return (eg. if you want a number as an argument write `x 0`). The last line of the function will be returned, use `;` for void.

White space does not matter, so this is also valid:
```python
func 0 add (a 0 b 0)
  a + b
```

You can also have multi-line functions:
```python
func 0 foo (x 0) {
  var i x + 1
  i * 2
}
```

Then just call it like this:
```python
add (1, 2) # notice that commas are used here
```

##### Void Functions
A void function will return `0.0`. Here is an example:
```python
func 0 voidFunc (a 0) {
  a + 1
  ;
}
```

### Loops
---
```python
for i = 0, i < 10 (
# do something with `i`
)
```

you can also pass a value to step by:

```python
for i = 0, i < 10, 5 (i) // this will increment `i` by `5` every time
```
### External Functions
If you want to use external functions, just use the `extern` keyword. The most common use of this is probably `printd`:
```python
extern 0 printd(x 0)
```

### Variables
---
Use `var` to make a variable.

Here is an example of creating a variable:
```python
var x 10
```

### Arrays
Arrays can be created using the `array` type:
```python
array a [1 2 3]
```
elements are **not** seperated by semi colins.

You can get and set elements in the array like this:
```python
# Set element
a[0] = 3

# Get element
printd(a[0]) #3
```

Nested arrays also work:
```python
[[1 2 3] [1 2 3] [1 2 3]]
```

### Opporators
These are your choices for opporators:
- `+`
- `-`
- `*`
- `<`

They should be pretty self explanitory.
Here are some examples of how to use them:
```python
4 + 4 * (3 - 1)
```

### Comments
Simply use `#` to comment lines :)

### Notes on the language

* Everything is passed by reference.
* Functions are compiled before everything else, so you cannot use variables you define at the top level (this will be fixed soon)

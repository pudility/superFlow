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

### Performance
---
Speed test of generating and testing 100 neural networks
```bash
superFlow(master): time python tmp/nn.py
python tmp/nn.py  11.24s user 0.11s system 98% cpu 11.550 total
superFlow(master): time ./built
./built  2.09s user 0.01s system 99% cpu 2.107 total<Paste>
```
This makes [it](https://github.com/pudility/superFlow/blob/master/nn_example.spr) about 5 times faster than an [equivilant python script](https://gist.github.com/miloharper/62fe5dcc581131c96276#file-short_version-py).

### Building neural networks
---
More coming soon! For now use the built in functions (Note these are not fully developed and often only work in specifit use cases).

Here is an example of a basic neural network:
```python
extern 0 printd(x 0);

array in [[0 0 1] [1 1 1] [1 0 1]]
array out [[0 1 1]]
array test [0 0 0]
array weight [[0 0 0]];

for i = 0, i < 10000
  weight = TEST (in, out, weight);

test = NORMALIZE(_EXP(_NEGATE(_DOT([[1 1 1]], weight)))); # change `[0 0 1]` to whatever you want to test. The out put should match the first element of the array
printd(test[0]);

func [[0]] TEST (in [[0]] out [[0]] bweight [[0]]) {
  array test [0 0 0];
  array weight [0 0 0];

  for i = 0, i < 3
    weight[i] = bweight[0][i];
  
  test = NORMALIZE(_EXP(_NEGATE(_DOT(in, bweight))));
  weight = ADD(
    weight, 
    MULTIPLY(
      MULTIPLY(
        DOT(in, SUBTRACT(out, test)), test
      ), 
      DECREMENT(test)
    )
  );
  
  array nweight [[0 0 0]];
  for i = 0, i < 3
    nweight[0][i] = weight[i];

  nweight;
}
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
for i = 0, i < 10, 5 (i) # this will increment `i` by `5` every time
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

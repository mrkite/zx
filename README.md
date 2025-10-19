# zx
Simple commandline calculator

I wrote this because I needed a simple non-nonsense command-line calculator.

# Execution

Run without arguments to get an interactive interface.

```shell
$ zx
zx version 1.0
© Copyright 2025 Sean Kasun
Type "quit" to quit
: 0x723 * 4
7308
: quit
```

Or you can use it in non-interactive modes.  Those non-interactive
modes are invoked by either piping a calculation to the program:
```shell
echo "0x723 * 4" | zx
7308
```

or including the calculation as a command-line argument:
```shell
$ zx 0x723 "*" 4
7308
```

Note that using command-line arguments isn't recommended because you need to escape
symbols like `*` due to your shell treating them as wildcards.

When using zx in a non-interactive mode, all extraneous output is disabled so you can use the results
in a script.

When piping into zx, you can perform multiple calculations with newlines. `$` can be used to reference
the previous result.

For example:
```shell
$ echo -e '2.5 * 3 \n $ ** 2 \n =h \n round $' | zx | tail -1
0x38
```
This first calculates `2.5 * 3`, then takes that result and squares it.  Then it switches the output
to hexadecimal, and rounds the final result.  Finally the whole thing is piped to `tail -1` to get
just the final result, since zx will output the results of each step along the way.

# Usage

Type `help` to get help.

|Input|Description|
|-----|-----------|
|`5 / 2` | integer math, results will be truncated |
|`5. / 2` | floating point math |
|`5 % 2` | integer modulo |
|`5 % 2.5` | floating point remainder |
|`5 ** 2` | exponential |
|`sqrt 5` | square root |
|`sin 0.5` | sine function |
|`cos 0.5` | cosine function |
|`tan 0.5` | tangent funciton |
|`20 \| 7` | bitwise OR |
|`61 & 15` | bitwise AND |
|`61 ^ 85` | bitwise XOR |
|`~56` | bitwise NOT |
|`1 << 4` | bitwise left shift |
|`16 >> 4` | bitwise right shift |
|`pi` | pi constant |
|`0x2e` | hexadecimal numbers start with `0x` |
|`0o755` | octal numbers start with `0o` |
|`0b110` | binary numbers start with `0b` |
|`(4 + 2) * 3` | use parentheses to override order of operations |
|`'a'` | character wrapped in single quotes is converted to its unicode value |
|`=d` | output decimal |
|`=h` | output hex |
|`=o` | output octal |
|`=b` | output binary |
|`=u` | output as unicode character |

# Quirks

Unlike many other calculators, floating point numbers can be input and output in hexadecimal.
Floating point numbers can also be output in octal or binary, but they cannot be input as such.

# Extra

You can use `$` to refer to a previous result, this will make it easier to see the result in
various bases.
```shell
: =h
: 42
0x2a
: =b
: $
0b101010
: =u
: $
'*' 42
```

You can get the value of a unicode character:
```shell
: 'あ'
12354
```

# Compiling

```shell
$ mkdir build
$ cd build
$ cmake ..
$ make
$ make install
```

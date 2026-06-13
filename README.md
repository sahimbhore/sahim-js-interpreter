# Mini JavaScript Interpreter in C++

A small, modular JavaScript interpreter written in modern C++.

This project reads JavaScript-like code from a file or standard input, parses it into an AST, and executes it directly in C++. It does not use Node.js, V8, or any external JavaScript engine.

The goal is not to implement the entire JavaScript language, but to build a clean and extensible interpreter for a useful subset of JavaScript.

---

## Quick Start


First, create an input file named `file.txt`(already created) in the same folder as the C++ source files.

Put the JavaScript code you want to test inside `file.txt`.

Example `file.txt`:

```js
let n = 7;

if (n % 2 === 0) {
  console.log("even");
} else {
  console.log("odd");
}
```
# Follow following steps by copy paste

## 1. Compile the interpreter:(in CMD/powershell terminal)

```bash
g++ -std=c++17 -Wall -Wextra -pedantic main.cpp lexer.cpp parser.cpp interpreter.cpp utils.cpp -o myjs
```


## 2.Run the input file:(after writing js code in file.txt)

#### On Windows CMD/powershell  terminal(VS CODE), run:
```CMD
.\myjs.exe file.txt
```

#### other

```bash
./myjs file.txt
```

Do not double-click or directly run `file.txt`. Always pass it as input to the interpreter.

You can edit `file.txt` whenever you want to test different JavaScript code, then run the command again.

Run using standard input on Linux/macOS:

```bash
./myjs < file.txt
```

Run with debug output on Linux/macOS:

```bash
./myjs file.txt --debug
```

Run with debug output on Windows PowerShell:

```powershell
.\myjs.exe file.txt --debug
```

---

## Project Structure

```text
.
├── main.cpp
├── lexer.h
├── lexer.cpp
├── parser.h
├── parser.cpp
├── interpreter.h
├── interpreter.cpp
├── utils.h
├── utils.cpp
└── README.md
```

---

## Features

### Input and CLI

- Run JavaScript code from a file
- Run JavaScript code through standard input
- Optional debug mode with `--debug`

### Variables

- `let`
- `const`
- Basic primitive values:
  - numbers
  - strings
  - booleans
  - `undefined` internally for missing values

### Operators

- Arithmetic:
  - `+`
  - `-`
  - `*`
  - `**`
  - `/`
  - `%`
- Comparison:
  - `===`
  - `!==`
  - `>`
  - `<`
  - `>=`
  - `<=`
- Logical:
  - `&&`
  - `||`
  - `!`
- Increment and decrement:
  - `i++`
  - `i--`
- Compound assignment:
  - `+=`
  - `-=`
  - `*=`
  - `/=`
  - `%=`

### Control Flow

- `if`
- `else if`
- `else`
- `while`
- `for`

### Functions

- Function declarations
- Parameters
- Function calls
- `return`
- Function-local scope through chained environments

### Arrays

Supported array features:

- Array literals, for example `[1, 2, 3]`
- Index access, for example `arr[0]`
- Indexed assignment, for example `arr[1] = 10`
- Spread inside array literals, for example `[...arr]`
- `.length`
- `.push()`
- `.pop()`
- `.reverse()`
- `.join()`
- `.includes()`

### Strings

Supported string features:

- String literals with single or double quotes
- String concatenation with `+`
- Index access, for example `name[0]`
- `.length`
- `.split()`
- `.substring()`
- `.includes()`

### Built-in Objects

Implemented built-ins:

- `console.log()`
- `Math.floor()`
- `Math.ceil()`
- `Math.round()`
- `Math.abs()`
- `Math.max()`
- `Math.min()`
- `Math.pow()`

---

## Architecture
file.js  →  your C++ interpreter (myjs.exe)  →  output in terminal
The interpreter is split into simple, focused modules.

### Lexer

Files:

- `lexer.h`
- `lexer.cpp`

The lexer reads raw source code and converts it into tokens such as identifiers, numbers, strings, keywords, operators, and punctuation.

It also tracks line numbers so that syntax and runtime errors can point to the correct location.

### Parser

Files:

- `parser.h`
- `parser.cpp`

The parser converts tokens into an Abstract Syntax Tree.

It supports statements such as variable declarations, blocks, loops, conditionals, function declarations, returns, and expression statements.

Expressions are parsed with operator precedence, including arithmetic, comparison, logical operators, calls, member access, array literals, and assignment.

### Interpreter

Files:

- `interpreter.h`
- `interpreter.cpp`

The interpreter walks the AST and executes it.

It manages runtime values, scopes, function calls, arrays, strings, built-in objects, and error handling.

### Utilities

Files:

- `utils.h`
- `utils.cpp`

Utility code includes structured error reporting and number formatting.

---

## How to Compile

Use a C++17 compiler:

```bash
g++ -std=c++17 -Wall -Wextra -pedantic main.cpp lexer.cpp parser.cpp interpreter.cpp utils.cpp -o myjs
```

On Windows, you may build:

```bash
g++ -std=c++17 -Wall -Wextra -pedantic main.cpp lexer.cpp parser.cpp interpreter.cpp utils.cpp -o myjs.exe
```

---


## Example

### Input

```js
function isArmstrong(n) {
  let original = n;
  let sum = 0;

  while (n > 0) {
    let digit = n % 10;
    sum = sum + digit * digit * digit;
    n = Math.floor(n / 10);
  }

  return sum === original;
}

let value = 153;

if (isArmstrong(value)) {
  console.log("armstrong");
} else {
  console.log("not armstrong");
}
```

### Output

```text
armstrong
```

Another example:

```js
let word = "madam";
let reversed = word.split("").reverse().join("");

console.log(reversed === word);
```

Output:

```text
true
```

---

## Error Handling

The interpreter uses structured errors instead of crashing.

Errors are reported in a clean format:

```text
Error: TypeError at line 1: invalid operation between string and number
```

Implemented error categories include:

- `SyntaxError`
- `ReferenceError`
- `TypeError`
- `RuntimeError`

Examples of handled errors:

### Undefined Variable

```js
console.log(x);
```

Output:

```text
Error: ReferenceError at line 1: variable 'x' is not defined
```

### Invalid Type Operation

```js
console.log("hello" - 2);
```

Output:

```text
Error: TypeError at line 1: invalid operation between string and number
```

### Division by Zero

```js
console.log(10 / 0);
```

Output:

```text
Error: RuntimeError at line 1: division by zero
```

### Array Index Out of Bounds

```js
let a = [1, 2];
console.log(a[5]);
```

Output:

```text
Error: RuntimeError at line 2: array index out of bounds
```

The interpreter also includes a basic loop guard to stop likely infinite loops.

---

## Limitations

This is a mini interpreter, not a full JavaScript runtime.

Not currently supported:

- `var`
- arrow functions
- classes
- objects with custom properties
- object literals like `{ name: "value" }`
- `new`
- `this`
- `break` and `continue`
- `switch`
- `try/catch/finally`
- `null`
- `NaN` as a source-level literal
- loose equality `==` and `!=`
- prefix increment/decrement like `++i`
- template literals
- regular expressions
- async/await and promises
- modules/import/export
- full JavaScript automatic semicolon insertion
- browser APIs
- Node.js APIs

Some behavior is intentionally simplified. For example, array and string methods only support the subset implemented in the interpreter.

---

## Why This Project Is Interesting

This project demonstrates the core pieces of a real language runtime in a compact and readable way.

It includes lexical analysis, parsing, AST construction, scoped execution, function calls, built-in runtime behavior, and structured error reporting. The design is intentionally modular, so new syntax, operators, built-ins, and runtime types can be added without rewriting the whole system.

It is a small project, but it follows the same broad pipeline used by much larger interpreters and language engines.

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://https://github.com/Melkor-1/Filter/edit/main/LICENSE)

# litc - Simple Literate Programming System

Inspired from [Literate programming in Haskell](https://wiki.haskell.org/Literate_programming).

`litc` is a Literate script filter. A literate script is a program in which comments are given the leading role, whilst program text must be explicitly flagged as such by placing it between `\begin{code}` and `\end{code}` in the first column of each line.

`litc` is a filter that can be used to strip away all of the comment lines out a literate script file.

The default code markers are `\begin{code}` and `\end{code}`, which can be changed with command-line arguments, provided that the begin and end markers are nonsimilar.

The following is a sample literate script:
```latex
Some text speaking about the code:

\begin{code}
#include <stdio.h>

int main(void) {
    puts("Hello World!");
}
\end{code}

Some more text..
```
The program would convert this to:

```
#include <stdio.h>

int main(void) {
    puts("Hello World!");
}
```

## Building

To build, simply do:

```bash
git clone --depth 1 https://github.com/Melkor-1/litc
make litc
```

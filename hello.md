This C program shalt output "Hello World!":

```c
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    puts("Hello World!");
    return EXIT_SUCCESS;
}
```

Note that `puts()` automatically appends a newline at the end, and
`EXIT_SUCCESS` from `stdlib.h` is simply a mnemonic for 0, but is
self-documentary.

We could also have used `printf()` instead of `puts()`.


# sizeof

`sizeof` prints the size of a C type.

It does not have a list of type sizes.

It asks the C compiler.

## Build

```sh
make
```

## Install

```sh
sudo make install
```

This installs:

```text
/usr/bin/sizeof
/etc/sizeof/headers.csrc
```

To remove it:

```sh
sudo make uninstall
```

## Usage

```sh
sizeof TYPE
```

Examples:

```sh
sizeof int
sizeof size_t
sizeof uint64_t
sizeof struct foo
```

## Headers

`sizeof` uses:

```text
/etc/sizeof/headers.csrc
```

as its default header list.

The file contains one header per line:

```text
stdint.h
stddef.h
stdbool.h
```

These become:

```c
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
```

A different header list can be used with:

```sh
sizeof -H headers.csrc uint64_t
```

or:

```sh
sizeof --add-headers-list headers.csrc uint64_t
```

Additional headers can be passed directly:

```sh
sizeof -i stdint.h uint64_t
sizeof -I myheader.h my_type
```

## How it works

`sizeof` generates a small C program containing:

```c
sizeof(TYPE)
```

The source is sent to `cc` through a pipe.

The compiler output is written to a `memfd` and executed with `fexecve()`.

No temporary files are created.

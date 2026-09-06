# cottage
A cosy HTTP web framework, written in C.

## Installation

### Requirements

Before you can compile use cottage, run:

- **Debian/Ubuntu/Mint**
  ```
  apt install build-essential git
  ```
- **Arch**
  ```
  pacman -S base-devel git
  ```
- **Fedora**
  ```
  sudo dnf install @development-tools git
  ```
In other words, ensure you have a C compiler such as GCC or Clang to build the project, and git to clone into cottage.

### Including cottage

cottage is a multi-header file framework, so all you need to do is clone it into your repository.
```
git clone https://github.com/vixthevix/cottage.git
```
Then, to include cottage into your main file:
```
#define COTTAGE_START //To enable cottage function definitions.
#include "cottage/cottage.h"
```

### Build

Assuming your main executable source is `main.c`:

- **GCC**
  ```
  gcc main.c -o main -lm
  ```

## More information
### Background
cottage began as a project to learn how to run a simple HTTP server in C, previously called `serversource`. I then decided to lose my mind and turn it into a 'cosy' web framework.

cottage includes many features, including:
- Custom HTML tags for:
  - Variable insertion
  - Components
  - Conditional HTML
  - Looped HTML
- Setting up a multi-client server
- Routing
- HTTP request deconstruction for custom analysis

In general, cottage is more of a passion project, rather than a serious framework that will compete with titans like NextJS and Svelte. I personally plan to use it for my own website and [IVnet](https://github.com/vixthevix/IVnet).

### What's next?

- A lot of things, I have not gotten around to geting this sorted yet, but things like adding a proper documentation file, TRY CATCH and windows support for example. I will put this stuff here when I get to it.


All in all, I hope you enjoy using cottage!

-vixthevix, a Human from Earth.
## 🐚 UNIX Shell

<p align="center">
  <img src="https://img.shields.io/badge/Language-C-blue.svg" />
  <img src="https://img.shields.io/badge/Platform-UNIX%2FLinux-lightgrey" />
  <img src="https://img.shields.io/badge/Compiler-GCC-success" />
  <img src="https://img.shields.io/badge/Status-Stable-brightgreen" />
</p>

A lightweight, custom UNIX-style command-line shell built using **C**

### ✨ Features
- Executing external programs `cat, touch, grep` etc.
```bash
cat input.txt
```
- Handling I/O redirection `<,>`
```bash
echo "Hello World" > input.txt
```
- Pipelines (single '|' support) `cmd1 | cmd2` ~~`| cmd3`~~ ~~`| cmd4`~~
```bash
ls | grep "main"
```
- Background Execution
```bash
ls &
```
- Built-in support `cd, exit, pwd`
```bash
cd test_dir
```
**all without relying on another shell.**

### 🛠️ Compile with
```bash
gcc main.c builtin.c global.c -o a
```

### ⭐ Support

If you found this project helpful or educational, consider giving it a ⭐ on GitHub!
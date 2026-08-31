## Templates

Templates let the compiler generate type-specific code from a single definition & are instantiated at compile time with zero runtime overhead.

```sh
➜  templates git:(main) ✗ make run
clang++ -std=c++17 -Wall -Wextra -pthread -o templates_example main.cpp
./templates_example
func template
  max(3, 7) = 7
  max(3.14, 2.71) = 3.14
class template
  Box<int> = 42
  Box<string> = hello
ptr template
  Box<int*> points to 99
non-type param
  FixedArray size = 3 values: 10 20 30
```

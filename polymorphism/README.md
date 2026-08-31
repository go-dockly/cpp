## Polymorphism (virtual functions)

Runtime polymorphism via virtual functions.

- Call through a base reference/pointer then the most-derived override is executed.
- Prefer `override` & `final` for clarity & safety.
- Abstract interfaces + `std::unique_ptr` / `std::shared_ptr` is the modern way to own heterogeneous collections.
- Dynamic dispatch has a small cost so use static polymorphism or `std::variant` if performance is needed.

```sh
➜  polymorphism git:(main) ✗ make run
clang++ -std=c++17 -Wall -Wextra -pthread -o polymorphism_example main.cpp
./polymorphism_example
static
  circle r=2
  rect 3x4
polymorphic via reference
  area = 12.5664
  area = 12
polymorphic via unique_ptr + vector
  circle r=1.5
  area = 7.06858
  rect 2x5
  area = 10
  circle r=3
  area = 28.2743
```
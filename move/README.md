

## Move example

```sh
➜  ptr git:(main) ✗ make run     
clang++ -std=c++17 -Wall -Wextra -pthread -o move_example main.cpp
./move_example
move construct
  construct A
  MOVE A
move into container
  construct temp
  MOVE temp
  construct direct
  MOVE temp
perfect forwarding
  construct from string
  construct from lvalue
  construct from lvalue
passing to func
  MOVE from string
  took ownership of from string
  destroy from string
  destroy from lvalue
  destroy from lvalue
  destroy direct
  destroy temp
  destroy A
```


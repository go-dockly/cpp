

## Structured bindings
Pairs, tuples, structs, arrays, map iteration, references

```sh
➜  cast git:(main) ✗ make run     
clang++ -std=c++17 -Wall -Wextra -pthread -o bindings_example main.cpp
./bindings_example
Alice is 30
42 3.14 pi
Point: 10,20
1 2 3
Bob → 90
Carol → 85
modified Point: 100,20
id=42 label=pi
```
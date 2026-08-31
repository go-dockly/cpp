## Containers (vector, map, unordered_map)

- Prefer `vector` over raw arrays or `new[]`.
- `map` keeps keys sorted; `unordered_map` & is usually faster for lookup.
- Structured bindings (`auto& [k, v]`) make iteration clean.
- `emplace` / `emplace_back` construct in-place and avoid temp objs.
- Capacity growth is geometric amortised O(1) `push_back`.

```sh
➜  containers git:(main) ✗ make run
clang++ -std=c++17 -Wall -Wextra -pthread -o containers_example main.cpp
./containers_example
vector
  size=5 capacity=6
  values: 1 2 3 4 5 
  after push 0  size=1 capacity=1
  after push 1  size=2 capacity=2
  after push 2  size=3 capacity=4
  after push 3  size=4 capacity=4
  after push 4  size=5 capacity=8
  after push 5  size=6 capacity=8
  after push 6  size=7 capacity=8
  after push 7  size=8 capacity=8
  after push 8  size=9 capacity=16
  after push 9  size=10 capacity=16
map (ordered by key)
  alice → 30
  bob → 25
  carol → 28
unordered_map (hash)
  carol → 92
  bob → 87
  alice → 95
```

### When to use which

| Container | Ordered? | Lookup | Best for |
|-----------|----------|--------|----------|
| `vector` | yes (insertion order) | O(1) random access | Default sequential container |
| `map` | yes (by key) | O(log n) | Sorted keys, range queries |
| `unordered_map` | no | Average O(1) | Fast key lookup, no order needed |

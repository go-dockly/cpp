

## Hazard pointers
for high-performance concurrent data structures without the overhead of reference counting on every access.
This example demonstrates the concept of protecting a pointer and deferred reclamation (simplified single hazard pointer per thread, global scan, no thread-local storage optimisation)

### The Problem Hazard Pointers Solve

In lock-free concurrent data structures (lock-free stack, queues, lists, hash tables, etc) a thread can read a pointer to a node then another thread can unlink and free that node. The first thread now has a dangling pointer → **use-after-free**.

`std::shared_ptr` / reference counting solves this, but every access does atomic reference-count updates. Under high contention those atomics become a scalability bottleneck.

Hazard pointers give **safe memory reclamation without per-access reference counting**.

### How hazard pointers work

1. Each thread owns a small set of **hazard pointers** (usually 1–4)
2. Before a thread dereferences a pointer `p`, it **publishes** `p` into one of its hazard pointers (an atomic store)
3. When a thread wants to free a node, it does **not** free it immediately. It puts the node on a **retirement list**
4. Periodically (or when the retirement list grows) the thread **scans** all hazard pointers of all threads. Any node that still appears in a hazard pointer is kept; the others are safely deleted

This guarantees that a node is never freed while another thread still has it in a hazard pointer.

### Benefits

| Benefit | How it matters |
|---------|----------------|
| **No per-access atomic ref-count** | Much better scalability under high contention than `shared_ptr` |
| **Bounded memory** | Retired nodes are reclaimed as soon as no hazard pointer protects them (unlike some epoch schemes that can delay reclamation indefinitely) |
| **Lock-free friendly** | Works with pure lock-free algorithms |
| **Simple correctness argument** | “If I published the pointer before using it, no one can free it” |
| **Works with C++** | Can be implemented with `std::atomic` only (no special language support required) |

Trade-off: pay a small cost when **retiring** nodes (scanning), not on every read. For read-heavy structures this is usually a win.

### Key Take-aways from Example

- `protect()` publishes the pointer **before** using it.
- The CAS loop re-checks the pointer after publishing (hazard-pointer pattern).
- Nodes are never deleted while they appear in any hazard pointer.
- Reclamation is deferred and batched so the cost is paid on the write/retire path, not on each read.

### When to Prefer Hazard Pointers over `shared_ptr`

| Situation | Prefer |
|-----------|--------|
| High-contention lock-free structure | Hazard pointers |
| Simple ownership, low contention, or single-threaded | `unique_ptr` / `shared_ptr` |
| Need automatic lifetime management with cycles | `shared_ptr` + `weak_ptr` |
| Read-mostly concurrent data | Hazard pointers (or RCU / epoch-based schemes) |

```sh
➜  ptr_smart git:(main) ✗ make run     
clang++ -std=c++17 -Wall -Wextra -pthread -o ptr_hazard_example main.cpp
./ptr_hazard_example
Created Entity 0
Created Entity 1
Entity value = 1
Created Entity 2
Entity value = 2
Created Entity 3
Created Entity 4
Entity value = 4
Created Entity 5
Destroyed Entity 1
Destroyed Entity 2
Destroyed Entity 4
Destroyed Entity 5
Entity value = 0
Created Entity 6
Entity value = 6
Created Entity 7
Entity value = 7
Entity value = 3
Destroyed Entity 6
Destroyed Entity 7
Destroyed Entity 3
Done.
```


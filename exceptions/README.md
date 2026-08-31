## RAII + Exceptions

Resource Acquisition Is Initialization: bind resource lifetime to object lifetime so cleanup is automatic, even when exceptions are thrown.

- Prefer RAII wrappers (`std::unique_ptr`, `std::lock_guard`, `std::fstream`, custom class) over naked resource handles
- Destructors should not throw (or program may call `std::terminate`)
- Combine with Rule of 0/3/5: if class owns a resource
- Exceptions + RAII give strong exception-safety guarantee almost for free

```sh
➜  exceptions git:(main) ✗ make run
clang++ -std=c++17 -Wall -Wextra -pthread -o exceptions_example main.cpp
./exceptions_example
normal path
  opened data.txt
  wrote "hello" to data.txt
  wrote "world" to data.txt
  closed data.txt
exception path RAII still cleans up
  opened data.txt
  wrote "hello" to data.txt
  closed data.txt
  caught: something went wrong
exit
```

### The problem RAII solves

Manual resource management (new/delete, fopen/fclose, lock/unlock) is error-prone. An early `return` or an exception could skip release call & leak or deadlock.

1. Acquire resource in the constructor
2. Release in destructor
3. C++ guarantees the destructor runs when the object leaves scope (normal exit or stack unwinding caused by an exception)
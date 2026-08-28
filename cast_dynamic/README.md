

## Dynamic vs Static Casting

```sh
➜  cast git:(main) ✗ make run     
clang++ -std=c++17 -Wall -Wextra -pthread -o cast_dynamic_example main.cpp
./cast_dynamic_example
Cast failed: enemy is not a Player!
```

[![73](https://img.youtube.com/vi/CiHfz6pTolQ/maxresdefault.jpg)](https://www.youtube.com/watch?v=CiHfz6pTolQ&list=PLlrATfBNZ98dudnM48yfGUldqGD0S4FFb&index=66)

### Comparison

| Feature | `dynamic_cast` | `static_cast` |
| --- | --- | --- |
| **Check Type** | Runtime (RTTI) | Compile-time |
| **Requirement** | Polymorphic base class (at least 1 `virtual` function) | Valid conversion path exists |
| **Failed Pointer Cast** | Returns `nullptr` | Undefined behavior if type is wrong |
| **Failed Reference Cast** | Throws `std::bad_cast` | Undefined behavior |
| **Performance** | Slower (requires vtable lookup) | Zero overhead (identical to st&ard C cast) |

---

### When to use which

#### 1. Safe Downcasting -> `dynamic_cast`

When you **don't know** the exact derived type at runtime & need to safely attempt a conversion.

```cpp
void processEntity(Entity* e) {
    // Safely check if the entity is actually a Player
    if (Player* p = dynamic_cast<Player*>(e)) {
        p->heal();
    } else {
        // H&le non-player entities
    }
}

```

#### 2. Guaranteed Downcasting -> `static_cast`

When you **know** the derived type (eg via an internal `Type` enum or application logic) & want to skip runtime check

```cpp
void updatePlayer(Entity* e) {
    // Fast, but crashes/corrupts memory if 'e' isn't actually a Player!
    Player* p = static_cast<Player*>(e);
    p->heal();
}

```

#### 3. Std Conversions -> `static_cast`

Use for non-polymorphic conversions, such as numeric conversions or casting `void*` back to concrete pointer

```cpp
double pi = 3.14159;
int rounded = static_cast<int>(pi); // convert double to int

void* rawBuffer = getBuffer();
int* data = static_cast<int*>(rawBuffer); // convert void* to int*

```

* **`dynamic_cast**` default for polymorphic downcasts when safety & dynamic type checking required
* **`static_cast**` use in performance-critical code loops only when the type is guaranteed through prior logic or for static conversions (eg primitives)
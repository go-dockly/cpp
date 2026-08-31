

## Lambdas

| Feature              | Syntax / Keyword          | Purpose                              |
|----------------------|---------------------------|--------------------------------------|
| Capture by value     | `[=]`                     | Copy variables into the lambda       |
| Capture by reference | `[&]`                     | Refer to original variables          |
| Init-capture         | `[z = expr]`              | Create and initialize a new variable |
| Generic lambda       | `[](auto a, auto b)`      | Template-like params.                |
| Mutable              | `mutable`                 | Allow modification of by-value captures |
| Predicate            | lambda passed to algorithm| Custom comparison / filtering logic  |
| Recursive            | `std::function` + `[&]`   | Enable self-calls                    |

### Basic lambda
```cpp
auto greet = []() { std::cout << "Hello from lambda\n"; };
greet();
```
lambda with no captures and no params
```
Hello from lambda
```

### Capture by value vs reference
```cpp
int x = 10;
auto by_value = [=]() { std::cout << "x by value: " << x << "\n"; };
auto by_ref   = [&]() { x += 5; std::cout << "x by ref: " << x << "\n"; };
by_value();
by_ref();
```
- `[=]` captures everything by **value** (a copy of `x` is stored inside the lambda).  
- `[&]` captures everything by **reference**. The lambda can modify the original `x`.  

### Init-capture - C++14
```cpp
int y = 20;
auto init_cap = [z = y + 1]() { std::cout << "init-capture z: " << z << "\n"; };
init_cap();
```
Init-captures to declare and initialize a new variable inside the capture list.  
`z` is created with the value `21`. Output:
```
init-capture z: 21
```

### Generic lambda - C++14
```cpp
auto add = [](auto a, auto b) { return a + b; };
std::cout << "generic: " << add(3, 4.5) << "\n";
```
Params of type `auto` make the lambda a **generic** (templated) callable.  
`3` + `4.5` or int promotes to double:
```
generic: 7.5
```

### Mutable lambda
```cpp
auto counter = [n = 0]() mutable { return ++n; };
std::cout << "counter: " << counter() << " " << counter() << "\n";
```
By default a lambda’s `operator()` is `const`, so captured-by-value vars can't be modified.  
The `mutable` keyword removes that. This allows the lambda to change its own copy of `n`.

- First call: `n` becomes 1 → returns 1  
- Second call: `n` becomes 2 → returns 2  

Output:
```
counter: 1 2
```

### Lambda as a sorting predicate
```cpp
std::vector<int> v{5, 1, 8, 3, 9};
std::sort(v.begin(), v.end(), [](int a, int b) { return a > b; });
```
The lambda is passed as a custom comparator to `std::sort`.  
`a > b` produces a **descending** order.
```
9 8 5 3 1 
```

### Recursive lambda
```cpp
std::function<int(int)> fib = [&](int n) {
    return n <= 1 ? n : fib(n-1) + fib(n-2);
};
std::cout << "fib(6): " << fib(6) << "\n";
```
Lambda can't directly refer to itself by name. Wrapping it in `std::function` and capturing `std::function` by reference gets recursion.


```sh
➜  cast git:(main) ✗ make run     
clang++ -std=c++17 -Wall -Wextra -pthread -o lambda_example main.cpp
./lambda_example
Hello from lambda
x by value: 10
x by ref: 15
init-capture z: 21
generic: 7.5
counter: 1 2
9 8 5 3 1 
fib(6): 8
```


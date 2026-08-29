

## Async

```sh
➜  cast git:(main) ✗ make run     
clang++ -std=c++17 -Wall -Wextra -pthread -o async_example main.cpp
./async_example
Launching async task...
Do other work while task_long runs...
Working... 1/5
Starting task_long (3 s)...
Working... 2/5
Working... 3/5
Working... 4/5
Working... 5/5
Waiting for async result...
task_long finished!
Received: "Result from async task_long"
```

[![79](https://img.youtube.com/vi/5HWCsmE9DrE/maxresdefault.jpg)](https://www.youtube.com/watch?v=5HWCsmE9DrE&list=PLlrATfBNZ98dudnM48yfGUldqGD0S4FFb&index=79)
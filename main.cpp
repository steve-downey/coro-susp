#include <experimental/coroutine>
#include <future>
#include <iostream>

template <typename R>
struct suspension {
  struct promise_type;
  using handle = std::experimental::coroutine_handle<promise_type>;

  struct promise_type {
    R result;
    using suspend_always = std::experimental::suspend_always;
    using suspend_never  = std::experimental::suspend_never;

    suspend_always initial_suspend() {
      return {};
    }

    suspend_always final_suspend() {
      return {};
    }

    suspension get_return_object() {
      return suspension{handle::from_promise(*this)};
    }

    void return_value(R const& r) {
      std::cout << "return value: " << r << '\n';
      result = r;
    }
  };

  operator R() {
    if (!_coro || _coro.done()) {
      return _r;
    }
    std::cout << "pre-resume\n";
    _coro.resume();
    std::cout << "after resume\n";
    _r = _coro.promise().result;
    std::cout << "about to destroy coro\n";
    std::cout << "Pre Coro=" << (bool)_coro << '\n';
    //
    _coro.destroy();
    _coro = nullptr;
    std::cout << "Post Coro=" << (bool)_coro << '\n';
    return _r;
  }

  suspension(handle h) : _coro(h) {
    std::cout << "cons by handle\n";
  }

  suspension() : _coro(nullptr) {
    std::cout << "default cons\n";
  }

  suspension(suspension&& rhs) : _coro(rhs._coro) {
    rhs._coro = nullptr;
    std::cout << "move cons\n";
  }

  suspension(suspension const&) = delete;

  ~suspension() {
    std::cout << "Destroy Coro=" << (bool)_coro << '\n';
    if (_coro) {
      _coro.destroy();
    }
  }

  handle _coro;
  R      _r;
};

template <typename F, typename... Args>
auto suspend(F&& f, Args&&... args)
    -> suspension<typename std::result_of_t<F(Args...)>> {
  std::cout << "Args:";
  (std::cout << ... << args) << '\n';
  std::cout << "before co_return\n";
  co_return std::invoke(f, args...);
}

int f(int i) {
  std::cout << "i:" << i << '\n';
  return i;
}

int main(int, char**) {
  std::cout << "main\n";
  std::cout << "about to create suspension\n";
  auto s = suspend(f, 2);
  std::cout << "after suspension\n";
  //    int i = s;
  std::cout << s << '\n';

  auto s3 = suspend(f, 3);
  return s;
}

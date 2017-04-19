#include <experimental/coroutine>
#include <future>
#include <iostream>
#include <functional>
#include <atomic>
#include <mutex>

template <typename R>
struct suspension {

  struct promise_type;
  using handle = std::experimental::coroutine_handle<promise_type>;

  typedef typename std::aligned_storage<sizeof(R),
                                        alignof(R)>::type
  Storage;

  struct promise_type {
    Storage result;
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
      //      *reinterpret_cast<R*>(&result) = r;
      ::new (&result) R(r);
    }
    void unhandled_exception() {
      std::terminate();
    }
  };


  operator R const&() {
    return get();
  }

  R const& get() const {
    int evaled = evaled_.load(std::memory_order_acquire);
    if (!evaled) {
      std::unique_lock<std::mutex> guard(lock_);
      if (!evaled_) {
        _coro.resume();
        *reinterpret_cast<R*>(&result_) =
          std::move(*reinterpret_cast<R*>(&_coro.promise().result));
        _coro.destroy();
        _coro = nullptr;
        evaled_.store(1, std::memory_order_release);
      }
    }
    return *reinterpret_cast<R*>(&result_);
  }

  suspension(handle h) : _coro(h) {
  }

  suspension() : _coro(nullptr) {
  }

  suspension(suspension&& rhs) : _coro(rhs._coro) {
    rhs._coro = nullptr;
  }

  suspension(suspension const&) = delete;

  ~suspension() {
    if (_coro) {
      _coro.destroy();
    }
  }


  mutable Storage result_;
  mutable std::atomic_int evaled_;
  mutable std::mutex lock_;
  mutable handle _coro;
};

template <typename F, typename... Args>
auto suspend(F&& f, Args&&... args)
    -> suspension<typename std::result_of_t<F(Args...)>> {
  co_return std::invoke(f, args...);
}

int f(int i) {
  //  std::cout << "i:" << i << '\n';
  return i;
}

namespace {
struct NoDefault
{
  int _i;
 public:
  NoDefault(int i) : _i(i) {}
  NoDefault() = delete;
};

}

NoDefault noDefault() {
  return NoDefault{7};
}

int main(int, char**) {
  // std::cout << "main\n";
  // std::cout << "about to create suspension\n";
  auto s = suspend(f, 2);
  // std::cout << "after suspension\n";
  //    int i = s;
  //std::cout << s << '\n';

  // std::cout << "about to create suspension3\n";
  auto s3 = suspend(f, 3);
  // std::cout << "after suspension\n";

  // std::cout << "about to create suspension4\n";
  auto s4 = suspend(noDefault);
  //  std::cout << "after suspension\n";
  //std::cout << s4.get()._i << '\n';

  return s + s4.get()._i;
}

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

  class state {
   private:
    Storage result_;
   public:
    state() {std::cout << "state\n";}
    ~state() {std::cout << "~state\n";}

    template <class Arg>
    void set_value(Arg&& arg) {
      ::new(&result_) R(std::forward<Arg>(arg));
    }

    typename std::add_lvalue_reference<R>::type
    copy(){
      return *reinterpret_cast<R*>(&result_);
    }

  };

  struct promise_type {
    std::shared_ptr<state> stateX_;
    using suspend_always = std::experimental::suspend_always;
    using suspend_never  = std::experimental::suspend_never;

    promise_type() : stateX_(nullptr) {}

    void set_state(std::shared_ptr<state> state) {
      stateX_ = state;
    }

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
      stateX_->set_value(r);
    }

    void return_value(R&& r) {
      stateX_->set_value(r);
    }

    void unhandled_exception() {
      std::terminate();
    }
  };


  operator R const&() {
    std::cout << "conv\n";
    return get();
  }

  R const& get() const {
    int evaled = evaled_.load(std::memory_order_acquire);
    std::cout << "get evaled: " << evaled_ << "\n";
    if (!evaled) {
      std::unique_lock<std::mutex> guard(lock_);
      std::cout << "got lock!\n";
      if (!evaled_) {
        _coro.resume();
        std::cout << "resume!\n";
        _coro.destroy();
        std::cout << "destroy!\n";
        _coro = nullptr;
        std::cout << "null!\n";
        evaled_.store(1, std::memory_order_release);
        std::cout << "stored!\n";
      }
    }
    return state_->copy();
  }

  suspension(handle h) : _coro(h),
                         evaled_(0),
                         state_(new state),
                         lock_()
  {
    _coro.promise().set_state(state_);
  }

  suspension() : _coro(nullptr),
                 evaled_(0),
                 state(nullptr),
                 lock_()
  {
  }

  suspension(suspension&& rhs)
  : _coro(std::move(rhs._coro)),
    evaled_(0),
    state_(std::move(rhs.state_)),
    lock_()
  {
    rhs._coro = nullptr;
  }

  suspension(suspension const&) = delete;

  ~suspension() {
    if (_coro) {
      _coro.destroy();
    }
  }


  mutable handle _coro;
  mutable std::atomic_int evaled_;
  mutable std::shared_ptr<state> state_;
  mutable std::mutex lock_;
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
  int i = s;
  std::cout << s << '\n';
  std::cout << s << '\n';

  // std::cout << "about to create suspension3\n";
  auto s3 = suspend(f, 3);
  // std::cout << "after suspension\n";

  // std::cout << "about to create suspension4\n";
  auto s4 = suspend(noDefault);
  //  std::cout << "after suspension\n";
  std::cout << s4.get()._i << '\n';

  //  return s + s4.get()._i;
}

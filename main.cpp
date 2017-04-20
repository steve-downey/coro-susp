#include <atomic>
#include <experimental/coroutine>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>

template <typename R>
struct suspension {

  struct promise_type;
  using handle = std::experimental::coroutine_handle<promise_type>;

  typedef typename std::aligned_storage<sizeof(R), alignof(R)>::type Storage;

  class state
  {
  private:
    Storage         result_;
    std::atomic_int evaled_;
    std::mutex      lock_;
    handle          coro_;

  public:
    state(handle const& h) : evaled_(0), coro_(h)
    {
    }

    state(state const&) = delete;

    ~state()
    {
      if (coro_) {
        coro_.destroy();
      }
    }

    template <class Arg>
    void set_value(Arg&& arg)
    {
      ::new (&result_) R(std::forward<Arg>(arg));
    }

    typename std::add_lvalue_reference<R>::type copy()
    {
      int evaled = evaled_.load(std::memory_order_acquire);
      if (!evaled) {
        std::unique_lock<std::mutex> guard(lock_);
        if (!evaled_) {
          coro_.resume();
          coro_.destroy();
          coro_ = nullptr;
          evaled_.store(1, std::memory_order_release);
        }
      }

      return *reinterpret_cast<R*>(&result_);
    }
  };

  struct promise_type {
    state* state_;
    using suspend_always = std::experimental::suspend_always;
    using suspend_never  = std::experimental::suspend_never;

    promise_type() : state_(nullptr)
    {
    }

    ~promise_type()
    {
    }

    suspend_always initial_suspend()
    {
      return {};
    }

    suspend_always final_suspend()
    {
      return {};
    }

    suspension get_return_object()
    {
      auto s = suspension{handle::from_promise(*this)};
      state_ = s.state_.get();
      return s;
    }

    void return_value(R const& r)
    {
      state_->set_value(r);
    }

    void return_value(R&& r)
    {
      state_->set_value(r);
    }

    void unhandled_exception()
    {
      std::terminate();
    }
  };

  operator R const&()
  {
    return get();
  }

  R const& get() const
  {
    return state_->copy();
  }

  suspension(handle h) : state_(new state(h))
  {
  }

  suspension() : state_(nullptr)
  {
  }

  suspension(suspension const&) = default;
  suspension(suspension&&)      = default;

  ~suspension() = default;

  mutable std::shared_ptr<state> state_;
};

template <typename F, typename... Args>
auto suspend(F&& f, Args&&... args)
    -> suspension<typename std::result_of_t<F(Args...)>>
{
  // std::cout << "capture args: ";
  // (std::cout << ... << args) << '\n';
  co_return std::invoke(f, args...);
}

int f(int i)
{
  //  std::cout << "i:" << i << '\n';
  return i;
}

namespace {
struct NoDefault {
  int _i;

public:
  NoDefault(int i) : _i(i)
  {
  }
  NoDefault() = delete;
};

std::ostream& operator<<(std::ostream& os, const NoDefault& v)
{
  return (os << "v._i=" << v._i);
}
}

NoDefault noDefault()
{
  return NoDefault{7};
}

suspension<int> getSusp(int i)
{
  //  std::cout << "12?\n";
  auto s = suspend(f, i);
  //  std::cout << "force:" << s << '\n';
  return s;
}

int main(int, char**)
{
  // std::cout << "main\n";
  // std::cout << "about to create suspension\n";
  auto s = suspend(f, 2);
  // std::cout << "after suspension\n";
  int i = s;
  std::cout << i << '\n';
  std::cout << s << '\n';

  // std::cout << "about to create suspension3\n";
  auto s3 = suspend(f, 3);
  // std::cout << "after suspension\n";

  // std::cout << "about to create suspension4\n";
  auto s4 = suspend(noDefault);
  //  std::cout << "after suspension\n";
  std::cout << s4.get()._i << '\n';

  auto s12 = getSusp(12);
  std::cout << s12 << '\n';

  auto s12a = s12;
  std::cout << s12a << '\n';

  auto s17  = getSusp(17);
  auto s17a = s17;
  std::cout << s17 << '\n';
  std::cout << s17a << '\n';

  //  return s + s4.get()._i;
}

// suspension.h                                                       -*-C++-*-
#ifndef INCLUDED_SUSPENSION
#define INCLUDED_SUSPENSION

#include <experimental/coroutine>
#include <iostream>
#include <mutex>

namespace suspension {

template <typename R>
class suspension
{
public:
  struct promise_type;

private:
  using handle = std::experimental::coroutine_handle<promise_type>;

  typedef std::aligned_storage_t<sizeof(R), alignof(R)> Storage;

  class state
  {
  private:
    Storage         result_;
    std::atomic_int evaled_;
    std::mutex      lock_;
    handle          coro_;

  public:
    state(promise_type* promise)
        : evaled_(0), coro_(handle::from_promise(*promise))
    {
    }

    state(state const&) = delete;

    state(R const& arg)
    {
      std::unique_lock<std::mutex> guard(lock_);
      ::new (&result_) R(arg);
      evaled_.store(1, std::memory_order_release);
    }

    state(R&& arg)
    {
      std::unique_lock<std::mutex> guard(lock_);
      ::new (&result_) R(std::move(arg));
      evaled_.store(1, std::memory_order_release);
    }

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

    typename std::add_lvalue_reference<R>::type eval()
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

    int is_evaled()
    {
      return evaled_;
    }
  };

public:
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
      auto s = suspension{this};
      state_ = s.state_.get();
      return s;
    }

    void return_value(R const& r)
    {
      state_->set_value(r);
    }

    void return_value(R&& r)
    {
      state_->set_value(std::move(r));
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
    return state_->eval();
  }

  bool is_forced()
  {
    return state_->is_evaled();
  }

  suspension(promise_type* p) : state_(new state(p))
  {
  }

  suspension() : state_(nullptr)
  {
  }

  suspension(suspension const&) = default;
  suspension(suspension&&)      = default;

  suspension(R const& value) : state_(new state(value))
  {
  }

  suspension(R&& value) : state_(new state(std::move(value)))
  {
  }

  ~suspension() = default;

private:
  std::shared_ptr<state> state_;
};

template <typename F, typename... Args>
auto suspend(F f, Args... args)
    -> suspension<std::result_of_t<std::decay_t<F>(std::decay_t<Args>...)>>
{
  co_return std::invoke(f, args...);
}

template <typename R>
R const& force(suspension<R> const& suspension)
{
  return suspension.get();
}

template <typename R>
R const& force(suspension<R>&& suspension)
{
  return std::move(suspension.get());
}

} // namespace suspension

#endif

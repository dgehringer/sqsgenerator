//
// Created by Dominik Gehringer on 06.01.25.
//

#ifndef SQSGEN_CORE_THREAD_POOL_H
#define SQSGEN_CORE_THREAD_POOL_H

#include <blockingconcurrentqueue.h>
#include <concurrentqueue.h>

#include <barrier>
#include <thread>

namespace sqsgen::core {

  template <class... Args> struct Task {
    std::function<void(Args...)> fn;
    std::tuple<Args...> args;

    explicit Task(std::function<void(Args...)> const &fn, Args &&...args)
        : fn(fn), args(std::forward<Args>(args)...) {}
  };

  namespace detail {

    template <class> struct is_task {
      static constexpr bool value = false;
    };

    template <class Fn, class... Args> struct is_task<Task<Fn, Args...>> {
      static constexpr bool value = true;
    };

  }  // namespace detail

  template <class... Tasks> using task_collection_t = std::vector<std::variant<Tasks...>>;

  template <class... Tasks>
    requires(detail::is_task<Tasks>::value && ...)
  class thread_pool {
  public:
    using task_t = std::variant<Tasks...>;

    template <std::integral T> explicit thread_pool(T size)
        : _size(static_cast<std::size_t>(size)), _tasks() {};

    template <class OtherTask> void enqueue(OtherTask &&task) {
      _tasks.enqueue(task);
      _task_condition.notify_all();
    }

    template <class... Args>
    void enqueue_fn(std::function<void(Args...)> const &fn, Args &&...args) {
      enqueue(Task<Args...>(fn, std::forward<Args>(args)...));
    }

    void start() {
      if (_threads.empty()) _threads = make_thread_handles(_size);
    }
    void stop() {
      if (!_stop.stop_requested()) _stop.request_stop();
    }
    void join() {
      if (!_threads.empty()) {
        for (auto &t : _threads) t.join();
        _thread_ids.clear();
      }
    }

    [[nodiscard]] int thread_id() const {
      if (_thread_ids.empty() || _threads.empty())
        throw std::runtime_error(
            "thread_pool::thread_id() can only be called if the thread pool is running");
      return _thread_ids.at(std::this_thread::get_id());
    }

    std::stop_token get_stop_token() { return _stop.get_token(); }

  private:
    std::size_t _size;
    std::condition_variable_any _task_condition;
    std::stop_source _stop;
    moodycamel::ConcurrentQueue<task_t> _tasks;
    std::vector<std::jthread> _threads;
    std::map<std::thread::id, int> _thread_ids;
    std::mutex _thread_id_mutex;

    std::jthread make_thread_handle(auto thread_id, auto &&barrier_id_map_init) {
      return std::jthread([&, thread_id] {
        std::stop_token st = _stop.get_token();
        std::mutex m;
        {
          std::scoped_lock lock(_thread_id_mutex);
          _thread_ids.emplace(std::this_thread::get_id(), thread_id);
        }
        barrier_id_map_init.arrive_and_wait();
        while (true) {
          if (st.stop_requested()) break;
          std::optional<task_t> current_task;
          std::unique_lock lock(m);
          if (!_task_condition.wait(lock, st, [&] { return _tasks.try_dequeue(current_task); }))
            break;
          assert(current_task.has_value());
          std::visit([&](auto &&task) { std::apply(task.fn, task.args); }, current_task.value());
          current_task.reset();
        }
      });
    }

    auto make_thread_handles(auto num_threads) {
      // we add one to the number of threads (for this thread from which the workers are forked)
      std::barrier barrier_id_init(num_threads + 1);
      _thread_ids.emplace(std::this_thread::get_id(), num_threads);
      auto handles = helpers::as<std::vector>{}(
          helpers::range(num_threads) | views::transform([this, &barrier_id_init](auto id) {
            return make_thread_handle(id, std::forward<decltype(barrier_id_init)>(barrier_id_init));
          }));
      // need to wait otherwise the barrier would go out of scope
      barrier_id_init.arrive_and_wait();
      return handles;
    }
  };

}  // namespace sqsgen::core

#endif  // SQSGEN_CORE_THREAD_POOL_H

#pragma once
#include <functional>
#include <deque>
#include <memory>
#include <iostream>

namespace Engine {

  struct Action {
    std::function<void()> do_func;
    std::function<void()> undo_func;

    Action(std::function<void()> doF, std::function<void()> undoF)
      : do_func(std::move(doF)), undo_func(std::move(undoF)) {
    }

    void do_action() { if (do_func) do_func(); }
    void undo_action() { if (undo_func) undo_func(); }
    void redo_action() { do_action(); }
  };

  namespace UndoManager {
    constexpr size_t MAX_HISTORY = 500;
    inline std::deque<std::unique_ptr<Action>> undo_stack;
    inline std::deque<std::unique_ptr<Action>> redo_stack;

    // Takes an Action but does not execute it
    inline void record(std::unique_ptr<Action> action) {
      undo_stack.push_back(std::move(action));
      while (undo_stack.size() > MAX_HISTORY)
        undo_stack.pop_front();
      redo_stack.clear();
    }

    // Takes lambdas directly but does not execute them
    template<typename DoFunc, typename UndoFunc>
    inline void record(DoFunc&& do_func, UndoFunc&& undo_func) {
      auto action = std::make_unique<Action>(
        std::forward<DoFunc>(do_func),
        std::forward<UndoFunc>(undo_func)
      );
      record(std::move(action));
    }

    // Takes an Action and executes it immediately
    inline void execute(std::unique_ptr<Action> action) {
      action->do_action();
      record(std::move(action));
      std::cout << "Adding to list\n";
    }

    // Take lambdas directly and executes immediately
    template<typename DoFunc, typename UndoFunc>
    inline void execute(DoFunc&& do_func, UndoFunc&& undo_func) {
      auto action = std::make_unique<Action>(
        std::forward<DoFunc>(do_func),
        std::forward<UndoFunc>(undo_func)
      );
      execute(std::move(action));
    }

    inline void undo() {
      if (!undo_stack.empty()) {
        auto action = std::move(undo_stack.back());
        undo_stack.pop_back();
        action->undo_action();
        redo_stack.push_back(std::move(action));
        while (redo_stack.size() > MAX_HISTORY)
          redo_stack.pop_front();
        std::cout << "Undo from list\n";
      }
      else
        std::cout << "Undo stack empty\n";
    }

    inline void redo() {
      if (!redo_stack.empty()) {
        auto action = std::move(redo_stack.back());
        redo_stack.pop_back();
        action->redo_action();
        undo_stack.push_back(std::move(action));
        while (undo_stack.size() > MAX_HISTORY)
          undo_stack.pop_front();
      }
    }
  }
}

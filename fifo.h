#ifndef FIFO_H
#define FIFO_H

#include <atomic>
#include "debug.h"

template <typename T>
struct Element {
  int64_t num;
  T t;

  Element() = default;
};

template <typename T>
class FIFO {
protected:
  const static bool DB = false;

  bool was_cycled;
  int first_idx, last_idx;
  std::atomic<bool> locked = true;
public:
  Element<T> *buff = nullptr;
public:
  int fifo_size;

  FIFO(int fifo_size_) : fifo_size(fifo_size_) {
    buff = new Element<T>[fifo_size + 7];
    unlock();
    clear();
  }

  ~FIFO() {
    delete[] buff;
  }

  Element<T> const * get(int64_t num) const {
    int idx = -1;
    if (num >= first_num() && num <= last_num())
      idx = idx_for_num(num);
    PRINTLNC(DB, "get dt - num=" << num << ", idx=" << idx);
    if (idx != -1)
      PRINTFC(DB, ", buff[idx].num=" << buff[idx].num << "\n")
    else
      FLUSHC(DB)

    if (idx != -1 && buff[idx].num >= num)
      return &buff[idx];
    else
      return nullptr;
  }

  Element<T>* get_to_set(int64_t num) {
    int idx = insert(num);
    PRINTLNFC(DB, "get dt to set - num=" << num << ", idx=" << idx <<
              ", f_idx=" << first_idx << ", l_idx=" << last_idx);
    if (idx != -1) {
      buff[idx].num = num;
      return &buff[idx];
    }
    else {
      return nullptr;
    }
  }

  void clear() {
    lock();
    was_cycled = false;
    first_idx = last_idx = -1;
    for (int i = 0; i < fifo_size; i++)
      buff[i].num = -1;
    unlock();
  }

  inline int64_t first_num() const {
    if (first_idx != -1)
        return buff[first_idx].num;
    return 0;
  }

  inline int64_t last_num() const {
    if (last_idx != -1)
      return buff[last_idx].num;
    return 0;
  }

  /**
   * @param num
   * @return idx
   */
  inline int idx_for_num(int64_t num) const {
    int idx = (last_idx + (num - last_num()));
    while (idx < 0)
      idx += fifo_size;
    while (idx >= fifo_size)
      idx -= fifo_size;
    return idx;
  }

  inline int idx_plus(int idx) {
    idx++;
    if (idx >= fifo_size)
      idx -= fifo_size;
    return idx;
  }

  inline void lock() {
    bool expected = false;
    while (!locked.compare_exchange_weak(expected, true));
  }

  inline void unlock() {
    locked = false;
  }
private:
  inline int insert(int64_t num) {
    if (last_idx == -1)
      return last_idx = first_idx = 0;
    else {
      if (num >= first_num()) {
        if (num <= last_num()) // insert between first and last
          return idx_for_num(num);
        else // now: c_num > last_num()
          return insert_later_than_last(num);
      }
      else
        return -1;
    }
  }

  inline int insert_later_than_last(int64_t num) {
    int idx;
    if (num < last_num() + fifo_size) {
      idx = idx_for_num(num);
      if (was_cycled)
        first_idx = idx_plus(idx);
      else {
        int old_f_idx = first_idx;
        first_idx = idx_for_num(std::max(first_num(), num - fifo_size + 1));
        if (old_f_idx > first_idx)
          was_cycled = true;
      }
      last_idx = idx;
    }
    else {
      idx = first_idx = last_idx = 0;
      was_cycled = false;
    }
    return idx;
  }
};

#endif // FIFO_H

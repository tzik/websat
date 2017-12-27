#pragma once

namespace std {

template <typename T>
struct remove_reference { using type = T; };

template <typename T>
struct remove_reference<T&> { using type = T; };

template <typename T>
struct remove_reference<T&&> { using type = T; };

template <typename T>
using remove_reference_t = typename remove_reference<T>::type;

template <typename T>
remove_reference_t<T>&& move(T&& x) {
  return static_cast<remove_reference_t<T>&&>(x);
}

template <typename T>
void swap(T& x, T& y) {
  T t = move(x);
  x = move(y);
  y = move(t);
}

}  // namespace std

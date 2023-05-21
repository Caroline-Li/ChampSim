#ifndef UTIL_H
#define UTIL_H

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

constexpr unsigned lg2(uint64_t n) { return n < 2 ? 0 : 1 + lg2(n / 2); }

constexpr bool is_pow2(uint64_t n) { return (n == 0) || ((n & (n - 1)) == 0); }

constexpr uint64_t bitmask(std::size_t begin, std::size_t end = 0) { return ((1ull << (begin - end)) - 1) << end; }

constexpr uint64_t splice_bits(uint64_t upper, uint64_t lower, std::size_t bits) { return (upper & ~bitmask(bits)) | (lower & bitmask(bits)); }

constexpr bool samepage(uint64_t addr, uint64_t addr2, uint32_t page_bits) { return (addr ^ addr2) >> page_bits == 0; }

template <typename T>
struct is_valid {
  using argument_type = T;
  is_valid() {}
  bool operator()(const argument_type& test) { return test.valid; }
};

template <typename T>
struct eq_addr {
  using argument_type = T;
  const decltype(argument_type::address) val;
  const std::size_t shamt = 0;

  explicit eq_addr(decltype(argument_type::address) val) : val(val) {}
  eq_addr(decltype(argument_type::address) val, std::size_t shamt) : val(val), shamt(shamt) {}

  bool operator()(const argument_type& test)
  {
    is_valid<argument_type> validtest;
    return validtest(test) && (test.address >> shamt) == (val >> shamt);
  }
};

template <typename T, typename BIN, typename U = T, typename UN_T = is_valid<T>, typename UN_U = is_valid<U>>
struct invalid_is_minimal {
  bool operator()(const T& lhs, const U& rhs)
  {
    UN_T lhs_unary;
    UN_U rhs_unary;
    BIN cmp;

    return !lhs_unary(lhs) || (rhs_unary(rhs) && cmp(lhs, rhs));
  }
};

template <typename T, typename BIN, typename U = T, typename UN_T = is_valid<T>, typename UN_U = is_valid<U>>
struct invalid_is_maximal {
  bool operator()(const T& lhs, const U& rhs)
  {
    UN_T lhs_unary;
    UN_U rhs_unary;
    BIN cmp;

    return !rhs_unary(rhs) || (lhs_unary(lhs) && cmp(lhs, rhs));
  }
};

template <typename T, typename U = T>
struct cmp_event_cycle {
  bool operator()(const T& lhs, const U& rhs) { return lhs.event_cycle < rhs.event_cycle; }
};

template <typename T>
struct min_event_cycle : invalid_is_maximal<T, cmp_event_cycle<T>> {
};

template <typename T, typename U = T>
struct cmp_lru {
  bool operator()(const T& lhs, const U& rhs) { return lhs.lru < rhs.lru; }
};

/*
 * A comparator to determine the LRU element. To use this comparator, the type
 * must have a member variable named "lru" and have a specialization of
 * is_valid<>.
 *
 * To use:
 *     auto lru_elem = std::max_element(std::begin(set), std::end(set),
 * lru_comparator<BLOCK>());
 *
 * The MRU element can be found using std::min_element instead.
 */
template <typename T, typename U = T>
struct lru_comparator : invalid_is_maximal<T, cmp_lru<T, U>, U> {
  using first_argument_type = T;
  using second_argument_type = U;
};

/*
 * A functor to reorder elements to a new LRU order.
 * The type must have a member variable named "lru".
 *
 * To use:
 *     std::for_each(std::begin(set), std::end(set),
 * lru_updater<BLOCK>(hit_element));
 */
template <typename T>
struct lru_updater {
  const decltype(T::lru) val;
  explicit lru_updater(decltype(T::lru) val) : val(val) {}

  template <typename U>
  explicit lru_updater(U iter) : val(iter->lru)
  {
  }

  void operator()(T& x)
  {
    if (x.lru == val)
      x.lru = 0;
    else
      ++x.lru;
  }
};

template <typename T, typename U = T>
struct ord_event_cycle {
  using first_argument_type = T;
  using second_argument_type = U;
  bool operator()(const first_argument_type& lhs, const second_argument_type& rhs)
  {
    is_valid<first_argument_type> first_validtest;
    is_valid<second_argument_type> second_validtest;
    return !second_validtest(rhs) || (first_validtest(lhs) && lhs.event_cycle < rhs.event_cycle);
  }
};

void gen_random(char* s, const int len);
uint32_t folded_xor(uint64_t value, uint32_t num_folds);

template <class T>
std::string array_to_string(std::vector<T> array, bool hex = false, uint32_t size = 0)
{
  std::stringstream ss;
  if (size == 0)
    size = array.size();
  for (uint32_t index = 0; index < size; ++index) {
    if (hex) {
      ss << std::hex << array[index] << std::dec;
    } else {
      ss << array[index];
    }
    ss << ",";
  }
  return ss.str();
}

class HashZoo
{
public:
  static uint32_t jenkins(uint32_t key);
  static uint32_t knuth(uint32_t key);
  static uint32_t murmur3(uint32_t key);
  static uint32_t jenkins32(uint32_t key);
  static uint32_t hash32shift(uint32_t key);
  static uint32_t hash32shiftmult(uint32_t key);
  static uint32_t hash64shift(uint32_t key);
  static uint32_t hash5shift(uint32_t key);
  static uint32_t hash7shift(uint32_t key);
  static uint32_t Wang6shift(uint32_t key);
  static uint32_t Wang5shift(uint32_t key);
  static uint32_t Wang4shift(uint32_t key);
  static uint32_t Wang3shift(uint32_t key);

  static uint32_t three_hybrid1(uint32_t key);
  static uint32_t three_hybrid2(uint32_t key);
  static uint32_t three_hybrid3(uint32_t key);
  static uint32_t three_hybrid4(uint32_t key);
  static uint32_t three_hybrid5(uint32_t key);
  static uint32_t three_hybrid6(uint32_t key);
  static uint32_t three_hybrid7(uint32_t key);
  static uint32_t three_hybrid8(uint32_t key);
  static uint32_t three_hybrid9(uint32_t key);
  static uint32_t three_hybrid10(uint32_t key);
  static uint32_t three_hybrid11(uint32_t key);
  static uint32_t three_hybrid12(uint32_t key);

  static uint32_t four_hybrid1(uint32_t key);
  static uint32_t four_hybrid2(uint32_t key);
  static uint32_t four_hybrid3(uint32_t key);
  static uint32_t four_hybrid4(uint32_t key);
  static uint32_t four_hybrid5(uint32_t key);
  static uint32_t four_hybrid6(uint32_t key);
  static uint32_t four_hybrid7(uint32_t key);
  static uint32_t four_hybrid8(uint32_t key);
  static uint32_t four_hybrid9(uint32_t key);
  static uint32_t four_hybrid10(uint32_t key);
  static uint32_t four_hybrid11(uint32_t key);
  static uint32_t four_hybrid12(uint32_t key);

  static uint32_t getHash(uint32_t selector, uint32_t key);
};

#endif
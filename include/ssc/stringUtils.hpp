#ifndef PROXICT_SSC_STRINGUTILS_HPP_
#define PROXICT_SSC_STRINGUTILS_HPP_

#include <sstream>
#include <string>
#include <utility>

namespace ssc::utils {

template <typename T, typename... TArgs>
[[maybe_unused]] std::ostream& serialize(std::ostream& stream, T&& first, TArgs&&... args) {
    stream << std::forward<T>(first);
    ((stream << std::forward<TArgs>(args)), ...);
    return stream;
}

template <typename T, typename... TArgs>
[[nodiscard]] std::string concatenate(T&& first, TArgs&&... args) {
    std::stringstream ss;
    serialize(ss, std::forward<T>(first), std::forward<TArgs>(args)...);
    return ss.str();
}

} // namespace ssc

#endif // PROXICT_SSC_STRINGUTILS_HPP_

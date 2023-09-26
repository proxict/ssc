#ifndef PROXICT_SSC_EXCEPTION_HPP_
#define PROXICT_SSC_EXCEPTION_HPP_

#include "ssc/stringUtils.hpp"

#include <stdexcept>

namespace ssc {

class Exception : public std::runtime_error {
public:
    template <typename... TArgs>
    explicit Exception(const std::string& message, TArgs&&... args)
        : std::runtime_error(utils::concatenate(message, std::forward<TArgs>(args)...)) {}

    virtual ~Exception() noexcept = default;

    Exception(Exception&&) noexcept = default;

    Exception(const Exception&) = delete;
    Exception& operator=(const Exception&) = delete;
    Exception& operator=(Exception&&) = delete;
};

} // namespace ssc

#endif // PROXICT_SSC_EXCEPTION_HPP_

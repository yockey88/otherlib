/**
 * \file core/crtp.hpp
 * \ref https://www.fluentcpp.com/2017/05/19/crtp-helper/
 **/
#ifndef OTHER_CORE_CRTP_HPP
#define OTHER_CORE_CRTP_HPP

namespace other {

  template <typename T, template <typename> class crtp_type>
  struct crtp {
    T& underlying() { return static_cast<T&>(*this); }
    T const& underlying() const { return static_cast<T const&>(*this); }

   private:
    crtp() {}
    friend crtp_type<T>;
  };

}  // namespace other

#endif  // OTHER_CORE_CRTP_HPP
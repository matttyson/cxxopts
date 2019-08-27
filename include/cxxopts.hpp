/*

Copyright (c) 2014, 2015, 2016, 2017 Jarryd Beck

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#ifndef CXXOPTS_HPP_INCLUDED
#define CXXOPTS_HPP_INCLUDED

#include <cstring>
#include <cctype>
#include <exception>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef __cpp_lib_optional
#include <optional>
#define CXXOPTS_HAS_OPTIONAL
#endif

#define CXXOPTS__VERSION_MAJOR 2
#define CXXOPTS__VERSION_MINOR 2
#define CXXOPTS__VERSION_PATCH 0

namespace cxxopts
{
  static constexpr struct {
    uint8_t major, minor, patch;
  } version = {
    CXXOPTS__VERSION_MAJOR,
    CXXOPTS__VERSION_MINOR,
    CXXOPTS__VERSION_PATCH
  };
}

namespace cxxopts
{
#ifdef CXXOPTS_WIDE
	using CxxString = std::wstring;
	using Char = wchar_t;
	using CxxStringstream = std::wstringstream;
	using CxxMatch = std::wsmatch;
	static int CxxStrcmp(const Char* str1, const Char* str2) {
		return ::wcscmp(str1, str2);
	}
	#define CxS(x) L##x

#ifdef _WIN32
	// Defined in Windows.h, but I don't want to pull the whole header in for
	// just one function.
	extern "C" int
	__stdcall
	WideCharToMultiByte(
		unsigned int CodePage,
		unsigned long dwFlags,
		const wchar_t* lpWideCharStr,
		int cchWideChar,
		char* lpMultiByteStr,
		int cbMultiByte,
		const char* lpDefaultChar,
		int* lpUsedDefaultChar
	);

	static std::string to_narrow(const CxxString& str) {
		const int len = WideCharToMultiByte(65001, 0,
			str.c_str(), (int)str.length(),
			NULL, 0, NULL, NULL);

		std::string output(len, '\0');

		WideCharToMultiByte(65001, 0,
			str.c_str(), (int)str.length(),
			const_cast<char*>(output.data()), len,
			NULL, NULL
		);

		return output;
	}
#else
#error Implement wide to narrow conversions for your platform
#endif
#else
	using CxxString = std::string;
	using Char = char;
	using CxxStringstream = std::stringstream;
	using CxxMatch = std::smatch;

	static int CxxStrcmp(const Char* str1, const Char* str2) {
		return ::strcmp(str1, str2);
	}
	static std::string to_narrow(const std::string& str) {
		return str;
	}
	#define CxS(x) x
#endif
}

#ifndef CXXOPTS_VECTOR_DELIMITER
#define CXXOPTS_VECTOR_DELIMITER CxS(',')
#endif


//when we ask cxxopts to use Unicode, help strings are processed using ICU,
//which results in the correct lengths being computed for strings when they
//are formatted for the help output
//it is necessary to make sure that <unicode/unistr.h> can be found by the
//compiler, and that icu-uc is linked in to the binary.

#ifdef CXXOPTS_USE_UNICODE
#include <unicode/unistr.h>

#ifdef CXXOPTS_WIDE
#error Not supported
#endif

namespace cxxopts
{
  typedef icu::UnicodeString UniString;

  inline
  UniString
  toLocalString(std::string s)
  {
    return icu::UnicodeString::fromUTF8(std::move(s));
  }

  class UnicodeStringIterator : public
    std::iterator<std::forward_iterator_tag, int32_t>
  {
    public:

    UnicodeStringIterator(const icu::UnicodeString* string, int32_t pos)
    : s(string)
    , i(pos)
    {
    }

    value_type
    operator*() const
    {
      return s->char32At(i);
    }

    bool
    operator==(const UnicodeStringIterator& rhs) const
    {
      return s == rhs.s && i == rhs.i;
    }

    bool
    operator!=(const UnicodeStringIterator& rhs) const
    {
      return !(*this == rhs);
    }

    UnicodeStringIterator&
    operator++()
    {
      ++i;
      return *this;
    }

    UnicodeStringIterator
    operator+(int32_t v)
    {
      return UnicodeStringIterator(s, i + v);
    }

    private:
    const icu::UnicodeString* s;
    int32_t i;
  };

  inline
  UniString&
  stringAppend(UniString&s, UniString a)
  {
    return s.append(std::move(a));
  }

  inline
  UniString&
  stringAppend(UniString& s, int n, UChar32 c)
  {
    for (int i = 0; i != n; ++i)
    {
      s.append(c);
    }

    return s;
  }

  template <typename Iterator>
  UniString&
  stringAppend(UniString& s, Iterator begin, Iterator end)
  {
    while (begin != end)
    {
      s.append(*begin);
      ++begin;
    }

    return s;
  }

  inline
  size_t
  stringLength(const UniString& s)
  {
    return s.length();
  }

  inline
  std::string
  toUTF8String(const UniString& s)
  {
#if defined (_WIN32) && defined (CXXOPTS_WIDE)
	  const int len = WideCharToMultiByte(65001, 0,
		  s.c_str(), (int)s.length(),
		  NULL, 0, NULL, NULL);

	  std::string output(len, '\0');

	  WideCharToMultiByte(65001, 0,
		  s.c_str(), (int)s.length(),
		  const_cast<char*>(output.data()), len,
		  NULL, NULL
#else
    std::string result;
    s.toUTF8String(result);

    return result;
#endif
  }

  inline
  bool
  empty(const UniString& s)
  {
    return s.isEmpty();
  }
}

namespace std
{
  inline
  cxxopts::UnicodeStringIterator
  begin(const icu::UnicodeString& s)
  {
    return cxxopts::UnicodeStringIterator(&s, 0);
  }

  inline
  cxxopts::UnicodeStringIterator
  end(const icu::UnicodeString& s)
  {
    return cxxopts::UnicodeStringIterator(&s, s.length());
  }
}

//ifdef CXXOPTS_USE_UNICODE
#else

namespace cxxopts
{
  using UniString = CxxString;

  template <typename T>
  T
  toLocalString(T&& t)
  {
    return std::forward<T>(t);
  }

  inline
  size_t
  stringLength(const UniString& s)
  {
    return s.length();
  }

  inline
  UniString&
  stringAppend(UniString&s, UniString a)
  {
    return s.append(std::move(a));
  }

  inline
  UniString&
  stringAppend(UniString& s, size_t n, char c)
  {
    return s.append(n, c);
  }

  template <typename Iterator>
  UniString&
  stringAppend(UniString& s, Iterator begin, Iterator end)
  {
    return s.append(begin, end);
  }

  template <typename T>
  std::string
  toUTF8String(T&& t)
  {
    return std::forward<T>(t);
  }

  inline
  bool
  empty(const UniString& s)
  {
    return s.empty();
  }
}

//ifdef CXXOPTS_USE_UNICODE
#endif

namespace cxxopts
{
  namespace
  {
#ifdef _WIN32
    const std::string LQUOTE("\'");
    const std::string RQUOTE("\'");
#else
    const std::string LQUOTE("‘");
    const std::string RQUOTE("’");
#endif
  }

  class Value : public std::enable_shared_from_this<Value>
  {
    public:

    virtual ~Value() = default;

    virtual
    std::shared_ptr<Value>
    clone() const = 0;

    virtual void
    parse(const CxxString& text) const = 0;

    virtual void
    parse() const = 0;

    virtual bool
    has_default() const = 0;

    virtual bool
    is_container() const = 0;

    virtual bool
    has_implicit() const = 0;

    virtual CxxString
    get_default_value() const = 0;

    virtual CxxString
    get_implicit_value() const = 0;

    virtual std::shared_ptr<Value>
    default_value(const CxxString& value) = 0;

    virtual std::shared_ptr<Value>
    implicit_value(const CxxString& value) = 0;

    virtual std::shared_ptr<Value>
    no_implicit_value() = 0;

    virtual bool
    is_boolean() const = 0;
  };

  class OptionException : public std::exception
  {
    public:
    OptionException(const std::string& message)
    : m_message(message)
    {
    }

    virtual const char*
    what() const noexcept
    {
      return m_message.c_str();
    }

    private:
    std::string m_message;
  };

  class OptionSpecException : public OptionException
  {
    public:

    OptionSpecException(const std::string& message)
    : OptionException(message)
    {
    }
  };

  class OptionParseException : public OptionException
  {
    public:
    OptionParseException(const std::string& message)
    : OptionException(message)
    {
    }
  };

  class option_exists_error : public OptionSpecException
  {
    public:
    option_exists_error(const std::string& option)
    : OptionSpecException("Option " + LQUOTE + option + RQUOTE + " already exists")
    {
    }
  };

  class invalid_option_format_error : public OptionSpecException
  {
    public:
    invalid_option_format_error(const std::string& format)
    : OptionSpecException("Invalid option format " + LQUOTE + format + RQUOTE)
    {
    }
  };

  class option_syntax_exception : public OptionParseException {
    public:
    option_syntax_exception(const std::string& text)
    : OptionParseException("Argument " + LQUOTE + text + RQUOTE +
        " starts with a - but has incorrect syntax")
    {
    }
  };

  class option_not_exists_exception : public OptionParseException
  {
    public:
    option_not_exists_exception(const std::string& option)
    : OptionParseException("Option " + LQUOTE + option + RQUOTE + " does not exist")
    {
    }
  };

  class missing_argument_exception : public OptionParseException
  {
    public:
    missing_argument_exception(const std::string& option)
    : OptionParseException(
        "Option " + LQUOTE + option + RQUOTE + " is missing an argument"
      )
    {
    }
  };

  class option_requires_argument_exception : public OptionParseException
  {
    public:
    option_requires_argument_exception(const std::string& option)
    : OptionParseException(
        "Option " + LQUOTE + option + RQUOTE + " requires an argument"
      )
    {
    }
  };

  class option_not_has_argument_exception : public OptionParseException
  {
    public:
    option_not_has_argument_exception
    (
      const std::string& option,
      const std::string& arg
    )
    : OptionParseException(
        "Option " + LQUOTE + option + RQUOTE +
        " does not take an argument, but argument " +
        LQUOTE + arg + RQUOTE + " given"
      )
    {
    }
  };

  class option_not_present_exception : public OptionParseException
  {
    public:
    option_not_present_exception(const std::string& option)
    : OptionParseException("Option " + LQUOTE + option + RQUOTE + " not present")
    {
    }
  };

  class argument_incorrect_type : public OptionParseException
  {
    public:
    argument_incorrect_type
    (
      const std::string& arg
    )
    : OptionParseException(
        "Argument " + LQUOTE + arg + RQUOTE + " failed to parse"
      )
    {
    }
  };

  class option_required_exception : public OptionParseException
  {
    public:
    option_required_exception(const std::string& option)
    : OptionParseException(
        "Option " + LQUOTE + option + RQUOTE + " is required but not present"
      )
    {
    }
  };

  template <typename T>
  void throw_or_mimic(const CxxString& text)
  {
    static_assert(std::is_base_of<std::exception, T>::value,
                  "throw_or_mimic only works on std::exception and "
                  "deriving classes");

#ifndef CXXOPTS_NO_EXCEPTIONS
    // If CXXOPTS_NO_EXCEPTIONS is not defined, just throw
    throw T{to_narrow(text)};
#else
    // Otherwise manually instantiate the exception, print what() to stderr,
    // and abort
    T exception{to_narrow(text)};
    std::cerr << exception.what() << std::endl;
    std::cerr << "Aborting (exceptions disabled)..." << std::endl;
    std::abort();
#endif
  }

  namespace values
  {
    namespace
    {
      std::basic_regex<Char> integer_pattern
        (CxS("(-)?(0x)?([0-9a-zA-Z]+)|((0x)?0)"));
      std::basic_regex<Char> truthy_pattern
        (CxS("(t|T)(rue)?|1"));
      std::basic_regex<Char> falsy_pattern
        (CxS("(f|F)(alse)?|0"));
    }

    namespace detail
    {
      template <typename T, bool B>
      struct SignedCheck;

      template <typename T>
      struct SignedCheck<T, true>
      {
        template <typename U>
        void
        operator()(bool negative, U u, const CxxString& text)
        {
          if (negative)
          {
            if (u > static_cast<U>((std::numeric_limits<T>::min)()))
            {
              throw_or_mimic<argument_incorrect_type>(text);
            }
          }
          else
          {
            if (u > static_cast<U>((std::numeric_limits<T>::max)()))
            {
              throw_or_mimic<argument_incorrect_type>(text);
            }
          }
        }
      };

      template <typename T>
      struct SignedCheck<T, false>
      {
        template <typename U>
        void
        operator()(bool, U, const CxxString&) {}
      };

      template <typename T, typename U>
      void
      check_signed_range(bool negative, U value, const CxxString& text)
      {
        SignedCheck<T, std::numeric_limits<T>::is_signed>()(negative, value, text);
      }
    }

    template <typename R, typename T>
    R
    checked_negate(T&& t, const CxxString&, std::true_type)
    {
      // if we got to here, then `t` is a positive number that fits into
      // `R`. So to avoid MSVC C4146, we first cast it to `R`.
      // See https://github.com/jarro2783/cxxopts/issues/62 for more details.
      return -static_cast<R>(t-1)-1;
    }

    template <typename R, typename T>
    T
    checked_negate(T&& t, const CxxString& text, std::false_type)
    {
      throw_or_mimic<argument_incorrect_type>(text);
      return t;
    }

    template <typename T>
    void
    integer_parser(const CxxString & text, T& value)
    {
      CxxMatch match;
      std::regex_match(text, match, integer_pattern);

      if (match.length() == 0)
      {
        throw_or_mimic<argument_incorrect_type>(text);
      }

      if (match.length(4) > 0)
      {
        value = 0;
        return;
      }

      using US = typename std::make_unsigned<T>::type;

      constexpr bool is_signed = std::numeric_limits<T>::is_signed;
      const bool negative = match.length(1) > 0;
      const uint8_t base = match.length(2) > 0 ? 16 : 10;

      auto value_match = match[3];

      US result = 0;

      for (auto iter = value_match.first; iter != value_match.second; ++iter)
      {
        US digit = 0;

        if (*iter >= CxS('0') && *iter <= CxS('9'))
        {
          digit = static_cast<US>(*iter - CxS('0'));
        }
        else if (base == 16 && *iter >= CxS('a') && *iter <= CxS('f'))
        {
          digit = static_cast<US>(*iter - CxS('a') + 10);
        }
        else if (base == 16 && *iter >= CxS('A') && *iter <= CxS('F'))
        {
          digit = static_cast<US>(*iter - CxS('A') + 10);
        }
        else
        {
          throw_or_mimic<argument_incorrect_type>(text);
        }

        US next = result * base + digit;
        if (result > next)
        {
          throw_or_mimic<argument_incorrect_type>(text);
        }

        result = next;
      }

      detail::check_signed_range<T>(negative, result, text);

      if (negative)
      {
        value = checked_negate<T>(result,
          text,
          std::integral_constant<bool, is_signed>());
      }
      else
      {
        value = static_cast<T>(result);
      }
    }

    template <typename T>
    void stringstream_parser(const CxxString& text, T& value)
    {
      CxxStringstream in(text);
      in >> value;
      if (!in) {
        throw_or_mimic<argument_incorrect_type>(text);
      }
    }

    inline
    void
    parse_value(const CxxString& text, uint8_t& value)
    {
      integer_parser(text, value);
    }

    inline
    void
    parse_value(const CxxString& text, int8_t& value)
    {
      integer_parser(text, value);
    }

    inline
    void
    parse_value(const CxxString& text, uint16_t& value)
    {
      integer_parser(text, value);
    }

    inline
    void
    parse_value(const CxxString& text, int16_t& value)
    {
      integer_parser(text, value);
    }

    inline
    void
    parse_value(const CxxString& text, uint32_t& value)
    {
      integer_parser(text, value);
    }

    inline
    void
    parse_value(const CxxString& text, int32_t& value)
    {
      integer_parser(text, value);
    }

    inline
    void
    parse_value(const CxxString& text, uint64_t& value)
    {
      integer_parser(text, value);
    }

    inline
    void
    parse_value(const CxxString& text, int64_t& value)
    {
      integer_parser(text, value);
    }

    inline
    void
    parse_value(const CxxString& text, bool& value)
    {
      CxxMatch result;
      std::regex_match(text, result, truthy_pattern);

      if (!result.empty())
      {
        value = true;
        return;
      }

      std::regex_match(text, result, falsy_pattern);
      if (!result.empty())
      {
        value = false;
        return;
      }

      throw_or_mimic<argument_incorrect_type>(text);
    }

    inline
    void
    parse_value(const CxxString& text, CxxString& value)
    {
      value = text;
    }

    // The fallback parser. It uses the stringstream parser to parse all types
    // that have not been overloaded explicitly.  It has to be placed in the
    // source code before all other more specialized templates.
    template <typename T>
    void
    parse_value(const CxxString& text, T& value) {
      stringstream_parser(text, value);
    }

    template <typename T>
    void
    parse_value(const CxxString& text, std::vector<T>& value)
    {
      CxxStringstream in(text);
      CxxString token;
      while(in.eof() == false && std::getline(in, token, CXXOPTS_VECTOR_DELIMITER)) {
        T v;
        parse_value(token, v);
        value.emplace_back(std::move(v));
      }
    }

#ifdef CXXOPTS_HAS_OPTIONAL
    template <typename T>
    void
    parse_value(const CxxString& text, std::optional<T>& value)
    {
      T result;
      parse_value(text, result);
      value = std::move(result);
    }
#endif

    inline
    void parse_value(const CxxString& text, Char& c)
    {
      if (text.length() != 1)
      {
        throw_or_mimic<argument_incorrect_type>(text);
      }

      c = text[0];
    }

    template <typename T>
    struct type_is_container
    {
      static constexpr bool value = false;
    };

    template <typename T>
    struct type_is_container<std::vector<T>>
    {
      static constexpr bool value = true;
    };

    template <typename T>
    class abstract_value : public Value
    {
      using Self = abstract_value<T>;

      public:
      abstract_value()
      : m_result(std::make_shared<T>())
      , m_store(m_result.get())
      {
      }

      abstract_value(T* t)
      : m_store(t)
      {
      }

      virtual ~abstract_value() = default;

      abstract_value(const abstract_value& rhs)
      {
        if (rhs.m_result)
        {
          m_result = std::make_shared<T>();
          m_store = m_result.get();
        }
        else
        {
          m_store = rhs.m_store;
        }

        m_default = rhs.m_default;
        m_implicit = rhs.m_implicit;
        m_default_value = rhs.m_default_value;
        m_implicit_value = rhs.m_implicit_value;
      }

      void
      parse(const CxxString& text) const
      {
        parse_value(text, *m_store);
      }

      bool
      is_container() const
      {
        return type_is_container<T>::value;
      }

      void
      parse() const
      {
        parse_value(m_default_value, *m_store);
      }

      bool
      has_default() const
      {
        return m_default;
      }

      bool
      has_implicit() const
      {
        return m_implicit;
      }

      std::shared_ptr<Value>
      default_value(const CxxString& value)
      {
        m_default = true;
        m_default_value = value;
        return shared_from_this();
      }

      std::shared_ptr<Value>
      implicit_value(const CxxString& value)
      {
        m_implicit = true;
        m_implicit_value = value;
        return shared_from_this();
      }

      std::shared_ptr<Value>
      no_implicit_value()
      {
        m_implicit = false;
        return shared_from_this();
      }

      CxxString
      get_default_value() const
      {
        return m_default_value;
      }

      CxxString
      get_implicit_value() const
      {
        return m_implicit_value;
      }

      bool
      is_boolean() const
      {
        return std::is_same<T, bool>::value;
      }

      const T&
      get() const
      {
        if (m_store == nullptr)
        {
          return *m_result;
        }
        else
        {
          return *m_store;
        }
      }

      protected:
      std::shared_ptr<T> m_result;
      T* m_store;

      bool m_default = false;
      bool m_implicit = false;

      CxxString m_default_value;
      CxxString m_implicit_value;
    };

    template <typename T>
    class standard_value : public abstract_value<T>
    {
      public:
      using abstract_value<T>::abstract_value;

      std::shared_ptr<Value>
      clone() const
      {
        return std::make_shared<standard_value<T>>(*this);
      }
    };

    template <>
    class standard_value<bool> : public abstract_value<bool>
    {
      public:
      ~standard_value() = default;

      standard_value()
      {
        set_default_and_implicit();
      }

      standard_value(bool* b)
      : abstract_value(b)
      {
        set_default_and_implicit();
      }

      std::shared_ptr<Value>
      clone() const
      {
        return std::make_shared<standard_value<bool>>(*this);
      }

      private:

      void
      set_default_and_implicit()
      {
        m_default = true;
        m_default_value = CxS("false");
        m_implicit = true;
        m_implicit_value = CxS("true");
      }
    };
  }

  template <typename T>
  std::shared_ptr<Value>
  value()
  {
    return std::make_shared<values::standard_value<T>>();
  }

  template <typename T>
  std::shared_ptr<Value>
  value(T& t)
  {
    return std::make_shared<values::standard_value<T>>(&t);
  }

  class OptionAdder;

  class OptionDetails
  {
    public:
    OptionDetails
    (
      const CxxString& short_,
      const CxxString& long_,
      const CxxString& desc,
      std::shared_ptr<const Value> val
    )
    : m_short(short_)
    , m_long(long_)
    , m_desc(desc)
    , m_value(val)
    , m_count(0)
    {
    }

    OptionDetails(const OptionDetails& rhs)
    : m_desc(rhs.m_desc)
    , m_count(rhs.m_count)
    {
      m_value = rhs.m_value->clone();
    }

    OptionDetails(OptionDetails&& rhs) = default;

    const CxxString&
    description() const
    {
      return m_desc;
    }

    const Value& value() const {
        return *m_value;
    }

    std::shared_ptr<Value>
    make_storage() const
    {
      return m_value->clone();
    }

    const CxxString&
    short_name() const
    {
      return m_short;
    }

    const CxxString&
    long_name() const
    {
      return m_long;
    }

    private:
    CxxString m_short;
    CxxString m_long;
    CxxString m_desc;
    std::shared_ptr<const Value> m_value;
    int m_count;
  };

  struct HelpOptionDetails
  {
    CxxString s;
    CxxString l;
    CxxString desc;
    bool has_default;
    CxxString default_value;
    bool has_implicit;
    CxxString implicit_value;
    CxxString arg_help;
    bool is_container;
    bool is_boolean;
  };

  struct HelpGroupDetails
  {
    CxxString name;
    CxxString description;
    std::vector<HelpOptionDetails> options;
  };

  class OptionValue
  {
    public:
    void
    parse
    (
      std::shared_ptr<const OptionDetails> details,
      const CxxString& text
    )
    {
      ensure_value(details);
      ++m_count;
      m_value->parse(text);
    }

    void
    parse_default(std::shared_ptr<const OptionDetails> details)
    {
      ensure_value(details);
      m_default = true;
      m_value->parse();
    }

    size_t
    count() const noexcept
    {
      return m_count;
    }

    // TODO: maybe default options should count towards the number of arguments
    bool
    has_default() const noexcept
    {
      return m_default;
    }

    template <typename T>
    const T&
    as() const
    {
      if (m_value == nullptr) {
        throw_or_mimic<std::domain_error>(CxS("No value"));
      }

#ifdef CXXOPTS_NO_RTTI
      return static_cast<const values::standard_value<T>&>(*m_value).get();
#else
      return dynamic_cast<const values::standard_value<T>&>(*m_value).get();
#endif
    }

    private:
    void
    ensure_value(std::shared_ptr<const OptionDetails> details)
    {
      if (m_value == nullptr)
      {
        m_value = details->make_storage();
      }
    }

    std::shared_ptr<Value> m_value;
    size_t m_count = 0;
    bool m_default = false;
  };

  class KeyValue
  {
    public:
    KeyValue(CxxString key_, CxxString value_)
    : m_key(std::move(key_))
    , m_value(std::move(value_))
    {
    }

    const
    CxxString&
    key() const
    {
      return m_key;
    }

    const
    CxxString&
    value() const
    {
      return m_value;
    }

    template <typename T>
    T
    as() const
    {
      T result;
      values::parse_value(m_value, result);
      return result;
    }

    private:
    CxxString m_key;
    CxxString m_value;
  };

  class ParseResult
  {
    public:

    ParseResult(
      const std::shared_ptr<
        std::unordered_map<CxxString, std::shared_ptr<OptionDetails>>
      >,
      std::vector<CxxString>,
      bool allow_unrecognised,
      int&, Char**&);

    size_t
    count(const CxxString& o) const
    {
      auto iter = m_options->find(o);
      if (iter == m_options->end())
      {
        return 0;
      }

      auto riter = m_results.find(iter->second);

      return riter->second.count();
    }

    const OptionValue&
    operator[](const CxxString& option) const
    {
      auto iter = m_options->find(option);

      if (iter == m_options->end())
      {
        throw_or_mimic<option_not_present_exception>(option);
      }

      auto riter = m_results.find(iter->second);

      return riter->second;
    }

    const std::vector<KeyValue>&
    arguments() const
    {
      return m_sequential;
    }

    private:

    void
    parse(int& argc, Char**& argv);

    void
    add_to_option(const CxxString& option, const CxxString& arg);

    bool
    consume_positional(CxxString a);

    void
    parse_option
    (
      std::shared_ptr<OptionDetails> value,
      const CxxString& name,
      const CxxString& arg = CxS("")
    );

    void
    parse_default(std::shared_ptr<OptionDetails> details);

    void
    checked_parse_arg
    (
      int argc,
      Char* argv[],
      int& current,
      std::shared_ptr<OptionDetails> value,
      const CxxString& name
    );

    const std::shared_ptr<
      std::unordered_map<CxxString, std::shared_ptr<OptionDetails>>
    > m_options;
    std::vector<CxxString> m_positional;
    std::vector<CxxString>::iterator m_next_positional;
    std::unordered_set<CxxString> m_positional_set;
    std::unordered_map<std::shared_ptr<OptionDetails>, OptionValue> m_results;

    bool m_allow_unrecognised;

    std::vector<KeyValue> m_sequential;
  };

  struct Option
  {
    Option
    (
      const CxxString& opts,
      const CxxString& desc,
      const std::shared_ptr<const Value>& value = ::cxxopts::value<bool>(),
      const CxxString& arg_help = CxS("")
    )
    : opts_(opts)
    , desc_(desc)
    , value_(value)
    , arg_help_(arg_help)
    {
    }

    CxxString opts_;
    CxxString desc_;
    std::shared_ptr<const Value> value_;
    CxxString arg_help_;
  };

  class Options
  {
    typedef std::unordered_map<CxxString, std::shared_ptr<OptionDetails>>
      OptionMap;
    public:

    Options(CxxString program, CxxString help_string = CxS(""))
    : m_program(std::move(program))
    , m_help_string(toLocalString(std::move(help_string)))
    , m_custom_help(CxS("[OPTION...]"))
    , m_positional_help(CxS("positional parameters"))
    , m_show_positional(false)
    , m_allow_unrecognised(false)
    , m_options(std::make_shared<OptionMap>())
    , m_next_positional(m_positional.end())
    {
    }

    Options&
    positional_help(CxxString help_text)
    {
      m_positional_help = std::move(help_text);
      return *this;
    }

    Options&
    custom_help(CxxString help_text)
    {
      m_custom_help = std::move(help_text);
      return *this;
    }

    Options&
    show_positional_help()
    {
      m_show_positional = true;
      return *this;
    }

    Options&
    allow_unrecognised_options()
    {
      m_allow_unrecognised = true;
      return *this;
    }

    ParseResult
    parse(int& argc, Char**& argv);

    OptionAdder
    add_options(CxxString group = CxS(""));

    void
    add_options
    (
      const CxxString& group,
      std::initializer_list<Option> options
    );

    void
    add_option
    (
      const CxxString& group,
      const Option& option
    );

    void
    add_option
    (
      const CxxString& group,
      const CxxString& s,
      const CxxString& l,
      CxxString desc,
      std::shared_ptr<const Value> value,
      CxxString arg_help
    );

    //parse positional arguments into the given option
    void
    parse_positional(CxxString option);

    void
    parse_positional(std::vector<CxxString> options);

    void
    parse_positional(std::initializer_list<CxxString> options);

    template <typename Iterator>
    void
    parse_positional(Iterator begin, Iterator end) {
      parse_positional(std::vector<CxxString>{begin, end});
    }

    CxxString
    help(const std::vector<CxxString>& groups = {}) const;

    const std::vector<CxxString>
    groups() const;

    const HelpGroupDetails&
    group_help(const CxxString& group) const;

    private:

    void
    add_one_option
    (
      const CxxString& option,
      std::shared_ptr<OptionDetails> details
    );

    UniString
    help_one_group(const CxxString& group) const;

    void
    generate_group_help
    (
      UniString& result,
      const std::vector<CxxString>& groups
    ) const;

    void
    generate_all_groups_help(UniString& result) const;

    CxxString m_program;
    UniString m_help_string;
    CxxString m_custom_help;
    CxxString m_positional_help;
    bool m_show_positional;
    bool m_allow_unrecognised;

    std::shared_ptr<OptionMap> m_options;
    std::vector<CxxString> m_positional;
    std::vector<CxxString>::iterator m_next_positional;
    std::unordered_set<CxxString> m_positional_set;

    //mapping from groups to help options
    std::map<CxxString, HelpGroupDetails> m_help;
  };

  class OptionAdder
  {
    public:

    OptionAdder(Options& options, CxxString group)
    : m_options(options), m_group(std::move(group))
    {
    }

    OptionAdder&
    operator()
    (
      const CxxString& opts,
      const CxxString& desc,
      std::shared_ptr<const Value> value
        = ::cxxopts::value<bool>(),
      CxxString arg_help = CxS("")
    );

    private:
    Options& m_options;
    CxxString m_group;
  };

  namespace
  {
    constexpr int OPTION_LONGEST = 30;
    constexpr int OPTION_DESC_GAP = 2;

    std::basic_regex<Char> option_matcher
      (CxS("--([[:alnum:]][-_[:alnum:]]+)(=(.*))?|-([[:alnum:]]+)"));

    std::basic_regex<Char> option_specifier
      (CxS("(([[:alnum:]]),)?[ ]*([[:alnum:]][-_[:alnum:]]*)?"));

    UniString
    format_option
    (
      const HelpOptionDetails& o
    )
    {
      auto& s = o.s;
      auto& l = o.l;

      UniString result = CxS("  ");

      if (s.size() > 0)
      {
        result += CxS("-") + toLocalString(s) + CxS(",");
      }
      else
      {
        result += CxS("   ");
      }

      if (l.size() > 0)
      {
        result += CxS(" --") + toLocalString(l);
      }

      auto arg = o.arg_help.size() > 0 ? toLocalString(o.arg_help) : CxS("arg");

      if (!o.is_boolean)
      {
        if (o.has_implicit)
        {
          result += CxS(" [=") + arg + CxS("(=") + toLocalString(o.implicit_value) + CxS(")]");
        }
        else
        {
          result += CxS(" ") + arg;
        }
      }

      return result;
    }

    UniString
    format_description
    (
      const HelpOptionDetails& o,
      size_t start,
      size_t width
    )
    {
      auto desc = o.desc;

      if (o.has_default && (!o.is_boolean || o.default_value != CxS("false")))
      {
        desc += toLocalString(CxS(" (default: ") + o.default_value + CxS(")"));
      }

      UniString result;

      auto current = std::begin(desc);
      auto startLine = current;
      auto lastSpace = current;

      auto size = size_t{};

      while (current != std::end(desc))
      {
        if (*current == CxS(' '))
        {
          lastSpace = current;
        }

        if (*current == CxS('\n'))
        {
          startLine = current + 1;
          lastSpace = startLine;
        }
        else if (size > width)
        {
          if (lastSpace == startLine)
          {
            stringAppend(result, startLine, current + 1);
            stringAppend(result, CxS("\n"));
            stringAppend(result, start, ' ');
            startLine = current + 1;
            lastSpace = startLine;
          }
          else
          {
            stringAppend(result, startLine, lastSpace);
            stringAppend(result, CxS("\n"));
            stringAppend(result, start, CxS(' '));
            startLine = lastSpace + 1;
          }
          size = 0;
        }
        else
        {
          ++size;
        }

        ++current;
      }

      //append whatever is left
      stringAppend(result, startLine, current);

      return result;
    }
  }

inline
ParseResult::ParseResult
(
  const std::shared_ptr<
    std::unordered_map<CxxString, std::shared_ptr<OptionDetails>>
  > options,
  std::vector<CxxString> positional,
  bool allow_unrecognised,
  int& argc, Char**& argv
)
: m_options(options)
, m_positional(std::move(positional))
, m_next_positional(m_positional.begin())
, m_allow_unrecognised(allow_unrecognised)
{
  parse(argc, argv);
}

inline
void
Options::add_options
(
  const CxxString &group,
  std::initializer_list<Option> options
)
{
 OptionAdder option_adder(*this, group);
 for (const auto &option: options)
 {
   option_adder(option.opts_, option.desc_, option.value_, option.arg_help_);
 }
}

inline
OptionAdder
Options::add_options(CxxString group)
{
  return OptionAdder(*this, std::move(group));
}

inline
OptionAdder&
OptionAdder::operator()
(
  const CxxString& opts,
  const CxxString& desc,
  std::shared_ptr<const Value> value,
  CxxString arg_help
)
{
  std::match_results<const Char*> result;
  std::regex_match(opts.c_str(), result, option_specifier);

  if (result.empty())
  {
    throw_or_mimic<invalid_option_format_error>(opts);
  }

  const auto& short_match = result[2];
  const auto& long_match = result[3];

  if (!short_match.length() && !long_match.length())
  {
    throw_or_mimic<invalid_option_format_error>(opts);
  } else if (long_match.length() == 1 && short_match.length())
  {
    throw_or_mimic<invalid_option_format_error>(opts);
  }

  auto option_names = []
  (
    const std::sub_match<const Char*>& short_,
    const std::sub_match<const Char*>& long_
  )
  {
    if (long_.length() == 1)
    {
      return std::make_tuple(long_.str(), short_.str());
    }
    else
    {
      return std::make_tuple(short_.str(), long_.str());
    }
  }(short_match, long_match);

  m_options.add_option
  (
    m_group,
    std::get<0>(option_names),
    std::get<1>(option_names),
    desc,
    value,
    std::move(arg_help)
  );

  return *this;
}

inline
void
ParseResult::parse_default(std::shared_ptr<OptionDetails> details)
{
  m_results[details].parse_default(details);
}

inline
void
ParseResult::parse_option
(
  std::shared_ptr<OptionDetails> value,
  const CxxString& /*name*/,
  const CxxString& arg
)
{
  auto& result = m_results[value];
  result.parse(value, arg);

  m_sequential.emplace_back(value->long_name(), arg);
}

inline
void
ParseResult::checked_parse_arg
(
  int argc,
  Char* argv[],
  int& current,
  std::shared_ptr<OptionDetails> value,
  const CxxString& name
)
{
  if (current + 1 >= argc)
  {
    if (value->value().has_implicit())
    {
      parse_option(value, name, value->value().get_implicit_value());
    }
    else
    {
      throw_or_mimic<missing_argument_exception>(name);
    }
  }
  else
  {
    if (value->value().has_implicit())
    {
      parse_option(value, name, value->value().get_implicit_value());
    }
    else
    {
      parse_option(value, name, argv[current + 1]);
      ++current;
    }
  }
}

inline
void
ParseResult::add_to_option(const CxxString& option, const CxxString& arg)
{
  auto iter = m_options->find(option);

  if (iter == m_options->end())
  {
    throw_or_mimic<option_not_exists_exception>(option);
  }

  parse_option(iter->second, option, arg);
}

inline
bool
ParseResult::consume_positional(CxxString a)
{
  while (m_next_positional != m_positional.end())
  {
    auto iter = m_options->find(*m_next_positional);
    if (iter != m_options->end())
    {
      auto& result = m_results[iter->second];
      if (!iter->second->value().is_container())
      {
        if (result.count() == 0)
        {
          add_to_option(*m_next_positional, a);
          ++m_next_positional;
          return true;
        }
        else
        {
          ++m_next_positional;
          continue;
        }
      }
      else
      {
        add_to_option(*m_next_positional, a);
        return true;
      }
    }
    else
    {
      throw_or_mimic<option_not_exists_exception>(*m_next_positional);
    }
  }

  return false;
}

inline
void
Options::parse_positional(CxxString option)
{
  parse_positional(std::vector<CxxString>{std::move(option)});
}

inline
void
Options::parse_positional(std::vector<CxxString> options)
{
  m_positional = std::move(options);
  m_next_positional = m_positional.begin();

  m_positional_set.insert(m_positional.begin(), m_positional.end());
}

inline
void
Options::parse_positional(std::initializer_list<CxxString> options)
{
  parse_positional(std::vector<CxxString>(std::move(options)));
}

inline
ParseResult
Options::parse(int& argc, Char**& argv)
{
  ParseResult result(m_options, m_positional, m_allow_unrecognised, argc, argv);
  return result;
}

inline
void
ParseResult::parse(int& argc, Char**& argv)
{
  int current = 1;

  int nextKeep = 1;

  bool consume_remaining = false;

  while (current != argc)
  {
    if (CxxStrcmp(argv[current], CxS("--")) == 0)
    {
      consume_remaining = true;
      ++current;
      break;
    }

    std::match_results<const Char*> result;
    std::regex_match(argv[current], result, option_matcher);

    if (result.empty())
    {
      //not a flag

      // but if it starts with a `-`, then it's an error
      if (argv[current][0] == CxS('-') && argv[current][1] != CxS('\0')) {
        if (!m_allow_unrecognised) {
          throw_or_mimic<option_syntax_exception>(argv[current]);
        }
      }

      //if true is returned here then it was consumed, otherwise it is
      //ignored
      if (consume_positional(argv[current]))
      {
      }
      else
      {
        argv[nextKeep] = argv[current];
        ++nextKeep;
      }
      //if we return from here then it was parsed successfully, so continue
    }
    else
    {
      //short or long option?
      if (result[4].length() != 0)
      {
        const CxxString& s = result[4];

        for (std::size_t i = 0; i != s.size(); ++i)
        {
          CxxString name(1, s[i]);
          auto iter = m_options->find(name);

          if (iter == m_options->end())
          {
            if (m_allow_unrecognised)
            {
              continue;
            }
            else
            {
              //error
              throw_or_mimic<option_not_exists_exception>(name);
            }
          }

          auto value = iter->second;

          if (i + 1 == s.size())
          {
            //it must be the last argument
            checked_parse_arg(argc, argv, current, value, name);
          }
          else if (value->value().has_implicit())
          {
            parse_option(value, name, value->value().get_implicit_value());
          }
          else
          {
            //error
            throw_or_mimic<option_requires_argument_exception>(name);
          }
        }
      }
      else if (result[1].length() != 0)
      {
        const CxxString& name = result[1];

        auto iter = m_options->find(name);

        if (iter == m_options->end())
        {
          if (m_allow_unrecognised)
          {
            // keep unrecognised options in argument list, skip to next argument
            argv[nextKeep] = argv[current];
            ++nextKeep;
            ++current;
            continue;
          }
          else
          {
            //error
            throw_or_mimic<option_not_exists_exception>(name);
          }
        }

        auto opt = iter->second;

        //equals provided for long option?
        if (result[2].length() != 0)
        {
          //parse the option given

          parse_option(opt, name, result[3]);
        }
        else
        {
          //parse the next argument
          checked_parse_arg(argc, argv, current, opt, name);
        }
      }

    }

    ++current;
  }

  for (auto& opt : *m_options)
  {
    auto& detail = opt.second;
    auto& value = detail->value();

    auto& store = m_results[detail];

    if(value.has_default() && !store.count() && !store.has_default()){
      parse_default(detail);
    }
  }

  if (consume_remaining)
  {
    while (current < argc)
    {
      if (!consume_positional(argv[current])) {
        break;
      }
      ++current;
    }

    //adjust argv for any that couldn't be swallowed
    while (current != argc) {
      argv[nextKeep] = argv[current];
      ++nextKeep;
      ++current;
    }
  }

  argc = nextKeep;

}

inline
void
Options::add_option
(
  const CxxString& group,
  const Option& option
)
{
    add_options(group, {option});
}

inline
void
Options::add_option
(
  const CxxString& group,
  const CxxString& s,
  const CxxString& l,
  CxxString desc,
  std::shared_ptr<const Value> value,
  CxxString arg_help
)
{
  auto stringDesc = toLocalString(std::move(desc));
  auto option = std::make_shared<OptionDetails>(s, l, stringDesc, value);

  if (s.size() > 0)
  {
    add_one_option(s, option);
  }

  if (l.size() > 0)
  {
    add_one_option(l, option);
  }

  //add the help details
  auto& options = m_help[group];

  options.options.emplace_back(HelpOptionDetails{s, l, stringDesc,
      value->has_default(), value->get_default_value(),
      value->has_implicit(), value->get_implicit_value(),
      std::move(arg_help),
      value->is_container(),
      value->is_boolean()});
}

inline
void
Options::add_one_option
(
  const CxxString& option,
  std::shared_ptr<OptionDetails> details
)
{
  auto in = m_options->emplace(option, details);

  if (!in.second)
  {
    throw_or_mimic<option_exists_error>(option);
  }
}

inline
UniString
Options::help_one_group(const CxxString& g) const
{
  typedef std::vector<std::pair<UniString, UniString>> OptionHelp;

  auto group = m_help.find(g);
  if (group == m_help.end())
  {
    return CxS("");
  }

  OptionHelp format;

  size_t longest = 0;

  UniString result;

  if (!g.empty())
  {
    result += toLocalString(CxS(" ") + g + CxS(" options:\n"));
  }

  for (const auto& o : group->second.options)
  {
    if (m_positional_set.find(o.l) != m_positional_set.end() &&
        !m_show_positional)
    {
      continue;
    }

    auto s = format_option(o);
    longest = (std::max)(longest, stringLength(s));
    format.push_back(std::make_pair(s, UniString()));
  }

  longest = (std::min)(longest, static_cast<size_t>(OPTION_LONGEST));

  //widest allowed description
  auto allowed = size_t{76} - longest - OPTION_DESC_GAP;

  auto fiter = format.begin();
  for (const auto& o : group->second.options)
  {
    if (m_positional_set.find(o.l) != m_positional_set.end() &&
        !m_show_positional)
    {
      continue;
    }

    auto d = format_description(o, longest + OPTION_DESC_GAP, allowed);

    result += fiter->first;
    if (stringLength(fiter->first) > longest)
    {
      result += CxS('\n');
      result += toLocalString(CxxString(longest + OPTION_DESC_GAP, ' '));
    }
    else
    {
      result += toLocalString(CxxString(longest + OPTION_DESC_GAP -
        stringLength(fiter->first),
        ' '));
    }
    result += d;
    result += CxS('\n');

    ++fiter;
  }

  return result;
}

inline
void
Options::generate_group_help
(
  UniString& result,
  const std::vector<CxxString>& print_groups
) const
{
  for (size_t i = 0; i != print_groups.size(); ++i)
  {
    const UniString& group_help_text = help_one_group(print_groups[i]);
    if (empty(group_help_text))
    {
      continue;
    }
    result += group_help_text;
    if (i < print_groups.size() - 1)
    {
      result += '\n';
    }
  }
}

inline
void
Options::generate_all_groups_help(UniString& result) const
{
  std::vector<UniString> all_groups;
  all_groups.reserve(m_help.size());

  for (auto& group : m_help)
  {
    all_groups.push_back(group.first);
  }

  generate_group_help(result, all_groups);
}

inline
CxxString
Options::help(const std::vector<CxxString>& help_groups) const
{
  UniString result = m_help_string + CxS("\nUsage:\n  ") +
    toLocalString(m_program) + CxS(" ") + toLocalString(m_custom_help);

  if (m_positional.size() > 0 && m_positional_help.size() > 0) {
    result += CxS(" ") + toLocalString(m_positional_help);
  }

  result += CxS("\n\n");

  if (help_groups.size() == 0)
  {
    generate_all_groups_help(result);
  }
  else
  {
    generate_group_help(result, help_groups);
  }

  return result;
  //return toUTF8String(result);
}

inline
const std::vector<CxxString>
Options::groups() const
{
  std::vector<CxxString> g;

  std::transform(
    m_help.begin(),
    m_help.end(),
    std::back_inserter(g),
    [] (const std::map<CxxString, HelpGroupDetails>::value_type& pair)
    {
      return pair.first;
    }
  );

  return g;
}

inline
const HelpGroupDetails&
Options::group_help(const CxxString& group) const
{
  return m_help.at(group);
}

}

#undef CxS

#endif //CXXOPTS_HPP_INCLUDED

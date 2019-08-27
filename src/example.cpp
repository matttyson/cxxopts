/*

Copyright (c) 2014 Jarryd Beck

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

#ifdef CXXOPTS_WIDE
#define STR(x) L##x
#else
#define STR(x) x
#endif

#include <iostream>

#include "cxxopts.hpp"

#ifdef CXXOPTS_WIDE
#define dcout std::wcout
#else
#define dcout dcout
#endif

cxxopts::ParseResult
#ifdef CXXOPTS_WIDE
parse(int argc, wchar_t* argv[])
#else
parse(int argc, char* argv[])
#endif
{
  try
  {
    cxxopts::Options options(argv[0], STR(" - example command line options"));
    options
      .positional_help(STR("[optional args]"))
      .show_positional_help();

    bool apple = false;

	options
      .allow_unrecognised_options()
      .add_options()
      (STR("a,apple"), STR("an apple"), cxxopts::value<bool>(apple))
      (STR("b,bob"), STR("Bob"))
      (STR("char"), STR("A character"), cxxopts::value<cxxopts::Char>())
      (STR("t,true"), STR("True"), cxxopts::value<bool>()->default_value(STR("true")))
      (STR("f, file"), STR("File"), cxxopts::value<std::vector<cxxopts::CxxString>>(), STR("FILE"))
      (STR("i,input"), STR("Input"), cxxopts::value<cxxopts::CxxString>())
      (STR("o,output"), STR("Output file"), cxxopts::value<cxxopts::CxxString>()
          ->default_value(STR("a.out"))->implicit_value(STR("b.def")), STR("BIN"))
      (STR("positional"),
        STR("Positional arguments: these are the arguments that are entered ")
        STR("without an option"), cxxopts::value<std::vector<cxxopts::CxxString>>())
      (STR("long-description"),
        STR("thisisareallylongwordthattakesupthewholelineandcannotbebrokenataspace"))
      (STR("help"), STR("Print help"))
      (STR("int"), STR("An integer"), cxxopts::value<int>(), STR("N"))
      (STR("float"), STR("A floating point number"), cxxopts::value<float>())
      (STR("vector"), STR("A list of doubles"), cxxopts::value<std::vector<double>>())
      (STR("option_that_is_too_long_for_the_help"), STR("A very long option"))
    #ifdef CXXOPTS_USE_UNICODE
      ("unicode", u8"A help option with non-ascii: à. Here the size of the"
        " string should be correct")
    #endif
    ;

    options.add_options(STR("Group"))
      (STR("c,compile"), STR("compile"))
      (STR("d,drop"), STR("drop"), cxxopts::value<std::vector<cxxopts::CxxString>>());

    options.parse_positional({STR("input"), STR("output"), STR("positional")});

    auto result = options.parse(argc, argv);

    if (result.count(STR("help")))
    {
      dcout << options.help({STR(""), STR("Group")}) << std::endl;
      exit(0);
    }

    if (apple)
    {
      dcout << STR("Saw option ‘a’") << result.count(STR("a")) << STR(" times ") <<
        std::endl;
    }

    if (result.count(STR("b")))
    {
      dcout << STR("Saw option ‘b’") << std::endl;
    }

    if (result.count(STR("char")))
    {
      dcout << STR("Saw a character ‘") << result[STR("char")].as<cxxopts::Char>() << STR("’") << std::endl;
    }

    if (result.count(STR("f")))
    {
      auto& ff = result[STR("f")].as<std::vector<cxxopts::CxxString>>();
      dcout << STR("Files") << std::endl;
      for (const auto& f : ff)
      {
        dcout << f << std::endl;
      }
    }

    if (result.count(STR("input")))
    {
      dcout << STR("Input = ") << result[STR("input")].as<cxxopts::CxxString>()
        << std::endl;
    }

    if (result.count(STR("output")))
    {
      dcout << STR("Output = ") << result[STR("output")].as<cxxopts::CxxString>()
        << std::endl;
    }

    if (result.count(STR("positional")))
    {
      dcout << STR("Positional = {");
      auto& v = result[STR("positional")].as<std::vector<cxxopts::CxxString>>();
      for (const auto& s : v) {
        dcout << s << STR(", ");
      }
      dcout << STR("}") << std::endl;
    }

    if (result.count(STR("int")))
    {
      dcout << STR("int = ") << result[STR("int")].as<int>() << std::endl;
    }

    if (result.count(STR("float")))
    {
      dcout << STR("float = ") << result[STR("float")].as<float>() << std::endl;
    }

    if (result.count(STR("vector")))
    {
      dcout << STR("vector = ");
      const auto values = result[STR("vector")].as<std::vector<double>>();
      for (const auto& v : values) {
        dcout << v << STR(", ");
      }
      dcout << std::endl;
    }

    dcout << STR("Arguments remain = ") << argc << std::endl;

    return result;

  } catch (const cxxopts::OptionException& e)
  {
    std::cout << "error parsing options: " << e.what() << std::endl;
    exit(1);
  }
}

#ifdef CXXOPTS_WIDE
int wmain(int argc, wchar_t* argv[])
#else
int main(int argc, char* argv[])
#endif
{
  auto result = parse(argc, argv);
  auto arguments = result.arguments();
  dcout << STR("Saw ") << arguments.size() << STR(" arguments") << std::endl;

  return 0;
}

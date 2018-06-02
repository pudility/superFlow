#include <string>
#include <fstream>
#include <streambuf>

/// Input code files (superflow)
static std::ifstream input_file_basic_add("input/comp/basic/add");
static std::ifstream input_file_basic_sub("input/comp/basic/sub");
static std::ifstream input_file_basic_mul("input/comp/basic/mul");
static std::ifstream input_file_basic_div("input/comp/basic/div");

static std::string string_basic_add;
static std::string string_basic_sub;
static std::string string_basic_mul;
static std::string string_basic_div;

static void input_init_strings () {
  std::ostringstream input_stream;

  // Read files into strings
  input_stream << input_file_basic_add.rdbuf();
  string_basic_add = input_stream.str();
  input_stream.clear();

  input_stream << input_file_basic_sub.rdbuf();
  string_basic_sub = input_stream.str();
  input_stream.clear();

  input_stream << input_file_basic_mul.rdbuf();
  string_basic_mul = input_stream.str();
  input_stream.clear();

  input_stream << input_file_basic_div.rdbuf();
  string_basic_div = input_stream.str();
  input_stream.clear();
}

/// Output files (plain text)
static std::ifstream output_file_basic_add("output/comp/basic/add");
static std::ifstream output_file_basic_sub("output/comp/basic/sub");
static std::ifstream output_file_basic_mul("output/comp/basic/mul");
static std::ifstream output_file_basic_div("output/comp/basic/div");

static std::string out_string_basic_add;
static std::string out_string_basic_sub;
static std::string out_string_basic_mul;
static std::string out_string_basic_div;

static void output_init_strings () {
  std::ostringstream output_stream;

  // Read files into strings
  output_stream << output_file_basic_add.rdbuf();
  out_string_basic_add = output_stream.str();
  output_stream.clear();

  output_stream << output_file_basic_sub.rdbuf();
  out_string_basic_sub = output_stream.str();
  output_stream.clear();

  output_stream << output_file_basic_mul.rdbuf();
  out_string_basic_mul = output_stream.str();
  output_stream.clear();

  output_stream << output_file_basic_div.rdbuf();
  out_string_basic_div = output_stream.str();
  output_stream.clear();

}

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


// Read files into strings
string_basic_add << input_file_basic_add.rdbuf();
string_basic_sub << input_file_basic_sub.rdbuf();
string_basic_mul << input_file_basic_mul.rdbuf();
string_basic_div << input_file_basic_div.rdbuf();

/// Output files (plain text)
static std::ifstream output_file_basic_add("output/comp/basic/add");
static std::ifstream output_file_basic_sub("output/comp/basic/sub");
static std::ifstream output_file_basic_mul("output/comp/basic/mul");
static std::ifstream output_file_basic_div("output/comp/basic/div");

static std::string out_string_basic_add;
static std::string out_string_basic_sub;
static std::string out_string_basic_mul;
static std::string out_string_basic_div;


// Read files into strings
out_string_basic_add << output_file_basic_add.rdbuf();
out_string_basic_sub << output_file_basic_sub.rdbuf();
out_string_basic_mul << output_file_basic_mul.rdbuf();
out_string_basic_div << output_file_basic_div.rdbuf();

#include "utils.hpp"
#include <regex>
#include "Log.hpp"
#include <cstdarg>
#include <sstream>

void
confess_at(const char *file, int line, const char *func,
            const char *pat, ...)
{
    std::stringstream ss;
    ss << "Error in function " << func << " at " << file << ":" << line << ": ";
    ss << pat << "\n";

    Slic3r::Log::error(std::string("Libslic3r"), ss.str() );
}

std::vector<std::string> 
split_at_regex(const std::string& input, const std::string& regex) {
    // passing -1 as the submatch index parameter performs splitting
    std::regex re(regex);
    std::sregex_token_iterator
        first{input.begin(), input.end(), re, -1},
        last;
    return {first, last};
}

std::string _trim_zeroes(std::string in) { return trim_zeroes(in); }
/// Remove extra zeroes generated from std::to_string on doubles
std::string trim_zeroes(std::string in) {
    std::string result {""};
    std::regex strip_zeroes("(0*)$ ");
    std::regex_replace (std::back_inserter(result), in.begin(), in.end(), strip_zeroes, "");
    if (result.back() == '.') result.append("0");
    return result;
}
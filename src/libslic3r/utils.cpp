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

namespace Slic3r {

static std::string g_var_dir;
static std::string g_resources_dir;
static std::string g_localization_dir;
static std::string g_data_dir;

void set_var_dir(const std::string &path) { g_var_dir = path; }
const std::string& var_dir() { return g_var_dir; }
std::string var(const std::string &file_name) { return g_var_dir + (g_var_dir.empty() ? "" : "/") + file_name; }

void set_resources_dir(const std::string &path) { g_resources_dir = path; }
const std::string& resources_dir() { return g_resources_dir; }

void set_local_dir(const std::string &path) { g_localization_dir = path; }
const std::string& localization_dir() { return g_localization_dir; }

void set_data_dir(const std::string &path) { g_data_dir = path; }
const std::string& data_dir() { return g_data_dir; }

} // namespace Slic3r
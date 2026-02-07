#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <Windows.h>
#endif
#include "utils.hpp"
#include <regex>
#include "Log.hpp"
#include <cstdarg>
#include <sstream>
#include <boost/filesystem.hpp>
#include <thread>
#include <chrono>
#include <fstream>

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

std::error_code rename_file(const std::string &from, const std::string &to)
{
    boost::filesystem::path path_from(from);
    boost::filesystem::path path_to(to);
    boost::system::error_code ec;
    
    // Windows-specific robust renaming
#ifdef _WIN32
    // Windows-specific robust renaming
    std::wstring wfrom = path_from.wstring();
    std::wstring wto = path_to.wstring();

    for (int i = 0; i < 10; ++i) { // 1 second total
        if (i > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Ensure the target file is not read-only (common cause of Access Denied)
        if (boost::filesystem::exists(path_to)) {
            SetFileAttributesW(wto.c_str(), FILE_ATTRIBUTE_NORMAL);
        }

        if (MoveFileExW(wfrom.c_str(), wto.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
            return std::error_code();
        }

        DWORD err = GetLastError();
        // If the source is missing, we can't continue.
        if (err == ERROR_FILE_NOT_FOUND && !boost::filesystem::exists(path_from)) {
            ec = boost::system::error_code(err, boost::system::system_category());
            break;
        }
        
        // Retry for sharing violations or access denied (often temporary locks)
        if (err != ERROR_SHARING_VIOLATION && err != ERROR_ACCESS_DENIED) {
             ec = boost::system::error_code(err, boost::system::system_category());
             break;
        }
        ec = boost::system::error_code(err, boost::system::system_category());
    }
#else
    // Generic implementation for other platforms
    for (int i = 0; i < 10; ++i) {
        if (i > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        if (boost::filesystem::exists(path_to)) {
            boost::system::error_code ec_remove;
            boost::filesystem::remove(path_to, ec_remove);
            if (ec_remove) {
                ec = ec_remove;
                continue;
            }
        }

        boost::filesystem::rename(path_from, path_to, ec);
        if (!ec)
            return std::error_code();
        
        if (ec != boost::system::errc::permission_denied &&
            ec != boost::system::errc::device_or_resource_busy) {
            break;
        }
    }
#endif
    return std::error_code(ec.value(), std::system_category());
}

CopyFileResult copy_file(const std::string &from, const std::string &to, std::string& error_message, const bool with_check)
{
    try {
        boost::filesystem::path path_from(from);
        boost::filesystem::path path_to(to);
        if (boost::filesystem::exists(path_to)) {
            boost::filesystem::remove(path_to);
        }
        boost::filesystem::copy_file(path_from, path_to);
        if (with_check) {
            if (check_copy(from, to) != SUCCESS)
                return FAIL_FILES_DIFFERENT;
        }
        return SUCCESS;
    } catch (const std::exception &ex) {
        error_message = ex.what();
        return FAIL_COPY_FILE;
    }
}

CopyFileResult check_copy(const std::string& origin, const std::string& copy)
{
    std::ifstream f1(origin, std::ios::binary);
    std::ifstream f2(copy, std::ios::binary);

    if (!f1.is_open()) return FAIL_CHECK_ORIGIN_NOT_OPENED;
    if (!f2.is_open()) return FAIL_CHECK_TARGET_NOT_OPENED;

    if (std::equal(std::istreambuf_iterator<char>(f1), std::istreambuf_iterator<char>(),
                   std::istreambuf_iterator<char>(f2)))
        return SUCCESS;
    else
        return FAIL_FILES_DIFFERENT;
}

} // namespace Slic3r
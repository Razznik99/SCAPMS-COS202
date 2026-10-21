// =============================================================================
//  File   : src/utils/FileHandler.cpp
// =============================================================================
#include "FileHandler.h"

#include <filesystem>
#include <fstream>

#include "Exceptions.h"
#include "StringUtils.h"

namespace fs = std::filesystem;

namespace scapms::utils {

std::vector<std::string> FileHandler::readLines(const std::string& path) {
    std::ifstream input(path, std::ios::in);
    if (!input.is_open()) {
        throw FileException("Unable to open the data file for reading", path);
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    if (input.bad()) {
        throw FileException("An I/O error occurred while reading the data file", path);
    }
    return lines;
}

void FileHandler::writeLines(const std::string& path,
                             const std::vector<std::string>& lines) {
    const fs::path target(path);
    if (target.has_parent_path()) {
        std::error_code ec;
        fs::create_directories(target.parent_path(), ec);
    }

    std::ofstream output(path, std::ios::out | std::ios::trunc);
    if (!output.is_open()) {
        throw FileException("Unable to open the data file for writing", path);
    }
    for (const auto& line : lines) {
        output << line << '\n';
    }
    output.flush();
    if (!output.good()) {
        throw FileException("An I/O error occurred while writing the data file", path);
    }
}

void FileHandler::appendLine(const std::string& path, const std::string& line) {
    std::ofstream output(path, std::ios::out | std::ios::app);
    if (!output.is_open()) {
        throw FileException("Unable to append to the data file", path);
    }
    output << line << '\n';
}

bool FileHandler::exists(const std::string& path) {
    std::error_code ec;
    return fs::exists(path, ec) && fs::is_regular_file(path, ec);
}

void FileHandler::ensureDirectory(const std::string& path) {
    if (path.empty()) {
        return;
    }
    std::error_code ec;
    fs::create_directories(path, ec);
    if (ec && !fs::is_directory(path)) {
        throw FileException("Unable to create the data directory", path);
    }
}

bool FileHandler::backup(const std::string& path) {
    if (!exists(path)) {
        return false;
    }
    std::error_code ec;
    fs::copy_file(path, path + ".bak", fs::copy_options::overwrite_existing, ec);
    return !ec;
}

std::string FileHandler::fileName(const std::string& path) {
    return fs::path(path).filename().string();
}

std::string FileHandler::extension(const std::string& path) {
    return toLower(fs::path(path).extension().string());
}

std::string FileHandler::joinPath(const std::string& directory,
                                  const std::string& name) {
    if (directory.empty()) {
        return name;
    }
    return (fs::path(directory) / name).string();
}

} // namespace scapms::utils

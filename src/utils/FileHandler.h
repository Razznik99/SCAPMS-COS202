// =============================================================================
//  File   : src/utils/FileHandler.h
//  Layer  : Utilities
//  Purpose: Thin, exception-safe wrapper around text-file I/O. The model layer
//           only ever sees vectors of lines, so the storage format can change
//           without touching business logic.
// =============================================================================
#ifndef SCAPMS_UTILS_FILEHANDLER_H
#define SCAPMS_UTILS_FILEHANDLER_H

#include <string>
#include <vector>

namespace scapms::utils {

class FileHandler {
public:
    /// Reads every line of a UTF-8 text file. Throws FileException on failure.
    static std::vector<std::string> readLines(const std::string& path);

    /// Writes lines to a UTF-8 text file (truncating). Throws FileException.
    static void writeLines(const std::string& path,
                           const std::vector<std::string>& lines);

    /// Appends a single line, creating the file when necessary.
    static void appendLine(const std::string& path, const std::string& line);

    /// True when the path exists and is a regular file.
    static bool exists(const std::string& path);

    /// Creates a directory (and parents) when missing.
    static void ensureDirectory(const std::string& path);

    /// Copies `path` to `path + ".bak"` when it exists. Best effort.
    static bool backup(const std::string& path);

    /// Last path component.
    static std::string fileName(const std::string& path);

    /// Lower-cased extension including the dot (".csv").
    static std::string extension(const std::string& path);

    /// Joins two path fragments with the platform separator.
    static std::string joinPath(const std::string& directory,
                                const std::string& name);
};

} // namespace scapms::utils

#endif // SCAPMS_UTILS_FILEHANDLER_H

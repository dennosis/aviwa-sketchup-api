#pragma once

#include "Config.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

struct FileMetadata
{
    std::string name;
    std::string path;
    std::string createdAt;
    std::string modifiedAt;
};

class FileService
{
private:
    std::string timeToString(fs::file_time_type ftime)
    {
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
        std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
        std::tm *gmt = std::localtime(&tt);
        std::stringstream ss;
        ss << std::put_time(gmt, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

public:
    std::vector<FileMetadata> listFilesByExtension(std::string extension)
    {
        std::vector<FileMetadata> fileList;

        if (!extension.empty() && extension[0] != '.')
        {
            extension = "." + extension;
        }

        fs::path rootPath = Config::getDataPath();

        if (!fs::exists(rootPath) || !fs::is_directory(rootPath))
        {
            fs::create_directories(rootPath);
            return fileList;
        }

        for (const auto &entry : fs::recursive_directory_iterator(rootPath))
        {
            if (fs::is_regular_file(entry) && entry.path().extension() == extension)
            {
                FileMetadata data;

                data.name = entry.path().filename().generic_string();
                std::string absPath = fs::absolute(entry.path()).string();
                std::replace(absPath.begin(), absPath.end(), '\\', '/');
                data.path = absPath;

                // No Windows/Linux, o filesystem padrão retorna o last_write_time
                // A data de criação "pura" pode variar por SO, então usamos a última modificação
                // para garantir compatibilidade cross-platform simples.
                data.modifiedAt = timeToString(fs::last_write_time(entry));
                data.createdAt = data.modifiedAt; // Simplificação comum em C++17

                fileList.push_back(data);
            }
        }

        return fileList;
    }
};

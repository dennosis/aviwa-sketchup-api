#pragma once

#include "utils/TempFileManager.hpp"
#include "model/TempFileModel.hpp"

#include <filesystem>
#include <chrono>
#include <random>
#include <string>
#include <optional>

class TempFileService
{
private:
    OATPP_COMPONENT(std::shared_ptr<TempPath>, m_tempPath);

    mutable std::mt19937_64 m_rng{std::random_device{}()};

    std::string uniqueToken() const
    {
        const auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto rand = m_rng();
        return std::to_string(ts) + "_" + std::to_string(rand);
    }

public:
    void init() const
    {
        TempFileManager::instance().init(m_tempPath->value);
    }

    void shutdown() const
    {
        TempFileManager::instance().shutdown();
    }

    // Para arquivos SKP gerados internamente: <dir>/<prefix>_<token>.skp
    [[nodiscard]]
    std::filesystem::path generatePath(const std::string &name,
                                       const std::string &extension = "") const
    {
        const auto token = uniqueToken();
        const auto ext = extension.empty() ? "" : ("." + extension);

        return std::filesystem::path(m_tempPath->value) / (name + "_" + token + ext);
    }

    [[nodiscard]]
    std::string registerFile(const std::filesystem::path &filepath,
                             const std::string &originalName,
                             const std::string &mimeType,
                             std::size_t sizeBytes) const
    {
        return TempFileManager::instance().registerFile(
            filepath, originalName, mimeType, sizeBytes);
    }

    [[nodiscard]]
    std::optional<TempFileEntry> get(const std::string &id) const
    {
        return TempFileManager::instance().get(id);
    }

    bool remove(const std::string &id) const
    {
        return TempFileManager::instance().remove(id);
    }

    [[nodiscard]]
    static long long defaultTtlSeconds()
    {
        return std::chrono::duration_cast<std::chrono::seconds>(
                   TempFileManager::DEFAULT_TTL)
            .count();
    }
};
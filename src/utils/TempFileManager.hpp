#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <thread>
#include <atomic>
#include <filesystem>
#include <functional>
#include <optional>

namespace fs = std::filesystem;

struct TempFileEntry {
    std::string id;
    fs::path    filepath;
    std::string originalName;
    std::string mimeType;
    std::size_t sizeBytes;
    std::chrono::steady_clock::time_point uploadedAt;
};

class TempFileManager {
public:
    // Duração antes de expirar (padrão: 2 minutos)
    static constexpr std::chrono::seconds DEFAULT_TTL{120};
    // Intervalo de varredura do cleanup (padrão: 30 segundos)
    static constexpr std::chrono::seconds CLEANUP_INTERVAL{30};

    static TempFileManager& instance();

    // Não copiável
    TempFileManager(const TempFileManager&)            = delete;
    TempFileManager& operator=(const TempFileManager&) = delete;

    // Inicializa o diretório base e inicia a thread de cleanup
    void init(const fs::path& baseDir,
              std::chrono::seconds ttl = DEFAULT_TTL);

    // Registra um arquivo já gravado em disco; retorna o ID gerado
    std::string registerFile(const fs::path& filepath,
                             const std::string& originalName,
                             const std::string& mimeType,
                             std::size_t sizeBytes);

    // Retorna a entrada, se existir e não tiver expirado
    std::optional<TempFileEntry> get(const std::string& id) const;

    // Remove explicitamente e deleta do disco
    bool remove(const std::string& id);

    // Caminho base onde os arquivos são gravados
    const fs::path& baseDir() const { return m_baseDir; }

    // Para a thread de cleanup (chamado no shutdown da aplicação)
    void shutdown();

    ~TempFileManager();

private:
    TempFileManager() = default;

    void cleanupLoop();
    void purgeExpired();
    std::string generateId() const;

    fs::path                                           m_baseDir;
    std::chrono::seconds                               m_ttl{DEFAULT_TTL};
    mutable std::mutex                                 m_mutex;
    std::unordered_map<std::string, TempFileEntry>     m_files;
    std::thread                                        m_cleanupThread;
    std::atomic<bool>                                  m_running{false};
};
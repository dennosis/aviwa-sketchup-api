#include "TempFileManager.hpp"

#include <random>
#include <sstream>
#include <iomanip>
#include <iostream>

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------
TempFileManager &TempFileManager::instance()
{
    static TempFileManager inst;
    return inst;
}

// ---------------------------------------------------------------------------
// init
// ---------------------------------------------------------------------------
void TempFileManager::init(const fs::path &baseDir, std::chrono::seconds ttl)
{
    m_baseDir = baseDir;
    m_ttl = ttl;

    fs::create_directories(m_baseDir);

    m_running = true;
    m_cleanupThread = std::thread(&TempFileManager::cleanupLoop, this);
}

// ---------------------------------------------------------------------------
// registerFile
// ---------------------------------------------------------------------------
std::string TempFileManager::registerFile(const fs::path &filepath,
                                          const std::string &originalName,
                                          const std::string &mimeType,
                                          std::size_t sizeBytes)
{
    TempFileEntry entry;
    entry.id = generateId();
    entry.filepath = filepath;
    entry.originalName = originalName;
    entry.mimeType = mimeType;
    entry.sizeBytes = sizeBytes;
    entry.uploadedAt = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lock(m_mutex);
    auto id = entry.id;
    m_files.emplace(id, std::move(entry));
    return id;
}

// ---------------------------------------------------------------------------
// get
// ---------------------------------------------------------------------------
std::optional<TempFileEntry> TempFileManager::get(const std::string &id) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_files.find(id);
    if (it == m_files.end())
        return std::nullopt;

    auto age = std::chrono::steady_clock::now() - it->second.uploadedAt;
    if (age > m_ttl)
        return std::nullopt; // expirado mas ainda não removido pelo cleanup

    return it->second;
}

// ---------------------------------------------------------------------------
// remove
// ---------------------------------------------------------------------------
bool TempFileManager::remove(const std::string &id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_files.find(id);
    if (it == m_files.end())
        return false;

    std::error_code ec;
    fs::remove(it->second.filepath, ec);
    if (ec)
    {
        std::cerr << "[TempFileManager] Erro ao remover "
                  << it->second.filepath << ": " << ec.message() << "\n";
    }
    m_files.erase(it);
    return true;
}

// ---------------------------------------------------------------------------
// shutdown
// ---------------------------------------------------------------------------
void TempFileManager::shutdown()
{
    m_running = false;
    if (m_cleanupThread.joinable())
        m_cleanupThread.join();
}

TempFileManager::~TempFileManager()
{
    shutdown();
}

// ---------------------------------------------------------------------------
// cleanupLoop (thread de background)
// ---------------------------------------------------------------------------
void TempFileManager::cleanupLoop()
{
    while (m_running)
    {
        std::this_thread::sleep_for(CLEANUP_INTERVAL);
        purgeExpired();
    }
}

void TempFileManager::purgeExpired()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto now = std::chrono::steady_clock::now();

    for (auto it = m_files.begin(); it != m_files.end();)
    {
        auto age = now - it->second.uploadedAt;
        if (age > m_ttl)
        {
            std::error_code ec;
            fs::remove(it->second.filepath, ec);
            std::cout << "[TempFileManager] Arquivo expirado removido: "
                      << it->second.id << " (" << it->second.originalName << ")\n";
            it = m_files.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

// ---------------------------------------------------------------------------
// generateId — UUID v4 simples sem dependências externas
// ---------------------------------------------------------------------------
std::string TempFileManager::generateId() const
{
    thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<uint64_t> dist;

    uint64_t a = dist(rng);
    uint64_t b = dist(rng);

    // Ajusta bits de versão (4) e variante (2)
    a = (a & 0xFFFFFFFFFFFF0FFFull) | 0x0000000000004000ull;
    b = (b & 0x3FFFFFFFFFFFFFFFull) | 0x8000000000000000ull;

    std::ostringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(8) << (a >> 32)
       << '-'
       << std::setw(4) << ((a >> 16) & 0xFFFF)
       << '-'
       << std::setw(4) << (a & 0xFFFF)
       << '-'
       << std::setw(4) << (b >> 48)
       << '-'
       << std::setw(12) << (b & 0x0000FFFFFFFFFFFFull);
    return ss.str();
}
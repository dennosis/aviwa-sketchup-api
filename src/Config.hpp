#pragma once

#include <string>
#include <filesystem>
#include <cstdlib>

class Config
{
public:
    static constexpr const char *SERVER_HOST = "0.0.0.0";
    static constexpr uint16_t SERVER_PORT = 8000;
    static constexpr const char *SKP_TEMP_FOLDER = "C:/temp/sketchup_models";

    /**
     * Retorna o caminho dos dados.
     * Prioridade 1: Variável de ambiente "APP_DATA_PATH"
     * Prioridade 2: {Caminho Atual do Projeto}/data
     */
    static std::string getDataPath()
    {
        // 1. Tenta buscar da variável de ambiente
        std::string envPath = getEnvVar("APP_DATA_PATH");
        if (!envPath.empty())
        {
            return envPath;
        }

        // 2. Lógica para subir de build/Debug para a raiz
        namespace fs = std::filesystem;

        // Caminho atual: .../projeto/build/Debug
        fs::path current = fs::current_path();

        // Sobe para 'build' e depois para a raiz do projeto
        fs::path projectRoot = current.parent_path().parent_path();

        // Retorna .../projeto/data
        return (projectRoot / "data").string();
    }

    // Auxiliar para buscar no Sistema Operacional
    static std::string getEnvVar(const std::string &key)
    {
        // No Windows moderno, getenv pode gerar avisos de segurança,
        // mas para projetos de homelab/estudo funciona perfeitamente.
        char *val = std::getenv(key.c_str());
        return (val == nullptr) ? "" : std::string(val);
    }
};

#pragma once

#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/web/protocol/http/Http.hpp"

namespace SkpResponseBuilder
{
    // -------------------------------------------------------------------------
    // Lê um arquivo binário para um buffer e remove o arquivo do disco.
    // -------------------------------------------------------------------------
    inline std::shared_ptr<std::string> readFileToBuffer(const std::string &skpPath)
    {
        std::ifstream ifs(skpPath, std::ios::binary | std::ios::ate);
        OATPP_ASSERT_HTTP(ifs.is_open(), oatpp::web::protocol::http::Status::CODE_500,
                          "Falha ao abrir arquivo gerado");

        auto size = ifs.tellg();
        ifs.seekg(0);
        auto buffer = std::make_shared<std::string>(size, '\0');
        ifs.read(buffer->data(), size);
        ifs.close();

        std::filesystem::remove(skpPath);

        return buffer;
    }

    // -------------------------------------------------------------------------
    // Monta a resposta de download a partir de um buffer já carregado.
    // -------------------------------------------------------------------------
    inline auto buildResponseFromBuffer(
        oatpp::web::server::api::ApiController *controller,
        const std::shared_ptr<std::string> &buffer,
        const std::string &filename = "model.skp")
        -> std::shared_ptr<oatpp::web::server::api::ApiController::OutgoingResponse>
    {
        auto response = controller->createResponse(
            oatpp::web::protocol::http::Status::CODE_200,
            oatpp::String(buffer->c_str(), buffer->size()));

        response->putHeader("Content-Type", "application/octet-stream");
        response->putHeader("Content-Disposition",
                            "attachment; filename=\"" + filename + "\"");
        return response;
    }
}
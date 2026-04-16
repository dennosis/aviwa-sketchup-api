#pragma once

#include "service/TempFileService.hpp"

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/web/mime/multipart/InMemoryDataProvider.hpp"
#include "oatpp/web/mime/multipart/Reader.hpp"
#include "oatpp/web/mime/multipart/PartList.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"
#include "oatpp/parser/json/mapping/ObjectMapper.hpp"

#include <fstream>
#include <sstream>

#include OATPP_CODEGEN_BEGIN(ApiController)

namespace mp = oatpp::web::mime::multipart;

class TempFileController : public oatpp::web::server::api::ApiController
{
private:
    OATPP_COMPONENT(std::shared_ptr<TempFileService>, m_tempFileService);

public:
    TempFileController(const std::shared_ptr<oatpp::data::mapping::ObjectMapper> &objectMapper)
        : oatpp::web::server::api::ApiController(objectMapper)
    {
        m_tempFileService->init();
    }

    static std::shared_ptr<TempFileController> createShared()
    {
        OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper);
        return std::make_shared<TempFileController>(objectMapper);
    }

    ENDPOINT("POST", "/temp/upload", uploadFile,
             REQUEST(std::shared_ptr<IncomingRequest>, request))
    {
        mp::PartList multipart(request->getHeaders());
        mp::Reader multipartReader(&multipart);
        multipartReader.setDefaultPartReader(
            mp::createInMemoryPartReader(50 * 1024 * 1024));
        request->transferBody(&multipartReader);

        auto part = multipart.getNamedPart("file");
        OATPP_ASSERT_HTTP(part, Status::CODE_400, "Campo 'file' ausente no multipart");
        OATPP_ASSERT_HTTP(part->getFilename(), Status::CODE_400, "filename ausente no campo 'file'");

        std::string mimeType = "application/octet-stream";
        if (auto ct = part->getHeaders().get("Content-Type"))
            mimeType = ct->c_str();

        const fs::path originalName = part->getFilename()->c_str();
        const std::string fileName = originalName.stem().string();
        const std::string fileExtension = originalName.extension().string().substr(1);

        const auto destPath = m_tempFileService->generatePath(fileName, fileExtension);

        {
            std::ofstream ofs(destPath, std::ios::binary);
            OATPP_ASSERT_HTTP(ofs.is_open(), Status::CODE_500, "Falha ao gravar arquivo temporário");

            auto stream = part->getPayload()->openInputStream();
            v_char8 buf[4096];
            oatpp::v_io_size n;
            while ((n = stream->readSimple(buf, sizeof(buf))) > 0)
                ofs.write(reinterpret_cast<const char *>(buf), n);
        }

        const auto fileSize = fs::file_size(destPath);
        const auto id = m_tempFileService->registerFile(destPath, fileName, mimeType, fileSize);

        auto json = oatpp::String(buildJson({
            {"id", id},
            {"name", fileName},
            {"mimeType", mimeType},
            {"size", std::to_string(fileSize)},
            {"expiresInSec", std::to_string(TempFileService::defaultTtlSeconds())},
        }));

        auto response = createResponse(Status::CODE_200, json);
        response->putHeader("Content-Type", "application/json");
        return response;
    }

    ENDPOINT("DELETE", "/temp/{id}", deleteFile,
             PATH(String, id))
    {
        OATPP_ASSERT_HTTP(
            m_tempFileService->remove(id->c_str()),
            Status::CODE_404,
            "Arquivo temporário não encontrado");

        return createResponse(Status::CODE_204, "");
    }

private:
    static std::string buildJson(
        std::initializer_list<std::pair<std::string, std::string>> pairs)
    {
        std::ostringstream ss;
        ss << '{';
        bool first = true;
        for (auto &[k, v] : pairs)
        {
            if (!first)
                ss << ',';
            first = false;
            const bool isNum = !v.empty() && std::all_of(v.begin(), v.end(), ::isdigit);
            ss << '"' << k << "\":";
            isNum ? ss << v : ss << '"' << v << '"';
        }
        ss << '}';
        return ss.str();
    }
};

#include OATPP_CODEGEN_END(ApiController)
#pragma once

#include "utils/TempFileManager.hpp"

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/web/mime/multipart/InMemoryDataProvider.hpp"
#include "oatpp/web/mime/multipart/TemporaryFileProvider.hpp"
#include "oatpp/web/mime/multipart/Reader.hpp"
#include "oatpp/web/mime/multipart/PartList.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"
#include "oatpp/parser/json/mapping/ObjectMapper.hpp"
#include "model/TempFileModel.hpp"

#include <fstream>
#include <sstream>

#include OATPP_CODEGEN_BEGIN(ApiController)

namespace mp = oatpp::web::mime::multipart;

class TempFileController : public oatpp::web::server::api::ApiController
{
private:
    OATPP_COMPONENT(std::shared_ptr<TempPath>, m_tempPath);

public:
    TempFileController(const std::shared_ptr<oatpp::data::mapping::ObjectMapper> &objectMapper)
        : oatpp::web::server::api::ApiController(objectMapper)
    {
        TempFileManager::instance().init(m_tempPath->value);
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
            mp::createInMemoryPartReader(50 * 1024 * 1024)); // 50 MB

        request->transferBody(&multipartReader);

        auto part = multipart.getNamedPart("file");
        OATPP_ASSERT_HTTP(part, Status::CODE_400, "Campo 'file' ausente no multipart");
        OATPP_ASSERT_HTTP(part->getFilename(), Status::CODE_400, "filename ausente no campo 'file'");

        std::string originalName = part->getFilename()->c_str();

        std::string mimeType = "application/octet-stream";
        auto ctHeader = part->getHeaders().get("Content-Type");
        if (ctHeader)
            mimeType = ctHeader->c_str();

        fs::path destPath = fs::path(m_tempPath->value) / (std::to_string(std::chrono::steady_clock::now()
                                                                              .time_since_epoch()
                                                                              .count()) +
                                                           "_" + originalName);

        {
            std::ofstream ofs(destPath, std::ios::binary);
            OATPP_ASSERT_HTTP(ofs.is_open(), Status::CODE_500,
                              "Falha ao gravar arquivo temporário");

            auto stream = part->getPayload()->openInputStream();
            v_char8 buf[4096];
            oatpp::v_io_size n;
            while ((n = stream->readSimple(buf, sizeof(buf))) > 0)
                ofs.write(reinterpret_cast<const char *>(buf), n);
        }

        auto fileSize = fs::file_size(destPath);
        auto id = TempFileManager::instance().registerFile(
            destPath, originalName, mimeType, fileSize);
        auto ttlSec = std::chrono::duration_cast<std::chrono::seconds>(
                          TempFileManager::DEFAULT_TTL)
                          .count();

        auto json = oatpp::String(buildJson({{"id", id},
                                             {"name", originalName},
                                             {"mimeType", mimeType},
                                             {"size", std::to_string(fileSize)},
                                             {"expiresInSec", std::to_string(ttlSec)}}));

        auto response = createResponse(Status::CODE_200, json);
        response->putHeader("Content-Type", "application/json");
        return response;
    }

    ENDPOINT("DELETE", "/temp/{id}", deleteFile,
             PATH(String, id))
    {
        bool removed = TempFileManager::instance().remove(id->c_str());
        OATPP_ASSERT_HTTP(removed, Status::CODE_404,
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
            bool isNum = !v.empty() && std::all_of(v.begin(), v.end(), ::isdigit);
            ss << '"' << k << "\":";
            if (isNum)
                ss << v;
            else
                ss << '"' << v << '"';
        }
        ss << '}';
        return ss.str();
    }
};

#include OATPP_CODEGEN_END(ApiController)
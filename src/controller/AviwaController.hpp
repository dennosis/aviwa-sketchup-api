#pragma once

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/parser/json/mapping/ObjectMapper.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"
#include "oatpp/encoding/Hex.hpp"

#include "service/AviwaService.hpp"
#include "utils/TempFileManager.hpp"

#include "dto/CommonDtos.hpp"
#include "dto/FileDtos.hpp"
#include "mapper/SketchupComponentMapper.hpp"
#include "mapper/FileMapper.hpp"
#include "model/TempFileModel.hpp"
#include "utils/SkpResponseBuilder.hpp"
#include "common/messages.hpp"
#include "common/validation.hpp"

#include OATPP_CODEGEN_BEGIN(ApiController)

class AviwaController : public oatpp::web::server::api::ApiController
{
private:
    OATPP_COMPONENT(std::shared_ptr<AviwaService>, m_aviwaService);

public:
    AviwaController(const std::shared_ptr<oatpp::data::mapping::ObjectMapper> &objectMapper)
        : oatpp::web::server::api::ApiController(objectMapper)
    {
    }

    static std::shared_ptr<AviwaController> createShared()
    {
        OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper);

        return std::make_shared<AviwaController>(objectMapper);
    }

    ENDPOINT("POST", "/create/painting", createTexturedRect,
             BODY_DTO(Object<CreateTexturedRectDto>, body))
    {
        REQUIRE_FIELD(body->imageId, "imageId");
        REQUIRE_FIELD(body->height, "height");
        REQUIRE_FIELD(body->width, "width");
        REQUIRE_FIELD(body->thickness, "thickness");

        OATPP_ASSERT_HTTP(body->width > 0 && body->height > 0,
                          Status::CODE_400, Messages::INVALID_PARAMS);

        return SkpResponseBuilder::buildDownloadResponse(this,
                                                         withTempFileGuard(body->imageId, [&](auto &entry)
                                                                           { return m_aviwaService->createPaintingFile(
                                                                                 entry.filepath.string(),
                                                                                 body->width, body->height, body->thickness); }),
                                                         "file.skp");
    }

    ENDPOINT("POST", "/create/structure/gabster", createGabsterStructure,
             BODY_DTO(Object<CreateGabsterStructureDto>, body))
    {
        REQUIRE_FIELD(body->fileId, "fileId");
        REQUIRE_FIELD(body->author, "author");
        REQUIRE_FIELD(body->title, "title");
        REQUIRE_FIELD(body->code, "code");
        REQUIRE_FIELD(body->gbsId, "gbsId");
        REQUIRE_FIELD(body->description, "description");

        return SkpResponseBuilder::buildDownloadResponse(this,
                                                         withTempFileGuard(body->fileId, [&](auto &entry)
                                                                           { return m_aviwaService->createGabsterStructure(
                                                                                 entry.filepath.string(),
                                                                                 body->author->c_str(), body->title->c_str(),
                                                                                 body->code->c_str(), body->gbsId->c_str(),
                                                                                 body->description->c_str()); }),
                                                         "file.skp");
    }

    ENDPOINT("POST", "/create/informative-image", createInformativeImage,
             BODY_DTO(Object<CreateInformativeImageDto>, body))
    {
        REQUIRE_FIELD(body->fileId, "fileId");
        REQUIRE_FIELD(body->imageId, "imageId");
        REQUIRE_FIELD(body->name, "name");

        return SkpResponseBuilder::buildDownloadResponse(
            this,
            withTempFileGuard(body->fileId, [&](auto &entry)
                              { return withTempFileGuard(body->imageId, [&](auto &entry_image)
                                                         { return m_aviwaService->AddImageAsLeftComponent(
                                                               entry.filepath.string(),
                                                               entry_image.filepath.string(),
                                                               body->name->c_str()); }); }),
            "file.skp");
    }
};

#include OATPP_CODEGEN_END(ApiController)

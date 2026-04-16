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

        auto buffer = withTempFileGuard(body->imageId, [&](auto &entry)
                                        {
        auto path = m_aviwaService->createPaintingFile(
            entry.filepath.string(),
            body->width, body->height, body->thickness);
        return SkpResponseBuilder::readFileToBuffer(path); });

        return SkpResponseBuilder::buildResponseFromBuffer(this, buffer, "file.skp");
    }

    ENDPOINT("POST", "/create/informative-image", createInformativeImage,
             BODY_DTO(Object<CreateInformativeImageDto>, body))
    {
        REQUIRE_FIELD(body->fileId, "fileId");
        REQUIRE_FIELD(body->imageId, "imageId");
        REQUIRE_FIELD(body->name, "name");

        if (body->scale && (*body->scale <= 0.0 || *body->scale > 1.0))
        {
            OATPP_ASSERT_HTTP(false, Status::CODE_400,
                              "scale deve ser > 0 e <= 1");
        }

        double scale = body->scale ? *body->scale : 1.0;

        auto buffer = withTempFileGuard(body->fileId, [&](auto &entry)
                                        {
        auto path = withTempFileGuard(body->imageId, [&](auto &entry_image)
        {
            return m_aviwaService->AddImageAsLeftComponent(
                entry.filepath.string(),
                entry_image.filepath.string(),
                body->name->c_str(),
                scale
            );
        });
        return SkpResponseBuilder::readFileToBuffer(path); });

        return SkpResponseBuilder::buildResponseFromBuffer(this, buffer, "file.skp");
    }
};

#include OATPP_CODEGEN_END(ApiController)

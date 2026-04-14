#pragma once

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/parser/json/mapping/ObjectMapper.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"
#include "oatpp/encoding/Hex.hpp"

#include "service/SketchUpService.hpp"
#include "service/FileService.hpp"

#include "dto/CommonDtos.hpp"
#include "dto/FileDtos.hpp"
#include "mapper/SketchupComponentMapper.hpp"
#include "utils/UrlUtils.hpp"
#include "mapper/FileMapper.hpp"
#include "common/validation.hpp"
#include "dto/SketchupDtos.hpp"
#include "common/validation.hpp"
#include "utils/SkpResponseBuilder.hpp"

#include OATPP_CODEGEN_BEGIN(ApiController)

class SketchUpController : public oatpp::web::server::api::ApiController
{
private:
    OATPP_COMPONENT(std::shared_ptr<SketchUpService>, m_sketchUpService);

    void validateUpdateDto(const oatpp::Object<UpdateAttributeDto> &updateDto)
    {
        REQUIRE_FIELD(updateDto->fileId, "fileId");
        REQUIRE_FIELD(updateDto->guid, "guid");
        REQUIRE_FIELD(updateDto->attributes, "attributes");
    }

    std::vector<SketchUpComponentAttribute> parseAttributes(
        const oatpp::List<oatpp::Object<AttributeItemDto>> &attributes)
    {
        std::vector<SketchUpComponentAttribute> attrs;
        for (const auto &item : *attributes)
        {
            REQUIRE_FIELD(item->dictName, "attributes[].dictName");
            REQUIRE_FIELD(item->key, "attributes[].key");
            REQUIRE_FIELD(item->value, "attributes[].value");

            attrs.push_back({item->dictName->c_str(),
                             item->key->c_str(),
                             item->value->c_str()});
        }
        return attrs;
    }

public:
    SketchUpController(const std::shared_ptr<oatpp::data::mapping::ObjectMapper> &objectMapper)
        : oatpp::web::server::api::ApiController(objectMapper)
    {
    }

    static std::shared_ptr<SketchUpController> createShared()
    {
        OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper);

        return std::make_shared<SketchUpController>(objectMapper);
    }

    ENDPOINT("GET", "/file/version", version, QUERY(String, fileId))
    {
        auto version = withTempFileGuard(fileId, [&](auto &entry)
                                         { return m_sketchUpService->getSkpVersion(entry.filepath.string()); });
        auto dto = FileVersionDto::createShared();
        dto->version = version.c_str();

        return createDtoResponse(Status::CODE_200, dto);
    }

    ENDPOINT("GET", "/file/definitions", definitions, QUERY(String, fileId))
    {
        auto definitions = withTempFileGuard(fileId, [&](auto &entry)
                                             { return m_sketchUpService->getDefinitionsAttributes(entry.filepath.string()); });

        return createDtoResponse(Status::CODE_200, SketchupComponentMapper::toDtoList(definitions));
    }

    ENDPOINT("GET", "/file/instances", instances, QUERY(String, fileId))
    {

        auto insts = withTempFileGuard(fileId, [&](auto &entry)
                                       { return m_sketchUpService->getInstancesAttributes(entry.filepath.string()); });

        return createDtoResponse(Status::CODE_200, SketchupComponentMapper::toDtoList(insts));
    }

    ENDPOINT("GET", "/file/structure", getStructure, QUERY(String, fileId))
    {

        auto treeData = withTempFileGuard(fileId, [&](auto &entry)
                                          { return m_sketchUpService->getModelTree(entry.filepath.string()); });
        auto treeResponse = ItemMapper::toDtoList(treeData);

        return createDtoResponse(Status::CODE_200, treeResponse);
    }

    ENDPOINT("POST", "/file/update/instance", updateInst,
             BODY_DTO(Object<UpdateAttributeDto>, body))
    {

        validateUpdateDto(body);
        auto attrs = parseAttributes(body->attributes);

        auto buffer = withTempFileGuard(body->fileId, [&](auto &entry)
                                        {
        auto path = m_sketchUpService->updateInstanceAttribute(
            entry.filepath.string(),
            body->guid->c_str(),
            attrs);

        return SkpResponseBuilder::readFileToBuffer(path); });

        return SkpResponseBuilder::buildResponseFromBuffer(this, buffer, "file.skp");
    }

    ENDPOINT("POST", "/file/update/definition", updateDef,
             BODY_DTO(Object<UpdateAttributeDto>, body))
    {
        validateUpdateDto(body);

        auto attrs = parseAttributes(body->attributes);

        auto buffer = withTempFileGuard(body->fileId, [&](auto &entry)
                                        {
        auto path = m_sketchUpService->updateDefinitionAttribute(
            entry.filepath.string(),
            body->guid->c_str(),
            attrs);

        return SkpResponseBuilder::readFileToBuffer(path); });

        return SkpResponseBuilder::buildResponseFromBuffer(this, buffer, "file.skp");
    }

    ENDPOINT("POST", "/file/create/wrap", createWrap, BODY_DTO(Object<CreateWrapDto>, body))
    {
        REQUIRE_FIELD(body->fileId, "fileId");
        REQUIRE_FIELD(body->name, "name");
        REQUIRE_FIELD(body->attributes, "attributes");

        auto attrs = parseAttributes(body->attributes);

        auto buffer = withTempFileGuard(body->fileId, [&](auto &entry)
                                        {
        auto path = m_sketchUpService->createWrapComponent(
            entry.filepath.string(),
            body->name->c_str(),
        attrs
        );
        return SkpResponseBuilder::readFileToBuffer(path); });

        return SkpResponseBuilder::buildResponseFromBuffer(this, buffer, "file.skp");
    }
};

#include OATPP_CODEGEN_END(ApiController)

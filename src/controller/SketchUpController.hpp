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

#include OATPP_CODEGEN_BEGIN(ApiController)

class SketchUpController : public oatpp::web::server::api::ApiController
{
private:
    std::shared_ptr<SketchUpService> m_sketchUpService;
    std::shared_ptr<FileService> m_fileService;

public:
    SketchUpController(const std::shared_ptr<oatpp::data::mapping::ObjectMapper> &objectMapper,
                       const std::shared_ptr<SketchUpService> &sketchUpService,
                       const std::shared_ptr<FileService> &fileService

                       )
        : oatpp::web::server::api::ApiController(objectMapper), m_sketchUpService(sketchUpService), m_fileService(fileService)
    {
    }

    static std::shared_ptr<SketchUpController> createShared()
    {
        OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper);
        OATPP_COMPONENT(std::shared_ptr<SketchUpService>, sketchUpService);
        OATPP_COMPONENT(std::shared_ptr<FileService>, fileService);

        return std::make_shared<SketchUpController>(objectMapper, sketchUpService, fileService);
    }

    ENDPOINT("GET", "/", root)
    {
        return createResponse(Status::CODE_200, "Servidor Oat++ Online!");
    }

    ENDPOINT("GET", "/version", version, QUERY(String, filepath))
    {
        auto filePathDecoded = UrlUtils::decode(filepath->c_str());
        auto v = m_sketchUpService->getSkpVersion(filePathDecoded);
        return createResponse(Status::CODE_200, "Versao do SketchUp: " + v);
    }

    ENDPOINT("GET", "/definitions", definitions, QUERY(String, filepath))
    {
        auto filePathDecoded = UrlUtils::decode(filepath->c_str());
        auto definitions = m_sketchUpService->getDefinitionsAttributes(filePathDecoded);
        return createDtoResponse(Status::CODE_200, SketchupComponentMapper::toDtoList(definitions));
    }

    ENDPOINT("GET", "/instances", instances, QUERY(String, filepath))
    {
        auto filePathDecoded = UrlUtils::decode(filepath->c_str());
        auto instances = m_sketchUpService->getInstancesAttributes(filePathDecoded);
        return createDtoResponse(Status::CODE_200, SketchupComponentMapper::toDtoList(instances));
    }

    ENDPOINT("GET", "/tree", getTree, QUERY(String, filepath))
    {
        auto filePathDecoded = UrlUtils::decode(filepath->c_str());
        auto treeData = m_sketchUpService->getModelTree(filePathDecoded);
        auto jsonResponse = ItemMapper::toDtoList(treeData);
        return createDtoResponse(Status::CODE_200, jsonResponse);
    }

    ENDPOINT("GET", "/files", listFiles)
    {
        auto files = m_fileService->listFilesByExtension(".skp");
        return createDtoResponse(Status::CODE_200, FileMapper::toDtoList(files));
    }

    // Endpoint para INSTÂNCIA
    ENDPOINT("POST", "/update/instance", updateInst, BODY_DTO(Object<UpdateAttributeDto>, updateDto))

    {
        // Agora as variáveis 'filepath', 'guid', 'key' e 'val' existem no escopo
        auto filePathDecoded = UrlUtils::decode(updateDto->filepath->c_str());

        bool ok = m_sketchUpService->updateInstanceAttribute(
            filePathDecoded,
            updateDto->guid->c_str(),
            updateDto->dictName->c_str(),
            updateDto->key->c_str(),
            updateDto->value->c_str());
        // Cria o DTO de resposta
        auto responseDto = ResultDto::createShared();
        responseDto->success = ok;

        if (ok)
        {
            responseDto->detail = "Atributo atualizado com sucesso na instancia.";
            return createDtoResponse(Status::CODE_200, responseDto);
        }
        else
        {
            responseDto->detail = "Erro: Nao foi possivel encontrar a instancia ou o arquivo esta bloqueado.";
            return createDtoResponse(Status::CODE_404, responseDto);
        }
    }

    // Endpoint para DEFINIÇÃO
    ENDPOINT("POST", "/update/definition", updateDef, BODY_DTO(Object<UpdateAttributeDto>, updateDto))

    {
        auto filePathDecoded = UrlUtils::decode(updateDto->filepath->c_str());

        bool ok = m_sketchUpService->updateDefinitionAttribute(
            filePathDecoded,
            updateDto->guid->c_str(),
            updateDto->dictName->c_str(),
            updateDto->key->c_str(),
            updateDto->value->c_str());

        return createResponse(ok ? Status::CODE_200 : Status::CODE_404, "Definição atualizada");
    }
};

#include OATPP_CODEGEN_END(ApiController)

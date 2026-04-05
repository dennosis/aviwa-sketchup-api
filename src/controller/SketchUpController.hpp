#pragma once

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/parser/json/mapping/ObjectMapper.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"
#include "oatpp/encoding/Hex.hpp"
#include "oatpp/web/mime/multipart/InMemoryDataProvider.hpp"
#include "oatpp/web/mime/multipart/TemporaryFileProvider.hpp"
#include "oatpp/web/mime/multipart/Reader.hpp"
#include "oatpp/web/mime/multipart/PartList.hpp"

#include "service/SketchUpService.hpp"
#include "service/FileService.hpp"

#include "dto/CommonDtos.hpp"
#include "dto/FileDtos.hpp"
#include "mapper/SketchupComponentMapper.hpp"
#include "utils/UrlUtils.hpp"
#include "mapper/FileMapper.hpp"

#include OATPP_CODEGEN_BEGIN(ApiController)
// TODO: Refatorar para usar multipart/form-data e receber o arquivo diretamente, sem precisar do caminho (que pode ser problemático dependendo do ambiente de execução)
// TODO: padronizar retorno dos endpoints, usando DTOs de resposta com campos de sucesso e detalhes (mensagens de erro ou informações adicionais)
namespace multipart = oatpp::web::mime::multipart;

class SketchUpController : public oatpp::web::server::api::ApiController
{
private:
    std::shared_ptr<SketchUpService> m_sketchUpService;
    std::shared_ptr<FileService> m_fileService;
    std::shared_ptr<std::string> m_tempPath;

public:
    SketchUpController(const std::shared_ptr<oatpp::data::mapping::ObjectMapper> &objectMapper,
                       const std::shared_ptr<SketchUpService> &sketchUpService,
                       const std::shared_ptr<FileService> &fileService,
                       const std::shared_ptr<std::string> &tempPath)
        : oatpp::web::server::api::ApiController(objectMapper), m_sketchUpService(sketchUpService), m_fileService(fileService), m_tempPath(tempPath)
    {
    }

    static std::shared_ptr<SketchUpController> createShared()
    {
        OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper);
        OATPP_COMPONENT(std::shared_ptr<SketchUpService>, sketchUpService);
        OATPP_COMPONENT(std::shared_ptr<FileService>, fileService);
        OATPP_COMPONENT(std::shared_ptr<std::string>, tempPath);

        return std::make_shared<SketchUpController>(objectMapper, sketchUpService, fileService, tempPath);
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

    ENDPOINT("POST", "/create/structure/gabster", createGabsterStructure, BODY_DTO(Object<CreateGabsterStructureDto>, updateDto))
    {
        auto filePathDecoded = UrlUtils::decode(updateDto->filepath->c_str());
        bool ok = m_sketchUpService->createGabsterStructure(filePathDecoded);
        return createResponse(ok ? Status::CODE_200 : Status::CODE_404, "Definição atualizada");
    }

    ENDPOINT("POST", "/v1/sketchup/create-textured-rect", createTexturedRect,
             REQUEST(std::shared_ptr<IncomingRequest>, request))
    {
        multipart::PartList multipart(request->getHeaders());
        multipart::Reader multipartReader(&multipart);

        multipartReader.setPartReader("filepath", multipart::createInMemoryPartReader(4 * 1024));
        multipartReader.setPartReader("width", multipart::createInMemoryPartReader(64));
        multipartReader.setPartReader("height", multipart::createInMemoryPartReader(64));
        multipartReader.setPartReader("thickness", multipart::createInMemoryPartReader(64));
        multipartReader.setPartReader("image",
                                      multipart::createTemporaryFilePartReader(m_tempPath->c_str()));

        request->transferBody(&multipartReader);

        auto partFilepath = multipart.getNamedPart("filepath");
        auto partWidth = multipart.getNamedPart("width");
        auto partHeight = multipart.getNamedPart("height");
        auto partThickness = multipart.getNamedPart("thickness");
        auto partImage = multipart.getNamedPart("image");

        OATPP_ASSERT_HTTP(partFilepath, Status::CODE_400, "campo 'filepath' ausente");
        OATPP_ASSERT_HTTP(partWidth, Status::CODE_400, "campo 'width' ausente");
        OATPP_ASSERT_HTTP(partHeight, Status::CODE_400, "campo 'height' ausente");
        OATPP_ASSERT_HTTP(partThickness, Status::CODE_400, "campo 'thickness' ausente");
        OATPP_ASSERT_HTTP(partImage, Status::CODE_400, "campo 'image' ausente");
        OATPP_ASSERT_HTTP(partImage->getFilename(), Status::CODE_400, "filename da imagem ausente");

        // ✅ getPayload() -> Resource* -> getInMemoryData() para parts in-memory
        std::string path = partFilepath->getPayload()->getInMemoryData()->c_str();
        double w = std::stod(partWidth->getPayload()->getInMemoryData()->c_str());
        double h = std::stod(partHeight->getPayload()->getInMemoryData()->c_str());
        double thickness = std::stod(partThickness->getPayload()->getInMemoryData()->c_str());
        std::string imgName = partImage->getFilename()->c_str();

        auto resultDto = CreateSkpResultDto::createShared();
        std::string message;

        if (path.empty() || w <= 0 || h <= 0)
        {
            resultDto->success = false;
            resultDto->detail = "Parametros invalidos.";
            return createDtoResponse(Status::CODE_400, resultDto);
        }

        // ✅ openInputStream() em Resource — funciona para InMemoryData e TemporaryFile
        // ✅ reinterpret_cast resolve v_char8 (unsigned char*) -> const char*
        bool ok = m_sketchUpService->createPaintingFile(
            UrlUtils::decode(path),
            imgName,
            [&](std::ostream *os)
            {
                auto stream = partImage->getPayload()->openInputStream();
                v_char8 buf[4096];
                oatpp::v_io_size n;
                while ((n = stream->readSimple(buf, sizeof(buf))) > 0)
                {
                    os->write(reinterpret_cast<const char *>(buf), n);
                }
            },
            w, h, thickness, message);

        resultDto->success = ok;
        resultDto->detail = message.c_str();
        resultDto->savedPath = path.c_str();

        return createDtoResponse(ok ? Status::CODE_200 : Status::CODE_500, resultDto);
    }
};

#include OATPP_CODEGEN_END(ApiController)

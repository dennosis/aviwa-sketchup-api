#pragma once
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/Types.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class AttributeEntryDto : public oatpp::DTO
{
    DTO_INIT(AttributeEntryDto, DTO)

    DTO_FIELD(String, key);
    DTO_FIELD(String, value);
};

class AttributeDictionaryDto : public oatpp::DTO
{
    DTO_INIT(AttributeDictionaryDto, DTO)

    DTO_FIELD(String, name);
    DTO_FIELD(List<Object<AttributeEntryDto>>, attributes);
};

class ComponentDataDto : public oatpp::DTO
{
    DTO_INIT(ComponentDataDto, DTO)

    DTO_FIELD(String, name);
    DTO_FIELD(String, guid);
    DTO_FIELD(String, description);
    DTO_FIELD(List<Object<AttributeDictionaryDto>>, dictionaries);
};

class DefinitionDto : public oatpp::DTO
{
    DTO_INIT(DefinitionDto, DTO)

    DTO_FIELD(String, name);
    DTO_FIELD(String, guid);
};

class ItemNodeDto : public oatpp::DTO
{
    DTO_INIT(ItemNodeDto, DTO)

    DTO_FIELD(String, type); // "instance" ou "group"
    DTO_FIELD(String, name); // Nome da instância ou do grupo
    DTO_FIELD(String, guid); // GUID da instância ou do grupo

    // Este campo só aparecerá no JSON se for preenchido (não nulo)
    DTO_FIELD(Object<DefinitionDto>, definition);

    DTO_FIELD(List<Object<ItemNodeDto>>, children);
};

class UpdateAttributeDto : public oatpp::DTO
{

    DTO_INIT(UpdateAttributeDto, DTO)

    DTO_FIELD(String, filepath, "file_path");
    DTO_FIELD(String, guid);
    DTO_FIELD(String, dictName, "dict_name");
    DTO_FIELD(String, key);
    DTO_FIELD(String, value);
};

class CreateGabsterStructureDto : public oatpp::DTO
{

    DTO_INIT(CreateGabsterStructureDto, DTO)

    DTO_FIELD(String, filepath, "file_path");
};

class ResultDto : public oatpp::DTO
{

    DTO_INIT(ResultDto, DTO)

    DTO_FIELD(Boolean, success);
    DTO_FIELD(String, detail);

    // Método utilitário para criar instâncias rapidamente
    static Object<ResultDto> createShared(bool success, const char *detail)
    {
        auto dto = ResultDto::createShared();
        dto->success = success;
        dto->detail = detail;
        return dto;
    }
};

class CreateSkpResultDto : public oatpp::DTO
{
    DTO_INIT(CreateSkpResultDto, DTO)

    DTO_FIELD(Boolean, success);
    DTO_FIELD(String, detail);
    DTO_FIELD(String, savedPath); // Caminho onde o arquivo foi salvo
};

#include OATPP_CODEGEN_END(DTO)
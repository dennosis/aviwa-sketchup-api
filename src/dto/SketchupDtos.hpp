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
    DTO_FIELD(String, name);
    DTO_FIELD(String, guid);

    DTO_FIELD(Object<DefinitionDto>, definition);

    DTO_FIELD(List<Object<ItemNodeDto>>, children);
};

class CreateInformativeImageDto : public oatpp::DTO
{

    DTO_INIT(CreateInformativeImageDto, DTO)
    DTO_FIELD(String, fileId, "fileId");
    DTO_FIELD(String, imageId, "imageId");
    DTO_FIELD(String, name, "name");
};

class CreateSkpResultDto : public oatpp::DTO
{
    DTO_INIT(CreateSkpResultDto, DTO)

    DTO_FIELD(Boolean, success);
    DTO_FIELD(String, detail);
    DTO_FIELD(String, savedPath);
};

class CreateTexturedRectDto : public oatpp::DTO
{
    DTO_INIT(CreateTexturedRectDto, DTO)
    DTO_FIELD(String, imageId, "imageId");
    DTO_FIELD(Float64, width);
    DTO_FIELD(Float64, height);
    DTO_FIELD(Float64, thickness);
};

class FileVersionDto : public oatpp::DTO
{
    DTO_INIT(FileVersionDto, DTO)
    DTO_FIELD(String, version);
};

class AttributeItemDto : public oatpp::DTO
{
    DTO_INIT(AttributeItemDto, DTO)
    DTO_FIELD(String, dictName);
    DTO_FIELD(String, key);
    DTO_FIELD(String, value);
};

class UpdateAttributeDto : public oatpp::DTO
{
    DTO_INIT(UpdateAttributeDto, DTO)
    DTO_FIELD(String, fileId);
    DTO_FIELD(String, guid);
    DTO_FIELD(List<Object<AttributeItemDto>>, attributes);
};

class CreateWrapDto : public oatpp::DTO
{
    DTO_INIT(CreateWrapDto, DTO)
    DTO_FIELD(String, fileId);
    DTO_FIELD(String, name);
    DTO_FIELD(List<Object<AttributeItemDto>>, attributes);
};

#include OATPP_CODEGEN_END(DTO)
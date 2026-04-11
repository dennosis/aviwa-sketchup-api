#pragma once

#include "dto/SketchupDtos.hpp"
#include "service/SketchupService.hpp"

class SketchupComponentMapper
{
public:
    static oatpp::Object<AttributeEntryDto> toDto(const AttributeEntry &a)
    {
        auto dto = AttributeEntryDto::createShared();
        dto->key = a.key.c_str();
        dto->value = a.value.c_str();
        return dto;
    }

    static oatpp::Object<AttributeDictionaryDto> toDto(const AttributeDictionaryData &d)
    {
        auto dto = AttributeDictionaryDto::createShared();
        dto->name = d.name.c_str();
        dto->attributes = oatpp::List<oatpp::Object<AttributeEntryDto>>::createShared();
        for (const auto &a : d.attributes)
        {
            dto->attributes->push_back(toDto(a));
        }
        return dto;
    }

    static oatpp::Object<ComponentDataDto> toDto(const ComponentData &c)
    {
        auto dto = ComponentDataDto::createShared();
        dto->name = c.name.c_str();
        dto->guid = c.guid.c_str();
        dto->description = c.description.c_str();
        dto->dictionaries = oatpp::List<oatpp::Object<AttributeDictionaryDto>>::createShared();
        for (const auto &d : c.dictionaries)
        {
            dto->dictionaries->push_back(toDto(d));
        }
        return dto;
    }

    static oatpp::List<oatpp::Object<ComponentDataDto>> toDtoList(const std::vector<ComponentData> &components)
    {
        auto list = oatpp::List<oatpp::Object<ComponentDataDto>>::createShared();
        for (const auto &c : components)
        {
            list->push_back(toDto(c));
        }
        return list;
    }
};
class ItemMapper
{
public:
    static oatpp::Object<ItemNodeDto> toDto(const ItemNode &node)
    {
        auto dto = ItemNodeDto::createShared();
        dto->children = oatpp::List<oatpp::Object<ItemNodeDto>>::createShared();

        if (std::holds_alternative<InstanceTypeNode>(node.item))
        {
            auto const &data = std::get<InstanceTypeNode>(node.item);
            dto->type = "instance";
            dto->name = data.name.c_str();
            dto->guid = data.guid.c_str();

            auto defDto = DefinitionDto::createShared();
            defDto->name = data.definition.name.c_str();
            defDto->guid = data.definition.guid.c_str();
            dto->definition = defDto;
        }
        else if (std::holds_alternative<GroupTypeNode>(node.item))
        {
            auto const &data = std::get<GroupTypeNode>(node.item);
            dto->type = "group";
            dto->name = data.name.c_str();
            dto->guid = data.guid.c_str();

            // dto->definition permanece NULL, então o Oat++ não o incluirá no JSON
        }

        for (const auto &child : node.children)
        {
            dto->children->push_back(toDto(child));
        }

        return dto;
    }

    static oatpp::List<oatpp::Object<ItemNodeDto>> toDtoList(const std::vector<ItemNode> &nodes)
    {
        auto list = oatpp::List<oatpp::Object<ItemNodeDto>>::createShared();
        for (const auto &node : nodes)
        {
            list->push_back(toDto(node));
        }
        return list;
    }
};
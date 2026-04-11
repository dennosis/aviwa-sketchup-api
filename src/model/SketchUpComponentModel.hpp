#pragma once

#include <string>
#include <vector>
#include <memory>
#include <variant>

struct AttributeEntry
{
    std::string key;
    std::string value;
};

struct AttributeDictionaryData
{
    std::string name;
    std::vector<AttributeEntry> attributes;
};

struct ComponentData
{
    std::string name;
    std::string guid;
    std::string description;
    std::vector<AttributeDictionaryData> dictionaries;
};

struct ComponentDefinitionData
{
    std::string name;
    std::string guid;
};

struct InstanceTypeNode
{
    std::string name;
    std::string guid;
    ComponentDefinitionData definition;
};

struct GroupTypeNode
{
    std::string name;
    std::string guid;
};

struct ItemNode
{
    std::variant<InstanceTypeNode, GroupTypeNode> item;
    std::vector<ItemNode> children;
};

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
// Dados específicos de uma Definição (o "molde")
struct ComponentDefinitionData
{
    std::string name;
    std::string guid;
};

// Tipo para Instâncias de Componentes
struct InstanceTypeNode
{
    std::string name;
    std::string guid;
    ComponentDefinitionData definition;
};

// Tipo para Grupos
struct GroupTypeNode
{
    std::string name;
    std::string guid;
};

// O Item da árvore que pode ser um dos dois tipos
struct ItemNode
{
    // std::variant age como uma "Union" segura
    std::variant<InstanceTypeNode, GroupTypeNode> item;
    std::vector<ItemNode> children;
};

#pragma once

#include <SketchUpAPI/common.h>
#include <SketchUpAPI/initialize.h>
#include <SketchUpAPI/model/model.h>
#include <SketchUpAPI/model/component_definition.h>
#include <SketchUpAPI/model/entity.h>
#include <SketchUpAPI/model/entities.h>
#include <SketchUpAPI/model/component_instance.h>
#include "model/SketchUpComponentModel.hpp"
#include "utils/SketchUpUtils.hpp"
#include "utils/AviwaUtils.hpp"
#include "oatpp/core/base/Environment.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include "model/TempFileModel.hpp"

class SketchUpService
{

private:
    OATPP_COMPONENT(std::shared_ptr<TempPath>, m_tempPath);

    void fillTreeRecursive(SUEntitiesRef entities, std::vector<ItemNode> &list)
    {
        size_t instanceCount = 0;
        SUEntitiesGetNumInstances(entities, &instanceCount);
        if (instanceCount > 0)
        {
            std::vector<SUComponentInstanceRef> instances(instanceCount);
            SUEntitiesGetInstances(entities, instanceCount, instances.data(), &instanceCount);

            for (auto &inst : instances)
            {
                ItemNode node;
                InstanceTypeNode instanceData;

                instanceData.name = SketchUpUtils::getInstanceName(inst);
                instanceData.guid = SketchUpUtils::getInstanceGuid(inst);

                SUComponentDefinitionRef def = SU_INVALID;
                SUComponentInstanceGetDefinition(inst, &def);
                instanceData.definition.name = SketchUpUtils::getDefinitionName(def);
                instanceData.definition.guid = SketchUpUtils::getDefinitionGuid(def);

                node.item = instanceData;

                SUEntitiesRef subEntities = SU_INVALID;
                SUComponentDefinitionGetEntities(def, &subEntities);
                fillTreeRecursive(subEntities, node.children);

                list.push_back(node);
            }
        }

        size_t groupCount = 0;
        SUEntitiesGetNumGroups(entities, &groupCount);
        if (groupCount > 0)
        {
            std::vector<SUGroupRef> groups(groupCount);
            SUEntitiesGetGroups(entities, groupCount, groups.data(), &groupCount);

            for (auto &grp : groups)
            {
                ItemNode node;
                GroupTypeNode groupData;

                groupData.name = SketchUpUtils::getGroupName(grp);
                groupData.guid = SketchUpUtils::getGroupGuid(grp);

                node.item = groupData;

                SUEntitiesRef subEntities = SU_INVALID;
                SUGroupGetEntities(grp, &subEntities);
                fillTreeRecursive(subEntities, node.children);

                list.push_back(node);
            }
        }
    }

    template <typename TEntity>
    std::string applyAttributeUpdate(
        const std::string &filepath,
        const std::string &dictName,
        const std::string &key,
        const std::string &value,
        std::function<TEntity(SUModelRef)> resolver,
        std::function<SUResult(TEntity, const std::string &,
                               const std::string &, SUTypedValueRef)>
            setter)
    {
        auto skpPath = std::filesystem::path(m_tempPath->value) /
                       ("file_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".skp");

        SUInitialize();

        SUModelRef model = SU_INVALID;
        if (SketchUpUtils::loadModel(filepath, model) != SU_ERROR_NONE)
        {
            SUTerminate();
            throw std::runtime_error("Erro ao carregar modelo");
        }

        TEntity target = resolver(model);
        if (!SUIsValid(target))
        {
            SUModelRelease(&model);
            SUTerminate();
            throw std::runtime_error("Entidade não encontrada para o GUID informado");
        }

        SUTypedValueRef typedVal = SU_INVALID;
        SUTypedValueCreate(&typedVal);
        SUTypedValueSetString(typedVal, value.c_str());

        if (setter(target, dictName, key, typedVal) == SU_ERROR_NONE)
            SUModelSaveToFile(model, skpPath.string().c_str());

        SUTypedValueRelease(&typedVal);
        SUModelRelease(&model);
        SUTerminate();
        return skpPath.string();
    }

public:
    std::string getSkpVersion(const std::string &filepath)
    {
        SUInitialize();
        SUModelRef model = SU_INVALID;

        if (SketchUpUtils::loadModel(filepath, model) != SU_ERROR_NONE)
        {
            SUTerminate();
            return "Erro: Nao foi possivel abrir o arquivo";
        }

        int major = 0, minor = 0, build = 0;
        SUModelGetVersion(model, &major, &minor, &build);
        SUModelRelease(&model);
        SUTerminate();

        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(build);
    }

    std::vector<ComponentData> getDefinitionsAttributes(const std::string &filepath)
    {
        std::vector<ComponentData> result;
        SUInitialize();
        SUModelRef model = SU_INVALID;
        if (SketchUpUtils::loadModel(filepath, model) != SU_ERROR_NONE)

        {
            // NOTE: colcoar um erro
            SUTerminate();
            return result;
        }

        size_t defCount = 0;
        SUModelGetNumComponentDefinitions(model, &defCount);

        if (defCount > 0)
        {
            std::vector<SUComponentDefinitionRef> defs(defCount);
            SUModelGetComponentDefinitions(model, defCount, defs.data(), &defCount);

            for (size_t i = 0; i < defCount; ++i)
            {
                size_t instanceCount = 0;
                SUComponentDefinitionGetNumInstances(defs[i], &instanceCount);

                // NOTE: Se não houver nenhuma cópia deste componente no modelo, pula para o próximo
                if (instanceCount == 0)
                {
                    continue;
                }

                ComponentData comp;

                // Nome
                SUStringRef nameRef = SU_INVALID;
                SUStringCreate(&nameRef);
                SUComponentDefinitionGetName(defs[i], &nameRef);
                comp.name = SketchUpUtils::suStringToStd(nameRef);
                SUStringRelease(&nameRef);

                // GUID
                SUStringRef guidRef = SU_INVALID;
                SUStringCreate(&guidRef);
                SUComponentDefinitionGetGuid(defs[i], &guidRef);
                comp.guid = SketchUpUtils::suStringToStd(guidRef);
                SUStringRelease(&guidRef);

                // Descrição
                SUStringRef descRef = SU_INVALID;
                SUStringCreate(&descRef);
                SUComponentDefinitionGetDescription(defs[i], &descRef);
                comp.description = SketchUpUtils::suStringToStd(descRef);
                SUStringRelease(&descRef);

                // Atributos (Dicionários)
                size_t dictCount = 0;
                SUEntityGetNumAttributeDictionaries(SUComponentDefinitionToEntity(defs[i]), &dictCount);

                if (dictCount > 0)
                {
                    std::vector<SUAttributeDictionaryRef> dicts(dictCount);
                    SUEntityGetAttributeDictionaries(SUComponentDefinitionToEntity(defs[i]),
                                                     dictCount, dicts.data(), &dictCount);

                    for (size_t d = 0; d < dictCount; ++d)
                    {
                        comp.dictionaries.push_back(SketchUpUtils::readDictionary(dicts[d]));
                    }
                }

                result.push_back(comp);
            }
        }

        SUModelRelease(&model);
        SUTerminate();
        return result;
    }

    std::vector<ComponentData> getInstancesAttributes(const std::string &filepath)
    {
        std::vector<ComponentData> result;
        SUInitialize();
        SUModelRef model = SU_INVALID;

        if (SketchUpUtils::loadModel(filepath, model) != SU_ERROR_NONE)
        {
            SUTerminate();
            return result;
        }

        SUEntitiesRef entities = SU_INVALID;
        SUModelGetEntities(model, &entities);

        size_t instanceCount = 0;
        SUEntitiesGetNumInstances(entities, &instanceCount);

        if (instanceCount > 0)
        {
            std::vector<SUComponentInstanceRef> instances(instanceCount);
            SUEntitiesGetInstances(entities, instanceCount, instances.data(), &instanceCount);

            for (size_t i = 0; i < instanceCount; ++i)
            {
                ComponentData comp;
                SUComponentInstanceRef instance = instances[i];

                SUStringRef nameRef = SU_INVALID;
                SUStringCreate(&nameRef);
                SUComponentInstanceGetName(instance, &nameRef);
                comp.name = SketchUpUtils::suStringToStd(nameRef);
                SUStringRelease(&nameRef);

                SUStringRef guidRef = SU_INVALID;
                SUStringCreate(&guidRef);
                SUComponentInstanceGetGuid(instance, &guidRef);
                comp.guid = SketchUpUtils::suStringToStd(guidRef);
                SUStringRelease(&guidRef);

                size_t dictCount = 0;
                SUEntityGetNumAttributeDictionaries(SUComponentInstanceToEntity(instance), &dictCount);

                if (dictCount > 0)
                {
                    std::vector<SUAttributeDictionaryRef> dicts(dictCount);
                    SUEntityGetAttributeDictionaries(SUComponentInstanceToEntity(instance),
                                                     dictCount, dicts.data(), &dictCount);

                    for (size_t d = 0; d < dictCount; ++d)
                    {
                        comp.dictionaries.push_back(SketchUpUtils::readDictionary(dicts[d]));
                    }
                }

                result.push_back(comp);
            }
        }

        SUModelRelease(&model);
        SUTerminate();
        return result;
    }

    std::vector<ItemNode> getModelTree(const std::string &filepath)
    {
        std::vector<ItemNode> rootList;

        SUInitialize();
        SUModelRef model = SU_INVALID;

        if (SketchUpUtils::loadModel(filepath, model) == SU_ERROR_NONE)
        {

            SUEntitiesRef modelEntities = SU_INVALID;
            SUModelGetEntities(model, &modelEntities);

            fillTreeRecursive(modelEntities, rootList);

            SUModelRelease(&model);
        }

        SUTerminate();
        return rootList;
    }

    std::string updateInstanceAttribute(const std::string &filepath,
                                        const std::string &guid,
                                        const std::string &dictName,
                                        const std::string &key,
                                        const std::string &value)
    {
        return applyAttributeUpdate<SUComponentInstanceRef>(
            filepath, dictName, key, value,
            [&](SUModelRef model)
            {
                SUEntitiesRef root = SU_INVALID;
                SUModelGetEntities(model, &root);
                SUEntityRef e = SketchUpUtils::findEntityByGuid(root, guid);
                return SUComponentInstanceFromEntity(e);
            },
            [](SUComponentInstanceRef inst, const std::string &d,
               const std::string &k, SUTypedValueRef v)
            {
                return SketchUpUtils::setAttributeWithValidation(inst, d, k, v);
            });
    }

    std::string updateDefinitionAttribute(const std::string &filepath,
                                          const std::string &guid,
                                          const std::string &dictName,
                                          const std::string &key,
                                          const std::string &value)
    {
        return applyAttributeUpdate<SUComponentDefinitionRef>(
            filepath, dictName, key, value,
            [&](SUModelRef model)
            {
                return SketchUpUtils::findDefinitionByGuid(model, guid);
            },
            [](SUComponentDefinitionRef def, const std::string &d,
               const std::string &k, SUTypedValueRef v)
            {
                return SketchUpUtils::setDefinitionAttribute(def, d, k, v);
            });
    }
};

#pragma once

#include <SketchUpAPI/common.h>
#include <SketchUpAPI/initialize.h>
#include <SketchUpAPI/model/model.h>
#include <SketchUpAPI/model/component_definition.h>
#include <SketchUpAPI/model/entity.h>
#include <SketchUpAPI/model/entities.h>
#include <SketchUpAPI/model/component_instance.h>
#include "model/SketchUpComponentModel.hpp"
#include "model/SketchUpComponentAttribute.hpp"
#include "utils/SketchUpUtils.hpp"
#include "utils/AviwaUtils.hpp"
#include "oatpp/core/base/Environment.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

class SketchUpService
{

private:
    template <typename TEntity>
    void applyAttributeUpdates(
        SUModelRef model,
        const std::vector<SketchUpComponentAttribute> &attributes,
        std::function<TEntity(SUModelRef)> resolver,
        std::function<SUResult(TEntity, const std::string &,
                               const std::string &, SUTypedValueRef)>
            setter)
    {
        TEntity target = resolver(model);
        if (!SUIsValid(target))
            throw std::runtime_error("Entidade não encontrada para o GUID informado");

        std::vector<std::string> errors;
        for (const auto &upd : attributes)
        {
            SUTypedValueRef typedVal = SU_INVALID;
            SUTypedValueCreate(&typedVal);
            SUTypedValueSetString(typedVal, upd.value.c_str());

            SUResult r = setter(target, upd.dictName, upd.key, typedVal);
            SUTypedValueRelease(&typedVal);

            if (r != SU_ERROR_NONE)
                errors.push_back("dict=" + upd.dictName + " key=" + upd.key +
                                 " err=" + std::to_string(r));
        }

        if (!errors.empty())
        {
            std::string joined;
            for (size_t i = 0; i < errors.size(); ++i)
            {
                if (i > 0)
                    joined += "; ";
                joined += errors[i];
            }
            throw std::runtime_error("Partial failure: " + joined);
        }
    }

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

public:
    SketchUpService() { SUInitialize(); }
    ~SketchUpService() { SUTerminate(); }

    std::string getSkpVersion(const std::string &filepath)
    {

        SUModelRef model = SU_INVALID;
        if (SketchUpUtils::loadModel(filepath, model) != SU_ERROR_NONE)
            return "Erro: Nao foi possivel abrir o arquivo";

        int major = 0, minor = 0, build = 0;
        SUModelGetVersion(model, &major, &minor, &build);
        SUModelRelease(&model);

        return std::to_string(major) + "." +
               std::to_string(minor) + "." +
               std::to_string(build);
    }

    std::vector<ComponentData> getDefinitionsAttributes(const std::string &filepath)
    {
        std::vector<ComponentData> result;

        SUModelRef model = SU_INVALID;
        if (SketchUpUtils::loadModel(filepath, model) != SU_ERROR_NONE)
            return result;

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

                // Se não houver nenhuma cópia deste componente no modelo, pula
                if (instanceCount == 0)
                    continue;

                ComponentData comp;

                SUStringRef nameRef = SU_INVALID;
                SUStringCreate(&nameRef);
                SUComponentDefinitionGetName(defs[i], &nameRef);
                comp.name = SketchUpUtils::suStringToStd(nameRef);
                SUStringRelease(&nameRef);

                SUStringRef guidRef = SU_INVALID;
                SUStringCreate(&guidRef);
                SUComponentDefinitionGetGuid(defs[i], &guidRef);
                comp.guid = SketchUpUtils::suStringToStd(guidRef);
                SUStringRelease(&guidRef);

                SUStringRef descRef = SU_INVALID;
                SUStringCreate(&descRef);
                SUComponentDefinitionGetDescription(defs[i], &descRef);
                comp.description = SketchUpUtils::suStringToStd(descRef);
                SUStringRelease(&descRef);

                size_t dictCount = 0;
                SUEntityGetNumAttributeDictionaries(
                    SUComponentDefinitionToEntity(defs[i]), &dictCount);

                if (dictCount > 0)
                {
                    std::vector<SUAttributeDictionaryRef> dicts(dictCount);
                    SUEntityGetAttributeDictionaries(
                        SUComponentDefinitionToEntity(defs[i]),
                        dictCount, dicts.data(), &dictCount);

                    for (size_t d = 0; d < dictCount; ++d)
                        comp.dictionaries.push_back(SketchUpUtils::readDictionary(dicts[d]));
                }

                result.push_back(comp);
            }
        }

        SUModelRelease(&model);
        return result;
    }

    std::vector<ComponentData> getInstancesAttributes(const std::string &filepath)
    {
        std::vector<ComponentData> result;

        SUModelRef model = SU_INVALID;
        if (SketchUpUtils::loadModel(filepath, model) != SU_ERROR_NONE)
            return result;

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
                SUEntityGetNumAttributeDictionaries(
                    SUComponentInstanceToEntity(instance), &dictCount);

                if (dictCount > 0)
                {
                    std::vector<SUAttributeDictionaryRef> dicts(dictCount);
                    SUEntityGetAttributeDictionaries(
                        SUComponentInstanceToEntity(instance),
                        dictCount, dicts.data(), &dictCount);

                    for (size_t d = 0; d < dictCount; ++d)
                        comp.dictionaries.push_back(SketchUpUtils::readDictionary(dicts[d]));
                }

                result.push_back(comp);
            }
        }

        SUModelRelease(&model);
        return result;
    }

    std::vector<ItemNode> getModelTree(const std::string &filepath)
    {
        std::vector<ItemNode> rootList;

        SUModelRef model = SU_INVALID;
        if (SketchUpUtils::loadModel(filepath, model) == SU_ERROR_NONE)
        {
            SUEntitiesRef modelEntities = SU_INVALID;
            SUModelGetEntities(model, &modelEntities);
            fillTreeRecursive(modelEntities, rootList);
            SUModelRelease(&model);
        }

        return rootList;
    }

    std::string updateInstanceAttribute(const std::string &filepath,
                                        const std::string &guid,
                                        const std::vector<SketchUpComponentAttribute> &attributes)
    {
        return editAndSaveModel(filepath, [&](SUModelRef model)
                                { applyAttributeUpdates<SUComponentInstanceRef>(
                                      model, attributes,
                                      [&](SUModelRef m)
                                      {
                                          SUEntitiesRef root = SU_INVALID;
                                          SUModelGetEntities(m, &root);
                                          SUEntityRef e = SketchUpUtils::findEntityByGuid(root, guid);
                                          return SUComponentInstanceFromEntity(e);
                                      },
                                      [](SUComponentInstanceRef inst, const std::string &d,
                                         const std::string &k, SUTypedValueRef v)
                                      {
                                          return SketchUpUtils::setAttributeWithValidation(inst, d, k, v);
                                      }); });
    }

    std::string updateDefinitionAttribute(const std::string &filepath,
                                          const std::string &guid,
                                          const std::vector<SketchUpComponentAttribute> &attributes)
    {
        return editAndSaveModel(filepath, [&](SUModelRef model)
                                { applyAttributeUpdates<SUComponentDefinitionRef>(
                                      model, attributes,
                                      [&](SUModelRef m)
                                      {
                                          return SketchUpUtils::findDefinitionByGuid(m, guid);
                                      },
                                      [](SUComponentDefinitionRef def, const std::string &d,
                                         const std::string &k, SUTypedValueRef v)
                                      {
                                          return SketchUpUtils::setDefinitionAttribute(def, d, k, v);
                                      }); });
    }

    std::string createWrapComponent(
        const std::string &filepath,
        const std::string &name,
        const std::vector<SketchUpComponentAttribute> &attributes = {})
    {
        return editAndSaveModel(filepath, [&](SUModelRef model)
                                {
            SUResult res = SketchUpUtils::wrapRootInstances(model, name, attributes);
            if (res != SU_ERROR_NONE)
                throw std::runtime_error(
                    "Erro ao criar wrap component: " + std::to_string(res)); });
    }

    template <typename Fn>
    static std::string editAndSaveModel(const std::string &filepath, Fn &&fn)
    {

        SUModelRef model = SU_INVALID;
        if (SketchUpUtils::loadModel(filepath, model) != SU_ERROR_NONE)
            OATPP_ASSERT_HTTP(false, Status::CODE_500, "Erro ao carregar modelo");

        try
        {
            fn(model);

            SUResult res = SUModelSaveToFile(model, filepath.c_str());
            SUModelRelease(&model);

            OATPP_ASSERT_HTTP(res == SU_ERROR_NONE, Status::CODE_500,
                              "Erro ao salvar modelo");

            return filepath;
        }
        catch (const std::exception &e)
        {
            SUModelRelease(&model);
            OATPP_ASSERT_HTTP(false, Status::CODE_500, e.what());
        }
    }

    template <typename Fn>
    static std::string saveModel(const std::string &filepath, Fn &&fn)
    {
        SUModelRef model = SU_INVALID;

        try
        {
            model = fn();

            SUResult res = SUModelSaveToFile(model, filepath.c_str());

            if (!SUIsInvalid(model))
            {
                SUModelRelease(&model);
                model = SU_INVALID; // Importante para o catch não tentar liberar de novo
            }

            OATPP_ASSERT_HTTP(res == SU_ERROR_NONE, Status::CODE_500, "Erro ao salvar modelo SketchUp");

            return filepath;
        }
        catch (const std::exception &e)
        {
            if (!SUIsInvalid(model))
            {
                SUModelRelease(&model);
            }

            OATPP_ASSERT_HTTP(false, Status::CODE_500, e.what());
            throw;
        }
    }
};
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
#include "utils/GabsterUtils.hpp"
#include "oatpp/core/base/Environment.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

class SketchUpService
{

private:
    // Função privada que faz o trabalho pesado de recursão
    void fillTreeRecursive(SUEntitiesRef entities, std::vector<ItemNode> &list)
    {
        // --- 1. PROCESSAR INSTÂNCIAS DE COMPONENTES ---
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

                // Dados da Instância
                instanceData.name = SketchUpUtils::getInstanceName(inst);
                instanceData.guid = SketchUpUtils::getInstanceGuid(inst);

                // Dados da Definição (O componente pai)
                SUComponentDefinitionRef def = SU_INVALID;
                SUComponentInstanceGetDefinition(inst, &def);
                instanceData.definition.name = SketchUpUtils::getDefinitionName(def);
                instanceData.definition.guid = SketchUpUtils::getDefinitionGuid(def);

                // Atribui ao variant
                node.item = instanceData;

                // RECURSÃO: Busca filhos dentro da definição deste componente
                SUEntitiesRef subEntities = SU_INVALID;
                SUComponentDefinitionGetEntities(def, &subEntities);
                fillTreeRecursive(subEntities, node.children);

                list.push_back(node);
            }
        }

        // --- 2. PROCESSAR GRUPOS ---
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

                // Atribui ao variant
                node.item = groupData;

                // RECURSÃO: Busca filhos dentro do grupo
                SUEntitiesRef subEntities = SU_INVALID;
                SUGroupGetEntities(grp, &subEntities);
                fillTreeRecursive(subEntities, node.children);

                list.push_back(node);
            }
        }
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
                // --- FILTRO DE INSTÂNCIAS ---
                size_t instanceCount = 0;
                SUComponentDefinitionGetNumInstances(defs[i], &instanceCount);

                // NOTE: Se não houver nenhuma cópia deste componente no modelo, pula para o próximo
                if (instanceCount == 0)
                {
                    continue;
                }
                // ----------------------------

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

                // 3. Pegar o NOME da Instância (pode ser diferente do nome da definição)
                SUStringRef nameRef = SU_INVALID;
                SUStringCreate(&nameRef);
                SUComponentInstanceGetName(instance, &nameRef);
                comp.name = SketchUpUtils::suStringToStd(nameRef);
                SUStringRelease(&nameRef);

                // 4. Pegar o GUID da Instância (Único para cada peça no cenário)
                SUStringRef guidRef = SU_INVALID;
                SUStringCreate(&guidRef);
                SUComponentInstanceGetGuid(instance, &guidRef);
                comp.guid = SketchUpUtils::suStringToStd(guidRef);
                SUStringRelease(&guidRef);

                // 5. Pegar Atributos da Instância
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

        // Uso da função utilitária
        if (SketchUpUtils::loadModel(filepath, model) == SU_ERROR_NONE)
        {

            SUEntitiesRef modelEntities = SU_INVALID;
            SUModelGetEntities(model, &modelEntities);

            // Executa a recursão
            fillTreeRecursive(modelEntities, rootList);

            // O Release continua aqui, pois o modelo foi "aberto" para esta operação
            SUModelRelease(&model);
        }

        SUTerminate();
        return rootList;
    }

    bool updateInstanceAttribute(const std::string &filepath,
                                 const std::string &guid,
                                 const std::string &dictName,
                                 const std::string &key,
                                 const std::string &value)
    {

        SUInitialize();
        SUModelRef model = SU_INVALID;
        bool success = false;

        if (SketchUpUtils::loadModel(filepath, model) == SU_ERROR_NONE)
        {
            SUEntitiesRef rootEntities = SU_INVALID;
            SUModelGetEntities(model, &rootEntities);

            // 1. Busca a entidade pelo GUID
            SUEntityRef target = SketchUpUtils::findEntityByGuid(rootEntities, guid);

            if (SUIsValid(target))
            {
                // Converter para o tipo ComponentInstance (necessário para a função de validação)
                SUComponentInstanceRef instance = SUComponentInstanceFromEntity(target);

                // 2. Preparar o valor
                SUTypedValueRef typedVal = SU_INVALID;
                SUTypedValueCreate(&typedVal);
                SUTypedValueSetString(typedVal, value.c_str());

                // 3. Chamar a função de validação do Utils
                SUResult res = SketchUpUtils::setAttributeWithValidation(instance, dictName, key, typedVal);

                if (res == SU_ERROR_NONE)
                {
                    SUModelSaveToFile(model, filepath.c_str());
                    success = true;
                }

                SUTypedValueRelease(&typedVal);
            }
            SUModelRelease(&model);
        }

        SUTerminate();
        return success;
    }

    /**
     * Atualiza ou cria um atributo na DEFINIÇÃO (afeta todas as instâncias)
     */
    bool updateDefinitionAttribute(const std::string &filepath,
                                   const std::string &guid,
                                   const std::string &dictName,
                                   const std::string &key,
                                   const std::string &value)
    {

        SUInitialize();
        SUModelRef model = SU_INVALID;
        bool success = false;

        if (SketchUpUtils::loadModel(filepath, model) == SU_ERROR_NONE)
        {

            // 1. Localizar a definição pelo nome
            SUComponentDefinitionRef definition = SU_INVALID;
            // (Assumindo que você tenha a findDefinitionByName no Utils ou implementada aqui)
            definition = SketchUpUtils::findDefinitionByGuid(model, guid);

            if (SUIsValid(definition))
            {
                // 2. Preparar o valor
                SUTypedValueRef typedVal = SU_INVALID;
                SUTypedValueCreate(&typedVal);
                SUTypedValueSetString(typedVal, value.c_str());

                // 3. Chamar a função de criação/update do Utils
                SUResult res = SketchUpUtils::setDefinitionAttribute(definition, dictName, key, typedVal);

                if (res == SU_ERROR_NONE)
                {
                    SUModelSaveToFile(model, filepath.c_str());
                    success = true;
                }

                SUTypedValueRelease(&typedVal);
            }
            SUModelRelease(&model);
        }

        SUTerminate();
        return success;
    }

    bool createGabsterStructure(const std::string &filepath)
    {

        SUInitialize();
        SUModelRef model = SU_INVALID;
        bool success = false;

        if (SketchUpUtils::loadModel(filepath, model) == SU_ERROR_NONE)
        {

            SUResult res = GabsterUtils::createGabsterStructure(model);

            if (res == SU_ERROR_NONE)
            {
                SUModelSaveToFile(model, filepath.c_str());
                success = true;
            }
        }

        SUTerminate();
        return success;
    }

    bool createPaintingFile(const std::string &targetPath,
                            const std::string &imageName,
                            std::function<void(std::ostream *)> imageWriter,
                            double width,
                            double height,
                            double thickness,
                            std::string &outMessage)
    {

        // 1. Criar caminho temporário para a imagem
        std::filesystem::path tmpPath = std::filesystem::temp_directory_path() / ("upload_" + imageName);
        std::string tmpImagePath = tmpPath.string();

        try
        {
            // 2. Salvar o stream da imagem no disco temporariamente
            std::ofstream file(tmpImagePath, std::ios::binary);
            imageWriter(&file);
            file.close();

            // 3. Inicializar SketchUp e Processar
            SUInitialize();

            SUResult res = AviwaUtils::createPaintingFile(targetPath, tmpImagePath, width, height, thickness);

            SUTerminate();

            // 4. Limpeza
            std::filesystem::remove(tmpPath);

            if (res == SU_ERROR_NONE)
            {
                outMessage = "Sucesso ao criar hierarquia ENTERPRISE->VOYAGER.";
                return true;
            }
            else
            {
                outMessage = "Erro na SketchUp API: " + std::to_string(res);
                return false;
            }
        }
        catch (const std::exception &e)
        {
            outMessage = std::string("Erro de IO: ") + e.what();
            if (std::filesystem::exists(tmpPath))
                std::filesystem::remove(tmpPath);
            return false;
        }
    }
};

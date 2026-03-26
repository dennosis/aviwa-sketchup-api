#pragma once

#include <SketchUpAPI/common.h>
#include <SketchUpAPI/model/attribute_dictionary.h>
#include <SketchUpAPI/model/typed_value.h>
#include <SketchUpAPI/model/group.h>
#include "model/SketchUpComponentModel.hpp"

class SketchUpUtils
{
public:
    static std::string suStringToStd(SUStringRef ref)
    {
        size_t len = 0;
        SUStringGetUTF8Length(ref, &len);
        std::string result(len, '\0');
        SUStringGetUTF8(ref, len + 1, &result[0], &len);
        return result;
    }

    static std::string typedValueToString(SUTypedValueRef value)
    {
        SUTypedValueType type;
        SUTypedValueGetType(value, &type);

        switch (type)
        {
        case SUTypedValueType_String:
        {
            SUStringRef str = SU_INVALID;
            SUStringCreate(&str);
            SUTypedValueGetString(value, &str);
            std::string result = suStringToStd(str);
            SUStringRelease(&str);
            return result;
        }
        case SUTypedValueType_Double:
        {
            double d = 0;
            SUTypedValueGetDouble(value, &d);
            return std::to_string(d);
        }
        case SUTypedValueType_Int32:
        {
            int32_t i = 0;
            SUTypedValueGetInt32(value, &i);
            return std::to_string(i);
        }
        case SUTypedValueType_Bool:
        {
            bool b = false;
            SUTypedValueGetBool(value, &b);
            return b ? "true" : "false";
        }
        case SUTypedValueType_Float:
        {
            double f = 0;
            SUTypedValueGetDouble(value, &f);
            return std::to_string(f);
        }
        default:
            return "<unsupported_type>";
        }
    }

    static AttributeDictionaryData readDictionary(SUAttributeDictionaryRef dict)
    {
        AttributeDictionaryData result;

        SUStringRef nameRef = SU_INVALID;
        SUStringCreate(&nameRef);
        SUAttributeDictionaryGetName(dict, &nameRef);
        result.name = suStringToStd(nameRef);
        SUStringRelease(&nameRef);

        size_t keyCount = 0;
        SUAttributeDictionaryGetNumKeys(dict, &keyCount);
        if (keyCount == 0)
            return result;

        std::vector<SUStringRef> keys(keyCount);
        for (auto &k : keys)
            SUStringCreate(&k);
        SUAttributeDictionaryGetKeys(dict, keyCount, keys.data(), &keyCount);

        for (size_t i = 0; i < keyCount; ++i)
        {
            std::string keyStr = suStringToStd(keys[i]);

            SUTypedValueRef typedVal = SU_INVALID;
            SUTypedValueCreate(&typedVal);

            if (SUAttributeDictionaryGetValue(dict, keyStr.c_str(), &typedVal) == SU_ERROR_NONE)
            {
                result.attributes.push_back({keyStr, typedValueToString(typedVal)});
            }

            SUTypedValueRelease(&typedVal);
            SUStringRelease(&keys[i]);
        }

        return result;
    }

    // --- INSTÂNCIAS ---
    static std::string getInstanceName(SUComponentInstanceRef instance)
    {
        SUStringRef str = SU_INVALID;
        SUStringCreate(&str);
        SUComponentInstanceGetName(instance, &str);
        std::string result = suStringToStd(str);
        SUStringRelease(&str);
        return result;
    }

    static std::string getInstanceGuid(SUComponentInstanceRef instance)
    {
        SUStringRef str = SU_INVALID;
        SUStringCreate(&str);
        SUComponentInstanceGetGuid(instance, &str);
        std::string result = suStringToStd(str);
        SUStringRelease(&str);
        return result;
    }

    // --- DEFINIÇÕES (Onde deu o erro C2039) ---
    static std::string getDefinitionName(SUComponentDefinitionRef definition)
    {
        SUStringRef str = SU_INVALID;
        SUStringCreate(&str);
        SUComponentDefinitionGetName(definition, &str);
        std::string result = suStringToStd(str);
        SUStringRelease(&str);
        return result;
    }

    static std::string getDefinitionGuid(SUComponentDefinitionRef definition)
    {
        SUStringRef str = SU_INVALID;
        SUStringCreate(&str);
        SUComponentDefinitionGetGuid(definition, &str);
        std::string result = suStringToStd(str);
        SUStringRelease(&str);
        return result;
    }

    // --- GRUPOS ---
    static std::string getGroupName(SUGroupRef group)
    {
        SUStringRef str = SU_INVALID;
        SUStringCreate(&str);
        SUGroupGetName(group, &str);
        std::string result = suStringToStd(str);
        SUStringRelease(&str);
        return result;
    }

    static std::string getGroupGuid(SUGroupRef group)
    {
        SUStringRef str = SU_INVALID;
        SUStringCreate(&str);
        SUGroupGetGuid(group, &str);
        std::string result = suStringToStd(str);
        SUStringRelease(&str);
        return result;
    }
    static SUResult loadModel(const std::string &filepath, SUModelRef &out_model)
    {
        out_model = SU_INVALID;

        // Inicialize o status com o valor padrão de sucesso
        SUModelLoadStatus status = SUModelLoadStatus_Success;

        SUResult res = SUModelCreateFromFileWithStatus(&out_model, filepath.c_str(), &status);

        if (res != SU_ERROR_NONE)
        {
            return res;
        }

        // Use a constante DIRETAMENTE sem o prefixo SUModelLoadStatus::
        if (status == SUModelLoadStatus_Success_MoreRecent)
        {
            OATPP_LOGW("SketchUpUtils", "Aviso: O arquivo '%s' e de uma versao mais nova.", filepath.c_str());
        }

        return SU_ERROR_NONE;
    }

    static SUResult setRawAttribute(SUEntityRef entity, const std::string &dictName, const std::string &key, SUTypedValueRef value)
    {
        SUAttributeDictionaryRef dict = SU_INVALID;

        // 1. Tenta pegar o dicionário existente pelo nome
        SUResult res = SUEntityGetAttributeDictionary(entity, dictName.c_str(), &dict);

        // 2. Se não existir (SU_ERROR_NO_DATA ou similar), precisamos criar e adicionar
        if (res != SU_ERROR_NONE)
        {
            // Cria o objeto de dicionário de atributos com o nome desejado
            res = SUAttributeDictionaryCreate(&dict, dictName.c_str());
            if (res != SU_ERROR_NONE)
                return res;

            // Adiciona o dicionário recém-criado à entidade
            res = SUEntityAddAttributeDictionary(entity, dict);

            if (res != SU_ERROR_NONE)
            {
                // Se falhou ao adicionar, precisamos liberar a memória do dicionário que criamos
                SUAttributeDictionaryRelease(&dict);
                return res;
            }
            // IMPORTANTE: Após SUEntityAddAttributeDictionary ter sucesso,
            // a entidade assume a posse (ownership), então NÃO chamamos Release aqui.
        }

        // 3. Agora que temos um dicionário válido (existente ou novo), gravamos o valor
        return SUAttributeDictionarySetValue(dict, key.c_str(), value);
    }

    /**
     * Define um atributo na instância apenas se ele já existir na instância ou na definição.
     * @param instance A instância que receberá o valor.
     * @param dictName Nome do dicionário (ex: "dynamic_attributes").
     * @param key Nome da chave (ex: "tmold").
     * @param value O novo valor a ser gravado.
     * @return SU_ERROR_NONE se sucesso, SU_ERROR_INVALID_ARGUMENT se o atributo não existir.
     */
    static SUResult setAttributeWithValidation(SUComponentInstanceRef instance, const std::string &dictName, const std::string &key, SUTypedValueRef value)
    {

        // 1. Verificar se o atributo existe na INSTÂNCIA
        SUAttributeDictionaryRef instDict = SU_INVALID;
        bool existsInInstance = false;
        if (SUEntityGetAttributeDictionary(SUComponentInstanceToEntity(instance), dictName.c_str(), &instDict) == SU_ERROR_NONE)
        {
            SUTypedValueRef testVal = SU_INVALID;
            SUTypedValueCreate(&testVal);
            if (SUAttributeDictionaryGetValue(instDict, key.c_str(), &testVal) == SU_ERROR_NONE)
            {
                existsInInstance = true;
            }
            SUTypedValueRelease(&testVal);
        }

        // 2. Se não existir na instância, verificar na DEFINIÇÃO
        bool existsInDefinition = false;
        SUComponentDefinitionRef def = SU_INVALID;
        SUComponentInstanceGetDefinition(instance, &def);

        SUAttributeDictionaryRef defDict = SU_INVALID;
        if (SUEntityGetAttributeDictionary(SUComponentDefinitionToEntity(def), dictName.c_str(), &defDict) == SU_ERROR_NONE)
        {
            SUTypedValueRef testVal = SU_INVALID;
            SUTypedValueCreate(&testVal);
            if (SUAttributeDictionaryGetValue(defDict, key.c_str(), &testVal) == SU_ERROR_NONE)
            {
                existsInDefinition = true;
            }
            SUTypedValueRelease(&testVal);
        }

        // 3. Validação final: se não existe em nenhum dos dois, dispara erro
        if (!existsInInstance && !existsInDefinition)
        {
            OATPP_LOGE("SketchUpUtils", "Erro: Atributo '%s' nao encontrado na instancia nem na definicao.", key.c_str());
            return SU_ERROR_INVALID_ARGUMENT;
        }

        // 4. Se passou na validação, grava o valor na INSTÂNCIA
        // (Lembre-se: mesmo que exista só na definição, a alteração de valor deve ser gravada na instância)
        return setRawAttribute(SUComponentInstanceToEntity(instance), dictName, key, value);
    }

    /**
     * Define ou cria um atributo diretamente na Definição do componente.
     * @param definition A referência da definição (SUComponentDefinitionRef).
     * @param dictName Nome do dicionário (ex: "dynamic_attributes").
     * @param key Nome da chave.
     * @param value O valor a ser gravado.
     * @return SU_ERROR_NONE em caso de sucesso.
     */
    static SUResult setDefinitionAttribute(SUComponentDefinitionRef definition,
                                           const std::string &dictName,
                                           const std::string &key,
                                           SUTypedValueRef value)
    {

        if (SUIsInvalid(definition))
            return SU_ERROR_INVALID_INPUT;

        // Converter a Definition para Entity para poder manipular atributos
        SUEntityRef entity = SUComponentDefinitionToEntity(definition);

        // Na definição, não validamos se existe. Se não existir, o setRawAttribute
        // (que criamos antes) já cuida de criar o dicionário e a chave.
        SUResult res = setRawAttribute(entity, dictName, key, value);

        if (res == SU_ERROR_NONE)
        {
            OATPP_LOGD("SketchUpUtils", "Atributo '%s' atualizado/criado na Definicao.", key.c_str());
        }
        else
        {
            OATPP_LOGE("SketchUpUtils", "Erro %d ao gravar na Definicao.", res);
        }

        return res;
    }

    /**
     * Busca recursivamente uma entidade (Componente ou Grupo) pelo seu GUID.
     * @param entities A coleção de entidades onde começar a busca (geralmente as do modelo).
     * @param guid O GUID string que estamos procurando.
     * @return SUEntityRef válida se encontrado, ou SU_INVALID caso contrário.
     */
    static SUEntityRef findEntityByGuid(SUEntitiesRef entities, const std::string &targetGuid)
    {
        if (SUIsInvalid(entities))
            return SU_INVALID;

        // 1. Tentar encontrar entre as Instâncias de Componentes
        size_t instanceCount = 0;
        SUEntitiesGetNumInstances(entities, &instanceCount);
        if (instanceCount > 0)
        {
            std::vector<SUComponentInstanceRef> instances(instanceCount);
            SUEntitiesGetInstances(entities, instanceCount, instances.data(), &instanceCount);

            for (auto instance : instances)
            {
                if (getInstanceGuid(instance) == targetGuid)
                {
                    return SUComponentInstanceToEntity(instance);
                }

                // Busca recursiva dentro da definição deste componente (filhos)
                SUComponentDefinitionRef def = SU_INVALID;
                SUComponentInstanceGetDefinition(instance, &def);
                SUEntitiesRef childEntities = SU_INVALID;
                SUComponentDefinitionGetEntities(def, &childEntities);

                SUEntityRef found = findEntityByGuid(childEntities, targetGuid);
                if (SUIsValid(found))
                    return found;
            }
        }

        // 2. Tentar encontrar entre os Grupos
        size_t groupCount = 0;
        SUEntitiesGetNumGroups(entities, &groupCount);
        if (groupCount > 0)
        {
            std::vector<SUGroupRef> groups(groupCount);
            SUEntitiesGetGroups(entities, groupCount, groups.data(), &groupCount);

            for (auto group : groups)
            {
                if (getGroupGuid(group) == targetGuid)
                {
                    return SUGroupToEntity(group);
                }

                // Busca recursiva dentro do grupo
                SUEntitiesRef groupEntities = SU_INVALID;
                SUGroupGetEntities(group, &groupEntities);

                SUEntityRef found = findEntityByGuid(groupEntities, targetGuid);
                if (SUIsValid(found))
                    return found;
            }
        }

        return SU_INVALID;
    }

    // Função auxiliar para achar a definição pelo nome
    static SUComponentDefinitionRef findDefinitionInModel(SUModelRef model, const std::string &name)
    {
        size_t count = 0;
        SUModelGetNumComponentDefinitions(model, &count);
        if (count == 0)
            return SU_INVALID;

        std::vector<SUComponentDefinitionRef> defs(count);
        SUModelGetComponentDefinitions(model, count, defs.data(), &count);

        for (auto &d : defs)
        {
            SUStringRef suName = SU_INVALID;
            SUStringCreate(&suName);
            SUComponentDefinitionGetName(d, &suName);
            std::string currentName = SketchUpUtils::suStringToStd(suName);
            SUStringRelease(&suName);

            if (currentName == name)
                return d;
        }
        return SU_INVALID;
    }

    static SUComponentDefinitionRef findDefinitionByGuid(SUModelRef model, const std::string &targetGuid)
    {
        if (SUIsInvalid(model))
            return SU_INVALID;

        size_t count = 0;
        SUModelGetNumComponentDefinitions(model, &count);
        if (count == 0)
            return SU_INVALID;

        std::vector<SUComponentDefinitionRef> defs(count);
        SUModelGetComponentDefinitions(model, count, defs.data(), &count);

        for (auto &def : defs)
        {
            SUStringRef suGuid = SU_INVALID;
            SUStringCreate(&suGuid);

            // Obtém o GUID da definição
            if (SUComponentDefinitionGetGuid(def, &suGuid) == SU_ERROR_NONE)
            {
                std::string currentGuid = suStringToStd(suGuid);
                SUStringRelease(&suGuid);

                if (currentGuid == targetGuid)
                {
                    return def;
                }
            }
            else
            {
                SUStringRelease(&suGuid);
            }
        }

        return SU_INVALID;
    }
};

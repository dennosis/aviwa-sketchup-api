#pragma once

#include <vector>
#include <string>
#include <SketchUpAPI/common.h>
#include <SketchUpAPI/model/attribute_dictionary.h>
#include <SketchUpAPI/model/component_definition.h>
#include <SketchUpAPI/model/component_instance.h>
#include <SketchUpAPI/model/entities.h>
#include <SketchUpAPI/model/group.h>
#include <SketchUpAPI/model/image.h>
#include <SketchUpAPI/model/model.h>
#include <SketchUpAPI/model/typed_value.h>
#include <SketchUpAPI/sketchup.h>
#include "model/SketchUpComponentModel.hpp"
#include "utils/SketchUpUtils.hpp"
#include <unordered_map>
#undef min
#undef max

class AviwaUtils
{
public:
    static SUModelRef createPaintingModel(const std::string &imagePath,
                                          double widthCm,
                                          double heightCm,
                                          double thicknessCm)
    {
        double w = widthCm / 2.54;
        double h = heightCm / 2.54;
        double t = thicknessCm / 2.54;

        SUModelRef model = SU_INVALID;
        SUResult res = SUModelCreate(&model);
        if (res != SU_ERROR_NONE)
            throw std::runtime_error("SUModelCreate falhou: " + std::to_string(res));

        SUComponentDefinitionRef paintingDef = SU_INVALID;
        res = SUComponentDefinitionCreate(&paintingDef);
        if (res != SU_ERROR_NONE)
        {
            SUModelRelease(&model);
            throw std::runtime_error("SUComponentDefinitionCreate falhou: " + std::to_string(res));
        }
        SUComponentDefinitionSetName(paintingDef, "PAINTING");

        SUEntitiesRef paintingEntities = SU_INVALID;
        SUComponentDefinitionGetEntities(paintingDef, &paintingEntities);

        SUMaterialRef matTexture = SU_INVALID;
        SUMaterialCreate(&matTexture);
        SUMaterialSetName(matTexture, "Capa_Quadro");

        SUTextureRef texture = SU_INVALID;
        if (SUTextureCreateFromFile(&texture, imagePath.c_str(), 1.0, 1.0) == SU_ERROR_NONE)
        {
            SUMaterialSetTexture(matTexture, texture);
            SUTextureSetDimensions(texture, w, h);
        }

        SUMaterialRef matBlack = SU_INVALID;
        SUMaterialCreate(&matBlack);
        SUMaterialSetName(matBlack, "Moldura_Preta");
        SUColor black = {0, 0, 0, 255};
        SUMaterialSetColor(matBlack, &black);
        SUMaterialSetColorizeType(matBlack, SUMaterialColorizeType_Shift);

        SUGeometryInputRef geomInput = SU_INVALID;
        SUGeometryInputCreate(&geomInput);

        SUPoint3D verts[8] = {
            {0, 0, 0},
            {w, 0, 0},
            {w, h, 0},
            {0, h, 0},
            {0, 0, -t},
            {w, 0, -t},
            {w, h, -t},
            {0, h, -t},
        };
        for (int i = 0; i < 8; ++i)
            SUGeometryInputAddVertex(geomInput, &verts[i]);

        auto addFace = [&](std::initializer_list<size_t> indices) -> size_t
        {
            SULoopInputRef loop = SU_INVALID;
            SULoopInputCreate(&loop);
            for (auto idx : indices)
                SULoopInputAddVertexIndex(loop, idx);
            size_t fi = 0;
            SUGeometryInputAddFace(geomInput, &loop, &fi);
            return fi;
        };

        size_t fFront = addFace({0, 1, 2, 3});
        size_t fBack = addFace({7, 6, 5, 4});
        size_t fBottom = addFace({0, 4, 5, 1});
        size_t fRight = addFace({1, 5, 6, 2});
        size_t fTop = addFace({2, 6, 7, 3});
        size_t fLeft = addFace({3, 7, 4, 0});

        SUMaterialInput matInputTexture = {};
        matInputTexture.num_uv_coords = 4;
        matInputTexture.material = matTexture;
        SUPoint2D uvs[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        for (int i = 0; i < 4; ++i)
        {
            matInputTexture.uv_coords[i] = uvs[i];
            matInputTexture.vertex_indices[i] = i;
        }
        SUGeometryInputFaceSetFrontMaterial(geomInput, fFront, &matInputTexture);

        SUMaterialInput matInputBlack = {};
        matInputBlack.num_uv_coords = 0;
        matInputBlack.material = matBlack;
        for (size_t fi : {fBack, fBottom, fRight, fTop, fLeft})
            SUGeometryInputFaceSetFrontMaterial(geomInput, fi, &matInputBlack);

        SUEntitiesFill(paintingEntities, geomInput, true);
        SUGeometryInputRelease(&geomInput);

        SUModelAddComponentDefinitions(model, 1, &paintingDef);
        SUEntitiesRef modelEntities = SU_INVALID;
        SUModelGetEntities(model, &modelEntities);
        SUComponentInstanceRef paintingInst = SU_INVALID;
        SUComponentDefinitionCreateInstance(paintingDef, &paintingInst);
        SUEntitiesAddInstance(modelEntities, paintingInst, nullptr);

        return model;
    }

    static void InitBBox(SUBoundingBox3D &bbox)
    {
        bbox.min_point = {DBL_MAX, DBL_MAX, DBL_MAX};
        bbox.max_point = {-DBL_MAX, -DBL_MAX, -DBL_MAX};
    }

    static void ExpandBBox(SUBoundingBox3D &bbox, const SUPoint3D &p)
    {
        bbox.min_point.x = std::min(bbox.min_point.x, p.x);
        bbox.min_point.y = std::min(bbox.min_point.y, p.y);
        bbox.min_point.z = std::min(bbox.min_point.z, p.z);

        bbox.max_point.x = std::max(bbox.max_point.x, p.x);
        bbox.max_point.y = std::max(bbox.max_point.y, p.y);
        bbox.max_point.z = std::max(bbox.max_point.z, p.z);
    }

    static SUPoint3D TransformPoint(const SUPoint3D &p, const SUTransformation &t)
    {
        SUPoint3D r;

        r.x = t.values[0] * p.x + t.values[4] * p.y + t.values[8] * p.z + t.values[12];
        r.y = t.values[1] * p.x + t.values[5] * p.y + t.values[9] * p.z + t.values[13];
        r.z = t.values[2] * p.x + t.values[6] * p.y + t.values[10] * p.z + t.values[14];

        return r;
    }

    // TODO: rever ele esta considerando a altura no y e nao no z
    static SUResult AddImageAsLeftComponent(
        SUModelRef model,
        const std::string &imagePath,
        const std::string &definitionName)
    {
        if (SUIsInvalid(model))
            return SU_ERROR_INVALID_INPUT;

        // 1. Calcular minX dos componentes existentes
        SUEntitiesRef rootEntities = SU_INVALID;
        SUModelGetEntities(model, &rootEntities);

        size_t count = 0;
        SUEntitiesGetNumInstances(rootEntities, &count);
        if (count == 0)
            return SU_ERROR_NONE;

        std::vector<SUComponentInstanceRef> instances(count);
        SUEntitiesGetInstances(rootEntities, count, instances.data(), &count);

        double minX = DBL_MAX;
        double maxHeight = 0.0;

        for (auto &inst : instances)
        {
            SUComponentDefinitionRef instDef = SU_INVALID;
            SUComponentInstanceGetDefinition(inst, &instDef);

            SUEntitiesRef defEntities = SU_INVALID;
            SUComponentDefinitionGetEntities(instDef, &defEntities);

            SUBoundingBox3D defBBox;
            InitBBox(defBBox);
            SUEntitiesGetBoundingBox(defEntities, &defBBox);

            SUTransformation tr;
            SUComponentInstanceGetTransform(inst, &tr);

            SUPoint3D corners[8] = {
                {defBBox.min_point.x, defBBox.min_point.y, defBBox.min_point.z},
                {defBBox.max_point.x, defBBox.min_point.y, defBBox.min_point.z},
                {defBBox.min_point.x, defBBox.max_point.y, defBBox.min_point.z},
                {defBBox.max_point.x, defBBox.max_point.y, defBBox.min_point.z},
                {defBBox.min_point.x, defBBox.min_point.y, defBBox.max_point.z},
                {defBBox.max_point.x, defBBox.min_point.y, defBBox.max_point.z},
                {defBBox.min_point.x, defBBox.max_point.y, defBBox.max_point.z},
                {defBBox.max_point.x, defBBox.max_point.y, defBBox.max_point.z}};

            SUBoundingBox3D worldBBox;
            InitBBox(worldBBox);
            for (auto &c : corners)
            {
                auto p = TransformPoint(c, tr);
                ExpandBBox(worldBBox, p);
            }

            minX = std::min(minX, worldBBox.min_point.x);

            // TODO: rever o maxHeight
            maxHeight = std::max(maxHeight, worldBBox.max_point.y - worldBBox.min_point.y);
        }

        if (minX == DBL_MAX)
            return SU_ERROR_GENERIC;

        // 2. Criar definição e registrar no modelo
        SUComponentDefinitionRef def = SU_INVALID;
        SUResult res = SUComponentDefinitionCreate(&def);
        if (res != SU_ERROR_NONE)
            return res;

        SUComponentDefinitionSetName(def, definitionName.c_str());

        res = SUModelAddComponentDefinitions(model, 1, &def);
        if (res != SU_ERROR_NONE)
        {
            SUComponentDefinitionRelease(&def);
            return res;
        }

        // 3. Criar imagem e pegar dimensões reais
        SUImageRef image = SU_INVALID;
        res = SUImageCreateFromFile(&image, imagePath.c_str());
        if (res != SU_ERROR_NONE)
            return res;

        double imgWidth = 0.0, imgHeight = 0.0;
        SUImageGetDimensions(image, &imgWidth, &imgHeight); // retorna em polegadas (unidade do modelo)

        double aspectRatio = (imgHeight > 0.0) ? imgWidth / imgHeight : 1.0;
        double targetHeight = maxHeight * 0.5;
        double targetWidth = targetHeight * aspectRatio;

        // escalar a imagem para o tamanho alvo via transform interno
        double scaleX = (imgWidth > 0.0) ? targetWidth / imgWidth : 1.0;
        double scaleY = (imgHeight > 0.0) ? targetHeight / imgHeight : 1.0;

        SUTransformation imgTr = {{scaleX, 0, 0, 0,
                                   0, scaleY, 0, 0,
                                   0, 0, 1, 0,
                                   0, 0, 0, 1}};
        SUImageSetTransform(image, &imgTr);

        SUEntitiesRef defEntities = SU_INVALID;
        SUComponentDefinitionGetEntities(def, &defEntities);

        res = SUEntitiesAddImage(defEntities, image);
        if (res != SU_ERROR_NONE)
            return res;

        // 4. Instanciar com translação apenas (tamanho já definido no transform da imagem)
        SUComponentInstanceRef inst = SU_INVALID;
        res = SUComponentDefinitionCreateInstance(def, &inst);
        if (res != SU_ERROR_NONE)
            return res;

        // Cria deslocamento para posicionar a imagem à esquerda dos componentes existentes, com uma pequena margem
        double offsetX = minX - targetWidth - (0.1 * targetWidth);

        SUTransformation tr = {{1, 0, 0, 0,
                                0, 1, 0, 0,
                                0, 0, 1, 0,
                                offsetX, 0, 0, 1}};

        SUComponentInstanceSetTransform(inst, &tr);

        // 5. Adicionar instância ao modelo
        res = SUEntitiesAddInstance(rootEntities, inst, NULL);
        if (res != SU_ERROR_NONE)
        {
            SUComponentInstanceRelease(&inst);
            return res;
        }

        return SU_ERROR_NONE;
    };
};

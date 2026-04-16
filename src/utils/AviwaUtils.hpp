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

enum class SpacePlane
{
    XY, // Chão (Plano Horizontal)
    XZ, // Frente (Plano Vertical)
    YZ  // Lado (Plano Vertical)
};

class AviwaUtils
{
public:
    static SUModelRef createPaintingModel(const std::string &imagePath,
                                          double widthCm,
                                          double heightCm,
                                          double thicknessCm,
                                          SpacePlane plane = SpacePlane::XZ)
    {
        // Conversão para polegadas (unidade interna do SketchUp)
        double w = widthCm / 2.54;
        double h = heightCm / 2.54;
        double t = thicknessCm / 2.54;

        SUModelRef model = SU_INVALID;
        SUResult res = SUModelCreate(&model);
        if (res != SU_ERROR_NONE)
            throw std::runtime_error("SUModelCreate falhou: " + std::to_string(res));

        // 1. Criar Definição do Componente
        SUComponentDefinitionRef paintingDef = SU_INVALID;
        res = SUComponentDefinitionCreate(&paintingDef);
        if (res != SU_ERROR_NONE)
        {
            SUModelRelease(&model);
            throw std::runtime_error("SUComponentDefinitionCreate falhou: " + std::to_string(res));
        }
        SUComponentDefinitionSetName(paintingDef, "PAINTING");
        SUModelAddComponentDefinitions(model, 1, &paintingDef);

        SUEntitiesRef paintingEntities = SU_INVALID;
        SUComponentDefinitionGetEntities(paintingDef, &paintingEntities);

        // 2. Configurar Materiais
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

        // 3. Criar Geometria (Sempre no plano XY, com profundidade em -Z)
        SUGeometryInputRef geomInput = SU_INVALID;
        SUGeometryInputCreate(&geomInput);

        SUPoint3D verts[8] = {
            {0, 0, 0}, {w, 0, 0}, {w, h, 0}, {0, h, 0}, // Frente
            {0, 0, -t},
            {w, 0, -t},
            {w, h, -t},
            {0, h, -t} // Verso
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

        // 4. Aplicar Textura na Face Frontal
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

        // Aplicar Preto nas outras faces
        SUMaterialInput matInputBlack = {};
        matInputBlack.material = matBlack;
        for (size_t fi : {fBack, fBottom, fRight, fTop, fLeft})
            SUGeometryInputFaceSetFrontMaterial(geomInput, fi, &matInputBlack);

        SUEntitiesFill(paintingEntities, geomInput, true);
        SUGeometryInputRelease(&geomInput);

        // 5. Instanciar e Rotacionar conforme o Plano
        SUEntitiesRef modelEntities = SU_INVALID;
        SUModelGetEntities(model, &modelEntities);

        SUComponentInstanceRef paintingInst = SU_INVALID;
        SUComponentDefinitionCreateInstance(paintingDef, &paintingInst);

        // Criamos uma identidade e aplicamos a rotação baseada no SpacePlane
        // Reaproveitando a lógica de ApplyPlaneRotation que discutimos
        SUTransformation transform = {{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}};
        transform = ApplyPlaneRotation(transform, plane);

        SUComponentInstanceSetTransform(paintingInst, &transform);
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

    static SUResult GetModelBounds(SUEntitiesRef rootEntities, double &outMinX, double &outMaxHeight)
    {
        size_t count = 0;
        SUEntitiesGetNumInstances(rootEntities, &count);
        if (count == 0)
            return SU_ERROR_GENERIC;

        std::vector<SUComponentInstanceRef> instances(count);
        SUEntitiesGetInstances(rootEntities, count, instances.data(), &count);

        outMinX = DBL_MAX;
        outMaxHeight = 0.0;

        for (auto &inst : instances)
        {
            SUComponentDefinitionRef instDef = SU_INVALID;
            SUComponentInstanceGetDefinition(inst, &instDef);

            SUEntitiesRef defEntities = SU_INVALID;
            SUComponentDefinitionGetEntities(instDef, &defEntities);

            SUBoundingBox3D defBBox;
            SUEntitiesGetBoundingBox(defEntities, &defBBox);

            SUTransformation tr;
            SUComponentInstanceGetTransform(inst, &tr);

            // (Mantendo sua lógica original de 8 pontos para garantir precisão com rotações)
            SUPoint3D corners[8] = {
                {defBBox.min_point.x, defBBox.min_point.y, defBBox.min_point.z},
                {defBBox.max_point.x, defBBox.min_point.y, defBBox.min_point.z},
                {defBBox.min_point.x, defBBox.max_point.y, defBBox.min_point.z},
                {defBBox.max_point.x, defBBox.max_point.y, defBBox.min_point.z},
                {defBBox.min_point.x, defBBox.min_point.y, defBBox.max_point.z},
                {defBBox.max_point.x, defBBox.min_point.y, defBBox.max_point.z},
                {defBBox.min_point.x, defBBox.max_point.y, defBBox.max_point.z},
                {defBBox.max_point.x, defBBox.max_point.y, defBBox.max_point.z}};

            for (auto &c : corners)
            {
                auto p = TransformPoint(c, tr);
                outMinX = std::min(outMinX, p.x);
                // Mantendo Y como altura conforme seu código original por enquanto
                outMaxHeight = std::max(outMaxHeight, p.y);
            }
        }
        return (outMinX == DBL_MAX) ? SU_ERROR_GENERIC : SU_ERROR_NONE;
    }

    static SUTransformation CalculateImageTransform(
        double imgW,
        double imgH,
        double targetHeight,
        double &outTargetWidth)
    {
        // Calcula proporção e largura final
        double aspectRatio = (imgH > 0.0) ? imgW / imgH : 1.0;
        outTargetWidth = targetHeight * aspectRatio;

        // Calcula os fatores de escala
        double scaleX = (imgW > 0.0) ? outTargetWidth / imgW : 1.0;
        double scaleY = (imgH > 0.0) ? targetHeight / imgH : 1.0;

        // Constrói a matriz (Identidade com escalas aplicadas)
        // Mantendo Y como altura conforme seu código original
        SUTransformation transform = {{scaleX, 0, 0, 0,
                                       0, scaleY, 0, 0,
                                       0, 0, 1, 0,
                                       0, 0, 0, 1}};

        return transform;
    }

    static SUResult PrepareImage(
        const std::string &path,
        double targetHeight,
        SUImageRef &outImage,
        double &outWidth)
    {
        SUResult res = SUImageCreateFromFile(&outImage, path.c_str());
        if (res != SU_ERROR_NONE)
            return res;

        double imgW, imgH;
        SUImageGetDimensions(outImage, &imgW, &imgH);

        // Usa a função de cálculo que isolamos anteriormente
        SUTransformation imgTr = CalculateImageTransform(imgW, imgH, targetHeight, outWidth);

        return SUImageSetTransform(outImage, &imgTr);
    }

    static SUResult CreateComponentFromImage(
        SUModelRef model,
        SUImageRef image,
        const std::string &name,
        SUComponentDefinitionRef &outDef)
    {
        SUComponentDefinitionCreate(&outDef);
        SUComponentDefinitionSetName(outDef, name.c_str());

        // Registra a definição no modelo
        SUResult res = SUModelAddComponentDefinitions(model, 1, &outDef);
        if (res != SU_ERROR_NONE)
            return res;

        SUEntitiesRef defEntities;
        SUComponentDefinitionGetEntities(outDef, &defEntities);

        // Adiciona a imagem já configurada às entidades da definição
        return SUEntitiesAddImage(defEntities, image);
    }

    static SUResult GetEntitiesBoundingBox(SUEntitiesRef entities, SUBoundingBox3D &outBBox)
    {
        size_t count = 0;
        SUEntitiesGetNumInstances(entities, &count);

        // Inicializa um BBox "vazio" (Extremos invertidos para expansão)
        outBBox.min_point = {DBL_MAX, DBL_MAX, DBL_MAX};
        outBBox.max_point = {-DBL_MAX, -DBL_MAX, -DBL_MAX};

        if (count == 0)
            return SU_ERROR_GENERIC;

        std::vector<SUComponentInstanceRef> instances(count);
        SUEntitiesGetInstances(entities, count, instances.data(), &count);

        for (auto &inst : instances)
        {
            SUComponentDefinitionRef instDef = SU_INVALID;
            SUComponentInstanceGetDefinition(inst, &instDef);

            SUEntitiesRef defEntities = SU_INVALID;
            SUComponentDefinitionGetEntities(instDef, &defEntities);

            SUBoundingBox3D defBBox;
            SUEntitiesGetBoundingBox(defEntities, &defBBox);

            SUTransformation tr;
            SUComponentInstanceGetTransform(inst, &tr);

            // Define os 8 cantos para garantir precisão com rotações
            SUPoint3D corners[8] = {
                {defBBox.min_point.x, defBBox.min_point.y, defBBox.min_point.z},
                {defBBox.max_point.x, defBBox.min_point.y, defBBox.min_point.z},
                {defBBox.min_point.x, defBBox.max_point.y, defBBox.min_point.z},
                {defBBox.max_point.x, defBBox.max_point.y, defBBox.min_point.z},
                {defBBox.min_point.x, defBBox.min_point.y, defBBox.max_point.z},
                {defBBox.max_point.x, defBBox.min_point.y, defBBox.max_point.z},
                {defBBox.min_point.x, defBBox.max_point.y, defBBox.max_point.z},
                {defBBox.max_point.x, defBBox.max_point.y, defBBox.max_point.z}};

            for (const auto &c : corners)
            {
                SUPoint3D worldP = TransformPoint(c, tr); // Sua função auxiliar de transformação

                // Expande o BBox de saída
                outBBox.min_point.x = std::min(outBBox.min_point.x, worldP.x);
                outBBox.min_point.y = std::min(outBBox.min_point.y, worldP.y);
                outBBox.min_point.z = std::min(outBBox.min_point.z, worldP.z);

                outBBox.max_point.x = std::max(outBBox.max_point.x, worldP.x);
                outBBox.max_point.y = std::max(outBBox.max_point.y, worldP.y);
                outBBox.max_point.z = std::max(outBBox.max_point.z, worldP.z);
            }
        }
        return SU_ERROR_NONE;
    }

    static SUTransformation ApplyPlaneRotation(const SUTransformation &baseTr, SpacePlane plane)
    {
        if (plane == SpacePlane::XY)
            return baseTr;

        // Inicializa como Identidade manualmente
        // Uma matriz identidade tem 1.0 na diagonal principal e 0.0 no resto
        SUTransformation rotation = {{1.0, 0.0, 0.0, 0.0,
                                      0.0, 1.0, 0.0, 0.0,
                                      0.0, 0.0, 1.0, 0.0,
                                      0.0, 0.0, 0.0, 1.0}};

        if (plane == SpacePlane::XZ)
        {
            // Rotaciona 90 graus no eixo X
            // m5 = cos(90)=0, m6 = sin(90)=1, m9 = -sin(90)=-1, m10 = cos(90)=0
            rotation.values[5] = 0.0;
            rotation.values[6] = 1.0;
            rotation.values[9] = -1.0;
            rotation.values[10] = 0.0;
        }
        else if (plane == SpacePlane::YZ)
        {
            // Rotaciona para o plano lateral
            rotation.values[0] = 0.0;
            rotation.values[1] = 1.0;
            rotation.values[5] = 0.0;
            rotation.values[6] = 1.0;
            rotation.values[8] = 1.0;
            rotation.values[10] = 0.0;
        }

        SUTransformation result;
        // Ordem: Rotação * Escala
        SUTransformationMultiply(&rotation, &baseTr, &result);

        return result;
    }

    static SUResult PrepareImage(
        const std::string &path,
        const SUBoundingBox3D &sceneBBox,
        double scale,
        SpacePlane plane,
        SUImageRef &outImage,
        double &outWidth)
    {
        SUResult res = SUImageCreateFromFile(&outImage, path.c_str());
        if (res != SU_ERROR_NONE)
            return res;

        double imgW, imgH;
        SUImageGetDimensions(outImage, &imgW, &imgH);

        // 1. Sempre calculamos a escala como se fosse no plano XY primeiro
        // (A "altura" do cenário depende do plano escolhido)
        double sceneHeight = (plane == SpacePlane::XY) ? (sceneBBox.max_point.y - sceneBBox.min_point.y) : (sceneBBox.max_point.z - sceneBBox.min_point.z);

        double targetHeight = sceneHeight * scale;
        double aspectRatio = (imgH > 0.0) ? imgW / imgH : 1.0;
        outWidth = targetHeight * aspectRatio;

        double sX = (imgW > 0.0) ? outWidth / imgW : 1.0;
        double sY = (imgH > 0.0) ? targetHeight / imgH : 1.0;

        // 2. Cria a matriz de escala base (Plano XY)
        SUTransformation baseTr = {{sX, 0, 0, 0,
                                    0, sY, 0, 0,
                                    0, 0, 1, 0,
                                    0, 0, 0, 1}};

        // 3. Aplica a rotação de plano de forma genérica
        SUTransformation finalTr = ApplyPlaneRotation(baseTr, plane);

        return SUImageSetTransform(outImage, &finalTr);
    }

    static SUResult AddImageAsLeftComponent(
        SUModelRef model,
        const std::string &imagePath,
        const std::string &name,
        double scale = 1.0,
        SpacePlane plane = SpacePlane::XZ)
    {
        if (SUIsInvalid(model))
            return SU_ERROR_INVALID_INPUT;

        SUEntitiesRef rootEntities;
        SUModelGetEntities(model, &rootEntities);

        // 1. Obter limites do cenário
        SUBoundingBox3D sceneBBox;
        if (GetEntitiesBoundingBox(rootEntities, sceneBBox) != SU_ERROR_NONE)
        {
            return SU_ERROR_NONE;
        }

        // 2. Preparar imagem (Cálculos de dimensão e escala agora internos)
        SUImageRef image = SU_INVALID;
        double imageWidth = 0.0;
        SUResult res = PrepareImage(imagePath, sceneBBox, scale, plane, image, imageWidth);
        if (res != SU_ERROR_NONE)
            return res;

        // 3. Criar definição
        SUComponentDefinitionRef imageDef = SU_INVALID;
        res = CreateComponentFromImage(model, image, name, imageDef);
        if (res != SU_ERROR_NONE)
            return res;

        // 4. Instanciar e posicionar (minX e translação final)
        SUComponentInstanceRef inst = SU_INVALID;
        SUComponentDefinitionCreateInstance(imageDef, &inst);

        double posX = sceneBBox.min_point.x - imageWidth - (imageWidth * 0.1);

        SUTransformation placementTr = {{1, 0, 0, 0,
                                         0, 1, 0, 0,
                                         0, 0, 1, 0,
                                         posX, 0, 0, 1}};

        SUComponentInstanceSetTransform(inst, &placementTr);

        return SUEntitiesAddInstance(rootEntities, inst, NULL);
    }
};

#pragma once

#include <vector>
#include <string>
#include <SketchUpAPI/common.h>
#include <SketchUpAPI/model/attribute_dictionary.h>
#include <SketchUpAPI/model/typed_value.h>
#include <SketchUpAPI/model/group.h>
#include <SketchUpAPI/sketchup.h>
#include "model/SketchUpComponentModel.hpp"
#include "utils/SketchUpUtils.hpp"
#include <unordered_map>

class AviwaUtils
{
public:
    static SUResult createPaintingFile(const std::string &skpPath, const std::string &imagePath, double widthCm, double heightCm, double thicknessCm)
    {
        double w = widthCm / 2.54;
        double h = heightCm / 2.54;
        double t = thicknessCm / 2.54;

        // 1. Criar modelo
        SUModelRef model = SU_INVALID;
        SUResult res = SUModelCreate(&model);
        if (res != SU_ERROR_NONE)
            return res;

        // 2. Criar definição da pintura (geometria + textura)
        SUComponentDefinitionRef paintingDef = SU_INVALID;
        res = SUComponentDefinitionCreate(&paintingDef);
        if (res != SU_ERROR_NONE)
        {
            SUModelRelease(&model);
            return res;
        }
        SUComponentDefinitionSetName(paintingDef, "PAINTING");

        SUEntitiesRef paintingEntities = SU_INVALID;
        SUComponentDefinitionGetEntities(paintingDef, &paintingEntities);

        // 3. Material com textura (face frontal)
        SUMaterialRef matTexture = SU_INVALID;
        SUMaterialCreate(&matTexture);
        SUMaterialSetName(matTexture, "Capa_Quadro");

        SUTextureRef texture = SU_INVALID;
        if (SUTextureCreateFromFile(&texture, imagePath.c_str(), 1.0, 1.0) == SU_ERROR_NONE)
        {
            SUMaterialSetTexture(matTexture, texture);
            SUTextureSetDimensions(texture, w, h);
        }

        // 4. Material preto (demais faces)
        SUMaterialRef matBlack = SU_INVALID;
        SUMaterialCreate(&matBlack);
        SUMaterialSetName(matBlack, "Moldura_Preta");
        SUColor black = {0, 0, 0, 255};
        SUMaterialSetColor(matBlack, &black);
        SUMaterialSetColorizeType(matBlack, SUMaterialColorizeType_Shift);

        // 5. Geometria com espessura
        SUGeometryInputRef geomInput = SU_INVALID;
        SUGeometryInputCreate(&geomInput);

        // 8 vértices: frente (z=0) e fundo (z=-t)
        SUPoint3D verts[8] = {
            {0, 0, 0},
            {w, 0, 0},
            {w, h, 0},
            {0, h, 0}, // frente 0-3
            {0, 0, -t},
            {w, 0, -t},
            {w, h, -t},
            {0, h, -t}, // fundo  4-7
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

        size_t fFront = addFace({0, 1, 2, 3});  // frontal  — textura
        size_t fBack = addFace({7, 6, 5, 4});   // traseira — preto
        size_t fBottom = addFace({0, 4, 5, 1}); // baixo    — preto
        size_t fRight = addFace({1, 5, 6, 2});  // direita  — preto
        size_t fTop = addFace({2, 6, 7, 3});    // cima     — preto
        size_t fLeft = addFace({3, 7, 4, 0});   // esquerda — preto

        // Textura na face frontal
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

        // Preto nas demais faces
        SUMaterialInput matInputBlack = {};
        matInputBlack.num_uv_coords = 0;
        matInputBlack.material = matBlack;
        for (size_t fi : {fBack, fBottom, fRight, fTop, fLeft})
            SUGeometryInputFaceSetFrontMaterial(geomInput, fi, &matInputBlack);

        SUEntitiesFill(paintingEntities, geomInput, true);
        SUGeometryInputRelease(&geomInput);

        // 6. Registra PAINTING e instancia na raiz
        SUModelAddComponentDefinitions(model, 1, &paintingDef);
        SUEntitiesRef modelEntities = SU_INVALID;
        SUModelGetEntities(model, &modelEntities);
        SUComponentInstanceRef paintingInst = SU_INVALID;
        SUComponentDefinitionCreateInstance(paintingDef, &paintingInst);
        SUEntitiesAddInstance(modelEntities, paintingInst, nullptr);

        // 8. Salva e libera
        res = SUModelSaveToFile(model, skpPath.c_str());
        SUModelRelease(&model);
        return res;
    }
};

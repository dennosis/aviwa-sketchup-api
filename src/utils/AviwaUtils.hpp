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
#include <array>

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
                                          const std::string &name,
                                          SpacePlane plane = SpacePlane::XZ)
    {
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
        SUComponentDefinitionSetName(paintingDef, name.c_str());
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

        // 3. Calcular rotação e aplicar diretamente nos vértices base (plano XY)
        SUPoint3D verts[8] = {
            {0, 0, 0}, {w, 0, 0}, {w, h, 0}, {0, h, 0}, // Frente
            {0, 0, -t},
            {w, 0, -t},
            {w, h, -t},
            {0, h, -t} // Verso
        };

        // Monta a matriz de rotação para o plano desejado
        SUTransformation rot = {{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}};
        rot = ApplyPlaneRotation(rot, plane);

        // SUTransformation é column-major:
        //   [ v[0]  v[4]  v[8]  v[12] ]
        //   [ v[1]  v[5]  v[9]  v[13] ]
        //   [ v[2]  v[6]  v[10] v[14] ]
        //   [ v[3]  v[7]  v[11] v[15] ]
        for (int i = 0; i < 8; ++i)
        {
            const double x = verts[i].x, y = verts[i].y, z = verts[i].z;
            verts[i] = {
                rot.values[0] * x + rot.values[4] * y + rot.values[8] * z + rot.values[12],
                rot.values[1] * x + rot.values[5] * y + rot.values[9] * z + rot.values[13],
                rot.values[2] * x + rot.values[6] * y + rot.values[10] * z + rot.values[14]};
        }

        // 4. Criar Geometria com vértices já no plano correto
        SUGeometryInputRef geomInput = SU_INVALID;
        SUGeometryInputCreate(&geomInput);

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

        // 5. Aplicar Textura na Face Frontal
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

        // 6. Instanciar com transform identidade — rotação já está na geometria
        SUEntitiesRef modelEntities = SU_INVALID;
        SUModelGetEntities(model, &modelEntities);

        SUComponentInstanceRef paintingInst = SU_INVALID;
        SUComponentDefinitionCreateInstance(paintingDef, &paintingInst);

        SUTransformation identity = {{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}};
        SUComponentInstanceSetTransform(paintingInst, &identity);
        SUEntitiesAddInstance(modelEntities, paintingInst, nullptr);

        return model;
    }

    static void addSweptFrameToModel(SUModelRef model,
                                     double widthCm,
                                     double heightCm,
                                     const std::string &name,
                                     const std::vector<SUPoint2D> &profile2D,
                                     SpacePlane plane = SpacePlane::XZ)
    {
        const double w = widthCm / 2.54;
        const double h = heightCm / 2.54;

        using V3 = std::array<double, 3>;

        auto applyPlane = [&](V3 v, const SUTransformation &rot) -> V3
        {
            return {
                rot.values[0] * v[0] + rot.values[4] * v[1] + rot.values[8] * v[2] + rot.values[12],
                rot.values[1] * v[0] + rot.values[5] * v[1] + rot.values[9] * v[2] + rot.values[13],
                rot.values[2] * v[0] + rot.values[6] * v[1] + rot.values[10] * v[2] + rot.values[14]};
        };

        SUTransformation rot = {{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}};
        rot = ApplyPlaneRotation(rot, plane);

        auto v3add = [](V3 a, V3 b) -> V3
        { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; };
        auto v3sub = [](V3 a, V3 b) -> V3
        { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; };
        auto v3scale = [](V3 a, double s) -> V3
        { return {a[0] * s, a[1] * s, a[2] * s}; };
        auto v3norm = [](V3 a) -> V3
        {
            double l = std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
            return l > 1e-10 ? V3{a[0] / l, a[1] / l, a[2] / l} : V3{0, 0, 0};
        };
        auto v3dot = [](V3 a, V3 b) -> double
        { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; };
        auto v3cross = [](V3 a, V3 b) -> V3
        {
            return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
        };

        const int N = 4;
        std::vector<V3> rawPath = {{0, 0, 0}, {w, 0, 0}, {w, h, 0}, {0, h, 0}};
        std::vector<V3> pathPts(N);
        for (int i = 0; i < N; ++i)
            pathPts[i] = applyPlane(rawPath[i], rot);

        V3 up = applyPlane({0, 0, -1}, rot);
        V3 center = applyPlane({w / 2, h / 2, 0}, rot);

        struct CornerFrame
        {
            V3 origin, right, up;
        };
        std::vector<CornerFrame> frames(N);

        for (int i = 0; i < N; ++i)
        {
            int prev = (i + N - 1) % N;
            int next = (i + 1) % N;

            V3 fIn = v3norm(v3sub(pathPts[i], pathPts[prev]));
            V3 fOut = v3norm(v3sub(pathPts[next], pathPts[i]));

            V3 bisector = v3norm(v3add(fIn, fOut));
            V3 planeNormal = applyPlane({0, 0, 1}, rot);

            V3 rightDir = v3norm(v3cross(bisector, planeNormal));
            V3 outward = v3norm(v3sub(pathPts[i], center));
            if (v3dot(rightDir, outward) < 0)
                rightDir = v3scale(rightDir, -1.0);

            double cosHalf = v3dot(bisector, fOut);
            double miterScale = (cosHalf > 1e-6) ? (1.0 / cosHalf) : 1.0;

            frames[i] = {pathPts[i], v3scale(rightDir, miterScale), up};
        }

        const int P = static_cast<int>(profile2D.size());
        SUGeometryInputRef geom = SU_INVALID;
        SUGeometryInputCreate(&geom);

        auto sweepPt = [&](int corner, int j) -> V3
        {
            const auto &f = frames[corner];
            return v3add(f.origin, v3add(v3scale(f.right, profile2D[j].x),
                                         v3scale(f.up, profile2D[j].y)));
        };

        for (int i = 0; i < N; ++i)
            for (int j = 0; j < P; ++j)
            {
                V3 pt = sweepPt(i, j);
                SUPoint3D v = {pt[0], pt[1], pt[2]};
                SUGeometryInputAddVertex(geom, &v);
            }

        auto idx = [&](int corner, int j) -> size_t
        { return (size_t)(((corner + N) % N) * P + j); };

        auto addFace = [&](std::vector<size_t> indices)
        {
            SULoopInputRef loop = SU_INVALID;
            SULoopInputCreate(&loop);
            for (auto k : indices)
                SULoopInputAddVertexIndex(loop, k);
            size_t fi = 0;
            SUGeometryInputAddFace(geom, &loop, &fi);
        };

        // Faces laterais — winding CCW visto de fora (normal aponta para fora)
        for (int i = 0; i < N; ++i)
        {
            int next = (i + 1) % N;
            for (int j = 0; j < P - 1; ++j)
                addFace({idx(i, j), idx(i, j + 1), idx(next, j + 1), idx(next, j)});
        }

        // Faces de tampa entre segmentos do perfil — winding corrigida
        for (int i = 0; i < N; ++i)
        {
            int next = (i + 1) % N;
            addFace({idx(i, 0), idx(next, 0), idx(next, P - 1), idx(i, P - 1)});
        }

        SUComponentDefinitionRef def = SU_INVALID;
        SUComponentDefinitionCreate(&def);
        SUComponentDefinitionSetName(def, name.c_str());
        SUModelAddComponentDefinitions(model, 1, &def);

        SUEntitiesRef defEntities = SU_INVALID;
        SUComponentDefinitionGetEntities(def, &defEntities);
        SUEntitiesFill(defEntities, geom, true);
        SUGeometryInputRelease(&geom);

        SUEntitiesRef modelEntities = SU_INVALID;
        SUModelGetEntities(model, &modelEntities);
        SUComponentInstanceRef inst = SU_INVALID;
        SUComponentDefinitionCreateInstance(def, &inst);

        SUTransformation identity = {{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}};
        SUComponentInstanceSetTransform(inst, &identity);
        SUEntitiesAddInstance(modelEntities, inst, nullptr);
    }

    static void applyImageMaterialToComponent(
        SUModelRef model,
        const std::string &componentGuid,
        const std::string &imagePath)
    {
        if (SUIsInvalid(model))
            throw std::runtime_error("Modelo inválido");

        // 1. Localizar a definição do componente
        SUComponentDefinitionRef targetDef = SU_INVALID;
        size_t defCount = 0;
        SUModelGetNumComponentDefinitions(model, &defCount);
        if (defCount > 0)
        {
            std::vector<SUComponentDefinitionRef> defs(defCount);
            SUModelGetComponentDefinitions(model, defCount, defs.data(), &defCount);

            for (auto &def : defs)
            {
                SUStringRef guidStr = SU_INVALID;
                SUStringCreate(&guidStr);
                SUComponentDefinitionGetGuid(def, &guidStr);

                size_t len = 0;
                SUStringGetUTF8Length(guidStr, &len);
                std::vector<char> buf(len + 1, '\0');
                SUStringGetUTF8(guidStr, len + 1, buf.data(), &len);
                SUStringRelease(&guidStr);

                if (componentGuid == std::string(buf.data()))
                {
                    targetDef = def;
                    break;
                }
            }
        }

        if (SUIsInvalid(targetDef))
            throw std::runtime_error("GUID não encontrado");

        // 2. Gerar nome único para o material para evitar corrupção
        // Adicionar um timestamp ou sufixo garante que o SKP não quebre por nomes duplicados
        std::string matName = "Mat_" + componentGuid;

        // 3. Criar Material e Textura de forma segura
        SUMaterialRef mat = SU_INVALID;
        if (SUMaterialCreate(&mat) != SU_ERROR_NONE)
            throw std::runtime_error("Falha ao criar material");

        SUMaterialSetName(mat, matName.c_str());

        SUTextureRef tex = SU_INVALID;
        // Note: 100.0, 100.0 define a escala inicial da textura no SketchUp
        if (SUTextureCreateFromFile(&tex, imagePath.c_str(), 1.0, 1.0) == SU_ERROR_NONE)
        {
            SUMaterialSetTexture(mat, tex);
            // Após SUMaterialSetTexture, o Material é dono da textura.
            // Não é estritamente necessário dar Release na textura aqui se ela foi vinculada,
            // mas o SDK recomenda dependendo da versão. O material cuidará disso.
        }
        else
        {
            SUMaterialRelease(&mat);
            throw std::runtime_error("Falha ao carregar imagem: " + imagePath);
        }

        // 4. ADICIONAR AO MODELO ANTES DE USAR
        // Isso é vital. O material precisa estar registrado no "pool" do modelo.
        if (SUModelAddMaterials(model, 1, &mat) != SU_ERROR_NONE)
        {
            // Se falhar, pode ser que o nome já exista.
            // Tente recuperar o material existente ou dar release.
            SUMaterialRelease(&mat);
            // Se o erro for por nome duplicado, o salvamento falharia se não tratássemos.
            return;
        }

        // 5. Aplicar nas faces
        SUEntitiesRef entities = SU_INVALID;
        SUComponentDefinitionGetEntities(targetDef, &entities);

        size_t faceCount = 0;
        SUEntitiesGetNumFaces(entities, &faceCount);

        if (faceCount > 0)
        {
            std::vector<SUFaceRef> faces(faceCount);
            SUEntitiesGetFaces(entities, faceCount, faces.data(), &faceCount);

            for (auto &face : faces)
            {
                if (SUIsValid(face))
                {
                    // Aplicar na frente e no verso para garantir visibilidade
                    SUFaceSetFrontMaterial(face, mat);
                    SUFaceSetBackMaterial(face, mat);
                }
            }
        }
    }

    static void applyColorMaterialToComponent(
        SUModelRef model,
        const std::string &componentGuid,
        uint8_t r,
        uint8_t g,
        uint8_t b,
        uint8_t a)
    {
        if (SUIsInvalid(model))
            throw std::runtime_error("applyColorMaterialToComponent: model inválido");

        // 1. Localizar definição pelo GUID
        size_t defCount = 0;
        SUModelGetNumComponentDefinitions(model, &defCount);
        if (defCount == 0)
            throw std::runtime_error("applyColorMaterialToComponent: modelo sem definições de componente");

        std::vector<SUComponentDefinitionRef> defs(defCount);
        SUModelGetComponentDefinitions(model, defCount, defs.data(), &defCount);

        SUComponentDefinitionRef targetDef = SU_INVALID;
        for (auto &def : defs)
        {
            SUStringRef guidStr = SU_INVALID;
            SUStringCreate(&guidStr);
            SUComponentDefinitionGetGuid(def, &guidStr);

            size_t len = 0;
            SUStringGetUTF8Length(guidStr, &len);
            std::vector<char> buf(len + 1, '\0');
            size_t outLen = 0;
            SUStringGetUTF8(guidStr, len + 1, buf.data(), &outLen);
            SUStringRelease(&guidStr);

            if (componentGuid == std::string(buf.data()))
            {
                targetDef = def;
                break;
            }
        }

        if (SUIsInvalid(targetDef))
            throw std::runtime_error("applyColorMaterialToComponent: GUID não encontrado: " + componentGuid);

        // 2. Criar material
        SUMaterialRef mat = SU_INVALID;
        if (SUMaterialCreate(&mat) != SU_ERROR_NONE)
            throw std::runtime_error("applyColorMaterialToComponent: SUMaterialCreate falhou");

        // 3. Definir nome
        SUMaterialSetName(mat, ("Color_" + componentGuid).c_str());

        // 4. Definir cor RGBA
        SUColor color{};
        color.red = r;
        color.green = g;
        color.blue = b;
        color.alpha = a;
        SUMaterialSetColor(mat, &color);

        // 5. Colorize type
        SUMaterialSetColorizeType(mat, SUMaterialColorizeType_Shift);

        // 6. Transparência
        if (a < 255)
        {
            SUMaterialSetOpacity(mat, static_cast<double>(a) / 255.0);
            SUMaterialSetUseOpacity(mat, true);
        }

        // 7. Adicionar ao modelo — ownership transferido, não chamar Release
        if (SUModelAddMaterials(model, 1, &mat) != SU_ERROR_NONE)
            throw std::runtime_error("applyColorMaterialToComponent: SUModelAddMaterials falhou");

        // 8. Aplicar nas faces
        SUEntitiesRef entities = SU_INVALID;
        SUComponentDefinitionGetEntities(targetDef, &entities);

        size_t faceCount = 0;
        SUEntitiesGetNumFaces(entities, &faceCount);

        if (faceCount > 0)
        {
            std::vector<SUFaceRef> faces(faceCount);
            SUEntitiesGetFaces(entities, faceCount, faces.data(), &faceCount);

            for (auto &face : faces)
            {
                if (SUIsValid(face))
                    SUFaceSetFrontMaterial(face, mat);
            }
        }
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

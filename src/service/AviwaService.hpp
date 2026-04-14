#pragma once

#include "utils/AviwaUtils.hpp"
#include "model/TempFileModel.hpp"
#include <string>

class AviwaService
{
private:
    OATPP_COMPONENT(std::shared_ptr<TempPath>, m_tempPath);

    std::string generateTempPath(const std::string &prefix) const
    {
        return (std::filesystem::path(m_tempPath->value) /
                (prefix + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                 ".skp"))
            .string();
    }

public:
    AviwaService() { SUInitialize(); }
    ~AviwaService() { SUTerminate(); }

    std::string createPaintingFile(const std::string &imagePath,
                                   double width,
                                   double height,
                                   double thickness)
    {
        auto skpPath = generateTempPath("painting_");

        SUModelRef model = AviwaUtils::createPaintingModel(imagePath, width, height, thickness);

        SUResult res = SUModelSaveToFile(model, skpPath.c_str());
        SUModelRelease(&model);

        if (res != SU_ERROR_NONE)
            throw std::runtime_error("Erro na SketchUp API: " + std::to_string(res));

        return skpPath;
    }

    std::string AddImageAsLeftComponent(const std::string &filePath,
                                        const std::string &imagePath,
                                        const std::string &name)
    {
        auto skpPath = generateTempPath("file_");

        SUModelRef model = SU_INVALID;
        SUResult res = SketchUpUtils::loadModel(filePath, model);
        if (res != SU_ERROR_NONE)
            throw std::runtime_error("Erro ao carregar modelo: " + std::to_string(res));

        res = AviwaUtils::AddImageAsLeftComponent(model, imagePath, name);
        if (res != SU_ERROR_NONE)
        {
            SUModelRelease(&model);
            throw std::runtime_error("Erro ao adicionar imagem: " + std::to_string(res));
        }

        res = SUModelSaveToFile(model, skpPath.c_str());
        SUModelRelease(&model);

        if (res != SU_ERROR_NONE)
            throw std::runtime_error("Erro na SketchUp API: " + std::to_string(res));

        return skpPath;
    }
};
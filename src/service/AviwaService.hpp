#pragma once

#include "service/TempFileService.hpp"
#include "service/SketchUpService.hpp"
#include "utils/AviwaUtils.hpp"
#include <string>

class AviwaService
{
private:
    OATPP_COMPONENT(std::shared_ptr<TempFileService>, m_tempFileService);
    OATPP_COMPONENT(std::shared_ptr<SketchUpService>, m_sketchUpService);

public:
    // TODO: Arrumar a posição do uqadro
    std::string createPaintingFile(const std::string &imagePath,
                                   double width,
                                   double height,
                                   double thickness)
    {

        auto filePath = m_tempFileService->generatePath("painting", "skp").string();

        return m_sketchUpService->saveModel(filePath, [&]()
                                            {
        SUModelRef model = AviwaUtils::createPaintingModel(imagePath, width, height, thickness);
                                     
                                                 return model; });
    }

    // Incluir parametro de escala
    std::string AddImageAsLeftComponent(const std::string &filePath,
                                        const std::string &imagePath,
                                        const std::string &name,
                                        double scale = 1.0)
    {

        return m_sketchUpService->editAndSaveModel(filePath, [&](SUModelRef model)
                                                   {
                                                 SUResult res = AviwaUtils::AddImageAsLeftComponent(model, imagePath, name, scale);
                                                 if (res != SU_ERROR_NONE)
                                                 {
                                                     SUModelRelease(&model);
                                                     throw std::runtime_error("Erro ao adicionar imagem: " + std::to_string(res));
                                                 }
                                                 return filePath; });
    }
};
#pragma once

#include "service/TempFileService.hpp"
#include "service/SketchUpService.hpp"
#include "utils/AviwaUtils.hpp"
#include "utils/ColorUtils.hpp"

#include <string>

class AviwaService
{
private:
    OATPP_COMPONENT(std::shared_ptr<TempFileService>, m_tempFileService);
    OATPP_COMPONENT(std::shared_ptr<SketchUpService>, m_sketchUpService);

public:
    std::string createPaintingFile(const std::string &imagePath,
                                   double width,
                                   double height,
                                   double thickness,
                                   const std::string &name = "PAINTING")
    {
        auto filePath = m_tempFileService->generatePath("painting", "skp").string();

        return m_sketchUpService->saveModel(filePath, [&]()
                                            {
        SUModelRef model = AviwaUtils::createPaintingModel(imagePath, width, height, thickness, name);

        return model; });
    }

    std::string createSweptFrame( // const std::string &imagePath,
        const std::string &filePath,
        double width,
        double height,
        const std::string &name = "FRAME",
        std::optional<std::vector<SUPoint2D>> profile = std::nullopt)
    {
        return m_sketchUpService->editAndSaveModel(filePath, [&](SUModelRef model)
                                                   {
                                         

            AviwaUtils::addSweptFrameToModel(model, width, height, name, profile.value());


                                                 return filePath; });
    }

    std::string applyImageMaterialToComponent(const std::string &filePath,
                                              const std::string &imagePath,
                                              const std::string &guid

    )
    {

        return m_sketchUpService->editAndSaveModel(filePath, [&](SUModelRef model)
                                                   {
                                                AviwaUtils::applyImageMaterialToComponent(model, guid, imagePath);
                                          
                                                 return filePath; });
    }

    std::string applyColorMaterialToComponent(const std::string &filePath,
                                              const std::string &guid,
                                              const std::string &hexColor)
    {
        RgbaColor c = hexToRgba(hexColor);

        return m_sketchUpService->editAndSaveModel(filePath, [&](SUModelRef model)
                                                   {
        AviwaUtils::applyColorMaterialToComponent(model, guid, c.r, c.g, c.b, c.a);




                                                 return filePath; });
    }

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
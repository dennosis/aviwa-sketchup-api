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

    std::string createGabsterStructure(const std::string &filePath,
                                       const std::string author,
                                       const std::string title,
                                       const std::string code,
                                       const std::string gbsId,
                                       const std::string description)
    {
        auto skpPath = generateTempPath("file_");

        SUModelRef model = SU_INVALID;
        SUResult res = SketchUpUtils::loadModel(filePath, model);
        if (res != SU_ERROR_NONE)
            throw std::runtime_error("Erro ao carregar modelo: " + std::to_string(res));

        res = SketchUpUtils::wrapRootInstances(model, "VOYAGER");
        if (res != SU_ERROR_NONE)
        {
            SUModelRelease(&model);
            throw std::runtime_error("Erro ao criar estrutura Gabster: " + std::to_string(res));
        }

        std::string descriptionHtml =
            "<font size=\"2\">\n"
            "  <font color=\"#808000\">\n"
            "    <b>Nome da Obra:</b> " +
            title + "<br>\n"
                    "    <b>Artista:</b> " +
            author + "<br>\n"
                     "    <b>Descrição:</b> " +
            description + "<br>\n"
                          "    <b>Código produto:</b> " +
            code + "\n"
                   "  </font>\n"
                   "</font>";

        res = SketchUpUtils::wrapRootInstances(model, "ENTERPRISE", {
                                                                        {"dynamic_attributes:description", descriptionHtml},
                                                                        {"dynamic_attributes:itemcode", "<a href=\"https://www.aviwa.com.br/product-page/conjunto-patagonia-70x70\">\n  <font size=\"2\">\n    <b></b>\n    <font color=\"#c46f0e\">Acesse a página do produto aqui</font>\n  </font>\n</a>"},
                                                                        {"dynamic_attributes:summary", "<font size=\"2\"><font color=\"#556B2F\"><b>Obra Autoral. Uso consciente em projeto. Mais detalhes no card.<br><b></font></font>"},
                                                                        {"dynamic_attributes:name", "<font color=\"#2F4F4F\"><font size=\"4\">AVIWA - Arte para arquitetura <br></font></font>"},
                                                                        {"dynamic_attributes:imageurl", "https://static.wixstatic.com/media/a6ffba_69e3cd8c392f42a29adb11cda9251247~mv2.png/v1/fill/w_512,h_336,al_c,q_85,usm_0.66_1.00_0.01,enc_avif,quality_auto/Logo%20Ajustada.png"},
                                                                        {"dynamic_attributes:gbsid", gbsId},
                                                                        {"dynamic_attributes:gbs_is_component", "1"},
                                                                    });

        if (res != SU_ERROR_NONE)
        {
            SUModelRelease(&model);
            throw std::runtime_error("Erro ao criar estrutura Gabster: " + std::to_string(res));
        }

        res = SUModelSaveToFile(model, skpPath.c_str());
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
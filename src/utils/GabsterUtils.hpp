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

class GabsterUtils
{
public:
    static SUResult createGabsterStructure(SUModelRef model)
    {

        SUResult res = SketchUpUtils::wrapRootInstances(model, "VOYAGER");
        if (res != SU_ERROR_NONE)
        {
            SUModelRelease(&model);
            return res;
        }

        res = SketchUpUtils::wrapRootInstances(model, "ENTERPRISE", {
                                                                        {"dynamic_attributes:description", "<font size=\"2\">\n  <font color=\"#808000\">\n    <b>Nome das Obras:</b> Notro e Neneo<br>\n    <b>Artista:</b> Karen Haas<br>\n    <b>Coleção:</b> Conjunto Patagônia<br>\n    <b>Composição:</b> 2 quadros<br>\n    <b>Status:</b> Até 10 peças Sob encomenda<br>\n    <b>Tamanho dos quadros:</b> 70x70<br>\n    <b>Código produto:</b> SER000126b\n  </font>\n</font>"},
                                                                        {"dynamic_attributes:itemcode", "<a href=\"https://www.aviwa.com.br/product-page/conjunto-patagonia-70x70\">\n  <font size=\"2\">\n    <b></b>\n    <font color=\"#c46f0e\">Acesse a página do produto aqui</font>\n  </font>\n</a>"},
                                                                        {"dynamic_attributes:summary", "<font size=\"2\"><font color=\"#556B2F\"><b>Obra Autoral. Uso consciente em projeto. Mais detalhes no card.<br><b></font></font>"},
                                                                        {"dynamic_attributes:name", "<font color=\"#2F4F4F\"><font size=\"4\">AVIWA - Arte para arquitetura <br></font></font>"},
                                                                        {"dynamic_attributes:imageurl", "https://static.wixstatic.com/media/a6ffba_69e3cd8c392f42a29adb11cda9251247~mv2.png/v1/fill/w_512,h_336,al_c,q_85,usm_0.66_1.00_0.01,enc_avif,quality_auto/Logo%20Ajustada.png"},
                                                                        {"dynamic_attributes:gbsid", "1682214"},

                                                                    });
        if (res != SU_ERROR_NONE)
        {
            SUModelRelease(&model);
            return res;
        }

        return SU_ERROR_NONE;
    }
};

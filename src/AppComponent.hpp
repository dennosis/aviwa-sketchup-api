#pragma once

#include "oatpp/web/server/HttpConnectionHandler.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"
#include "oatpp/parser/json/mapping/ObjectMapper.hpp"
#include "oatpp/core/macro/component.hpp"
#include "Config.hpp"
#include "service/SketchUpService.hpp"
#include "service/FileService.hpp"

class AppComponent
{
public:
  /**
   * 1. ObjectMapper CORRIGIDO: Usamos o tipo genérico 'data::mapping'
   * para que o Controller consiga encontrá-lo.
   */
  OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper)([]
                                                                                            {
    auto mapper = oatpp::parser::json::mapping::ObjectMapper::createShared();
    
    // Não escapa '/' como '\/'
    mapper->getSerializer()->getConfig()->escapeFlags = 0;
    
    // Não escapa caracteres não-ASCII como \uXXXX
    mapper->getSerializer()->getConfig()->useBeautifier = false;
    mapper->getSerializer()->getConfig()->escapeFlags = 
        oatpp::parser::json::Utils::FLAG_ESCAPE_SOLIDUS & 0; // desativa escape de /
    
    return mapper; }());

  /**
   * 2. ConnectionProvider
   */

  OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, serverConnectionProvider)([]
                                                                                                              { return oatpp::network::tcp::server::ConnectionProvider::createShared({Config::SERVER_HOST, Config::SERVER_PORT, oatpp::network::Address::IP_4}); }());

  /**
   * 3. Router
   */
  OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, httpRouter)([]
                                                                                      { return oatpp::web::server::HttpRouter::createShared(); }());

  /**
   * 4. ConnectionHandler
   */
  OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, serverConnectionHandler)([]
                                                                                                      {
    OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router); 
    return oatpp::web::server::HttpConnectionHandler::createShared(router); }());

  // Criamos o componente do Service
  OATPP_CREATE_COMPONENT(std::shared_ptr<SketchUpService>, sketchUpService)([]
                                                                            { return std::make_shared<SketchUpService>(); }());

  // ... dentro da classe AppComponent ...
  OATPP_CREATE_COMPONENT(std::shared_ptr<FileService>, fileService)([]
                                                                    { return std::make_shared<FileService>(); }());
};
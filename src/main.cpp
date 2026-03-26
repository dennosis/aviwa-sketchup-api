#pragma once

#include "oatpp/network/Server.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"
#include "oatpp/web/server/HttpConnectionHandler.hpp"
#include "oatpp/core/base/Environment.hpp"
#include "controller/SketchUpController.hpp"
#include "AppComponent.hpp"

#include <iostream>
#include <csignal>

// -------------------------------------------------------
// Signal handler para encerramento gracioso (Ctrl+C)
// -------------------------------------------------------
namespace
{
    oatpp::network::Server *g_server = nullptr;

    void onSignal(int signal)
    {
        OATPP_LOGI("Server", "Sinal %d recebido. Encerrando...", signal);
        if (g_server)
        {
            g_server->stop();
        }
    }
}

// -------------------------------------------------------
// Setup de rotas
// -------------------------------------------------------
void registerControllers(const std::shared_ptr<oatpp::web::server::HttpRouter> &router)
{
    router->addController(SketchUpController::createShared());
    // router->addController(OutroController::createShared());
}

// -------------------------------------------------------
// Lógica principal do servidor
// -------------------------------------------------------
void run()
{
    AppComponent components;

    OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router);
    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, connectionHandler);
    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, connectionProvider);

    registerControllers(router);

    oatpp::network::Server server(connectionProvider, connectionHandler);
    g_server = &server;

    auto host = connectionProvider->getProperty("host").toString();
    auto port = connectionProvider->getProperty("port").toString();

    OATPP_LOGI("Server", "-----------------------------------");
    OATPP_LOGI("Server", "Servidor iniciado em http://%s:%s", host->c_str(), port->c_str());
    OATPP_LOGI("Server", "Pressione Ctrl+C para encerrar.");
    OATPP_LOGI("Server", "-----------------------------------");

    server.run();

    OATPP_LOGI("Server", "Servidor encerrado.");
}

// -------------------------------------------------------
// Entry point
// -------------------------------------------------------
int main()
{
    std::signal(SIGINT, onSignal);  // Ctrl+C
    std::signal(SIGTERM, onSignal); // kill

    oatpp::base::Environment::init();

    try
    {
        run();
    }
    catch (const std::exception &e)
    {
        OATPP_LOGE("Server", "Erro fatal: %s", e.what());
        oatpp::base::Environment::destroy();
        return 1;
    }

    oatpp::base::Environment::destroy();
    return 0;
}
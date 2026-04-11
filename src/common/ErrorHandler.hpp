// src/common/ErrorHandler.hpp
#pragma once
#include "oatpp/web/server/handler/ErrorHandler.hpp"
#include "oatpp/web/protocol/http/outgoing/ResponseFactory.hpp"
#include "oatpp/parser/json/mapping/ObjectMapper.hpp"

class ErrorHandler : public oatpp::web::server::handler::ErrorHandler
{
private:
    std::shared_ptr<oatpp::data::mapping::ObjectMapper> m_objectMapper;

public:
    ErrorHandler(const std::shared_ptr<oatpp::data::mapping::ObjectMapper> &mapper)
        : m_objectMapper(mapper) {}

    std::shared_ptr<oatpp::web::protocol::http::outgoing::Response>
    handleError(const oatpp::web::protocol::http::Status &status,
                const oatpp::String &message,
                const Headers &headers) override
    {
        auto dto = oatpp::UnorderedFields<oatpp::Any>::createShared();
        dto["status"] = (oatpp::Int32)status.code;
        dto["error"] = oatpp::String(message);

        return oatpp::web::protocol::http::outgoing::ResponseFactory::createResponse(
            status, dto, m_objectMapper);
    }
};
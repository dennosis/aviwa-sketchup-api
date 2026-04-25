#pragma once
#include <string>
#include "common/oatpp_aliases.hpp"
#include "common/messages.hpp"
#include "utils/TempFileManager.hpp"

template <typename Fn>
auto withTempFileGuard(const oatpp::String &fileId, Fn &&fn)
{
    auto entry = TempFileManager::instance().get(fileId->c_str());
    OATPP_ASSERT_HTTP(entry.has_value(), Status::CODE_404,
                      Messages::TEMP_FILE_NOT_FOUND);

    try
    {
        auto result = fn(entry.value());
        TempFileManager::instance().remove(fileId->c_str());
        return result;
    }
    catch (const oatpp::web::protocol::http::HttpError e)
    {
        TempFileManager::instance().remove(fileId->c_str());
        throw;
    }
    catch (const std::exception &e)
    {
        TempFileManager::instance().remove(fileId->c_str());
        OATPP_ASSERT_HTTP(false, Status::CODE_500, e.what());
        throw;
    }
}

#define REQUIRE_FIELD(field, name) \
    OATPP_ASSERT_HTTP(field, Status::CODE_400, "campo '" name "' ausente")
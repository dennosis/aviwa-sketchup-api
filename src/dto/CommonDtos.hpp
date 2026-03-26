#pragma once

#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/Types.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class MessageDto : public oatpp::DTO
{
  DTO_INIT(MessageDto, DTO)

  DTO_FIELD(Int32, id);
  DTO_FIELD(String, text);
  DTO_FIELD(Boolean, status);
};

#include OATPP_CODEGEN_END(DTO)

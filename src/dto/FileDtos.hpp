#pragma once

#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/Types.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class FileMetadataDto : public oatpp::DTO
{
  DTO_INIT(FileMetadataDto, DTO)
  DTO_FIELD(String, name);
  DTO_FIELD(String, path);
  DTO_FIELD(String, createdAt);
  DTO_FIELD(String, modifiedAt);
};

#include OATPP_CODEGEN_END(DTO)

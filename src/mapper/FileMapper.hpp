#pragma once

#include "oatpp/core/Types.hpp"
#include "dto/FileDtos.hpp"

class FileMapper
{
public:
    static oatpp::Object<FileMetadataDto> toDto(const FileMetadata &f)
    {
        auto dto = FileMetadataDto::createShared();
        dto->name = f.name.c_str();
        dto->path = f.path.c_str();
        dto->createdAt = f.createdAt.c_str();
        dto->modifiedAt = f.modifiedAt.c_str();
        return dto;
    }

    static oatpp::List<oatpp::Object<FileMetadataDto>> toDtoList(const std::vector<FileMetadata> &files)
    {
        auto list = oatpp::List<oatpp::Object<FileMetadataDto>>::createShared();
        for (const auto &f : files)
        {
            list->push_back(toDto(f));
        }
        return list;
    }
};

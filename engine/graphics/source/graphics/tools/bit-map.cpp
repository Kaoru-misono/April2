#include "bit-map.hpp"
#include "rhi/format.hpp"

#include <core/log/logger.hpp>
#include <fstream>
#include <algorithm>
#include <cstring>

#include <stb_image.h>
#include <stb_image_write.h>

namespace april::graphics
{

    static uint32_t getFormatBytesPerPixel(ResourceFormat format)
    {
        // TODO: rhi/format.hpp
        // return getFormatBytesPerBlock(format);
        return 4;
    }

    Bitmap::Bitmap(uint32_t width, uint32_t height, ResourceFormat format)
        : m_width(width)
        , m_height(height)
        , m_format(format)
    {
        m_rowPitch = getFormatRowPitch(format, width);

        m_size = size_t(m_height) * m_rowPitch;
        m_data = std::make_unique<uint8_t[]>(m_size);
    }

    Bitmap::Bitmap(uint32_t width, uint32_t height, ResourceFormat format, uint8_t const* pData)
        : Bitmap(width, height, format)
    {
        if (pData)
        {
            std::memcpy(m_data.get(), pData, m_size);
        }
    }

    auto Bitmap::create(uint32_t width, uint32_t height, ResourceFormat format, uint8_t const* pData) -> UniqueConstPtr
    {
        return UniqueConstPtr(new Bitmap(width, height, format, pData));
    }

    auto Bitmap::createFromFile(std::filesystem::path const& path, bool isTopDown, ImportFlags importFlags) -> UniqueConstPtr
    {
        if (!std::filesystem::exists(path))
        {
            AP_ERROR("File not found: {}", path.string());
            return nullptr;
        }

        int w, h, channels;
        void* pixelData = nullptr;
        ResourceFormat format = ResourceFormat::Unknown;
        bool loadedAsFloat = false;

        std::string pathStr = path.string();

        if (stbi_is_hdr(pathStr.c_str()))
        {
            pixelData = stbi_loadf(pathStr.c_str(), &w, &h, &channels, 4);
            format = ResourceFormat::RGBA32Float;
            loadedAsFloat = true;
            channels = 4;
        }
        else
        {
            pixelData = stbi_load(pathStr.c_str(), &w, &h, &channels, 4);
            format = ResourceFormat::RGBA8Unorm;
            loadedAsFloat = false;
            channels = 4;
        }

        if (!pixelData)
        {
            AP_ERROR("Failed to load image '{}': {}", pathStr, stbi_failure_reason());
            return nullptr;
        }

        if (!isTopDown)
        {
            int rowBytes = w * channels * (loadedAsFloat ? sizeof(float) : sizeof(uint8_t));
            uint8_t* bytes = static_cast<uint8_t*>(pixelData);
            std::vector<uint8_t> tempRow(rowBytes);

            for (int y = 0; y < h / 2; ++y)
            {
                uint8_t* topRow = bytes + y * rowBytes;
                uint8_t* botRow = bytes + (h - 1 - y) * rowBytes;
                std::memcpy(tempRow.data(), topRow, rowBytes);
                std::memcpy(topRow, botRow, rowBytes);
                std::memcpy(botRow, tempRow.data(), rowBytes);
            }
        }

        auto pBitmap = UniqueConstPtr(new Bitmap(static_cast<uint32_t>(w), static_cast<uint32_t>(h), format));

        size_t expectedSize = static_cast<size_t>(w) * h * channels * (loadedAsFloat ? sizeof(float) : sizeof(uint8_t));

        if (pBitmap->getSize() >= expectedSize)
        {
            std::memcpy(pBitmap->getData(), pixelData, expectedSize);
        }
        else
        {
            AP_ERROR("Bitmap size mismatch during import.");
            stbi_image_free(pixelData);
            return nullptr;
        }

        stbi_image_free(pixelData);
        return pBitmap;
    }

    auto Bitmap::saveImage(
        std::filesystem::path const& path,
        uint32_t width,
        uint32_t height,
        FileFormat fileFormat,
        ExportFlags exportFlags,
        ResourceFormat resourceFormat,
        bool isTopDown,
        void* pData
    ) -> void
    {
        if (!pData)
        {
            AP_ERROR("No data provided to saveImage.");
            return;
        }

        std::string pathStr = path.string();
        int channels = 4;

        bool isFloat = getFormatType(resourceFormat) == FormatType::Float;

        void* pSaveData = pData;
        std::vector<uint8_t> flipBuffer;

        if (!isTopDown)
        {
            size_t pixelSize = isFloat ? sizeof(float) * channels : sizeof(uint8_t) * channels;
            size_t rowPitch = width * pixelSize;
            size_t totalSize = rowPitch * height;

            flipBuffer.resize(totalSize);
            uint8_t const* srcBytes = static_cast<uint8_t const*>(pData);
            uint8_t* dstBytes = flipBuffer.data();

            for (uint32_t y = 0; y < height; ++y)
            {
                std::memcpy(dstBytes + (height - 1 - y) * rowPitch, srcBytes + y * rowPitch, rowPitch);
            }
            pSaveData = flipBuffer.data();
        }

        int ret = 0;
        int stride = width * channels * (isFloat ? sizeof(float) : sizeof(uint8_t));

        if (isFloat)
        {
            if (fileFormat == FileFormat::ExrFile)
            {
                AP_WARN("EXR export not supported by STB backend. Saving as HDR instead.");
                if (pathStr.find(".exr") != std::string::npos)
                    pathStr.replace(pathStr.find(".exr"), 4, ".hdr");
            }

            ret = stbi_write_hdr(pathStr.c_str(), width, height, channels, static_cast<float*>(pSaveData));
        }
        else
        {
            switch (fileFormat)
            {
            case FileFormat::PngFile:
                ret = stbi_write_png(pathStr.c_str(), width, height, channels, pSaveData, stride);
                break;
            case FileFormat::BmpFile:
                ret = stbi_write_bmp(pathStr.c_str(), width, height, channels, pSaveData);
                break;
            case FileFormat::TgaFile:
                ret = stbi_write_tga(pathStr.c_str(), width, height, channels, pSaveData);
                break;
            case FileFormat::JpegFile:
                ret = stbi_write_jpg(pathStr.c_str(), width, height, channels, pSaveData, 90);
                break;
            default:
                AP_WARN("Unsupported format for LDR save. Defaulting to PNG.");
                ret = stbi_write_png(pathStr.c_str(), width, height, channels, pSaveData, stride);
                break;
            }
        }

        if (ret == 0)
        {
            AP_ERROR("Failed to save image to {}", pathStr);
        }
    }

    auto Bitmap::saveImageDialog(Texture* pTexture) -> void
    {
        AP_ERROR("saveImageDialog is not implemented in this version.");
    }

    auto Bitmap::getFileDialogFilters(ResourceFormat format) -> FileDialogFilterVec
    {
        FileDialogFilterVec filters;
        bool showHdr = true;
        bool showLdr = true;

        if (format != ResourceFormat::Unknown)
        {
            showHdr = getFormatType(format) == FormatType::Float;
            showLdr = !showHdr;
        }

        if (showHdr)
        {
            filters.push_back({"Radiance HDR", "hdr"});
            filters.push_back({"Portable Float Map", "pfm"});
        }

        if (showLdr)
        {
            filters.push_back({"Portable Network Graphics", "png"});
            filters.push_back({"JPEG", "jpg"});
            filters.push_back({"Bitmap", "bmp"});
            filters.push_back({"Targa", "tga"});
        }

        return filters;
    }

    auto Bitmap::getFileExtFromResourceFormat(ResourceFormat format) -> std::string
    {
        auto filters = getFileDialogFilters(format);
        if (filters.empty()) return "png";
        return filters.front().ext;
    }

    auto Bitmap::getFormatFromFileExtension(std::string const& ext) -> FileFormat
    {
        std::string extLower = ext;
        std::transform(extLower.begin(), extLower.end(), extLower.begin(), ::tolower);

        if (extLower == "png") return FileFormat::PngFile;
        if (extLower == "jpg" || extLower == "jpeg") return FileFormat::JpegFile;
        if (extLower == "tga") return FileFormat::TgaFile;
        if (extLower == "bmp") return FileFormat::BmpFile;
        if (extLower == "pfm") return FileFormat::PfmFile;
        if (extLower == "exr") return FileFormat::ExrFile;
        if (extLower == "dds") return FileFormat::DdsFile;

        return FileFormat::PngFile; // Default
    }

} // namespace april::graphics

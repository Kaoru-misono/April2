// This file is part of the April Engine.
// Based on the architecture of NVIDIA Falcor (https://github.com/NVIDIAGameWorks/Falcor).
// Original Copyright (c) NVIDIA CORPORATION.

#pragma once

#include "rhi/format.hpp"
#include <core/tools/enum.hpp>

#include <memory>
#include <filesystem>
#include <vector>
#include <string>
#include <cstdint>

namespace april::graphics
{
    class Texture;

    /**
     * A class representing a memory bitmap
     */
    class Bitmap
    {
    public:
        enum class ExportFlags : uint32_t
        {
            None = 0u,              //< Default
            ExportAlpha = 1u << 0,  //< Save alpha channel as well
            Lossy = 1u << 1,        //< Try to store in a lossy format
            Uncompressed = 1u << 2, //< Prefer faster load to a more compact file size
            ExrFloat16 = 1u << 3,   //< Use half-float instead of float when writing EXRs
        };

        enum class ImportFlags : uint32_t
        {
            None = 0u,                  ///< Default.
            ConvertToFloat16 = 1u << 0, ///< Convert HDR images to 16-bit float per channel on import.
        };

        enum class FileFormat
        {
            PngFile,  //< PNG file for lossless compressed 8-bits images with optional alpha
            JpegFile, //< JPEG file for lossy compressed 8-bits images without alpha
            TgaFile,  //< TGA file for lossless uncompressed 8-bits images with optional alpha
            BmpFile,  //< BMP file for lossless uncompressed 8-bits images with optional alpha
            PfmFile,  //< PFM file for floating point HDR images with 32-bit float per channel
            ExrFile,  //< EXR file for floating point HDR images with 16/32-bit float per channel
            DdsFile,  //< DDS file for storing GPU resource formats, including block compressed formats
                      //< See ImageIO. TODO: Remove(?) Bitmap IO implementation when ImageIO supports other formats
        };

        using UniquePtr = std::unique_ptr<Bitmap>;
        using UniqueConstPtr = std::unique_ptr<Bitmap const>;

        /**
         * Create from memory.
         * @param[in] width Width in pixels.
         * @param[in] height Height in pixels
         * @param[in] format Resource format.
         * @param[in] pData Pointer to data. Data will be copied internally during creation and does not need to be managed by the caller.
         * @return A new bitmap object.
         */
        static auto create(uint32_t width, uint32_t height, ResourceFormat format, uint8_t const* pData) -> UniqueConstPtr;

        /**
         * Create a new object from file.
         * @param[in] path Path to load from (absolute or relative to working directory).
         * @param[in] isTopDown Control the memory layout of the image. If true, the top-left pixel is the first pixel in the buffer, otherwise
         * the bottom-left pixel is first.
         * @param[in] importFlags Flags to control how the file is imported. See ImportFlags above.
         * @return If loading was successful, a new object. Otherwise, nullptr.
         */
        static auto createFromFile(std::filesystem::path const& path, bool isTopDown, ImportFlags importFlags = ImportFlags::None) -> UniqueConstPtr;

        /**
         * Store a memory buffer to a file.
         * @param[in] path Path to write to.
         * @param[in] width The width of the image.
         * @param[in] height The height of the image.
         * @param[in] fileFormat The destination file format. See FileFormat enum above.
         * @param[in] exportFlags The flags to export the file. See ExportFlags above.
         * @param[in] ResourceFormat the format of the resource data
         * @param[in] isTopDown Control the memory layout of the image. If true, the top-left pixel will be stored first, otherwise the
         * bottom-left pixel will be stored first
         * @param[in] pData Pointer to the buffer containing the image
         */
        static auto saveImage(
            std::filesystem::path const& path,
            uint32_t width,
            uint32_t height,
            FileFormat fileFormat,
            ExportFlags exportFlags,
            ResourceFormat resourceFormat,
            bool isTopDown,
            void* pData
        ) -> void;

        /**
         * Open dialog to save image to a file
         * @param[in] pTexture Texture to save to file
         */
        static auto saveImageDialog(Texture* pTexture) -> void;

        /// Get a pointer to the bitmap's data store
        auto getData() const -> uint8_t* { return m_data.get(); }

        /// Get the width of the bitmap
        auto getWidth() const -> uint32_t { return m_width; }

        /// Get the height of the bitmap
        auto getHeight() const -> uint32_t { return m_height; }

        /// Get the data format
        auto getFormat() const -> ResourceFormat { return m_format; }

        /// Get the row pitch in bytes. For compressed formats this corresponds to one row of blocks, not pixels.
        auto getRowPitch() const -> uint32_t { return m_rowPitch; }

        /// Get the data size in bytes
        auto getSize() const -> size_t { return m_size; }

        // Placeholder for filter vector definition
        struct FileDialogFilter { std::string desc; std::string ext; };
        using FileDialogFilterVec = std::vector<FileDialogFilter>;

        /**
         * Get the file dialog filter vec for images.
         * @param[in] format If set to ResourceFormat::Unknown, will return all the supported image file formats. If set to something else, will
         * only return file types which support this format.
         */
        static auto getFileDialogFilters(ResourceFormat format = ResourceFormat::Unknown) -> FileDialogFilterVec;

        /**
         * Get a file extension from a resource format
         */
        static auto getFileExtFromResourceFormat(ResourceFormat format) -> std::string;

        /**
         * Get the file format flags for the image extension
         * @param[in] ext The image file extension to get the
         */
        static auto getFormatFromFileExtension(std::string const& ext) -> FileFormat;

    protected:
        Bitmap() = default;
        Bitmap(uint32_t width, uint32_t height, ResourceFormat format);
        Bitmap(uint32_t width, uint32_t height, ResourceFormat format, uint8_t const* pData);

        std::unique_ptr<uint8_t[]> m_data;
        uint32_t m_width{0};    ///< Width in pixels.
        uint32_t m_height{0};   ///< Height in pixels.
        uint32_t m_rowPitch{0}; ///< Row pitch in bytes.
        size_t m_size{0};       ///< Total size in bytes.
        ResourceFormat m_format{ResourceFormat::Unknown};
    };

    AP_ENUM_CLASS_OPERATORS(Bitmap::ExportFlags);
    AP_ENUM_CLASS_OPERATORS(Bitmap::ImportFlags);
} // namespace april::graphics

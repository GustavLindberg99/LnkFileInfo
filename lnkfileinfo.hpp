/*
 * LNK file info class
 * By Gustav Lindberg
 * Version 2.0.1
 * https://github.com/GustavLindberg99/LnkFileInfo
 * Information about how LNK files work is from https://github.com/lcorbasson/lnk-parse/blob/master/lnk-parse.pl
 */

#ifndef LNKFILEINFO_HPP
#define LNKFILEINFO_HPP

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#endif

/**
 * The LnkFileInfo class is a class that parses LNK files and shows information about them, such as their target path, their icon, etc. Although LNK files are Windows-specific, this library is cross-platform, which means that it can also be used on non-Windows systems to parse LNK files that have been copied there from Windows.
 */
class LnkFileInfo final {
public:
    /**
     * This enum is used together with the `targetHasAttribute` method to check if the target has a given attribute according to the LNK file. For more information about what each of these attributes mean, see [File attribute - Wikipedia](https://en.wikipedia.org/wiki/File_attribute).
     */
    enum Attribute: uint16_t {
        ReadOnly         = 0x0001,
        Hidden           = 0x0002,
        System           = 0x0004,
        VolumeLabel      = 0x0008,
        Directory        = 0x0010,
        Archive          = 0x0020,
        NtfsEfs          = 0x0040,
        Normal           = 0x0080,
        Temporary        = 0x0100,
        Sparse           = 0x0200,
        ReparsePointData = 0x0400,
        Compressed       = 0x0800,
        Offline          = 0x1000
    };

    /**
     * This enum is used together with the `targetVolumeType` method to indicate which type of volume the target is on.
     */
    enum VolumeType: uint8_t {
        Unknown         = 0,
        NoRootDirectory = 1,
        Removable       = 2,
        HardDrive       = 3,
        NetworkDrive    = 4,
        CdRom           = 5,
        RamDrive        = 6
    };

    /**
     * Base class of any exception that is thrown from this library.
     */
    struct Exception: public std::filesystem::filesystem_error {
        Exception(const std::string& what, const LnkFileInfo* file):
            std::filesystem::filesystem_error::filesystem_error(what, std::filesystem::path(file->_filePath), std::error_code()){}
        Exception(const std::filesystem::filesystem_error& e): std::filesystem::filesystem_error::filesystem_error(e){}
    };

    /**
     * Exception that is thrown if reading a file failed.
     */
    struct IoError: public Exception {
        using Exception::Exception;
    };

    /**
     * Exception that is thrown if an LNK file is invalid.
     */
    struct InvalidLnkFile: public Exception {
        using Exception::Exception;
    };

    /**
     * Constructs a new LnkFileInfo object that gives information about the given LNK file.
     *
     * @param filePath  The path of the LNK file. Can be an absolute or a relative path.
     *
     * @throws LnkFileInfo::IoError if opening the file failed.
     * @throws LnkFileInfo::InvalidLnkFile if the file is not a valid LNK file. Does *not* throw if the LNK file is valid but the target doesn't exist, use `std::filesystem::exists(lnkFileInfo.absoluteTargetPath())` to check for that.
     */
    explicit LnkFileInfo(std::string filePath): _filePath(std::move(filePath)) {
        this->refresh();
        try{
            this->_absoluteFilePath = std::filesystem::absolute(this->filePath()).string();
        }
        catch(const std::filesystem::filesystem_error& e){
            throw IoError(e);
        }
    }

    /**
     * Returns the absolute path of the LNK file itself, including the file name.
     */
    std::string absoluteFilePath() const {
        return this->_absoluteFilePath;
    }

    /**
     * Returns the absolute path of the target file. If the LNK file points to a nonexistent file, returns the absolute path of that nonexistent file.
     */
    std::string absoluteTargetPath() const {
        return this->_targetPath;
    }

    /**
     * Returns the command line arguments of the LNK file, if any, not including the target. For example, if the LNK file points to `cmd.exe /v /c python.exe`, this method will return `/v /c python.exe`.
     */
    std::string commandLineArgs() const {
        return this->_commandLineArgs;
    }

    /**
     * Returns the description of the LNK file. The description is the custom text that appears when hovering over the LNK file in Windows Explorer, and can be edited in Windows Explorer by going to Properties -> Comment. If the LNK file has no custom description, this method returns an empty string.
     */
    std::string description() const {
        return this->_description;
    }

    /**
     * Returns the path of the LNK file itself as specified in the constructor, including the file name. Can be absolute or relative.
     */
    std::string filePath() const {
        return this->_filePath;
    }

    /**
     * Returns `true` if the LNK file has a custom icon (including if the icon was manually set to be the same as its target), and `false` if it doesn't (meaning the icon shown in Windows Explorer is the same as the target's icon). See also `iconPath()` and `iconIndex()`.
     */
    bool hasCustomIcon() const noexcept {
        return !this->_iconPath.empty();
    }

    /**
     * If the LNK file has a custom icon, returns the path to the file containing that icon. Returns an empty string if the LNK file has no custom icon. See also `hasCustomIcon()` and `iconIndex()`.
     */
    std::string iconPath() const {
        return this->_iconPath;
    }

    /**
     * If the LNK file has a custom icon, returns the index of that icon in the icon file. Returns zero if the LNK file has no custom icon. See also `hasCustomIcon()` and `iconPath()`.
     */
    int iconIndex() const noexcept {
        return this->_iconIndex;
    }

    /**
     * Re-reads all the information about the LNK file from the file system.
     *
     * @throws LnkFileInfo::IoError if opening the file failed.
     * @throws LnkFileInfo::InvalidLnkFile if the file is not a valid LNK file.
     */
    void refresh(){
        //Open the file
        std::ifstream file(utf8ToNativeEncoding(this->_filePath), std::ios::binary);
        if(!file.good()){
            throw IoError("Failed to open file", this);
        }
        const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
        file.close();

        //Check the headers
        if(this->readInteger<uint8_t>(bytes, 0) != 0x4C){
            throw InvalidLnkFile("Invalid header", this);
        }
        const uint16_t start = 78 + this->readInteger<uint16_t>(bytes, 76);
        const uint8_t fileinfoHeader = this->readInteger<uint8_t>(bytes, start + 4);
        if(fileinfoHeader != 0x1C && fileinfoHeader != 0x24){
            throw InvalidLnkFile("Invalid fileinfo header", this);
        }

        //Target info
        this->_targetAttributes = this->readInteger<uint16_t>(bytes, 24);
        this->_targetSize = this->readInteger<uint32_t>(bytes, 52);
        this->_targetIsOnNetwork = this->readInteger<uint8_t>(bytes, start + 8) & 0x02;

        //Path and volume info
        size_t pathOffset;
        if(this->_targetIsOnNetwork){
            const uint32_t volumeOffset = start + this->readInteger<uint32_t>(bytes, start + 20);
            this->_targetVolumeType = VolumeType::NetworkDrive;
            this->_targetVolumeSerial = 0;
            const std::string volumeName = this->readNullTerminatedString(bytes, volumeOffset + 20);
            this->_targetVolumeName = volumeName;
            pathOffset = volumeOffset + 21 + volumeName.size();
            const std::string targetDrive = this->readNullTerminatedString(bytes, pathOffset);
            pathOffset += targetDrive.size() + 1;
            this->_targetPath = targetDrive + "\\" + this->readNullTerminatedString(bytes, pathOffset);
        }
        else{
            const uint32_t volumeOffset = start + this->readInteger<uint32_t>(bytes, start + 12);
            this->_targetVolumeType = static_cast<VolumeType>(this->readInteger<uint32_t>(bytes, volumeOffset + 4));
            this->_targetVolumeSerial = this->readInteger<uint32_t>(bytes, volumeOffset + 8);
            this->_targetVolumeName = this->readNullTerminatedString(bytes, volumeOffset + 16);
            pathOffset = start + this->readInteger<uint32_t>(bytes, start + 16);
            this->_targetPath = this->readNullTerminatedString(bytes, pathOffset);
        }
        if(fileinfoHeader == 0x24){
            this->_targetPath = this->readFixedLengthString(bytes, pathOffset + this->_targetPath.size(), this->_targetPath.size() * 2);
        }

        //Additional info
        const uint8_t flags = this->readInteger<uint8_t>(bytes, 20);
        size_t nextLocation = start + this->readInteger<uint32_t>(bytes, start);
        if(flags & Flag::HasDescription){
            const std::pair data = this->readStringWithPrependedLength(bytes, nextLocation);
            this->_description = data.first;
            nextLocation = data.second;
        }
        if(flags & Flag::HasRelativePath){
            const std::pair data = this->readStringWithPrependedLength(bytes, nextLocation);
            this->_relativeTargetPath = data.first;
            nextLocation = data.second;
        }
        if(flags & Flag::HasWorkingDirectory){
            const std::pair data = this->readStringWithPrependedLength(bytes, nextLocation);
            this->_workingDirectory = data.first;
            nextLocation = data.second;
        }
        if(flags & Flag::HasCommandLineArgs){
            const std::pair data = this->readStringWithPrependedLength(bytes, nextLocation);
            this->_commandLineArgs = data.first;
            nextLocation = data.second;
        }
        if(flags & Flag::HasCustomIcon){
            this->_iconPath = this->readStringWithPrependedLength(bytes, nextLocation).first;
            this->_iconIndex = this->readInteger<uint32_t>(bytes, 56);
        }
    }

    /**
     * Returns the the path of the target relative to the LNK file, as specified in the LNK file. This can be useful for example if the LNK file and the target are both on a removeable drive for which the drive letter has changed, or if a common parent folder to the target and the LNK file has been moved or renamed. If this information is not present in the LNK file, returns an empty string.
     *
     * This method only reads the information present in the LNK file, so the information might not be up to date.
     */
    std::string relativeTargetPath() const {
        return this->_relativeTargetPath;
    }

    /**
     * Returns true if the target has the attribute `attribute`, and false otherwise.
     *
     * @param attribute The attribute to check for, as an instance of the LnkFileInfo::Attribute enum.
     */
    bool targetHasAttribute(Attribute attribute) const noexcept {
        return this->_targetAttributes & attribute;
    }

    /**
     * Returns `true` if the target is on a network drive, and `false` otherwise.
     */
    bool targetIsOnNetwork() const noexcept {
        return this->_targetIsOnNetwork;
    }

    /**
     * Returns the size of the target file in bytes. This method only reads the information present in the LNK file, so the information might not be up to date. For up to date information, you can use `std::filesystem::file_size`.
     */
    unsigned int targetSize() const noexcept {
        return this->_targetSize;
    }

    /**
     * Returns the serial number of the volume that the target is on. Returns zero if target is on a network drive.
     */
    int targetVolumeSerial() const noexcept {
        return this->_targetVolumeSerial;
    }

    /**
     * Returns the type of volume the target is on as a `LnkFileInfo::VolumeType`.
     */
    VolumeType targetVolumeType() const noexcept {
        return this->_targetVolumeType;
    }

    /**
     * Returns the name of the drive the target is on as shown in the This PC folder if that drive has a custom name, and an empty string otherwise. Note that on most Windows computers, while the hard drive is called "Local Disk" by default, this is not a custom name so an empty string will be returnd in that case.
     */
    std::string targetVolumeName() const {
        return this->_targetVolumeName;
    }

    /**
     * Returns the working directory specified in the LNK file. This can be edited in Windows Explorer by going to Properties -> Start in.
     */
    std::string workingDirectory() const {
        return this->_workingDirectory;
    }

    /**
     * Returns `true` if this LnkFileInfo object refers to the same LNK file as `other`, and `false` otherwise.
     */
    bool operator==(const LnkFileInfo &other) const noexcept {
        return this->_absoluteFilePath == other._absoluteFilePath;
    }

    /**
     * Returns `false` if this LnkFileInfo object refers to the same LNK file as `other`, and `true` otherwise.
     */
    bool operator!=(const LnkFileInfo &other) const noexcept {
        return !(*this == other);
    }

private:
    enum Flag{
        HasShellIdList      = 0x01,
        PointsToFileDir     = 0x02,
        HasDescription      = 0x04,
        HasRelativePath     = 0x08,
        HasWorkingDirectory = 0x10,
        HasCommandLineArgs  = 0x20,
        HasCustomIcon       = 0x40
    };

    /**
     * Reads an integer of a given length from the LNK file.
     *
     * @param bytes The bytes contained in the LNK file.
     * @param i     The offset to start reading at at.
     *
     * @tparam T    The integer type to read.
     *
     * @return The integer.
     */
    template<typename T>
    T readInteger(const std::vector<uint8_t> &bytes, size_t i) const {
        if(i + sizeof(T) > bytes.size()){
            throw InvalidLnkFile("Index out of range", this);
        }
        T result = 0;
        for(size_t j = 0; j < sizeof(T); j++){
            result += bytes.at(i + j) * (T(1) << (j * 8));
        }
        return result;
    }

    /**
     * Reads a null-terminated Latin1-encoded string from the LNK file.
     *
     * @param bytes The bytes contained in the LNK file.
     * @param i     The offset to start reading at at.
     *
     * @return The string encoded as UTF-8.
     */
    std::string readNullTerminatedString(const std::vector<uint8_t> &bytes, size_t i) const {
        std::string result;
        while(uint8_t currentCharacter = this->readInteger<uint8_t>(bytes, i++)){
            //If it's an ASCII character, Latin1 and UTF-8 are the same.
            if(currentCharacter < 0x80){
                result += currentCharacter;
            }
            //If it's not an ASCII character, convert it to UTF-8.
            else{
                result += 0xc0 | currentCharacter >> 6;
                result += 0x80 | (currentCharacter & 0x3f);
            }
        }
        return result;
    }

    /**
     * Reads a UTF-16 codepoint from the LNK file.
     *
     * Code for UTF16 to UTF8 conversion from https://github.com/Davipb/utf8-utf16-converter
     *
     * @param bytes The bytes contained in the LNK file.
     * @param i     The offset to start reading at at.
     * @param end   The maximum offset to read at.
     *
     * @return A pair containing the codepoint encoded as UTF-8 and the number of bytes read.
     */
    std::pair<std::string, size_t> readUtf16Codepoint(const std::vector<uint8_t> &bytes, size_t i, size_t end) const {
        constexpr uint32_t GENERIC_SURROGATE_MASK = 0xF800;
        constexpr uint32_t GENERIC_SURROGATE_VALUE = 0xD800;
        constexpr uint32_t HIGH_SURROGATE_VALUE = 0xD800;
        constexpr uint32_t LOW_SURROGATE_VALUE = 0xDC00;
        constexpr uint32_t SURROGATE_MASK = 0xFC00;
        constexpr uint32_t SURROGATE_CODEPOINT_MASK = 0x03FF;
        constexpr uint32_t SURROGATE_CODEPOINT_BITS = 10;
        constexpr uint32_t SURROGATE_CODEPOINT_OFFSET = 0x10000;
        constexpr uint32_t INVALID_CODEPOINT = 0xFFFD;

        const uint16_t high = this->readInteger<uint16_t>(bytes, i);
        size_t bytesRead = 2;
        uint32_t codepoint;
        if((high & GENERIC_SURROGATE_MASK) != GENERIC_SURROGATE_VALUE){
            codepoint = high;
        }
        else if((high & SURROGATE_MASK) != HIGH_SURROGATE_VALUE || end < i + 4){
            codepoint = INVALID_CODEPOINT;
        }
        else{
            const uint16_t low = this->readInteger<uint16_t>(bytes, i + 2);
            if((low & SURROGATE_MASK) != LOW_SURROGATE_VALUE){
                codepoint = INVALID_CODEPOINT;
            }
            else{
                codepoint = (((high & SURROGATE_CODEPOINT_MASK) << SURROGATE_CODEPOINT_BITS) | (low & SURROGATE_CODEPOINT_MASK)) + SURROGATE_CODEPOINT_OFFSET;
                bytesRead += 2;
            }
        }

        const int numberOfBytes = codepoint > 0xFFFF ? 4 : codepoint > 0x7FF ? 3 : codepoint > 0x7F ? 2 : 1;
        int continuationMask = 0x3F;
        int continuationValue = 0x80;
        std::string result;
        for(int j = numberOfBytes; j > 0; j--){
            if(j == 1){
                switch(numberOfBytes){
                case 1:
                    continuationMask = 0x7F;
                    continuationValue = 0x00;
                    break;
                case 2:
                    continuationMask = 0x1F;
                    continuationValue = 0xC0;
                    break;
                case 3:
                    continuationMask = 0x0F;
                    continuationValue = 0xE0;
                    break;
                case 4:
                    continuationMask = 0x07;
                    continuationValue = 0xF0;
                    break;
                }
            }
            const char cont = (codepoint & continuationMask) | continuationValue;
            result = cont + result;
            codepoint >>= 6;
        }
        return std::make_pair(result, bytesRead);
    }

    /**
     * Reads a string for which the first two bytes indicate the length, then the rest is the string itself encoded in UTF-16.
     *
     * @param bytes The bytes contained in the LNK file.
     * @param i     The offset to start reading at at (i.e. the offset containing the length of the string).
     *
     * @return Pair containing the string encoded as UTF-8 and the offset after the end of the string.
     */
    std::pair<std::string, size_t> readStringWithPrependedLength(const std::vector<uint8_t> &bytes, size_t i) const {
        const size_t end = i + this->readInteger<uint16_t>(bytes, i) * 2;
        i += 2;
        std::string result;
        while(i <= end){
            const auto [codepoint, bytesRead] = this->readUtf16Codepoint(bytes, i, end);
            result += codepoint;
            i += bytesRead;
        }
        return std::make_pair(result, end + 2);
    }

    /**
     * Reads a UTF-16 encoded string with a fixed length.
     *
     * @param bytes     The bytes contained in the LNK file.
     * @param i         The offset to start reading at.
     * @param length    The number of bytes to read.
     *
     * @return The string encoded as UTF-8.
     */
    std::string readFixedLengthString(const std::vector<uint8_t> &bytes, size_t offset, size_t length) const {
        std::string result;
        size_t i = 0;
        while(i < length){
            const auto [codepoint, bytesRead] = this->readUtf16Codepoint(bytes, i + offset + 2, bytes.size());
            result += codepoint;
            i += bytesRead;
        }
        return result;
    }

    #ifdef _WIN32
        static std::wstring utf8ToNativeEncoding(const std::string& utf8){
            //Since converting to UTF16 is only needed on Windows, it's OK to use the Windows API for this (on other OSes std::ifstream takes UTF8 directly).
            if(utf8.empty()){
                return L"";
            }
            const int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
            std::wstring utf16(sizeNeeded, 0);
            MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), utf16.data(), sizeNeeded);
            return utf16;
        }
    #else
        static std::string utf8ToNativeEncoding(const std::string& utf8){
            return utf8;
        }
    #endif

    std::string _filePath;
    std::string _absoluteFilePath;
    std::string _targetPath;
    std::string _targetVolumeName;
    std::string _description, _relativeTargetPath, _workingDirectory, _commandLineArgs, _iconPath;
    uint32_t _targetSize;
    uint32_t _iconIndex;
    uint32_t _targetVolumeSerial;
    uint16_t _targetAttributes;
    VolumeType _targetVolumeType;
    bool _targetIsOnNetwork;
};

#endif // LNKFILEINFO_HPP

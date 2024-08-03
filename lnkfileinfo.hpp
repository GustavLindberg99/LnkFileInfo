/*
 * LNK file info class
 * By Gustav Lindberg
 * Version 1.0.1
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

/**
 * The LnkFileInfo class is a class that parses LNK files and shows information about them, such as their target path, their icon, etc. Although LNK files are Windows-specific, this library is cross-platform, which means that it can also be used on non-Windows systems to parse LNK files that have been copied there from Windows.
 */
class LnkFileInfo{
public:
    /**
     * This enum is used together with the `targetHasAttribute` method to check if the target has a given attribute according to the LNK file. For more information about what each of these attributes mean, see [File attribute - Wikipedia](https://en.wikipedia.org/wiki/File_attribute).
     */
    enum Attribute{
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
    enum VolumeType{
        Unknown         = 0,
        NoRootDirectory = 1,
        Removable       = 2,
        HardDrive       = 3,
        NetworkDrive    = 4,
        CdRom           = 5,
        RamDrive        = 6
    };

    /**
     * Constructs an empty LnkFileInfo object. This object will be invalid until a new object is assigned to it.
     */
    LnkFileInfo(): _isValid(false), _targetAttributes(0), _targetSize(0), _targetIsOnNetwork(false), _iconIndex(0), _targetVolumeType(VolumeType::Unknown), _targetVolumeSerial(0){}

    /**
     * Constructs a new LnkFileInfo object that gives information about the given LNK file.
     *
     * @param filePath  The path of the LNK file. Can be an absolute or a relative path.
     */
    LnkFileInfo(const std::string &filePath): _filePath(filePath), _isValid(true){
        this->refresh();
    }

    /**
     * Virtual destructor in case the library user wants to create a subclass.
     */
    virtual ~LnkFileInfo() = default;

    /**
     * Returns the absolute path of the LNK file itself, including the file name.
     */
    std::string absoluteFilePath() const{
        if(!this->exists()){
            return "";
        }
        return std::filesystem::absolute(this->filePath()).string();
    }

    /**
     * Returns the absolute path of the target file. If the LNK file does not exist or is invalid, returns an empty string. If the LNK file exists and is valid but points to a nonexistent file, returns the absolute path of that nonexistent file.
     */
    std::string absoluteTargetPath() const{
        return this->_targetPath;
    }

    /**
     * Returns the command line arguments of the LNK file, if any, not including the target. For example, if the LNK file points to `cmd.exe /v /c python.exe`, this method will return `/v /c python.exe`.
     */
    std::string commandLineArgs() const{
        return this->_commandLineArgs;
    }

    /**
     * Returns the description of the LNK file. The description is the custom text that appears when hovering over the LNK file in Windows Explorer, and can be edited in Windows Explorer by going to Properties -> Comment. If the LNK file has no custom description, this method returns an empty string.
     */
    std::string description() const{
        return this->_description;
    }

    /**
     * Returns `true` if the LNK file exists, regardless of whether or not it is a valid LNK file, and returns `false` otherwise. See also `isValid()` and `targetExists()`.
     */
    bool exists() const{
        return std::filesystem::exists(this->filePath());
    }

    /**
     * Returns the path of the LNK file itself as specified in the constructor, including the file name. Can be absolute or relative.
     */
    std::string filePath() const{
        return this->_filePath;
    }

    /**
     * Returns `true` if the LNK file has a custom icon (including if the icon was manually set to be the same as its target), and `false` if it doesn't (meaning the icon shown in Windows Explorer is the same as the target's icon). See also `iconPath()` and `iconIndex()`.
     */
    bool hasCustomIcon() const{
        return !this->iconPath().empty();
    }

    /**
     * If the LNK file has a custom icon, returns the path to the file containing that icon. Returns an empty string if the LNK file has no custom icon. See also `hasCustomIcon()` and `iconIndex()`.
     */
    std::string iconPath() const{
        return this->_iconPath;
    }

    /**
     * If the LNK file has a custom icon, returns the index of that icon in the icon file. Returns zero if the LNK file has no custom icon. See also `hasCustomIcon()` and `iconPath()`.
     */
    int iconIndex() const{
        return this->_iconIndex;
    }

    /**
     * Returns `true` if the LNK file exists and is a valid LNK file (regardless of whether or not the target exists), and `false` otherwise. See also `exists()` and `targetExists()`.
     */
    bool isValid() const{
        return this->_isValid;
    }

    /**
     * Re-reads all the information about the LNK file from the file system.
     */
    void refresh(){
        try{
            //Open the file
            std::ifstream file(this->_filePath, std::ios::binary);
            if(!file.good()){
                throw std::runtime_error("Failed to open file");
            }
            const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
            file.close();

            //Check the headers
            if(readInteger<uint8_t>(bytes, 0) != 0x4C){
                throw std::runtime_error("Invalid header");
            }
            const uint16_t start = 78 + readInteger<uint16_t>(bytes, 76);
            if(readInteger<uint8_t>(bytes, start + 4) != 0x1C){
                throw std::runtime_error("Invalid fileinfo header");
            }

            //Target info
            this->_targetAttributes = readInteger<uint16_t>(bytes, 24);
            this->_targetSize = readInteger<uint32_t>(bytes, 52);
            this->_targetIsOnNetwork = readInteger<uint8_t>(bytes, start + 8) & 0x02;

            //Path and volume info
            if(this->_targetIsOnNetwork){
                const uint32_t volumeOffset = start + readInteger<uint32_t>(bytes, start + 20);
                this->_targetVolumeType = VolumeType::NetworkDrive;
                this->_targetVolumeSerial = 0;
                const std::string volumeName = readNullTerminatedString(bytes, volumeOffset + 20);
                this->_targetVolumeName = volumeName;
                const size_t pathOffset = volumeOffset + 21 + volumeName.size();
                const std::string targetDrive = readNullTerminatedString(bytes, pathOffset);
                this->_targetPath = targetDrive + "\\" + readNullTerminatedString(bytes, pathOffset + 1 + targetDrive.size());
            }
            else{
                const uint32_t volumeOffset = start + readInteger<uint32_t>(bytes, start + 12);
                this->_targetVolumeType = static_cast<VolumeType>(readInteger<uint32_t>(bytes, volumeOffset + 4));
                this->_targetVolumeSerial = readInteger<uint32_t>(bytes, volumeOffset + 8);
                this->_targetVolumeName = readNullTerminatedString(bytes, volumeOffset + 16);
                const uint32_t pathOffset = readInteger<uint32_t>(bytes, start + 16);
                this->_targetPath = readNullTerminatedString(bytes, start + pathOffset);
            }

            //Additional info
            const uint8_t flags = readInteger<uint8_t>(bytes, 20);
            size_t nextLocation = start + readInteger<uint32_t>(bytes, start);
            if(flags & Flag::HasDescription){
                const std::pair data = readFixedLengthString(bytes, nextLocation);
                this->_description = data.first;
                nextLocation = data.second;
            }
            if(flags & Flag::HasRelativePath){
                const std::pair data = readFixedLengthString(bytes, nextLocation);
                this->_relativeTargetPath = data.first;
                nextLocation = data.second;
            }
            if(flags & Flag::HasWorkingDirectory){
                const std::pair data = readFixedLengthString(bytes, nextLocation);
                this->_workingDirectory = data.first;
                nextLocation = data.second;
            }
            if(flags & Flag::HasCommandLineArgs){
                const std::pair data = readFixedLengthString(bytes, nextLocation);
                this->_commandLineArgs = data.first;
                nextLocation = data.second;
            }
            if(flags & Flag::HasCustomIcon){
                this->_iconPath = readFixedLengthString(bytes, nextLocation).first;
                this->_iconIndex = readInteger<uint32_t>(bytes, 56);
            }
        }
        catch(const std::runtime_error&){
            //Reset everything to avoid garbage values
            this->_isValid = false;
            this->_targetAttributes = 0;
            this->_targetSize = 0;
            this->_targetIsOnNetwork = false;
            this->_iconIndex = 0;
            this->_targetPath = "";
            this->_targetVolumeType = VolumeType::Unknown;
            this->_targetVolumeSerial = 0;
            this->_targetVolumeName = "";
            this->_description = this->_relativeTargetPath = this->_workingDirectory = this->_commandLineArgs = this->_iconPath = "";
        }
    }

    /**
     * Returns the the path of the target relative to the LNK file, as specified in the LNK file. This can be useful for example if the LNK file and the target are both on a removeable drive for which the drive letter has changed, or if a common parent folder to the target and the LNK file has been moved or renamed. If this information is not present in the LNK file, returns an empty string.
     *
     * This method only reads the information present in the LNK file, so the information might not be up to date.
     */
    std::string relativeTargetPath() const{
        return this->_relativeTargetPath;
    }

    /**
     * Returns `true` if the target exists, and `false` otherwise. See also `exists()` and `isValid()`.
     */
    bool targetExists() const{
        return std::filesystem::exists(this->absoluteTargetPath());
    }

    /**
     * Returns true if the target has the attribute `attribute`, and false otherwise.
     *
     * @param attribute The attribute to check for, as an instance of the LnkFileInfo::Attribute enum.
     */
    bool targetHasAttribute(Attribute attribute) const{
        return this->_targetAttributes & attribute;
    }

    /**
     * Returns `true` if the target is on a network drive, and `false` otherwise.
     */
    bool targetIsOnNetwork() const{
        return this->_targetIsOnNetwork;
    }

    /**
     * Returns the size of the target file in bytes. This method only reads the information present in the LNK file, so the information might not be up to date. For up to date information, you can use `std::filesystem::file_size`.
     */
    unsigned int targetSize() const{
        return this->_targetSize;
    }

    /**
     * Returns the serial number of the volume that the target is on. Returns zero if target is on a network drive.
     */
    int targetVolumeSerial() const{
        return this->_targetVolumeSerial;
    }

    /**
     * Returns the type of volume the target is on as a `LnkFileInfo::VolumeType`.
     */
    VolumeType targetVolumeType() const{
        return this->_targetVolumeType;
    }

    /**
     * Returns the name of the drive the target is on as shown in the This PC folder if that drive has a custom name, and an empty string otherwise. Note that on most Windows computers, while the hard drive is called "Local Disk" by default, this is not a custom name so an empty string will be returnd in that case.
     */
    std::string targetVolumeName() const{
        return this->_targetVolumeName;
    }

    /**
     * Returns the working directory specified in the LNK file. This can be edited in Windows Explorer by going to Properties -> Start in.
     */
    std::string workingDirectory() const{
        return this->_workingDirectory;
    }

    /**
     * Returns `true` if this LnkFileInfo object refers to the same LNK file as `other`, and `false` otherwise. The behavior is undefined if both LnkFileInfo objects are either empty or refer to LNK files that do not exist.
     */
    bool operator==(const LnkFileInfo &other) const{
        return this->absoluteFilePath() == other.absoluteFilePath();
    }

    /**
     * Returns `false` if this LnkFileInfo object refers to the same LNK file as `other`, and `true` otherwise. The behavior is undefined if both LnkFileInfo objects are either empty or refer to LNK files that do not exist.
     */
    bool operator!=(const LnkFileInfo &other) const{
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

    template<typename T>
    static T readInteger(const std::vector<uint8_t> &bytes, size_t i){
        if(i + sizeof(T) > bytes.size()){
            throw std::runtime_error("Index out of range");
        }
        T result = 0;
        for(size_t j = 0; j < sizeof(T); j++){
            result += bytes.at(i + j) * (T(1) << (j * 8));
        }
        return result;
    }

    static std::string readNullTerminatedString(const std::vector<uint8_t> &bytes, size_t i){
        std::string result;
        while(char currentCharacter = readInteger<uint8_t>(bytes, i++)){
            result += currentCharacter;
        }
        return result;
    }

    static std::pair<std::string, size_t> readFixedLengthString(const std::vector<uint8_t> &bytes, size_t i){    //Reads a string for which the first two bytes indicate the length, then the rest is the string itself encoded in UTF-16
        //Code for UTF16 to UTF8 conversion from https://github.com/Davipb/utf8-utf16-converter
        constexpr uint32_t GENERIC_SURROGATE_MASK = 0xF800;
        constexpr uint32_t GENERIC_SURROGATE_VALUE = 0xD800;
        constexpr uint32_t HIGH_SURROGATE_VALUE = 0xD800;
        constexpr uint32_t LOW_SURROGATE_VALUE = 0xDC00;
        constexpr uint32_t SURROGATE_MASK = 0xFC00;
        constexpr uint32_t SURROGATE_CODEPOINT_MASK = 0x03FF;
        constexpr uint32_t SURROGATE_CODEPOINT_BITS = 10;
        constexpr uint32_t SURROGATE_CODEPOINT_OFFSET = 0x10000;
        constexpr uint32_t INVALID_CODEPOINT = 0xFFFD;

        const size_t end = i + readInteger<uint16_t>(bytes, i) * 2;
        std::string result;
        for(i += 2; i <= end; i += 2){
            const uint16_t high = readInteger<uint16_t>(bytes, i);
            uint32_t codepoint;
            if((high & GENERIC_SURROGATE_MASK) != GENERIC_SURROGATE_VALUE){
                codepoint = high;
            }
            else if((high & SURROGATE_MASK) != HIGH_SURROGATE_VALUE || end < i + 4){
                codepoint = INVALID_CODEPOINT;
            }
            else{
                const uint16_t low = readInteger<uint16_t>(bytes, i + 2);
                if((low & SURROGATE_MASK) != LOW_SURROGATE_VALUE){
                    codepoint = INVALID_CODEPOINT;
                }
                else{
                    codepoint = (((high & SURROGATE_CODEPOINT_MASK) << SURROGATE_CODEPOINT_BITS) | (low & SURROGATE_CODEPOINT_MASK)) + SURROGATE_CODEPOINT_OFFSET;
                    i += 2;
                }
            }

            const int numberOfBytes = codepoint > 0xFFFF ? 4 : codepoint > 0x7FF ? 3 : codepoint > 0x7F ? 2 : 1;
            int continuationMask = 0x3F;
            int continuationValue = 0x80;
            std::string bytes;
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
                bytes = cont + bytes;
                codepoint >>= 6;
            }
            result += bytes;
        }
        return std::make_pair(result, end + 2);
    }

    std::string _filePath;
    bool _isValid;
    uint16_t _targetAttributes;
    uint32_t _targetSize;
    bool _targetIsOnNetwork;
    uint32_t _iconIndex;
    std::string _targetPath;
    VolumeType _targetVolumeType;
    uint32_t _targetVolumeSerial;
    std::string _targetVolumeName;
    std::string _description, _relativeTargetPath, _workingDirectory, _commandLineArgs, _iconPath;
};

#endif // LNKFILEINFO_HPP

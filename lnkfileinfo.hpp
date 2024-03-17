/*
 * LNK file info class
 * By Gustav Lindberg
 * Version 1.0.0
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

class LnkFileInfo{
public:
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

    enum VolumeType{
        Unknown         = 0,
        NoRootDirectory = 1,
        Removable       = 2,
        HardDrive       = 3,
        NetworkDrive    = 4,
        CdRom           = 5,
        RamDrive        = 6
    };

    LnkFileInfo(): _isValid(false), _targetAttributes(0), _targetSize(0), _targetIsOnNetwork(false), _iconIndex(0), _targetVolumeType(VolumeType::Unknown), _targetVolumeSerial(0){}

    LnkFileInfo(const std::string &filePath): _filePath(filePath), _isValid(true){
        this->refresh();
    }

    //Virtual destructor in case the library user wants to create a subclass
    virtual ~LnkFileInfo() = default;

    std::string absoluteFilePath() const{
        return std::filesystem::absolute(this->filePath()).string();
    }

    std::string absoluteTargetPath() const{
        return std::filesystem::absolute(this->_targetPath).string();
    }
    
    std::string commandLineArgs() const{
        return this->_commandLineArgs;
    }
    
    std::string description() const{
        return this->_description;
    }

    bool exists() const{
        return std::filesystem::exists(this->filePath());
    }

    std::string filePath() const{
        return this->_filePath;
    }

    bool hasCustomIcon() const{
        return !this->iconPath().empty();
    }
    
    std::string iconPath() const{
        return this->_iconPath;
    }

    int iconIndex() const{
        return this->_iconIndex;
    }

    bool isValid() const{
        return this->_isValid;
    }

    void refresh(){
        try{
            //Open the file
            std::ifstream file(this->_filePath, std::ios::binary);
            if(!file.good()){
                throw std::exception("Failed to open file");
            }
            const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
            file.close();

            //Check the headers
            if(readInteger<uint8_t>(bytes, 0) != 0x4C){
                throw std::exception("Invalid header");
            }
            const uint16_t start = 78 + readInteger<uint16_t>(bytes, 76);
            if(readInteger<uint8_t>(bytes, start + 4) != 0x1C){
                throw std::exception("Invalid fileinfo header");
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
        catch(const std::exception&){
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

    std::string relativeTargetPath() const{
        return this->_relativeTargetPath;
    }

    bool targetExists() const{
        return std::filesystem::exists(this->absoluteTargetPath());
    }

    bool targetHasAttribute(Attribute attribute) const{
        return this->_targetAttributes & attribute;
    }

    bool targetIsOnNetwork() const{
        return this->_targetIsOnNetwork;
    }

    unsigned int targetSize() const{
        return this->_targetSize;
    }

    int targetVolumeSerial() const{
        return this->_targetVolumeSerial;
    }

    VolumeType targetVolumeType() const{
        return this->_targetVolumeType;
    }

    std::string targetVolumeName() const{
        return this->_targetVolumeName;
    }

    std::string workingDirectory() const{
        return this->_workingDirectory;
    }

    bool operator==(const LnkFileInfo &other) const{
        return this->absoluteFilePath() == other.absoluteFilePath();
    }

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
            throw std::exception("Index out of range");
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

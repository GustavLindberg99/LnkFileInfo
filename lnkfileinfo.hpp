/*
 * LNK file info class for Qt
 * By Gustav Lindberg
 * Version 1.0.0
 * Information about how LNK files work is from https://github.com/lcorbasson/lnk-parse/blob/master/lnk-parse.pl
 */

#include <QFile>
#include <QFileInfo>

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
    LnkFileInfo(const QString &file): LnkFileInfo(QFileInfo(file)){}
    LnkFileInfo(const QFile &file): LnkFileInfo(QFileInfo(file)){}
    LnkFileInfo(const QFileInfo &file): _fileInfo(file), _isValid(true){
        this->refresh();
    }

    QString absolutePath() const{
        return QFileInfo(*this).absoluteFilePath();
    }

    QString absoluteTargetPath() const{
        return this->_absoluteTargetPath;
    }

    QString commandLineArgs() const{
        return this->_commandLineArgs;
    }

    QString description() const{
        return this->_description;
    }

    bool exists() const{
        return QFileInfo(*this).exists();
    }

    bool hasCustomIcon() const{
        return !this->iconPath().isEmpty();
    }

    QString iconPath() const{
        return this->_iconPath;
    }

    int iconIndex() const{
        return this->_iconIndex;
    }

    bool isValid() const{
        return this->_isValid;
    }

    void refresh(){
        QFile file = *this;
        try{
            if(!file.open(QFile::ReadOnly)){
                throw std::exception("Failed to open file");
            }
            QByteArray bytes = file.readAll();

            //Check the headers
            if(readInteger<quint8>(bytes, 0) != 0x4C){
                throw std::exception("Invalid header");
            }
            const quint16 start = 78 + readInteger<quint16>(bytes, 76);
            if(readInteger<quint8>(bytes, start + 4) != 0x1C){
                throw std::exception("Invalid fileinfo header");
            }

            //Target info
            this->_targetAttributes = readInteger<quint16>(bytes, 24);
            this->_targetSize = readInteger<quint32>(bytes, 52);
            this->_targetIsOnNetwork = readInteger<quint8>(bytes, start + 8) & 0x02;

            //Path and volume info
            if(this->_targetIsOnNetwork){
                const quint32 volumeOffset = start + readInteger<quint32>(bytes, start + 20);
                this->_targetVolumeType = VolumeType::NetworkDrive;
                this->_targetVolumeSerial = 0;
                const QByteArray volumeName = readNullTerminatedString(bytes, volumeOffset + 20);
                this->_targetVolumeName = volumeName;
                const quint32 pathOffset = volumeOffset + 21 + volumeName.size();
                const QByteArray targetDrive = readNullTerminatedString(bytes, pathOffset);
                this->_absoluteTargetPath = targetDrive + "\\" + readNullTerminatedString(bytes, pathOffset + 1 + targetDrive.size());
            }
            else{
                const quint32 volumeOffset = start + readInteger<quint32>(bytes, start + 12);
                this->_targetVolumeType = static_cast<VolumeType>(readInteger<quint32>(bytes, volumeOffset + 4));
                this->_targetVolumeSerial = readInteger<quint32>(bytes, volumeOffset + 8);
                this->_targetVolumeName = readNullTerminatedString(bytes, volumeOffset + 16);
                const quint32 pathOffset = readInteger<quint32>(bytes, start + 16);
                this->_absoluteTargetPath = readNullTerminatedString(bytes, start + pathOffset);
            }

            //Additional info
            const quint8 flags = readInteger<quint8>(bytes, 20);
            quint32 nextLocation = start + readInteger<quint32>(bytes, start);
            if(flags & Flag::HasDescription){
                const QPair data = readFixedLengthString(bytes, nextLocation);
                this->_description = data.first;
                nextLocation = data.second;
            }
            if(flags & Flag::HasRelativePath){
                const QPair data = readFixedLengthString(bytes, nextLocation);
                this->_relativeTargetPath = data.first;
                nextLocation = data.second;
            }
            if(flags & Flag::HasWorkingDirectory){
                const QPair data = readFixedLengthString(bytes, nextLocation);
                this->_workingDirectory = data.first;
                nextLocation = data.second;
            }
            if(flags & Flag::HasCommandLineArgs){
                const QPair data = readFixedLengthString(bytes, nextLocation);
                this->_commandLineArgs = data.first;
                nextLocation = data.second;
            }
            if(flags & Flag::HasCustomIcon){
                this->_iconPath = readFixedLengthString(bytes, nextLocation).first;
                this->_iconIndex = readInteger<quint32>(bytes, 56);
            }
        }
        catch(const std::exception&){
            //Reset everything to avoid garbage values
            this->_isValid = false;
            this->_targetAttributes = 0;
            this->_targetSize = 0;
            this->_targetIsOnNetwork = false;
            this->_iconIndex = 0;
            this->_absoluteTargetPath = "";
            this->_targetVolumeType = VolumeType::Unknown;
            this->_targetVolumeSerial = 0;
            this->_targetVolumeName = "";
            this->_description = this->_relativeTargetPath = this->_workingDirectory = this->_commandLineArgs = this->_iconPath = "";
        }
    }

    bool targetExists() const{
        return this->targetFileInfo().exists();
    }

    QFile targetFile() const{
        return QFile(this->_absoluteTargetPath);
    }

    QFileInfo targetFileInfo() const{
        return QFileInfo(this->_absoluteTargetPath);
    }

    bool targetHasAttribute(Attribute attribute) const{
        return this->_targetAttributes & attribute;
    }

    bool targetIsOnNetwork() const{
        return this->_targetIsOnNetwork;
    }

    unsigned int targetSize() const{    //The size of the target in bytes
        return this->_targetSize;
    }

    int targetVolumeSerial() const{
        return this->_targetVolumeSerial;
    }

    VolumeType targetVolumeType() const{
        return this->_targetVolumeType;
    }

    QString targetVolumeName() const{
        return this->_targetVolumeName;
    }

    QString workingDirectory() const{
        return this->_workingDirectory;
    }

    bool operator==(const LnkFileInfo &other) const{
        return QFileInfo(*this) == QFileInfo(other);
    }

    bool operator!=(const LnkFileInfo &other) const{
        return !(*this == other);
    }

    operator QFile() const{
        return QFile(this->absolutePath());
    }

    operator QFileInfo() const{
        return this->_fileInfo;
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
    static T readInteger(const QByteArray &bytes, quint32 i){
        if(i + sizeof(T) > bytes.size()){
            throw std::exception("Index out of range");
        }
        T result = 0;
        for(quint32 j = 0; j < sizeof(T); j++){
            result += quint8(bytes.at(i + j)) * (T(1) << (j * 8));    //It's necessary to convert bytes.at(...) to quint8, othewise it's signed so it will cause overflow if it's negative
        }
        return result;
    }

    static QByteArray readNullTerminatedString(const QByteArray &bytes, quint32 i){
        QByteArray result;
        while(char currentCharacter = readInteger<quint8>(bytes, i++)){
            result += currentCharacter;
        }
        return result;
    }

    static QPair<QString, quint32> readFixedLengthString(const QByteArray &bytes, quint32 i){    //Reads a string for which the first two bytes indicate the length, then the rest is the string itself encoded in UTF-16
        const quint32 end = i + readInteger<quint16>(bytes, i) * 2;
        QString result;
        for(i += 2; i <= end; i += 2){
            const quint16 currentCharacter = readInteger<quint16>(bytes, i);
            result += QString::fromUtf16(&currentCharacter, 1);
        }
        return QPair(result, end + 2);
    }

    QFileInfo _fileInfo;    //This can't be const otherwise the copy assignment operator won't automatically be generated by the compiler
    bool _isValid;
    quint16 _targetAttributes;
    quint32 _targetSize;
    bool _targetIsOnNetwork;
    quint32 _iconIndex;
    QString _absoluteTargetPath;
    VolumeType _targetVolumeType;
    quint32 _targetVolumeSerial;
    QString _targetVolumeName;
    QString _description, _relativeTargetPath, _workingDirectory, _commandLineArgs, _iconPath;
};

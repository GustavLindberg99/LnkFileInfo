#include <gtest/gtest.h>
#include <lnkfileinfo.hpp>

/**
 * Test that trying to open a nonexistent file throws an exception.
 */
TEST(LnkFileInfoTest, NonexistentFile){
    EXPECT_THROW(LnkFileInfo{TEST_LNK_FILES_DIR "/nonexistent.lnk"}, LnkFileInfo::IoError);
}

/**
 * Test that trying to open a non-LNK file throws an exception.
 */
TEST(LnkFileInfoTest, InvalidLnkFile){
    EXPECT_THROW(LnkFileInfo{TEST_LNK_FILES_DIR "/../unittest.cpp"}, LnkFileInfo::InvalidLnkFile);
}

/**
 * Test equality operators, copy constructors and move constructors.
 */
TEST(LnkFileInfoTest, EqualityCopyMove){
    LnkFileInfo lnk1{TEST_LNK_FILES_DIR "/BasicLnkFile.lnk"};
    LnkFileInfo lnk2 = lnk1;
    LnkFileInfo lnk3{TEST_LNK_FILES_DIR "/BasicLnkFile.lnk"};
    LnkFileInfo lnk4{TEST_LNK_FILES_DIR "/UsbLnkFile.lnk"};
    EXPECT_EQ(lnk1, lnk2);
    EXPECT_EQ(lnk1, lnk3);
    EXPECT_NE(lnk1, lnk4);
    EXPECT_EQ(lnk2, lnk3);
    EXPECT_NE(lnk2, lnk4);
    EXPECT_NE(lnk3, lnk4);

    LnkFileInfo lnk5 = std::move(lnk1);
    lnk1 = lnk4;
    EXPECT_NE(lnk1, lnk2);
    EXPECT_NE(lnk1, lnk3);
    EXPECT_EQ(lnk1, lnk4);
    EXPECT_NE(lnk1, lnk5);
    EXPECT_EQ(lnk5, lnk2);
    EXPECT_EQ(lnk5, lnk3);
    EXPECT_NE(lnk5, lnk4);
}

/**
 * Test LNK file pointing to a file.
 */
TEST(LnkFileInfoTest, BasicLnkFile){
    LnkFileInfo lnk{TEST_LNK_FILES_DIR "/BasicLnkFile.lnk"};
    EXPECT_EQ(lnk.absoluteTargetPath(), "C:\\Users\\glind\\Target.txt");
    EXPECT_EQ(lnk.commandLineArgs(), "");
    EXPECT_EQ(lnk.description(), "");
    EXPECT_EQ(lnk.hasCustomIcon(), false);
    EXPECT_EQ(lnk.iconPath(), "");
    EXPECT_EQ(lnk.iconIndex(), 0);
    EXPECT_EQ(lnk.relativeTargetPath(), ".\\Target.txt");
    EXPECT_EQ(lnk.targetIsOnNetwork(), false);
    EXPECT_EQ(lnk.targetSize(), 12);
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::ReadOnly));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Hidden));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::System));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::VolumeLabel));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Directory));
    EXPECT_TRUE(lnk.targetHasAttribute(LnkFileInfo::Archive));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::NtfsEfs));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Normal));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Temporary));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Sparse));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::ReparsePointData));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Compressed));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Offline));
    EXPECT_EQ(lnk.targetVolumeSerial(), 1852545763);
    EXPECT_EQ(lnk.targetVolumeType(), LnkFileInfo::HardDrive);
    EXPECT_EQ(lnk.targetVolumeName(), "Windows-SSD");
    EXPECT_EQ(lnk.workingDirectory(), "C:\\Users\\glind");
}

/**
 * Test basic LNK file pointing to a file on a USB drive.
 */
TEST(LnkFileInfoTest, UsbLnkFile){
    LnkFileInfo lnk{TEST_LNK_FILES_DIR "/UsbLnkFile.lnk"};
    EXPECT_EQ(lnk.absoluteTargetPath(), "D:\\Target.txt");
    EXPECT_EQ(lnk.commandLineArgs(), "");
    EXPECT_EQ(lnk.description(), "");
    EXPECT_EQ(lnk.hasCustomIcon(), false);
    EXPECT_EQ(lnk.iconPath(), "");
    EXPECT_EQ(lnk.iconIndex(), 0);
    EXPECT_EQ(lnk.relativeTargetPath(), ".\\Target.txt");
    EXPECT_EQ(lnk.targetIsOnNetwork(), false);
    EXPECT_EQ(lnk.targetSize(), 12);
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::ReadOnly));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Hidden));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::System));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::VolumeLabel));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Directory));
    EXPECT_TRUE(lnk.targetHasAttribute(LnkFileInfo::Archive));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::NtfsEfs));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Normal));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Temporary));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Sparse));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::ReparsePointData));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Compressed));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Offline));
    EXPECT_EQ(lnk.targetVolumeSerial(), 1157238549);
    EXPECT_EQ(lnk.targetVolumeType(), LnkFileInfo::Removable);
    EXPECT_EQ(lnk.targetVolumeName(), "ASFT GUSTAV");
    EXPECT_EQ(lnk.workingDirectory(), "D:\\");
}

/**
 * Test basic LNK file pointing to a directory and that has a description and a custom icon.
 */
TEST(LnkFileInfoTest, DirectoryLnkFile){
    LnkFileInfo lnk{TEST_LNK_FILES_DIR "/DirectoryLnkFile.lnk"};
    EXPECT_EQ(lnk.absoluteTargetPath(), "C:\\Users\\glind\\Target");
    EXPECT_EQ(lnk.commandLineArgs(), "");
    EXPECT_EQ(lnk.description(), "A description");
    EXPECT_EQ(lnk.hasCustomIcon(), true);
    EXPECT_EQ(lnk.iconPath(), "C:\\WINDOWS\\system32\\imageres.dll");
    EXPECT_EQ(lnk.iconIndex(), 8);
    EXPECT_EQ(lnk.relativeTargetPath(), ".\\Target");
    EXPECT_EQ(lnk.targetIsOnNetwork(), false);
    EXPECT_EQ(lnk.targetSize(), 0);
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::ReadOnly));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Hidden));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::System));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::VolumeLabel));
    EXPECT_TRUE(lnk.targetHasAttribute(LnkFileInfo::Directory));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Archive));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::NtfsEfs));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Normal));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Temporary));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Sparse));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::ReparsePointData));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Compressed));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Offline));
    EXPECT_EQ(lnk.targetVolumeSerial(), 1852545763);
    EXPECT_EQ(lnk.targetVolumeType(), LnkFileInfo::HardDrive);
    EXPECT_EQ(lnk.targetVolumeName(), "Windows-SSD");
    EXPECT_EQ(lnk.workingDirectory(), "");
}

/**
 * Test LNK file pointing to a file whose name contains non-ASCII Latin1 characters, with a description containing non-ASCII Latin1 characters.
 */
TEST(LnkFileInfoTest, Latin1LnkFile){
    LnkFileInfo lnk{TEST_LNK_FILES_DIR "/ÅÄÖLnkFile.lnk"};
    EXPECT_EQ(lnk.absoluteTargetPath(), "C:\\Users\\glind\\Det här är en fil.txt");
    EXPECT_EQ(lnk.commandLineArgs(), "");
    EXPECT_EQ(lnk.description(), "Det här är en kommentar");
    EXPECT_EQ(lnk.hasCustomIcon(), false);
    EXPECT_EQ(lnk.iconPath(), "");
    EXPECT_EQ(lnk.iconIndex(), 0);
    EXPECT_EQ(lnk.relativeTargetPath(), ".\\Det här är en fil.txt");
    EXPECT_EQ(lnk.targetIsOnNetwork(), false);
    EXPECT_EQ(lnk.targetSize(), 6);
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::ReadOnly));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Hidden));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::System));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::VolumeLabel));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Directory));
    EXPECT_TRUE(lnk.targetHasAttribute(LnkFileInfo::Archive));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::NtfsEfs));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Normal));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Temporary));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Sparse));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::ReparsePointData));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Compressed));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Offline));
    EXPECT_EQ(lnk.targetVolumeSerial(), 1852545763);
    EXPECT_EQ(lnk.targetVolumeType(), LnkFileInfo::HardDrive);
    EXPECT_EQ(lnk.targetVolumeName(), "Windows-SSD");
    EXPECT_EQ(lnk.workingDirectory(), "C:\\Users\\glind");
}

/**
 * Test LNK file pointing to a file whose name contains Unicode characters, with a description containing Unicode characters. The target is read-only, hidden and compressed.
 */
TEST(LnkFileInfoTest, Emoji1LnkFile){
    LnkFileInfo lnk{TEST_LNK_FILES_DIR "/EmojiLnkFile.lnk"};
    EXPECT_EQ(lnk.absoluteTargetPath(), "C:\\Users\\glind\\Target😊.txt");
    EXPECT_EQ(lnk.commandLineArgs(), "");
    EXPECT_EQ(lnk.description(), "This is a description 😊.");
    EXPECT_EQ(lnk.hasCustomIcon(), false);
    EXPECT_EQ(lnk.iconPath(), "");
    EXPECT_EQ(lnk.iconIndex(), 0);
    EXPECT_EQ(lnk.relativeTargetPath(), ".\\Target😊.txt");
    EXPECT_EQ(lnk.targetIsOnNetwork(), false);
    EXPECT_EQ(lnk.targetSize(), 16);
    EXPECT_TRUE(lnk.targetHasAttribute(LnkFileInfo::ReadOnly));
    EXPECT_TRUE(lnk.targetHasAttribute(LnkFileInfo::Hidden));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::System));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::VolumeLabel));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Directory));
    EXPECT_TRUE(lnk.targetHasAttribute(LnkFileInfo::Archive));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::NtfsEfs));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Normal));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Temporary));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Sparse));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::ReparsePointData));
    EXPECT_TRUE(lnk.targetHasAttribute(LnkFileInfo::Compressed));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Offline));
    EXPECT_EQ(lnk.targetVolumeSerial(), 1852545763);
    EXPECT_EQ(lnk.targetVolumeType(), LnkFileInfo::HardDrive);
    EXPECT_EQ(lnk.targetVolumeName(), "Windows-SSD");
    EXPECT_EQ(lnk.workingDirectory(), "C:\\Users\\glind");
}

/**
 * Test LNK file pointing to a file whose name contains Unicode characters. Needed in addition to EmojiLnkFile test since the offsets are slightly different depending on whether the target name has an odd or even number of characters.
 */
TEST(LnkFileInfoTest, Emoji1LnkFile2){
    LnkFileInfo lnk{TEST_LNK_FILES_DIR "/😊LnkFile.lnk"};
    EXPECT_EQ(lnk.absoluteTargetPath(), "C:\\Users\\glind\\AppData\\Local\\Temp\\😊😂🤣🤣.txt");
    EXPECT_EQ(lnk.commandLineArgs(), "");
    EXPECT_EQ(lnk.description(), "");
    EXPECT_EQ(lnk.hasCustomIcon(), false);
    EXPECT_EQ(lnk.iconPath(), "");
    EXPECT_EQ(lnk.iconIndex(), 0);
    EXPECT_EQ(lnk.relativeTargetPath(), ".\\😊😂🤣🤣.txt");
    EXPECT_EQ(lnk.targetIsOnNetwork(), false);
    EXPECT_EQ(lnk.targetSize(), 11);
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::ReadOnly));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Hidden));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::System));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::VolumeLabel));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Directory));
    EXPECT_TRUE(lnk.targetHasAttribute(LnkFileInfo::Archive));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::NtfsEfs));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Normal));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Temporary));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Sparse));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::ReparsePointData));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Compressed));
    EXPECT_FALSE(lnk.targetHasAttribute(LnkFileInfo::Offline));
    EXPECT_EQ(lnk.targetVolumeSerial(), 1852545763);
    EXPECT_EQ(lnk.targetVolumeType(), LnkFileInfo::HardDrive);
    EXPECT_EQ(lnk.targetVolumeName(), "Windows-SSD");
    EXPECT_EQ(lnk.workingDirectory(), "C:\\Users\\glind\\AppData\\Local\\Temp");
}

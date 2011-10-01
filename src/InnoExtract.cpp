
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <vector>
#include <bitset>

#include <lzma.h>

#include "loader/SetupLoader.hpp"

#include "setup/CustomMessageEntry.hpp"
#include "setup/DirectoryEntry.hpp"
#include "setup/FileEntry.hpp"
#include "setup/LanguageEntry.hpp"
#include "setup/PermissionEntry.hpp"
#include "setup/SetupComponentEntry.hpp"
#include "setup/SetupHeader.hpp"
#include "setup/SetupTaskEntry.hpp"
#include "setup/SetupTypeEntry.hpp"
#include "setup/Version.hpp"

#include "stream/BlockReader.hpp"

#include "util/LoadingUtils.hpp"
#include "util/Output.hpp"
#include "util/Utils.hpp"

using std::cout;
using std::string;
using std::endl;
using std::setw;
using std::setfill;
using std::hex;
using std::dec;

#pragma pack(push,1)

struct BlockHeader {
	u32 storedSize; // Total bytes written, including the CRCs
	u8 compressed; // True if data is compressed, False if not
};

#pragma pack(pop)

int main(int argc, char * argv[]) {
	
	if(argc <= 1) {
		std::cout << "usage: innoextract <Inno Setup installer>" << endl;
		return 1;
	}
	
	std::ifstream ifs(argv[1], strm::in | strm::binary | strm::ate);
	
	if(!ifs.is_open()) {
		error << "error opening file";
		return 1;
	}
	
	u64 fileSize = ifs.tellg();
	if(!fileSize) {
		error << "cannot read file or empty file";
		return 1;
	}
	
	SetupLoader::Offsets offsets;
	if(!SetupLoader::getOffsets(ifs, offsets)) {
		error << "failed to load setup loader offsets";
		// TODO try offset0 = 0
		return 1;
	}
	
	cout << std::boolalpha;
	
	cout << color::white;
	cout << "loaded offsets:" << endl;
	cout << "- total size: " << offsets.totalSize << endl;
	cout << "- exe: @ " << hex << offsets.exeOffset << dec << "  compressed: " << offsets.exeCompressedSize << "  uncompressed: " << offsets.exeUncompressedSize << endl;
	cout << "- exe checksum: " << hex << setfill('0') << setw(8) << offsets.exeChecksum << dec << " (" << (offsets.exeChecksumMode == ChecksumAdler32 ? "Alder32" : "CRC32") << ')' << endl;
	cout << "- messageOffset: " << hex << offsets.messageOffset << dec << endl;
	cout << "- offset:  0: " << hex << offsets.offset0 << "  1: " << offsets.messageOffset << dec << endl;
	cout << color::reset;
	
	ifs.seekg(offsets.offset0);
	
	InnoVersion version;
	version.load(ifs);
	if(ifs.fail()) {
		error << "error reading setup data version!";
		return 1;
	}
	
	cout << "version: " << color::white << version << color::reset << endl;
	
	std::istream * _is = BlockReader::get(ifs, version);
	if(!_is) {
		error << "error reading block";
		return 1;
	}
	std::istream & is = *_is;
	
	is.exceptions(strm::badbit | strm::failbit);
	
	/*
	std::ofstream ofs("dump.bin", strm::trunc | strm::out | strm::binary);
	
	do {
		char buf[4096];
		size_t in = is.read(buf, ARRAY_SIZE(buf)).gcount();
		cout << in << endl;
		ofs.write(buf, in);
	} while(!is.eof());
	
	if(is.bad()) {
		error << "read error";
	} else if(!is.eof()) {
		warning << "not eof";
	}
	
	return 0;*/
	
	SetupHeader header;
	header.load(is, version);
	if(is.fail()) {
		error << "error reading setup data header!";
		return 1;
	}
	
	cout << endl;
	
	cout << IfNotEmpty("App name", header.appName);
	cout << IfNotEmpty("App ver name", header.appVerName);
	cout << IfNotEmpty("App id", header.appId);
	cout << IfNotEmpty("Copyright", header.appCopyright);
	cout << IfNotEmpty("Publisher", header.appPublisher);
	cout << IfNotEmpty("Publisher URL", header.appPublisherURL);
	cout << IfNotEmpty("Support phone", header.appSupportPhone);
	cout << IfNotEmpty("Support URL", header.appSupportURL);
	cout << IfNotEmpty("Updates URL", header.appUpdatesURL);
	cout << IfNotEmpty("Version", header.appVersion);
	cout << IfNotEmpty("Default dir name", header.defaultDirName);
	cout << IfNotEmpty("Default group name", header.defaultGroupName);
	cout << IfNotEmpty("Uninstall icon name", header.uninstallIconName);
	cout << IfNotEmpty("Base filename", header.baseFilename);
	cout << IfNotEmpty("Uninstall files dir", header.uninstallFilesDir);
	cout << IfNotEmpty("Uninstall display name", header.uninstallDisplayName);
	cout << IfNotEmpty("Uninstall display icon", header.uninstallDisplayIcon);
	cout << IfNotEmpty("App mutex", header.appMutex);
	cout << IfNotEmpty("Default user name", header.defaultUserInfoName);
	cout << IfNotEmpty("Default user org", header.defaultUserInfoOrg);
	cout << IfNotEmpty("Default user serial", header.defaultUserInfoSerial);
	cout << IfNotEmpty("Readme", header.appReadmeFile);
	cout << IfNotEmpty("Contact", header.appContact);
	cout << IfNotEmpty("Comments", header.appComments);
	cout << IfNotEmpty("Modify path", header.appModifyPath);
	cout << IfNotEmpty("Uninstall reg key", header.createUninstallRegKey);
	cout << IfNotEmpty("Uninstallable", header.uninstallable);
	cout << IfNotEmpty("License", header.licenseText);
	cout << IfNotEmpty("Info before text", header.infoBeforeText);
	cout << IfNotEmpty("Info after text", header.infoAfterText);
	cout << IfNotEmpty("Uninstaller signature", header.signedUninstallerSignature);
	cout << IfNotEmpty("Compiled code", header.compiledCodeText);
	
	cout << IfNotZero("Lead bytes", header.leadBytes);
	
	cout << IfNotZero("Language entries", header.numLanguageEntries);
	cout << IfNotZero("Custom message entries", header.numCustomMessageEntries);
	cout << IfNotZero("Permission entries", header.numPermissionEntries);
	cout << IfNotZero("Type entries", header.numTypeEntries);
	cout << IfNotZero("Component entries", header.numComponentEntries);
	cout << IfNotZero("Task entries", header.numTaskEntries);
	cout << IfNotZero("Dir entries", header.numDirectoryEntries);
	cout << IfNotZero("File entries", header.numFileEntries);
	cout << IfNotZero("File location entries", header.numFileLocationEntries);
	cout << IfNotZero("Icon entries", header.numIconEntries);
	cout << IfNotZero("Ini entries", header.numIniEntries);
	cout << IfNotZero("Registry entries", header.numRegistryEntries);
	cout << IfNotZero("Delete entries", header.numInstallDeleteEntries);
	cout << IfNotZero("Uninstall delete entries", header.numUninstallDeleteEntries);
	cout << IfNotZero("Run entries", header.numRunEntries);
	cout << IfNotZero("Uninstall run entries", header.numUninstallRunEntries);
	
	cout << IfNotZero("License size", header.licenseSize);
	cout << IfNotZero("Info before size", header.infoBeforeSize);
	cout << IfNotZero("Info after size", header.infoAfterSize);
	
	cout << IfNot("Min version", header.minVersion, WindowsVersion::none);
	cout << IfNot("Only below version", header.onlyBelowVersion, WindowsVersion::none);
	
	cout << hex;
	cout << IfNotZero("Back color", header.backColor);
	cout << IfNotZero("Back color2", header.backColor2);
	cout << IfNotZero("Wizard image back color", header.wizardImageBackColor);
	cout << IfNotZero("Wizard small image back color", header.wizardSmallImageBackColor);
	cout << dec;
	
	if(header.options & (shPassword|shEncryptionUsed)) {
		cout << "Password type: " << color::cyan << header.passwordType << color::reset << endl;
		// TODO print password
		// TODO print salt
	}
	
	cout << IfNotZero("Extra disk space required", header.extraDiskSpaceRequired);
	cout << IfNotZero("Slices per disk", header.slicesPerDisk);
	
	cout << IfNot("Install mode", header.installMode, SetupHeader::NormalInstallMode);
	cout << "Uninstall log mode: " << color::cyan << header.uninstallLogMode << color::reset << endl;
	cout << "Uninstall style: " << color::cyan << header.uninstallStyle << color::reset << endl;
	cout << "Dir exists warning: " << color::cyan << header.dirExistsWarning << color::reset << endl;
	cout << IfNot("Privileges required", header.privilegesRequired, SetupHeader::NoPrivileges);
	cout << "Show language dialog: " << color::cyan << header.showLanguageDialog << color::reset << endl;
	cout << IfNot("Danguage detection", header.languageDetectionMethod, SetupHeader::NoLanguageDetection);
	cout << "Compression: " << color::cyan << header.compressMethod << color::reset << endl;
	cout << "Architectures allowed: " << color::cyan << header.architecturesAllowed << color::reset << endl;
	cout << "Architectures installed in 64-bit mode: " << color::cyan << header.architecturesInstallIn64BitMode << color::reset << endl;
	
	if(header.options & shSignedUninstaller) {
		cout << IfNotZero("Size before signing uninstaller", header.signedUninstallerOrigSize);
		cout << IfNotZero("Uninstaller header checksum", header.signedUninstallerHdrChecksum);
	}
	
	cout << "Disable dir page: " << color::cyan << header.disableDirPage << color::reset << endl;
	cout << "Disable program group page: " << color::cyan << header.disableProgramGroupPage << color::reset << endl;
	
	cout << IfNotZero("Uninstall display size", header.uninstallDisplaySize);
	
	cout << "Options: " << color::green << header.options << color::reset << endl;
	
	cout << color::reset;
	
	if(header.numLanguageEntries) {
		cout << endl << "Language entries:" << endl;
	}
	std::vector<LanguageEntry> languages;
	languages.resize(header.numLanguageEntries);
	for(size_t i = 0; i < header.numLanguageEntries; i++) {
		
		LanguageEntry & entry = languages[i];
		entry.load(is, version);
		if(is.fail()) {
			error << "error reading language entry #" << i;
		}
		
		cout << " - " << Quoted(entry.name) << ':' << endl;
		cout << IfNotEmpty("  Language name", entry.languageName);
		cout << IfNotEmpty("  Dialog font", entry.dialogFontName);
		cout << IfNotEmpty("  Title font", entry.titleFontName);
		cout << IfNotEmpty("  Welcome font", entry.welcomeFontName);
		cout << IfNotEmpty("  Copyright font", entry.copyrightFontName);
		cout << IfNotEmpty("  Data", entry.data);
		cout << IfNotEmpty("  License", entry.licenseText);
		cout << IfNotEmpty("  Info before text", entry.infoBeforeText);
		cout << IfNotEmpty("  Info after text", entry.infoAfterText);
		
		cout << "  Language id: " << color::cyan << hex << entry.languageId << dec << color::reset << endl;
		
		cout << IfNotZero("  Codepage", entry.codepage);
		cout << IfNotZero("  Dialog font size", entry.dialogFontSize);
		cout << IfNotZero("  Dialog font standard height", entry.dialogFontStandardHeight);
		cout << IfNotZero("  Title font size", entry.titleFontSize);
		cout << IfNotZero("  Welcome font size", entry.welcomeFontSize);
		cout << IfNotZero("  Copyright font size", entry.copyrightFontSize);
		cout << IfNot("  Right to left", entry.rightToLeft, false);
		
	};
	
	if(version < INNO_VERSION(4, 0, 0)) {
		
		cout << endl;
		
		std::string wizardImage;
		is >> BinaryString(wizardImage);
		cout << "Wizard image: " << wizardImage.length() << " bytes" << endl;
		
		if(version > INNO_VERSION(1, 3, 26)) {
			// TODO header.wizardSmallImageBackColor is missing after 5.0.4
			std::string wizardSmallImage;
			is >> BinaryString(wizardSmallImage);
			cout << "Wizard small image: " << wizardSmallImage.length() << " bytes" << endl;
		}
		
		if(header.compressMethod == SetupHeader::BZip2
		   || (header.compressMethod == SetupHeader::LZMA1 && version == INNO_VERSION(4, 1, 5))
		   || (header.compressMethod == SetupHeader::Zlib && version >= INNO_VERSION(4, 2, 6))) {
			
			std::string decompressorDll;
			is >> BinaryString(decompressorDll);
			cout << "Decompressor dll: " << decompressorDll.length() << " bytes" << endl;
		}
		
		if(is.fail()) {
			error << "error reading misc setup data";
		}
		
	}
	
	if(header.numCustomMessageEntries) {
		cout << endl << "Custom message entries:" << endl;
	}
	for(size_t i = 0; i < header.numCustomMessageEntries; i++) {
		
		CustomMessageEntry entry;
		entry.load(is, version);
		if(is.fail()) {
			error << "error reading custom message entry #" << i;
		}
		
		if(entry.language >= 0 ? size_t(entry.language) >= languages.size() : entry.language != -1) {
			warning << "unexpected language index: " << entry.language;
		}
		
		int codepage;
		if(entry.language == -1) {
			codepage = version.codepage();
		} else {
			codepage = languages[entry.language].codepage;
		}
		
		string decoded;
		toUtf8(entry.value, decoded, codepage);
		
		cout << " - " << Quoted(entry.name);
		if(entry.language == -1) {
			cout << " (default) = ";
		} else {
			cout << " (" << color::cyan << languages[entry.language].name << color::reset << ") = ";
		}
		cout << Quoted(decoded) << endl;
		
	};
	
	if(header.numPermissionEntries) {
		cout << endl << "Permission entries:" << endl;
	}
	for(size_t i = 0; i < header.numPermissionEntries; i++) {
		
		PermissionEntry entry;
		entry.load(is, version);
		if(is.fail()) {
			error << "error reading permission entry #" << i;
		}
		
		cout << " - " << entry.permissions.length() << " bytes";
		
	};
	
	if(header.numTypeEntries) {
		cout << endl << "Type entries:" << endl;
	}
	for(size_t i = 0; i < header.numTypeEntries; i++) {
		
		SetupTypeEntry entry;
		entry.load(is, version);
		if(is.fail()) {
			error << "error reading type entry #" << i;
		}
		
		cout << " - " << Quoted(entry.name) << ':' << endl;
		cout << IfNotEmpty("  Description", entry.description);
		cout << IfNotEmpty("  Languages", entry.languages);
		cout << IfNotEmpty("  Check", entry.check);
		
		cout << IfNot("  Min version", entry.minVersion, header.minVersion);
		cout << IfNot("  Only below version", entry.onlyBelowVersion, header.onlyBelowVersion);
		
		cout << IfNotZero("  Options", entry.options);
		cout << IfNot("  Type", entry.type, SetupTypeEntry::User);
		cout << IfNotZero("  Size", entry.size);
		
	};
	
	if(header.numComponentEntries) {
		cout << endl << "Component entries:" << endl;
	}
	for(size_t i = 0; i < header.numComponentEntries; i++) {
		
		SetupComponentEntry entry;
		entry.load(is, version);
		if(is.fail()) {
			error << "error reading component entry #" << i;
		}
		
		cout << " - " << Quoted(entry.name) << ':' << endl;
		cout << IfNotEmpty("  Types", entry.types);
		cout << IfNotEmpty("  Description", entry.description);
		cout << IfNotEmpty("  Languages", entry.languages);
		cout << IfNotEmpty("  Check", entry.check);
		
		cout << IfNotZero("  Extra disk space required", entry.extraDiskSpaceRequired);
		cout << IfNotZero("  Level", entry.level);
		cout << IfNot("  Used", entry.used, true);
		
		cout << IfNot("  Min version", entry.minVersion, header.minVersion);
		cout << IfNot("  Only below version", entry.onlyBelowVersion, header.onlyBelowVersion);
		
		cout << IfNotZero("  Options", entry.options);
		cout << IfNotZero("  Size", entry.size);
		
	};
	
	if(header.numTaskEntries) {
		cout << endl << "Task entries:" << endl;
	}
	for(size_t i = 0; i < header.numTaskEntries; i++) {
		
		SetupTaskEntry entry;
		entry.load(is, version);
		if(is.fail()) {
			error << "error reading task entry #" << i;
		}
		
		cout << " - " << Quoted(entry.name) << ':' << endl;
		cout << IfNotEmpty("  Description", entry.description);
		cout << IfNotEmpty("  Group description", entry.groupDescription);
		cout << IfNotEmpty("  Components", entry.components);
		cout << IfNotEmpty("  Languages", entry.languages);
		cout << IfNotEmpty("  Check", entry.check);
		
		cout << IfNotZero("  Level", entry.level);
		cout << IfNot("  Used", entry.used, true);
		
		cout << IfNot("  Min version", entry.minVersion, header.minVersion);
		cout << IfNot("  Only below version", entry.onlyBelowVersion, header.onlyBelowVersion);
		
		cout << IfNotZero("  Options", entry.options);
		
	};
	
	if(header.numDirectoryEntries) {
		cout << endl << "Directory entries:" << endl;
	}
	for(size_t i = 0; i < header.numDirectoryEntries; i++) {
		
		DirectoryEntry entry;
		entry.load(is, version);
		if(is.fail()) {
			error << "error reading directory entry #" << i;
		}
		
		cout << " - " << Quoted(entry.name) << ':' << endl;
		cout << IfNotEmpty("  Components", entry.components);
		cout << IfNotEmpty("  Tasks", entry.tasks);
		cout << IfNotEmpty("  Languages", entry.languages);
		cout << IfNotEmpty("  Check", entry.check);
		if(!entry.permissions.empty()) {
			cout << "  Permissions: " << entry.permissions.length() << " bytes";
		}
		cout << IfNotEmpty("  After install", entry.afterInstall);
		cout << IfNotEmpty("  Before install", entry.beforeInstall);
		
		cout << IfNotZero("  Attributes", entry.attributes);
		
		cout << IfNot("  Min version", entry.minVersion, header.minVersion);
		cout << IfNot("  Only below version", entry.onlyBelowVersion, header.onlyBelowVersion);
		
		cout << IfNot("  Permission entry", entry.permission, -1);
		
		cout << IfNotZero("  Options", entry.options);
		
	};
	
	if(header.numFileEntries) {
		cout << endl << "File entries:" << endl;
	}
	for(size_t i = 0; i < header.numFileEntries; i++) {
		
		FileEntry entry;
		entry.load(is, version);
		if(is.fail()) {
			error << "error reading file entry #" << i;
		}
		
		if(entry.destination.empty()) {
			cout << " - File #" << i << ':' << endl;
		} else {
			cout << " - " << Quoted(entry.destination) << ':' << endl;
		}
		cout << IfNotEmpty("  Source", entry.source);
		cout << IfNotEmpty("  Install font name", entry.installFontName);
		cout << IfNotEmpty("  Strong assembly name", entry.strongAssemblyName);
		cout << IfNotEmpty("  Components", entry.components);
		cout << IfNotEmpty("  Tasks", entry.tasks);
		cout << IfNotEmpty("  Languages", entry.languages);
		cout << IfNotEmpty("  Check", entry.check);
		cout << IfNotEmpty("  After install", entry.afterInstall);
		cout << IfNotEmpty("  Before install", entry.beforeInstall);
		
		cout << IfNot("  Min version", entry.minVersion, header.minVersion);
		cout << IfNot("  Only below version", entry.onlyBelowVersion, header.onlyBelowVersion);
		
		cout << IfNot("  Location entry", entry.location, -1);
		cout << IfNotZero("  Attributes", entry.attributes);
		cout << IfNotZero("  Size", entry.externalSize);
		
		cout << IfNot("  Permission entry", entry.permission, -1);
		
		cout << IfNotZero("  Options", entry.options);
		
		cout << IfNot("  Type", entry.type, FileEntry::UserFile);
		
	};
	
	return 0;
}
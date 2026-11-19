#include "game/source.h"
#include "game/sha256.h"
#include "kernel/crypto.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <limits>

namespace urban::game {
static_assert(std::endian::native==std::endian::little, "Xbox loader requires little-endian host");
namespace {
constexpr uint64_t kMaxXbe=256u<<20;
[[noreturn]] void Fail(const char* code,const char* message){throw SourceError(code,message);}
bool Fits(uint64_t at,uint64_t bytes,uint64_t end){return at<=end && bytes<=end-at;}
std::vector<uint8_t> ReadXbeFile(const std::filesystem::path& path) {
    if(std::filesystem::is_symlink(path)||!std::filesystem::is_regular_file(path))Fail("invalid_source","Executable must be a regular file");
    uint64_t size=std::filesystem::file_size(path);
    if(size>kMaxXbe)Fail("invalid_xbe","Executable exceeds the bounded reader limit");
    std::ifstream file(path,std::ios::binary);std::vector<uint8_t> out(size);
    if(!file.read(reinterpret_cast<char*>(out.data()),std::streamsize(size)))Fail("truncated_xbe","Executable could not be read completely");
    return out;
}
std::string Upper(std::string s){for(char& c:s)if(c>='a'&&c<='z')c=char(c-'a'+'A');return s;}
}
Metadata InspectXbe(const std::vector<uint8_t>& bytes) {
    auto get=[&](uint64_t at){if(!Fits(at,4,bytes.size()))Fail("truncated_xbe","Executable is truncated");uint32_t value;std::memcpy(&value,bytes.data()+at,4);return value;};
    if(bytes.size()<0x178||bytes.size()>kMaxXbe||std::memcmp(bytes.data(),"XBEH",4))Fail("invalid_xbe","Not an Xbox executable");
    uint64_t base=get(0x104),headers=get(0x108),image=get(0x10C);
    if(headers<0x178||headers>bytes.size()||image<headers||image>kMaxXbe||base<0x10000||base+image>=0x7FFE0000ull)Fail("invalid_xbe","Executable image bounds are invalid");
    auto header=[&](uint64_t va,uint64_t length){if(va<base||!Fits(va-base,length,headers))Fail("invalid_xbe","Header pointer is out of bounds");return va-base;};
    uint32_t count=get(0x11C);if(!count||count>4096)Fail("invalid_xbe","Invalid section count");
    uint64_t table=header(get(0x120),uint64_t(count)*56);
    struct Section {uint64_t va,size,raw,raw_size;};std::vector<Section> sections;
    for(uint32_t n=0;n<count;n++) {
        uint64_t at=table+uint64_t(n)*56,va=get(at+4),size=get(at+8),raw=get(at+12),raw_size=get(at+16);
        if(va<base||!Fits(va-base,size,image)||raw_size>size||!Fits(raw,raw_size,bytes.size()))Fail("invalid_xbe","Section bounds are invalid");
        uint64_t name=header(get(at+20),1);
        if(std::find(bytes.begin()+name,bytes.begin()+std::min(headers,name+256),uint8_t(0))==bytes.begin()+std::min(headers,name+256))Fail("invalid_xbe","Section name is not terminated");
        alignas(8) uint8_t state[128],digest[20];uint32_t length=uint32_t(raw_size);
        crypto::ShaInit(state);crypto::ShaUpdate(state,reinterpret_cast<uint8_t*>(&length),4);crypto::ShaUpdate(state,bytes.data()+raw,raw_size);crypto::ShaFinal(state,digest);
        if(std::memcmp(digest,bytes.data()+at+36,20))Fail("section_integrity","Executable section digest differs");
        sections.push_back({va,size,raw,raw_size});
    }
    auto mapped=[&](uint64_t va,uint64_t length)->uint64_t {
        if(va>=base&&Fits(va-base,length,headers))return va-base;
        for(const auto& s:sections)if(va>=s.va&&Fits(va-s.va,length,s.raw_size))return s.raw+va-s.va;
        Fail("invalid_xbe","Executable address is not backed by file data");
    };
    uint64_t certificate=header(get(0x118),12);uint32_t cert_size=get(certificate);
    if(cert_size<0x1D0)Fail("invalid_xbe","Certificate is incomplete");header(base+certificate,cert_size);
    uint32_t ep=get(0x128)^0xA8FC57ABu,thunk=get(0x158)^0x5B6D40B6u;
    auto inside=[&](uint64_t va){return va>=base&&va<base+image;};
    if(!inside(ep)||!inside(thunk)){ep=get(0x128)^0x94859D4Bu;thunk=get(0x158)^0xEFB1F152u;}
    if(!inside(ep)||!inside(thunk))Fail("invalid_xbe","Executable entry key is unknown");mapped(ep,1);
    bool ended=false;
    for(uint64_t n=0;n<65536;n++){uint32_t ordinal=get(mapped(uint64_t(thunk)+n*4,4));if(!ordinal){ended=true;break;}if(!(ordinal&0x80000000u))Fail("invalid_xbe","Kernel import is not an ordinal");}
    if(!ended)Fail("invalid_xbe","Kernel import table has no terminator");
    uint32_t libraries=get(0x160);if(libraries>4096)Fail("invalid_xbe","Invalid library count");if(libraries)header(get(0x164),uint64_t(libraries)*16);
    if(uint32_t tls=get(0x12C))mapped(tls,24);
    return {Sha256(bytes),get(certificate+8),get(certificate+0xAC),ep,false};
}
Source Source::Open(const std::filesystem::path& path,const Profile& profile) {
    if(profile.xbe_sha256.size()!=64 || !std::all_of(profile.xbe_sha256.begin(),profile.xbe_sha256.end(),[](char c){return(c>='0'&&c<='9')||(c>='a'&&c<='f');})||!profile.title_id||!profile.entry)Fail("invalid_profile","Build profile is invalid");
    if(std::filesystem::is_symlink(path))Fail("invalid_source","Selected source must not be a symbolic link");
    Source out;
    if(std::filesystem::is_directory(path)) {
        std::filesystem::path executable;
        for(const auto& item:std::filesystem::directory_iterator(path))if(Upper(item.path().filename().string())=="DEFAULT.XBE") {
            if(!executable.empty())Fail("invalid_source","Executable names collide ignoring case");executable=item.path();
        }
        if(executable.empty())Fail("missing_xbe","Folder has no default.xbe");
        out.xbe=ReadXbeFile(executable);out.game_directory=path;
    } else {
        if(!std::filesystem::is_regular_file(path))Fail("invalid_source","Selected image must be a regular file");
        auto disc=std::make_unique<kernel::Disc>();std::string error;
        if(!disc->Open(path,error)) {
            std::ifstream file(path,std::ios::binary);std::vector<char> probe(1u<<20);file.read(probe.data(),probe.size());std::string first(probe.data(),size_t(file.gcount()));
            if(first.find("PLAYSTATION")!=std::string::npos||first.find("SYSTEM.CNF")!=std::string::npos)Fail("ps2_image","PS2 image detected; choose the Xbox version");
            if(first.starts_with("PK\x03\x04")||first.starts_with("Rar!")||first.starts_with("7z\xBC\xAF\x27\x1C"))Fail("compressed_image","Extract the archive before selecting the ISO");
            Fail("invalid_disc","Xbox disc structure is invalid or truncated");
        }
        const auto* entry=disc->Find("default.xbe");
        if(!entry||entry->dir)Fail("missing_xbe","Image has no default.xbe");
        if(entry->size>kMaxXbe)Fail("invalid_xbe","Executable exceeds the bounded reader limit");
        if(!disc->Load("default.xbe",out.xbe))Fail("truncated_xbe","Executable could not be read completely");
        out.disc_=std::move(disc);out.is_iso=true;out.game_directory=path.parent_path();
    }
    out.metadata=InspectXbe(out.xbe);
    if(out.metadata.title_id!=profile.title_id)Fail("wrong_title","Image belongs to another game");
    if(out.metadata.entry!=profile.entry)Fail("entry_mismatch","Image entry is incompatible with this compiled runtime");
    out.metadata.exact_revision=out.metadata.sha256==profile.xbe_sha256;
    return out;
}
std::vector<uint8_t> Source::ReadTextArchive(std::string_view name) const {
    if(activated_ || !metadata.exact_revision || !disc_ ||
       (name!="globallanguage.lst" && name!="99-01.ent" && name!="12-01.ent"))Fail("locale_resource","Unsupported caption archive");
    const auto* entry=disc_->Find(name);
    if(!entry || entry->dir || entry->size>(64u<<20))Fail("locale_resource","Caption archive unavailable");
    std::vector<uint8_t> bytes;
    if(!disc_->Load(name,bytes))Fail("locale_resource","Caption archive read failed");return bytes;
}
void Source::ReplaceTextArchive(std::string_view name,std::vector<uint8_t> bytes) {
    if(activated_ || !metadata.exact_revision || !disc_ ||
       (name!="globallanguage.lst" && name!="99-01.ent" && name!="12-01.ent") ||
       !disc_->ReplaceTextArchive(name,std::move(bytes)))Fail("locale_resource","Caption override rejected");
}
void Source::Activate(bool allow_other_revision) {
    if(activated_)Fail("already_activated","Source was already activated");
    if(!metadata.exact_revision&&!allow_other_revision)Fail("revision_confirmation","Another revision requires an explicit experimental choice");
    kernel::MountDisc(std::move(disc_));activated_=true;
}
}

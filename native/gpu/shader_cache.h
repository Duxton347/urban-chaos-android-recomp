#pragma once
// Disposable private cache. Source/compiler identity and payload integrity are checked before reuse.
#include "game/sha256.h"
#include <filesystem>
#include <span>
#include <vector>
#include <string>
#include <array>
#include <mutex>
#include <atomic>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
namespace urban::gpu {
class ShaderCache {
 static constexpr size_t Header=140,MaxPayload=8u<<20,Budget=64u<<20,MaxFiles=1024;
 std::filesystem::path root_;std::string compiler_;std::mutex mutex_;
 static inline std::atomic<unsigned> serial_{0};
 static bool Hex(std::string_view s){return s.size()==64&&s.find_first_not_of("0123456789abcdef")==s.npos;}
 static std::string Digest(std::span<const uint8_t> b){return game::Sha256(b);}
 static bool Transfer(int fd,uint8_t* bytes,size_t size,bool writing){while(size){ssize_t n=writing?::write(fd,bytes,size) : ::read(fd,bytes,size);if(n<0&&errno==EINTR)continue;if(n<=0)return false;bytes+=n;size-=size_t(n);}return true;}
 static bool Spirv(std::span<const uint32_t> words){if(words.size()<5||words[0]!=0x07230203||words[1]>0x00010300||words[3]==0||words[4]!=0)return false;for(size_t at=5;at<words.size();){size_t n=words[at]>>16;if(!n||n>words.size()-at)return false;at+=n;}return true;}
 std::string Key(std::string_view source,std::string_view entry,unsigned stage)const{std::string bytes=compiler_+"\n"+std::to_string(stage)+"\n"+std::string(entry)+"\n"+std::string(source);return Digest({reinterpret_cast<const uint8_t*>(bytes.data()),bytes.size()});}
 public:
 enum class Result { Disabled,Miss,Invalid,Hit };
 bool Open(const std::filesystem::path& root,std::string compiler){std::lock_guard guard(mutex_);root_.clear();compiler_=std::move(compiler);if(root.empty()||!Hex(compiler_))return false;std::error_code ec;std::filesystem::create_directories(root,ec);if(ec||std::filesystem::is_symlink(root,ec))return false;auto dir=root/compiler_;std::filesystem::create_directory(dir,ec);if(ec||std::filesystem::is_symlink(dir,ec)||!std::filesystem::is_directory(dir,ec))return false;root_=dir;return true;}
 Result Load(std::string_view source,std::string_view entry,unsigned stage,std::vector<uint32_t>& words){std::lock_guard guard(mutex_);words.clear();if(root_.empty())return Result::Disabled;auto key=Key(source,entry,stage);int fd=::open((root_/(key+".spv")).c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return errno==ENOENT?Result::Miss:Result::Invalid;struct stat s{};bool ok=!fstat(fd,&s)&&S_ISREG(s.st_mode)&&s.st_size>=ssize_t(Header+20)&&s.st_size<=ssize_t(Header+MaxPayload);std::vector<uint8_t> bytes;if(ok){bytes.resize(size_t(s.st_size));ok=Transfer(fd,bytes.data(),bytes.size(),false);}::close(fd);if(!ok)return Result::Invalid;uint32_t size=0;std::memcpy(&size,bytes.data()+136,4);ok=std::memcmp(bytes.data(),"UCSPV01\0",8)==0&&std::memcmp(bytes.data()+8,key.data(),64)==0&&size==bytes.size()-Header&&size%4==0;if(!ok)return Result::Invalid;auto payload=std::span<const uint8_t>(bytes).subspan(Header);auto digest=Digest(payload);if(std::memcmp(bytes.data()+72,digest.data(),64))return Result::Invalid;words.resize(size/4);std::memcpy(words.data(),payload.data(),size);if(!Spirv(words)){words.clear();return Result::Invalid;}return Result::Hit;}
 bool Store(std::string_view source,std::string_view entry,unsigned stage,std::span<const uint32_t> words){std::lock_guard guard(mutex_);if(root_.empty()||words.size()>MaxPayload/4||!Spirv(words))return false;std::error_code ec;size_t total=0,files=0;for(std::filesystem::directory_iterator i(root_,ec),end;i!=end&&!ec;i.increment(ec)){auto status=i->symlink_status(ec);if(ec)return false;if(!std::filesystem::is_regular_file(status))continue;auto n=i->file_size(ec);if(ec||n>Budget||total>Budget-n||++files>MaxFiles)return false;total+=size_t(n);}if(ec||files>=MaxFiles||Header+words.size_bytes()>Budget-total)return false;auto key=Key(source,entry,stage),digest=Digest({reinterpret_cast<const uint8_t*>(words.data()),words.size_bytes()});std::vector<uint8_t> bytes(Header+words.size_bytes());std::memcpy(bytes.data(),"UCSPV01\0",8);std::memcpy(bytes.data()+8,key.data(),64);std::memcpy(bytes.data()+72,digest.data(),64);uint32_t size=uint32_t(words.size_bytes());std::memcpy(bytes.data()+136,&size,4);std::memcpy(bytes.data()+Header,words.data(),size);auto final=root_/(key+".spv"),tmp=root_/(key+"."+std::to_string(getpid())+"."+std::to_string(serial_++)+".tmp");int fd=::open(tmp.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);if(fd<0)return false;bool ok=Transfer(fd,bytes.data(),bytes.size(),true);if(::close(fd))ok=false;if(ok){std::filesystem::rename(tmp,final,ec);ok=!ec;}if(!ok)std::filesystem::remove(tmp,ec);return ok;}
};
}

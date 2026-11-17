// Metadata and read-only mounting probe. No game code, assets or guest execution are linked.
#include "game/source.h"
#include "game/sha256.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <thread>
int main(int argc,char** argv) {
    try {
        if(argc==3 && std::string(argv[1])=="--hash") {
            std::ifstream f(argv[2],std::ios::binary);if(!f||std::filesystem::file_size(argv[2])>(16u<<20))return 2;
            std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)),{});
            std::puts(urban::game::Sha256(data).c_str());return 0;
        }
        if(argc<5||argc>10)return 2;
        urban::game::Profile profile{argv[2],uint32_t(std::stoul(argv[3],nullptr,16)),uint32_t(std::stoul(argv[4],nullptr,16))};
        auto source=urban::game::Source::Open(argv[1],profile);
        bool allow=false;std::string resource,preserve;
        for(int n=5;n<argc;n++) {
            if(std::string(argv[n])=="--allow-other")allow=true;
            else if(std::string(argv[n])=="--preserve-on-reject"&&n+1<argc)preserve=argv[++n];
            else if(std::string(argv[n])=="--read"&&n+1<argc)resource=argv[++n];else return 2;
        }
        if(!source.metadata.exact_revision&&!allow) {
            std::printf("{\"status\":\"needs_confirmation\",\"sha256\":\"%s\",\"gameplay_validated\":false}\n",source.metadata.sha256.c_str());return 3;
        }
        source.Activate(allow);
        bool mount_preserved=false;
        if(!preserve.empty()) {
            const auto* previous=kernel::MountedDisc();auto other=urban::game::Source::Open(preserve,profile);
            try { other.Activate();return 8; } catch(const urban::game::SourceError& error) {
                if(error.code!="revision_confirmation" || previous!=kernel::MountedDisc())return 9;
                mount_preserved=true;
            }
        }
        std::string resource_hash;
        if(!resource.empty()) {
            const auto* disc=kernel::MountedDisc();const auto* file=disc?disc->Find(resource):nullptr;
            if(!file||file->dir||file->size>(1u<<20))return 4;
            std::vector<uint8_t> a(file->size),b(file->size);
            uint32_t count_a=0,count_b=0;
            std::thread first([&]{count_a=disc->Read(*file,0,a.data(),file->size);});
            std::thread second([&]{count_b=disc->Read(*file,0,b.data(),file->size);});
            first.join();second.join();if(count_a!=file->size||count_b!=file->size||a!=b)return 5;
            uint8_t tail[16];if(disc->Read(*file,file->size,tail,16)!=0)return 6;
            if(file->size && disc->Read(*file,file->size-1,tail,16)!=1)return 7;
            resource_hash=urban::game::Sha256(a);
        }
        std::printf("{\"status\":\"mounted\",\"iso\":%s,\"experimental\":%s,\"title_id\":\"%08X\",\"version\":%u,\"entry\":\"%08X\",\"sha256\":\"%s\",\"resource_sha256\":\"%s\",\"mount_preserved\":%s,\"gameplay_validated\":false}\n",source.is_iso?"true":"false",source.metadata.exact_revision?"false":"true",source.metadata.title_id,source.metadata.version,source.metadata.entry,source.metadata.sha256.c_str(),resource_hash.c_str(),mount_preserved?"true":"false");
        return 0;
    } catch(const urban::game::SourceError& e) {
        std::printf("{\"status\":\"rejected\",\"code\":\"%s\",\"gameplay_validated\":false}\n",e.code.c_str());return 2;
    } catch(const std::exception&) {
        std::puts("{\"status\":\"rejected\",\"code\":\"io_or_arguments\",\"gameplay_validated\":false}");return 2;
    }
}

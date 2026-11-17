// Private research entry: shared folder/ISO recognition and mount, then the real SDK boot.
#include "app/app.h"
#include "core/log.h"
#include "game/source.h"
#include "image_symbols.h"
#include "build_source_profile.h"
#if IMAGE_RESUMABLE_GUEST && !defined(XBE_RESUMABLE_GUEST)
#error Continuation-aware guest requires the matching runtime dispatcher
#endif
#if !IMAGE_RESUMABLE_GUEST && defined(XBE_RESUMABLE_GUEST)
#error Runtime continuation mode differs from generated guest
#endif
#include "kernel/kernel.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>
int main(int argc,char** argv) {
    std::string selected;bool experimental=false,check_only=false;
    for(int n=1;n<argc;n++) {
        std::string argument=argv[n];
        if(argument=="--allow-other-revision")experimental=true;
        else if(argument=="--check-source")check_only=true;
        else if(argument=="--instance" && n+1<argc)setenv("INSTANCE",argv[++n],1);
        else if(argument.starts_with("--")||!selected.empty())return 2;
        else selected=argument;
    }
    if(selected.empty()&&std::getenv("GAME_DIR"))selected=std::getenv("GAME_DIR");
    if(selected.empty()){std::fputs("usage: urban_research <Xbox ISO or folder> [--check-source] [--allow-other-revision]\n",stderr);return 2;}
    setenv("GAME_DIR",selected.c_str(),1);
    logx::Init((app::ExeDir()/"logs").string(),app::kGame.log);
    app::Load();if(app::kGame.init)app::kGame.init();
    try {
        auto source=urban::game::Source::Open(std::filesystem::absolute(selected),
            {URBAN_BUILD_XBE_SHA256,URBAN_BUILD_TITLE_ID,X86EntryPoint});
        if(!source.metadata.exact_revision&&!experimental) {
            LOGE("source: revision differs; use an explicit experimental choice, compatibility is not validated");logx::Flush();return 3;
        }
        LOGI("source: kind=%s revision=%s title=%08X version=%u sha256=%s",source.is_iso?"iso":"folder",
            source.metadata.exact_revision?"expected":"experimental",source.metadata.title_id,source.metadata.version,source.metadata.sha256.c_str());
        source.Activate(experimental);
        if(check_only){LOGI("source: validated and mounted; game boot was not requested");logx::Flush();return 0;}
        if(app::kGame.window)app::kGame.window(nullptr);
        std::string hdd=app::Instance()>1 ? "hdd"+std::to_string(app::Instance()):"hdd";
        if(!kernel::Boot(std::move(source.xbe),IMAGE_XBE,source.game_directory.string(),(app::ExeDir()/hdd).string()))return 1;
        for(;;){std::this_thread::sleep_for(std::chrono::seconds(1));logx::Flush();}
    } catch(const urban::game::SourceError& error) {
        LOGE("source: rejected (%s): %s",error.code.c_str(),error.what());logx::Flush();return 2;
    } catch(const std::exception&) {
        LOGE("source: selection could not be read");logx::Flush();return 2;
    }
}

// Private research entry: shared folder/ISO recognition and mount, then the real SDK boot.
#include "app/app.h"
#include "core/log.h"
#include "game/source.h"
#include "diagnostics/journal.h"
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
    auto& report=urban::diagnostics::ProcessJournal();
    auto stamp=std::chrono::system_clock::now().time_since_epoch().count();
    bool detailed=std::getenv("URBAN_LOG_DETAIL")&&std::string_view(std::getenv("URBAN_LOG_DETAIL"))=="1";
    if(report.Start(app::ExeDir()/"logs"/"reports"/std::to_string(stamp),URBAN_BUILD_ID,URBAN_BUILD_XBE_SHA256,detailed)) {
        report.Emit(urban::diagnostics::Code::SessionBegin);
        logx::SetObserver([](logx::Level level,const char* format,const char* message){
            urban::diagnostics::ProcessJournal().Observe(int(level),format,message);
        });
    }
    logx::Init((app::ExeDir()/"logs").string(),app::kGame.log);
    if(!report.Healthy())LOGW("diagnostics unavailable: UC-LOG-001");
    app::Load();if(app::kGame.init)app::kGame.init();
    try {
        auto source=urban::game::Source::Open(std::filesystem::absolute(selected),
            {URBAN_BUILD_XBE_SHA256,URBAN_BUILD_TITLE_ID,X86EntryPoint});
        if(!source.metadata.exact_revision&&!experimental) {
            report.Emit(urban::diagnostics::Code::RevisionChoice);
            report.Finish();
            LOGE("source: revision differs; use an explicit experimental choice, compatibility is not validated");logx::Flush();return 3;
        }
        LOGI("source: kind=%s revision=%s title=%08X version=%u sha256=%s",source.is_iso?"iso":"folder",
            source.metadata.exact_revision?"expected":"experimental",source.metadata.title_id,source.metadata.version,source.metadata.sha256.c_str());
        source.Activate(experimental);
        if(check_only){LOGI("source: validated and mounted; game boot was not requested");report.Finish();logx::Flush();return 0;}
        if(app::kGame.window)app::kGame.window(nullptr);
        std::string hdd=app::Instance()>1 ? "hdd"+std::to_string(app::Instance()):"hdd";
        report.Emit(urban::diagnostics::Code::BootStart,{{"title_id",source.metadata.title_id},{"entry",source.metadata.entry},{"version",source.metadata.version}});
        if(!kernel::Boot(std::move(source.xbe),IMAGE_XBE,source.game_directory.string(),(app::ExeDir()/hdd).string())) {
            report.Emit(urban::diagnostics::Code::BootFailed);report.Finish();return 1;
        }
        report.Emit(urban::diagnostics::Code::BootAccepted);
        for(;;){std::this_thread::sleep_for(std::chrono::seconds(1));logx::Flush();}
    } catch(const urban::game::SourceError& error) {
        report.Emit(urban::diagnostics::SourceCode(error.code));
        LOGE("source: rejected (%s): %s",error.code.c_str(),error.what());report.Finish();logx::Flush();return 2;
    } catch(const std::exception&) {
        report.Emit(urban::diagnostics::Code::SourceIo);
        LOGE("source: selection could not be read");report.Finish();logx::Flush();return 2;
    }
}

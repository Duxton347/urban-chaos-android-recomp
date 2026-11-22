#pragma once
// Authored diagnostic metadata only. Never pass game bytes, paths or formatted log messages.
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

namespace urban::diagnostics {
enum class Code {
 SessionBegin, SessionEnd, BootStart, BootAccepted, BootFailed, RevisionChoice,
 SourceInvalid, SourceMissing, SourceIntegrity, SourceTitle, SourceEntry, SourcePs2,
 SourceCompressed, SourceIo, SourceOther, RuntimeWarning, RuntimeError,
 MissingResource, MissingFunction, MissingKernel, GuestTrap, FrameStats, AudioStats,
 AudioOutputFailure, AudioOutputState, VisibilityReport, AndroidHost, UnsupportedPageSize, GpuDevice, GpuCountFailure,
 ThreadProgress, IoProgress, IoCompletion, AudioTiming, GpuTiming, InputLatency, LocaleReady, LocaleRejected, LocaleState, CodeCandidates, GpuSynchronization, Trace
};
struct Definition { const char* code; const char* level; const char* reason; bool known; };
inline Definition Describe(Code c) {
 switch(c) {
 case Code::SessionBegin:return {"UC-SESSION-001","info","Diagnostic session started",true};
 case Code::SessionEnd:return {"UC-SESSION-002","info","Host returned normally",true};
 case Code::BootStart:return {"UC-BOOT-001","info","Original game boot requested",true};
 case Code::BootAccepted:return {"UC-BOOT-002","info","Kernel accepted the boot request",true};
 case Code::BootFailed:return {"UC-BOOT-003","error","Kernel refused the boot request; root cause not classified",false};
 case Code::RevisionChoice:return {"UC-SOURCE-001","warn","Different executable revision requires explicit experimental choice",true};
 case Code::SourceInvalid:return {"UC-SOURCE-002","error","Disc or executable structure is invalid or truncated",true};
 case Code::SourceMissing:return {"UC-SOURCE-003","error","Selected source has no default.xbe",true};
 case Code::SourceIntegrity:return {"UC-SOURCE-004","error","Executable section integrity check failed",true};
 case Code::SourceTitle:return {"UC-SOURCE-005","error","Title ID differs from the compiled game",true};
 case Code::SourceEntry:return {"UC-SOURCE-006","error","Executable entry differs from the compiled game",true};
 case Code::SourcePs2:return {"UC-SOURCE-007","error","PS2 image selected; Xbox image required",true};
 case Code::SourceCompressed:return {"UC-SOURCE-008","error","Archive must be extracted before ISO selection",true};
 case Code::SourceIo:return {"UC-SOURCE-009","error","Source could not be read; underlying IO cause not classified",false};
 case Code::SourceOther:return {"UC-SOURCE-010","error","Source was rejected; root cause not classified",false};
 case Code::RuntimeWarning:return {"UC-RUNTIME-001","warn","Runtime emitted a warning; inspect the format template",false};
 case Code::RuntimeError:return {"UC-RUNTIME-002","error","Runtime emitted an error; inspect the format template",false};
 case Code::MissingResource:return {"UC-DISC-001","warn","A requested resource was not found; optional probing is also possible",false};
 case Code::MissingFunction:return {"UC-CPU-001","error","Indirect target has no recompiled function",true};
 case Code::MissingKernel:return {"UC-KERNEL-001","error","Requested kernel export is not implemented",true};
 case Code::GuestTrap:return {"UC-CPU-002","error","Guest trap recorded; root cause requires analysis",false};
 case Code::FrameStats:return {"UC-GPU-001","info","Renderer presentation counters",true};
 case Code::VisibilityReport:return {"UC-GPU-002","info","GPU visibility report completed after readback",true};
 case Code::AndroidHost:return {"UC-ANDROID-001","info","Android host characteristics",true};
 case Code::UnsupportedPageSize:return {"UC-ANDROID-002","error","Current guest memory mapping requires 4096-byte host pages",true};
 case Code::GpuDevice:return {"UC-GPU-003","info","Selected Vulkan device capabilities; phase 0 supported, phase 1 enabled",true};
 case Code::GpuCountFailure:return {"UC-GPU-004","error","Counted draw cannot be implemented; visibility result was not fabricated",true};
 case Code::AudioStats:return {"UC-AUDIO-001","info","Mixer counters; this does not prove audible output",true};
 case Code::AudioOutputFailure:return {"UC-AUDIO-002","error","SDL output operation failed; underlying driver cause requires analysis",false};
 case Code::AudioOutputState:return {"UC-AUDIO-003","info","SDL output state changed; this does not suspend guest execution",true};
 case Code::ThreadProgress:return {"UC-CPU-003","info","Thread progress snapshot; kind 0 idle, 1 guest, 2 kernel ordinal, 3 HLE name hash, 4 Vulkan operation",true};
 case Code::IoProgress:return {"UC-DISC-002","info","Cumulative IO and empty active stream counters; short reads can include normal EOF",true};
 case Code::IoCompletion:return {"UC-DISC-003","info","APC delivery and last completed read metadata; resource ID is a path hash, no name or data",true};
 case Code::AudioTiming:return {"UC-AUDIO-004","info","Cumulative callback timing; slow blocks are measured, not a driver fault diagnosis",true};
 case Code::GpuTiming:return {"UC-GPU-005","info","Cumulative host Vulkan API time; operation 0 wait, 1 acquire, 2 module, 3 draw, 4 assembly, 5 textures, 6 pipeline, 7 submit, 8 readback, 9 visibility polling; nested times overlap",true};
 case Code::InputLatency:return {"UC-INPUT-002","info","Touch press delivery to guest polling; not total display or game response latency",true};
 case Code::LocaleReady:return {"UC-LOCALE-003","info","Verified optional native text archives installed read-only before boot",true};
 case Code::LocaleRejected:return {"UC-LOCALE-002","error","Optional native caption cache rejected; no game boot attempted",true};
 case Code::LocaleState:return {"UC-LOCALE-005","info","Optional archive request and active override count; no text or filesystem paths",true};
 case Code::CodeCandidates:return {"UC-CPU-004","info","Code-range candidates sampled from bounded current-thread stack; not an unwound call stack",true};
 case Code::GpuSynchronization:return {"UC-GPU-006","info","Visibility result polls, actual async submissions and idle blocking flushes skipped",true};
 case Code::Trace:return {"UC-TRACE-001","debug","Runtime format template; argument values intentionally omitted",false};
 }
 return {"UC-RUNTIME-002","error","Unclassified diagnostic event",false};
}
inline Code SourceCode(std::string_view c) {
 if(c=="revision_confirmation")return Code::RevisionChoice;
 if(c=="missing_xbe")return Code::SourceMissing;
 if(c=="section_integrity")return Code::SourceIntegrity;
 if(c=="wrong_title")return Code::SourceTitle;
 if(c=="entry_mismatch")return Code::SourceEntry;
 if(c=="ps2_image")return Code::SourcePs2;
 if(c=="compressed_image")return Code::SourceCompressed;
 if(c=="invalid_disc"||c=="invalid_xbe"||c=="truncated_xbe"||c=="invalid_source")return Code::SourceInvalid;
 return Code::SourceOther;
}
inline bool Token(std::string_view value) {
 if(value.empty()||value.size()>80)return false;
 for(char c:value)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'))return false;
 return true;
}
inline std::string Quoted(std::string_view value) {
 std::string out="\"";
 for(unsigned char c:value.substr(0,256)) {
  if(c=='"'||c=='\\'){out+='\\';out+=char(c);}
  else if(c>=32&&c<127)out+=char(c);
  else out+='?';
 }
 return out+'"';
}
class Journal {
 std::mutex mutex_;
 std::filesystem::path directory_;
 FILE* file_=nullptr;
 size_t bytes_=0,limit_=1024*1024;
 uint64_t sequence_=0;
 std::atomic<bool> healthy_{false};
 bool detailed_=false;
 std::chrono::steady_clock::time_point start_;
 bool Open() {
  auto p=directory_/"runtime.0.jsonl";
  if(std::filesystem::is_symlink(p))return false;
  file_=std::fopen(p.string().c_str(),"wb");bytes_=0;return file_!=nullptr;
 }
 bool Rotate() {
  if(file_){std::fclose(file_);file_=nullptr;}
  std::error_code ec;
  std::filesystem::remove(directory_/"runtime.3.jsonl",ec);if(ec)return false;
  for(int n=2;n>=0;n--){
   auto from=directory_/("runtime."+std::to_string(n)+".jsonl");
   if(std::filesystem::is_symlink(from))return false;
   if(std::filesystem::exists(from)) {
    std::filesystem::rename(from,directory_/("runtime."+std::to_string(n+1)+".jsonl"),ec);
    if(ec)return false;
   }
  }
  return Open();
 }
public:
 // Directory is a fresh private session folder, never the game/saves directory.
 bool Start(const std::filesystem::path& directory,std::string_view build,std::string_view xbe_hash,
            bool detailed=false,size_t limit=1024*1024) noexcept {
  try {
   std::lock_guard lock(mutex_);
   if(file_||!Token(build)||xbe_hash.size()!=64||limit<4096||limit>1024*1024)return false;
   for(char c:xbe_hash)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;
   if(std::filesystem::exists(directory))return false;
   std::filesystem::create_directories(directory);directory_=directory;
   start_=std::chrono::steady_clock::now();limit_=limit;detailed_=detailed;sequence_=0;
   std::string manifest="{\"schema\":1,\"build\":"+Quoted(build)+",\"xbe_sha256\":"+Quoted(xbe_hash)+
       ",\"sdk\":\"c1f7d60c8728c6151bcfbb1d05e397eaf0618dc7\",\"detailed\":"+(detailed?"true":"false")+"}\n";
   FILE* m=std::fopen((directory_/"manifest.json").string().c_str(),"wb");
   if(!m)return false;
   bool ok=std::fwrite(manifest.data(),1,manifest.size(),m)==manifest.size();
   ok=std::fclose(m)==0&&ok;if(!ok)return false;
   m=std::fopen((directory_/"session.open").string().c_str(),"wb");if(!m)return false;
   ok=std::fclose(m)==0;
   healthy_=ok&&Open();return healthy_;
  }catch(...){healthy_=false;return false;}
 }
 bool Healthy() {std::lock_guard lock(mutex_);return healthy_;}
 bool Emit(Code code,std::initializer_list<std::pair<std::string_view,uint64_t>> fields={},
           std::string_view format_template={}) noexcept {
  try {
   std::lock_guard lock(mutex_);if(!healthy_)return false;
   auto now=std::chrono::system_clock::now().time_since_epoch();
   auto elapsed=std::chrono::steady_clock::now()-start_;
   auto d=Describe(code);
   std::string line="{\"schema\":1,\"seq\":"+std::to_string(++sequence_)+
      ",\"utc_ms\":"+std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(now).count())+
      ",\"elapsed_ms\":"+std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count())+
      ",\"thread\":"+std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()))+
      ",\"code\":"+Quoted(d.code)+",\"level\":"+Quoted(d.level)+",\"reason\":"+Quoted(d.reason)+
      ",\"cause_known\":"+(d.known?"true":"false")+",\"detail\":{";
   bool first=true;
   for(auto [key,value]:fields) {
    if(!Token(key)||key.size()>40)return false;
    if(!first)line+=',';first=false;
    line+=Quoted(key)+':'+std::to_string(value);
   }
   line+="},\"template\":"+Quoted(format_template)+"}\n";
   if(line.size()>limit_)return false;
   if(bytes_+line.size()>limit_&&!Rotate()){healthy_=false;return false;}
   healthy_=std::fwrite(line.data(),1,line.size(),file_)==line.size()&&std::fflush(file_)==0;
   bytes_+=line.size();return healthy_;
  }catch(...){healthy_=false;return false;}
 }
 void Finish() noexcept {
  bool recorded=Emit(Code::SessionEnd);
  try {
   std::lock_guard lock(mutex_);
   if(file_){std::fclose(file_);file_=nullptr;}
   if(recorded)std::filesystem::remove(directory_/"session.open");
   healthy_=false;
  }catch(...){}
 }
 // Receives the original SDK format string, not expanded text. Expanded text is only
 // parsed for these fixed numeric patterns and is never copied into the report.
 void Observe(int level,const char* format,const char* expanded) noexcept {
  try {
   if(!format||!expanded)return;
   unsigned frame=0,draws=0,empty=0,target=0,ret=0,sp=0,ordinal=0;
   unsigned operation=0,state=0;
   if(std::sscanf(expanded,"audio-output: failure operation %u",&operation)==1) {
    Emit(Code::AudioOutputFailure,{{"operation",operation}});return;
   }
   if(std::sscanf(expanded,"audio-output: state %u",&state)==1) {
    Emit(Code::AudioOutputState,{{"state",state}});return;
   }
   unsigned long long samples=0,active=0,bad=0;double peak=0;
   unsigned long long vulkan_frame=0;unsigned visible_samples=0;
   unsigned vendor=0,device=0,api=0,driver=0,precise=0,phase=0;
   if(std::string_view(format)=="vulkan-device: vendor %u device %u api %u driver %u precise %u phase %u" &&
      std::sscanf(expanded,"vulkan-device: vendor %u device %u api %u driver %u precise %u phase %u",&vendor,&device,&api,&driver,&precise,&phase)==6) {
    Emit(Code::GpuDevice,{{"vendor",vendor},{"device",device},{"api",api},{"driver",driver},{"precise",precise},{"phase",phase}});return;
   }
   if(std::string_view(format)=="vulkan-count-failure: operation %u" && std::sscanf(expanded,"vulkan-count-failure: operation %u",&operation)==1) {
    Emit(Code::GpuCountFailure,{{"operation",operation}});return;
   }
   if((std::string_view(format)=="renderer: frame %llu (Vulkan headless)" || std::string_view(format)=="renderer: frame %llu (Vulkan Android)") &&
      std::sscanf(expanded,"renderer: frame %llu",&vulkan_frame)==1) {
    Emit(Code::FrameStats,{{"frame",vulkan_frame},{"backend",2},{"headless",std::string_view(format).find("headless")!=std::string_view::npos?1u:0u}});return;
   }
   if(std::string_view(format)=="vulkan: visibility report completed, samples=%u" &&
      std::sscanf(expanded,"vulkan: visibility report completed, samples=%u",&visible_samples)==1) {
    Emit(Code::VisibilityReport,{{"samples",visible_samples}});return;
   }
   if(std::sscanf(expanded,"renderer: frame %u, %u draws, %u empty frames",&frame,&draws,&empty)==3) {
    Emit(Code::FrameStats,{{"frame",frame},{"draws",draws},{"empty",empty}});return;
   }
   if(std::sscanf(expanded,"audio-metrics: frames %llu active-samples %llu peak %lf nonfinite %llu",&samples,&active,&peak,&bad)==4) {
    uint64_t bounded=std::isfinite(peak)&&peak>=0?uint64_t(std::min(peak,1000.0)*1000000):0;
    // Keep standard audio summaries at ten second intervals; detailed mode keeps all.
    if(detailed_||samples%480000<48000)Emit(Code::AudioStats,{{"frames",samples},{"active_samples",active},{"peak_ppm",bounded},{"nonfinite",bad}});
    return;
   }
   if(std::sscanf(expanded,"indirect call to %X has no recompiled function (return address %X, esp %X)",&target,&ret,&sp)==3) {
    Emit(Code::MissingFunction,{{"target",target},{"return_address",ret},{"esp",sp}});return;
   }
   if(std::sscanf(expanded,"kernel export %u",&ordinal)==1&&std::string_view(format).find("not implemented")!=std::string_view::npos) {
    Emit(Code::MissingKernel,{{"ordinal",ordinal}});return;
   }
   if(std::sscanf(expanded,"guest trap at %X:",&target)==1){Emit(Code::GuestTrap,{{"address",target}},format);return;}
   if(std::string_view(format)=="missing %s"){Emit(Code::MissingResource,{},format);return;}
   if(level==2||level==3)Emit(level==3?Code::RuntimeError:Code::RuntimeWarning,{},format);
   else if(detailed_)Emit(Code::Trace,{},format);
  }catch(...){}
 }
 ~Journal(){if(file_)std::fclose(file_);}
};
// Deliberate process lifetime: SDK worker threads may still log while main unwinds.
inline Journal& ProcessJournal(){static auto* journal=new Journal;return *journal;}
}

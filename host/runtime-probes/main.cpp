#include "app/app.h"
#include <cstring>
#include "harness.h"
// Test fixture for the backing memory pointer normally initialized by Boot. No title loader linked.
namespace kernel { uint8_t* g_base=nullptr; }
const app::GameInfo app::kGame = {.title=L"SDK synthetic probes", .name=L"SdkProbes"};
int BackendBusySelfTest();
int VertexModeSelfTest();
int ReferenceVisibilitySelfTest(bool reject_points=true);
int main(int argc,char** argv) {
    if(argc!=2)return 2;
    if(!std::strcmp(argv[1],"busy"))return BackendBusySelfTest();
    if(!std::strcmp(argv[1],"vertex"))return VertexModeSelfTest();
    if(!std::strcmp(argv[1],"visibility"))return ReferenceVisibilitySelfTest();
    return 2;
}

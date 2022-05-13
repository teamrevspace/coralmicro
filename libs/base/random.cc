#include "libs/base/random.h"

#include <cstdio>

#include "third_party/freertos_kernel/include/semphr.h"
#include "third_party/nxp/rt1176-sdk/devices/MIMXRT1176/drivers/cm7/fsl_cache.h"
#include "third_party/nxp/rt1176-sdk/devices/MIMXRT1176/drivers/fsl_caam.h"

namespace coral::micro {

using namespace random;

void Random::RequestHandler(Request* req) {
    Response resp;
    caam_handle_t handle;
    handle.jobRing = kCAAM_JobRing0;
    status_t status =
        CAAM_RNG_GetRandomData(CAAM, &handle, kCAAM_RngStateHandle0, req->out,
                               req->len, kCAAM_RngDataAny, NULL);
    DCACHE_InvalidateByRange(reinterpret_cast<uint32_t>(req->out), req->len);
    resp.success = (status == kStatus_Success);
    req->callback(resp);
}

bool Random::GetRandomNumber(void* out, size_t len) {
    Request req;
    req.out = out;
    req.len = len;
    Response resp = SendRequest(req);
    return resp.success;
}

}  // namespace coral::micro

// spikes/render_shared_d3d11.cpp — P3.1-A2 spike OPT-B (SESI A2, standalone).
// Status: PROBE + P3.6 adapter verification ONLY. Full fan-out was NOT coded:
// B1 (native D3D11 render API) is empirically UNAVAILABLE in the vendored
// libmpv (rc=-19 NOT_IMPLEMENTED, see verdict below). Per A2 spec, B2 (ANGLE)
// deploy analysis is reported in docs/spike-results.md BEFORE any fan-out
// coding. No untested render path is committed here by design.
//
// Run: render_shared_d3d11.exe --probe <video>
//   Exit 0 = d3d11 render backend exists + hwdec d3d11va active.
//   (Actual: exit 1, B1 UNAVAILABLE — see docs/spike-results.md A2 section.)
//
// ABI HONESTY: vendored headers (client API v2.5) declare only "opengl"/"sw".
// DLL reports runtime api v131077 (0x20005 = v2.5). The "d3d11" API string +
// param IDs 1000/1001 + init struct below are reconstructed from upstream
// render_d3d11.h knowledge; the probe distinguishes backend-missing
// (NOT_IMPLEMENTED) from params-missing (INVALID_PARAMETER) empirically.
//
// Build (manual, NOT in CMake — engine/ untouched):
//   vcvars64.bat && cl.exe /EHsc /std:c++17 /W4 spikes\render_shared_d3d11.cpp ^
//     /I vendor\libmpv\include vendor\libmpv\lib\mpv.lib ^
//     d3d11.lib dxgi.lib user32.lib psapi.lib ^
//     /Fe:build\spikes\render_shared_d3d11.exe

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_6.h>

#include <mpv/client.h>
#include <mpv/render.h>

#include <clocale>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

// ---- Reconstructed D3D11 render API (upstream render_d3d11.h) ----
#define MPV_RENDER_API_TYPE_D3D11_RECON "d3d11"
#define MPV_RENDER_PARAM_D3D11_INIT_PARAMS_RECON \
    ((mpv_render_param_type)1000)

struct MpvD3D11InitParamsRecon {
    ID3D11Device* device;                // required
    ID3D11DeviceContext* device_context;  // required (immediate)
};

static void Log(const char* fmt, ...) {
    char buf[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    printf("%s\n", buf);
    fflush(stdout);
}

// ---- P3.6 heuristic verification (A2 req 5) ----
// Primary: IDXGIFactory6::EnumAdapterByGpuPreference(MINIMUM_POWER).
// Fallback table: 0x10DE discrete always; 0x8086 integrated unless "Arc"
// in Description; 0x1002 ambiguous -> dedicated-memory threshold.
struct AdapterPick {
    IDXGIAdapter1* adapter = nullptr;  // caller releases
    std::string desc;
    unsigned vendor = 0;
    unsigned long long dedicated_mb = 0;
    unsigned long long shared_mb = 0;
    std::string via;  // "GpuPreference(MINIMUM_POWER)" | "VendorId-fallback"
};

static std::string NarrowDesc(const DXGI_ADAPTER_DESC1& d) {
    char b[128] = {};
    WideCharToMultiByte(CP_UTF8, 0, d.Description, -1, b, (int)sizeof(b),
                        nullptr, nullptr);
    return std::string(b);
}

static bool PickLowPowerAdapter(AdapterPick* out) {
    IDXGIFactory6* f6 = nullptr;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory6),
                                    reinterpret_cast<void**>(&f6));
    if (SUCCEEDED(hr) && f6) {
        IDXGIAdapter1* ad = nullptr;
        // i = adapter index; 0 = lowest-power GPU per docs.
        hr = f6->EnumAdapterByGpuPreference(
            0, DXGI_GPU_PREFERENCE_MINIMUM_POWER, __uuidof(IDXGIAdapter1),
            reinterpret_cast<void**>(&ad));
        if (SUCCEEDED(hr) && ad) {
            DXGI_ADAPTER_DESC1 d{};
            if (SUCCEEDED(ad->GetDesc1(&d))) {
                out->adapter = ad;
                out->desc = NarrowDesc(d);
                out->vendor = d.VendorId;
                out->dedicated_mb = d.DedicatedVideoMemory / (1024 * 1024);
                out->shared_mb = d.SharedSystemMemory / (1024 * 1024);
                out->via = "GpuPreference(MINIMUM_POWER)";
                f6->Release();
                return true;
            }
            ad->Release();
        }
        // Log HIGH_PERFORMANCE for the record, then fall through.
        IDXGIAdapter1* hi = nullptr;
        if (SUCCEEDED(f6->EnumAdapterByGpuPreference(
                0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                __uuidof(IDXGIAdapter1),
                reinterpret_cast<void**>(&hi))) &&
            hi) {
            DXGI_ADAPTER_DESC1 d{};
            if (SUCCEEDED(hi->GetDesc1(&d)))
                Log("[gpu] HIGH_PERFORMANCE[0] desc='%s' vend=0x%04x",
                    NarrowDesc(d).c_str(), d.VendorId);
            hi->Release();
        }
        f6->Release();
    } else {
        Log("[gpu] IDXGIFactory6 unavailable hr=0x%08lx, VendorId fallback",
            (unsigned long)hr);
    }
    // Fallback: enumerate, apply vendor table.
    IDXGIFactory1* f1 = nullptr;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1),
                                  reinterpret_cast<void**>(&f1))) ||
        !f1)
        return false;
    bool ok = false;
    for (int i = 0;; ++i) {
        IDXGIAdapter1* ad = nullptr;
        if (f1->EnumAdapters1(i, &ad) == DXGI_ERROR_NOT_FOUND) break;
        DXGI_ADAPTER_DESC1 d{};
        if (ad && SUCCEEDED(ad->GetDesc1(&d))) {
            if (d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {  // skip Basic Render
                ad->Release();
                continue;
            }
            const std::string desc = NarrowDesc(d);
            bool integrated = false;
            if (d.VendorId == 0x10DE)
                integrated = false;
            else if (d.VendorId == 0x8086)
                integrated =
                    (desc.find("Arc") == std::string::npos &&
                     desc.find("ARC") == std::string::npos);
            else if (d.VendorId == 0x1002)
                integrated = (d.DedicatedVideoMemory < (512ull * 1024 * 1024));
            else
                integrated =
                    (d.SharedSystemMemory > d.DedicatedVideoMemory);
            Log("[gpu] fallback enum%d desc='%s' vend=0x%04x ded=%lluMB %s",
                i, desc.c_str(), d.VendorId,
                (unsigned long long)d.DedicatedVideoMemory / (1024 * 1024),
                integrated ? "integrated" : "discrete");
            if (integrated && !ok) {
                out->adapter = ad;
                out->desc = desc;
                out->vendor = d.VendorId;
                out->dedicated_mb =
                    d.DedicatedVideoMemory / (1024 * 1024);
                out->shared_mb = d.SharedSystemMemory / (1024 * 1024);
                out->via = "VendorId-fallback";
                ok = true;
                continue;  // keep `ad` (owned by out)
            }
        }
        if (ad) ad->Release();
    }
    f1->Release();
    return ok;
}

static void SetOpt(mpv_handle* mpv, const char* k, const char* v) {
    const int rc = mpv_set_option_string(mpv, k, v);
    if (rc < 0)
        Log("[mpv] option '%s=%s' refused (%s)", k, v, mpv_error_string(rc));
}

static std::string HwdecCurrent(mpv_handle* mpv) {
    char* s = nullptr;
    if (mpv_get_property(mpv, "hwdec-current", MPV_FORMAT_STRING, &s) >= 0 &&
        s) {
        std::string out = s;
        mpv_free(s);
        return out;
    }
    return "unknown";
}

// Device creation flags per upstream render_d3d11.h requirement: BGRA
// support mandatory for swapchain interop; VIDEO support for d3d11va.
static bool CreateD3D11OnAdapter(IDXGIAdapter1* adapter, ID3D11Device** dev,
                                 ID3D11DeviceContext** ctx) {
    static const D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
    };
    const UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT |
                       D3D11_CREATE_DEVICE_VIDEO_SUPPORT;
    D3D_FEATURE_LEVEL got = D3D_FEATURE_LEVEL_9_1;
    HRESULT hr = D3D11CreateDevice(
        adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, levels,
        (UINT)(sizeof(levels) / sizeof(levels[0])), D3D11_SDK_VERSION, dev,
        &got, ctx);
    if (FAILED(hr) || !*dev || !*ctx) {
        Log("[d3d11] D3D11CreateDevice failed hr=0x%08lx", (unsigned long)hr);
        return false;
    }
    Log("[d3d11] device ok feature_level=0x%x flags=BGRA|VIDEO", (unsigned)got);
    return true;
}

// SEH guard for reconstructed-ABI call (POD-only function: __try is illegal
// alongside C++ object unwinding, C2712).
static int MpvCreateGuarded(mpv_handle* mpv, mpv_render_param* params,
                            mpv_render_context** out) {
    int rc = -100;
    __try {
        rc = mpv_render_context_create(out, mpv, params);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        rc = -999;
        *out = nullptr;
    }
    return rc;
}

// ================= PROBE =================
static int RunProbe(const std::string& video) {
    Log("[a2-probe] B1 capability check");
    Log("[a2-probe] libmpv runtime api v%lu (header v2.5 has no d3d11 decl)",
        mpv_client_api_version());
    AdapterPick pick{};
    if (!PickLowPowerAdapter(&pick) || !pick.adapter) {
        Log("[fail] no low-power adapter (P3.6 heuristic found nothing)");
        return 1;
    }
    Log("[gpu] PICK desc='%s' vend=0x%04x ded=%lluMB shared=%lluMB via=%s",
        pick.desc.c_str(), pick.vendor, pick.dedicated_mb, pick.shared_mb,
        pick.via.c_str());

    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    if (!CreateD3D11OnAdapter(pick.adapter, &dev, &ctx)) {
        pick.adapter->Release();
        return 1;
    }

    setlocale(LC_NUMERIC, "C");
    mpv_handle* mpv = mpv_create();
    if (!mpv) {
        Log("[fail] mpv_create null");
        return 1;
    }
    SetOpt(mpv, "vo", "libmpv");
    SetOpt(mpv, "hwdec", "d3d11va");
    SetOpt(mpv, "audio", "no");
    SetOpt(mpv, "loop-file", "inf");
    SetOpt(mpv, "keep-open", "yes");
    SetOpt(mpv, "idle", "yes");
    if (mpv_initialize(mpv) < 0) {
        Log("[fail] mpv_initialize failed");
        mpv_terminate_destroy(mpv);
        return 1;
    }
    mpv_request_log_messages(mpv, "info");
    mpv_observe_property(mpv, 0, "hwdec-current", MPV_FORMAT_STRING);

    // api_type only — distinguishes backend-missing (NOT_IMPLEMENTED) from
    // params-missing (INVALID_PARAMETER) per render.h docs.
    mpv_render_param p_only[] = {
        {(mpv_render_param_type)MPV_RENDER_PARAM_API_TYPE,
         (void*)MPV_RENDER_API_TYPE_D3D11_RECON},
        {(mpv_render_param_type)0, nullptr},
    };
    mpv_render_context* probe_ctx = nullptr;
    const int rc_only = MpvCreateGuarded(mpv, p_only, &probe_ctx);
    Log("[a2-probe] create(api-only) rc=%d (%s)", rc_only,
        rc_only == -999 ? "SEH-AV (ABI guess wrong?)"
                        : mpv_error_string(rc_only));
    if (probe_ctx) {
        mpv_render_context_free(probe_ctx);
        probe_ctx = nullptr;
    }
    if (rc_only == MPV_ERROR_NOT_IMPLEMENTED) {
        Log("[verdict] B1 UNAVAILABLE — d3d11 backend not in this libmpv "
            "build. STOP: report B2 (ANGLE) deploy analysis, do not code fan-out.");
        mpv_terminate_destroy(mpv);
        ctx->Release();
        dev->Release();
        pick.adapter->Release();
        return 1;
    }

    // With init params (zero-padded recon struct).
    unsigned char pad[64] = {};
    MpvD3D11InitParamsRecon* ip =
        reinterpret_cast<MpvD3D11InitParamsRecon*>(pad);
    ip->device = dev;
    ip->device_context = ctx;
    mpv_render_param p_full[] = {
        {(mpv_render_param_type)MPV_RENDER_PARAM_API_TYPE,
         (void*)MPV_RENDER_API_TYPE_D3D11_RECON},
        {MPV_RENDER_PARAM_D3D11_INIT_PARAMS_RECON, ip},
        {(mpv_render_param_type)0, nullptr},
    };
    const int rc_full = MpvCreateGuarded(mpv, p_full, &probe_ctx);
    Log("[a2-probe] create(api+init) rc=%d (%s)", rc_full,
        rc_full == -999 ? "SEH-AV (struct layout guess wrong)"
                        : mpv_error_string(rc_full));
    if (rc_full < 0 || !probe_ctx) {
        Log("[verdict] B1 BLOCKED at context create. STOP: evaluate B2.");
        mpv_terminate_destroy(mpv);
        ctx->Release();
        dev->Release();
        pick.adapter->Release();
        return 1;
    }
    Log("[a2-probe] render_context_create OK (d3d11 backend exists)");

    const char* cmd[] = {"loadfile", video.c_str(), nullptr};
    if (mpv_command(mpv, cmd) < 0) {
        Log("[fail] loadfile refused");
        mpv_render_context_free(probe_ctx);
        mpv_terminate_destroy(mpv);
        return 1;
    }
    std::string hwdec_seen = "unknown";
    bool started = false;
    const DWORD t0 = GetTickCount();
    while (GetTickCount() - t0 < 8000) {
        mpv_event* ev = mpv_wait_event(mpv, 100);
        if (!ev || ev->event_id == MPV_EVENT_NONE) continue;
        if (ev->event_id == MPV_EVENT_START_FILE ||
            ev->event_id == MPV_EVENT_FILE_LOADED)
            started = true;
        else if (ev->event_id == MPV_EVENT_LOG_MESSAGE) {
            auto* lm = (mpv_event_log_message*)ev->data;
            if (lm && lm->text && strstr(lm->text, "hardware decoding"))
                Log("[mpv-log] %s", lm->text);
        } else if (ev->event_id == MPV_EVENT_PROPERTY_CHANGE) {
            auto* p = (mpv_event_property*)ev->data;
            if (p && p->name && strcmp(p->name, "hwdec-current") == 0 &&
                p->format == MPV_FORMAT_STRING && p->data) {
                const char* v = *(const char**)p->data;
                hwdec_seen = v ? v : "no";
                Log("[prop] hwdec-current=%s", hwdec_seen.c_str());
            }
        }
        if (started && hwdec_seen != "unknown") break;
    }
    hwdec_seen = HwdecCurrent(mpv);
    Log("[a2-probe] playback=%d hwdec-current=%s", started ? 1 : 0,
        hwdec_seen.c_str());
    mpv_render_context_free(probe_ctx);
    mpv_terminate_destroy(mpv);
    ctx->Release();
    dev->Release();
    pick.adapter->Release();
    const bool ok = started && hwdec_seen == "d3d11va";
    Log("[verdict] %s", ok ? "B1 GO — hwdec d3d11va ACTIVE via d3d11 render ctx"
                           : "B1 MARGINAL — backend exists but hwdec not d3d11va");
    return ok ? 0 : 1;
}

int main(int argc, char** argv) {
    std::string video;
    bool probe = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--probe")
            probe = true;
        else if (!a.empty() && a[0] != '-')
            video = a;
    }
    if (!probe || video.empty()) {
        fprintf(stderr, "usage: render_shared_d3d11.exe --probe <video>\n"
                        "(fan-out deferred: B1 unavailable, see docs)\n");
        return 2;
    }
    return RunProbe(video);
}

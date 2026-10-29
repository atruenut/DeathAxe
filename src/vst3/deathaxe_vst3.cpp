// DeathAxe - VST3 wrapper (processor + controller + factory) around the portable DSP engine.
// Instrument: MIDI in -> stereo audio out.  Presets are exposed as a VST3 program list.
#include "../engine.h"
#include "../presets.h"
#include "../userpresets.h"
#include "../ui/editor.h"
#include "../ui/win32_window.h"
#include "public.sdk/source/common/pluginview.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"

#include "public.sdk/source/main/pluginfactory.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/vstspeaker.h"
#include "pluginterfaces/vst/ivstunits.h"

#include <cstdio>
#include <cstring>
#if defined(__SSE__) || defined(_M_X64)
#include <xmmintrin.h>
#define DA_HAVE_SSE 1
#endif

using namespace Steinberg;
using namespace Steinberg::Vst;

#define DA_NAME      "DeathAxe"
#define DA_VENDOR    "DeathAxe"
#define DA_VERSION   "1.2.0"

static const FUID kProcessorUID(0xD3A7B1C5, 0x5E8F4A21, 0x9B3C6D70, 0x1F2E4A88);
static const FUID kControllerUID(0x7C1E9A44, 0xB2D05F13, 0x84A6E3C9, 0x2D7B5E01);

static const int32 kStateVersion = 1;
static const UnitID kPresetUnit = 1;

// program list = factory presets (user presets are files, managed from the editor)
static int programIndex(double norm) {
    const int total = dg::numPresets();
    return std::max(0, std::min(total - 1, (int)(norm * (total - 1) + 0.5)));
}

static void asciiTo16(const char* s, TChar* out, int cap) {
    int i = 0;
    for (; s[i] && i < cap - 1; ++i) out[i] = (TChar)(unsigned char)s[i];
    out[i] = 0;
}

// =============================================================== processor
class DeathAxeProcessor : public AudioEffect {
public:
    DeathAxeProcessor() { setControllerClass(kControllerUID); }

    static FUnknown* createInstance(void*) { return (IAudioProcessor*)new DeathAxeProcessor(); }

    tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE {
        tresult r = AudioEffect::initialize(context);
        if (r != kResultOk) return r;
        addEventInput(STR16("MIDI In"), 1);
        addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
        dg::presetTable();   // build the table now, never on the audio thread
        for (int i = 0; i < dg::kNumParams; ++i) cur[i] = dg::kParams[i].defV;
        return kResultOk;
    }

    tresult PLUGIN_API setBusArrangements(SpeakerArrangement* inputs, int32 numIns,
                                          SpeakerArrangement* outputs, int32 numOuts) SMTG_OVERRIDE {
        if (numIns == 0 && numOuts == 1 && outputs[0] == SpeakerArr::kStereo) return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
        return kResultFalse;
    }

    tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) SMTG_OVERRIDE {
        return symbolicSampleSize == kSample32 ? kResultTrue : kResultFalse;
    }

    tresult PLUGIN_API setupProcessing(ProcessSetup& setup) SMTG_OVERRIDE {
        engine.init(setup.sampleRate);
        engine.loadPreset(cur);
        engine.setPitchBend(bend);  engine.setModWheel(mod);
        return AudioEffect::setupProcessing(setup);
    }

    tresult PLUGIN_API setActive(TBool state) SMTG_OVERRIDE {
        if (!state) engine.allNotesOff(); else engine.reset();
        return AudioEffect::setActive(state);
    }

    tresult PLUGIN_API process(ProcessData& data) SMTG_OVERRIDE {
#ifdef DA_HAVE_SSE
        const unsigned oldCsr = _mm_getcsr();
        _mm_setcsr(oldCsr | 0x8040);                 // flush denormals to zero
#endif
        // ---- parameters (program change first, then individual values)
        if (data.inputParameterChanges) {
            const int32 n = data.inputParameterChanges->getParameterCount();
            for (int pass = 0; pass < 2; ++pass)
                for (int32 i = 0; i < n; ++i) {
                    IParamValueQueue* q = data.inputParameterChanges->getParameterData(i);
                    if (!q || q->getPointCount() <= 0) continue;
                    const ParamID id = q->getParameterId();
                    const int mine = (id == dg::kPidProgram) ? 0 : 1;
                    if (mine != pass) continue;
                    int32 off; ParamValue v;
                    if (q->getPoint(q->getPointCount() - 1, off, v) != kResultTrue) continue;
                    if (id == dg::kPidProgram) {
                        dg::presetValues(programIndex(v), cur);
                        engine.loadPreset(cur);
                    } else if (id < (ParamID)dg::kNumParams) {
                        cur[id] = dg::toPlain((int)id, v);
                        engine.setParam((int)id, cur[id]);
                    } else if (id == dg::kPidPitchBend) { bend = (float)v; engine.setPitchBend(bend); }
                    else if (id == dg::kPidModWheel)   { mod = (float)v;  engine.setModWheel(mod); }
                }
        }

        if (data.numOutputs < 1 || data.outputs[0].numChannels < 2 || !data.outputs[0].channelBuffers32) {
#ifdef DA_HAVE_SSE
            _mm_setcsr(oldCsr);
#endif
            return kResultOk;
        }
        float* L = data.outputs[0].channelBuffers32[0];
        float* R = data.outputs[0].channelBuffers32[1];
        const int32 total = data.numSamples;

        // ---- sample-accurate MIDI
        int32 pos = 0;
        IEventList* events = data.inputEvents;
        const int32 ne = events ? events->getEventCount() : 0;
        for (int32 i = 0; i < ne; ++i) {
            Event e;
            if (events->getEvent(i, e) != kResultOk) continue;
            if (e.type != Event::kNoteOnEvent && e.type != Event::kNoteOffEvent) continue;
            int32 off = std::max(pos, std::min(total, e.sampleOffset));
            if (off > pos) { engine.process(L + pos, R + pos, off - pos); pos = off; }
            if (e.type == Event::kNoteOnEvent) {
                if (e.noteOn.velocity <= 0.f) engine.noteOff(e.noteOn.pitch);
                else engine.noteOn(e.noteOn.pitch, e.noteOn.velocity);
            } else engine.noteOff(e.noteOff.pitch);
        }
        if (pos < total) engine.process(L + pos, R + pos, total - pos);
        data.outputs[0].silenceFlags = 0;
#ifdef DA_HAVE_SSE
        _mm_setcsr(oldCsr);
#endif
        return kResultOk;
    }

    tresult PLUGIN_API setState(IBStream* state) SMTG_OVERRIDE {
        if (!state) return kResultFalse;
        int32 ver = 0, n = 0, got = 0;
        if (state->read(&ver, 4, &got) != kResultOk || got != 4 || ver != kStateVersion) return kResultFalse;
        if (state->read(&n, 4, &got) != kResultOk || got != 4) return kResultFalse;
        for (int i = 0; i < n; ++i) {
            float v;
            if (state->read(&v, 4, &got) != kResultOk || got != 4) return kResultFalse;
            if (i < dg::kNumParams) cur[i] = v;
        }
        engine.loadPreset(cur);
        return kResultOk;
    }

    tresult PLUGIN_API getState(IBStream* state) SMTG_OVERRIDE {
        if (!state) return kResultFalse;
        int32 ver = kStateVersion, n = dg::kNumParams, w = 0;
        state->write(&ver, 4, &w);
        state->write(&n, 4, &w);
        for (int i = 0; i < n; ++i) state->write(&cur[i], 4, &w);
        return kResultOk;
    }

private:
    dg::Engine engine;
    float cur[dg::kNumParams];
    float bend = 0.5f, mod = 0.0f;
};

// =============================================================== parameters
class DAParam : public Parameter {
public:
    DAParam(int id, const dg::ParamDef& d, UnitID unit)
        : Parameter(ascii16(d.name, 0), (ParamID)id, ascii16(d.unit, 1), dg::toNorm(id, d.defV),
                    dg::stepCount(d), ParameterInfo::kCanAutomate, unit),
          def(d), pid(id) {}
    void toString(ParamValue v, String128 out) const SMTG_OVERRIDE {
        char txt[64];
        const float p = dg::toPlain(pid, v);
        if (def.type == dg::T_ENUM) dg::enumChoice(def, (int)(p - def.minV + 0.5f), txt, 64);
        else if (def.type == dg::T_BOOL) std::snprintf(txt, 64, "%s", p > 0.5f ? "On" : "Off");
        else if (def.type == dg::T_INT) std::snprintf(txt, 64, "%d", (int)std::lround(p));
        else if (def.maxV <= 1.0f && def.unit[0] == 0) std::snprintf(txt, 64, "%.0f %%", p * 100.0f);
        else if (std::fabs(p) >= 100.f) std::snprintf(txt, 64, "%.0f", p);
        else std::snprintf(txt, 64, "%.1f", p);
        asciiTo16(txt, out, 128);
    }
    bool fromString(const TChar* s, ParamValue& v) const SMTG_OVERRIDE {
        char txt[64];  int i = 0;
        for (; s[i] && i < 63; ++i) txt[i] = (char)s[i];
        txt[i] = 0;
        if (def.type == dg::T_ENUM) {
            const int n = dg::enumCount(def);
            for (int k = 0; k < n; ++k) {
                char c[64];  dg::enumChoice(def, k, c, 64);
                if (!strcasecmp_(c, txt)) { v = dg::toNorm(pid, def.minV + k); return true; }
            }
        }
        if (def.type == dg::T_BOOL) { v = (txt[0] == '1' || txt[0] == 'o' || txt[0] == 'O') && (txt[1] != 'f' && txt[1] != 'F') ? 1.0 : 0.0; return true; }
        float f = (float)std::atof(txt);
        if (def.maxV <= 1.0f && def.unit[0] == 0 && def.type == dg::T_LIN) f *= 0.01f;
        v = dg::toNorm(pid, f);
        return true;
    }
    ParamValue toPlain(ParamValue v) const SMTG_OVERRIDE { return dg::toPlain(pid, v); }
    ParamValue toNormalized(ParamValue p) const SMTG_OVERRIDE { return dg::toNorm(pid, (float)p); }
private:
    static const TChar* ascii16(const char* t, int slot) {
        static TChar buf[2][128];
        asciiTo16(t, buf[slot], 128);
        return buf[slot];
    }
    static int strcasecmp_(const char* a, const char* b) {
        for (; *a && *b; ++a, ++b) {
            const char x = (*a >= 'A' && *a <= 'Z') ? *a + 32 : *a, y = (*b >= 'A' && *b <= 'Z') ? *b + 32 : *b;
            if (x != y) return 1;
        }
        return *a != *b;
    }
    dg::ParamDef def;  int pid;
};

// =============================================================== editor view
class DeathAxeController;

#ifdef _WIN32
class DeathAxeView : public CPluginView, public IPlugViewContentScaleSupport {
public:
    DeathAxeView(DeathAxeController* c, dg::ui::Host* h) : ctrl(c), host(h) {
        rect.left = rect.top = 0;  rect.right = dg::ui::kEditorW;  rect.bottom = dg::ui::kEditorH;
    }
    ~DeathAxeView() override { win.reset(); }

    tresult PLUGIN_API isPlatformTypeSupported(FIDString type) SMTG_OVERRIDE {
        return strcmp(type, kPlatformTypeHWND) == 0 ? kResultTrue : kResultFalse;
    }
    tresult PLUGIN_API attached(void* parent, FIDString type) SMTG_OVERRIDE {
        if (!parent || strcmp(type, kPlatformTypeHWND) != 0) return kResultFalse;
        win.reset(new dg::ui::WinWindow(host));
        if (!win->create((HWND)parent, extraScale)) { win.reset(); return kResultFalse; }
        rect.right = win->pixelW();  rect.bottom = win->pixelH();
        return CPluginView::attached(parent, type);
    }
    tresult PLUGIN_API removed() SMTG_OVERRIDE { win.reset(); return CPluginView::removed(); }
    tresult PLUGIN_API onSize(ViewRect* r) SMTG_OVERRIDE { (void)r; return kResultTrue; }       // fixed size
    tresult PLUGIN_API canResize() SMTG_OVERRIDE { return kResultFalse; }
    tresult PLUGIN_API checkSizeConstraint(ViewRect* r) SMTG_OVERRIDE {
        if (r) { r->right = r->left + rect.getWidth(); r->bottom = r->top + rect.getHeight(); }
        return kResultTrue;
    }
    tresult PLUGIN_API getSize(ViewRect* size) SMTG_OVERRIDE {
        if (!size) return kInvalidArgument;
        if (win) { rect.right = win->pixelW(); rect.bottom = win->pixelH(); }
        else {   // before attach: report the size at the expected scale
            const float sc = std::max(1.0f, extraScale);
            rect.right = (int)std::lround(dg::ui::kEditorW * sc);  rect.bottom = (int)std::lround(dg::ui::kEditorH * sc);
        }
        *size = rect;  return kResultTrue;
    }
    tresult PLUGIN_API setContentScaleFactor(ScaleFactor f) SMTG_OVERRIDE {
        // hosts that scale themselves pass the factor here; remember it for the next attach
        if (!win && f > 0.5f && f < 4.0f) extraScale = 1.0f;   // the window reads the real DPI itself; avoid double scaling
        return kResultTrue;
    }
    OBJ_METHODS(DeathAxeView, CPluginView)
    tresult PLUGIN_API queryInterface(const TUID iid, void** obj) SMTG_OVERRIDE {
        QUERY_INTERFACE(iid, obj, IPlugViewContentScaleSupport::iid, IPlugViewContentScaleSupport)
        return CPluginView::queryInterface(iid, obj);
    }
    REFCOUNT_METHODS(CPluginView)
private:
    DeathAxeController* ctrl;  dg::ui::Host* host;
    std::unique_ptr<dg::ui::WinWindow> win;
    float extraScale = 1.0f;
};
#endif

// =============================================================== controller
class DeathAxeController : public EditControllerEx1, public IMidiMapping, public dg::ui::Host {
public:
    static FUnknown* createInstance(void*) { return (IEditController*)new DeathAxeController(); }

    tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE {
        tresult r = EditControllerEx1::initialize(context);
        if (r != kResultOk) return r;

        // factory presets appear in the host's own preset menu as a program list
        addUnit(new Unit(STR16("Factory Presets"), kPresetUnit, kRootUnitId, (ProgramListID)dg::kPidProgram));
        auto* list = new ProgramList(STR16("Preset"), (ProgramListID)dg::kPidProgram, kPresetUnit);
        for (int i = 0; i < dg::numPresets(); ++i) {
            String128 nm;  asciiTo16(dg::presetName(i), nm, 128);
            list->addProgram(nm);
        }
        addProgramList(list);
        parameters.addParameter(list->getParameter());

        for (int i = 0; i < dg::kNumParams; ++i) parameters.addParameter(new DAParam(i, dg::kParams[i], kRootUnitId));
        parameters.addParameter(STR16("Pitch Bend"), nullptr, 0, 0.5, ParameterInfo::kCanAutomate | ParameterInfo::kIsHidden, dg::kPidPitchBend);
        parameters.addParameter(STR16("Mod Wheel"), nullptr, 0, 0.0, ParameterInfo::kCanAutomate | ParameterInfo::kIsHidden, dg::kPidModWheel);
        return kResultOk;
    }

    tresult PLUGIN_API setComponentState(IBStream* state) SMTG_OVERRIDE {
        if (!state) return kResultFalse;
        int32 ver = 0, n = 0, got = 0;
        if (state->read(&ver, 4, &got) != kResultOk || got != 4 || ver != kStateVersion) return kResultFalse;
        if (state->read(&n, 4, &got) != kResultOk || got != 4) return kResultFalse;
        for (int i = 0; i < n; ++i) {
            float v;
            if (state->read(&v, 4, &got) != kResultOk || got != 4) return kResultFalse;
            if (i < dg::kNumParams) EditController::setParamNormalized((ParamID)i, dg::toNorm(i, v));
        }
        pstate = dg::ui::PresetState();     // let the editor re-detect which preset this is
        return kResultOk;
    }

    tresult PLUGIN_API setParamNormalized(ParamID tag, ParamValue value) SMTG_OVERRIDE {
        tresult r = EditControllerEx1::setParamNormalized(tag, value);
        if (tag == dg::kPidProgram) {
            const int idx = programIndex(value);
            float vals[dg::kNumParams];
            dg::presetValues(idx, vals);
            for (int i = 0; i < dg::kNumParams; ++i) EditControllerEx1::setParamNormalized((ParamID)i, dg::toNorm(i, vals[i]));
            pstate.valid = true;  pstate.name = dg::presetName(idx);  pstate.factory = idx;  pstate.user = false;  pstate.path.clear();
            std::memcpy(pstate.ref, vals, sizeof vals);
            if (componentHandler) componentHandler->restartComponent(kParamValuesChanged);
        }
        return r;
    }

    tresult PLUGIN_API getMidiControllerAssignment(int32 /*bus*/, int16 /*channel*/, CtrlNumber num, ParamID& id) SMTG_OVERRIDE {
        if (num == kPitchBend) { id = dg::kPidPitchBend; return kResultTrue; }
        if (num == kCtrlModWheel) { id = dg::kPidModWheel; return kResultTrue; }
        return kResultFalse;
    }

    IPlugView* PLUGIN_API createView(FIDString name) SMTG_OVERRIDE {
#ifdef _WIN32
        if (name && strcmp(name, ViewType::kEditor) == 0) return new DeathAxeView(this, this);
#else
        (void)name;
#endif
        return nullptr;
    }

    // ---- dg::ui::Host: the editor talks to the parameters through the normal edit gestures
    float get(int pid) override { return dg::toPlain(pid, getParamNormalized((ParamID)pid)); }
    void begin(int pid) override { beginEdit((ParamID)pid); }
    void set(int pid, float plain) override {
        const double n = dg::toNorm(pid, plain);
        setParamNormalized((ParamID)pid, n);
        performEdit((ParamID)pid, n);
    }
    void end(int pid) override { endEdit((ParamID)pid); }
    dg::ui::PresetState& presetState() override { return pstate; }

    dg::ui::PresetState& presetStateRef() { return pstate; }
    DELEGATE_REFCOUNT(EditControllerEx1)
    tresult PLUGIN_API queryInterface(const TUID iid, void** obj) SMTG_OVERRIDE {
        QUERY_INTERFACE(iid, obj, IMidiMapping::iid, IMidiMapping)
        return EditControllerEx1::queryInterface(iid, obj);
    }
private:
    dg::ui::PresetState pstate;
};

// =============================================================== module + factory
bool InitModule() { return true; }
bool DeinitModule() { return true; }

BEGIN_FACTORY_DEF(DA_VENDOR, "", "")

    DEF_CLASS2(INLINE_UID_FROM_FUID(kProcessorUID), PClassInfo::kManyInstances, kVstAudioEffectClass, DA_NAME,
               Vst::kDistributable, Vst::PlugType::kInstrumentSynth, DA_VERSION, kVstVersionString,
               DeathAxeProcessor::createInstance)

    DEF_CLASS2(INLINE_UID_FROM_FUID(kControllerUID), PClassInfo::kManyInstances, kVstComponentControllerClass,
               DA_NAME "Controller", 0, "", DA_VERSION, kVstVersionString, DeathAxeController::createInstance)

END_FACTORY

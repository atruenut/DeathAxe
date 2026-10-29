// Drives the real DeathAxe controller + processor classes (no DAW needed): program change, editor host interface, state.
#include "../src/vst3/deathaxe_vst3.cpp"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "public.sdk/source/common/memorystream.h"
#include <cstdlib>
static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); ++fails; } } while (0)

static std::string ascii(const String128 s) { std::string o; for (int i = 0; s[i] && i < 128; ++i) o += (char)s[i]; return o; }

static std::vector<float> stateOf(DeathAxeProcessor& p) {
    MemoryStream ms;  p.getState(&ms);
    std::vector<float> v(dg::kNumParams);
    ms.seek(8, IBStream::kIBSeekSet, nullptr);
    int32 n; for (int i = 0; i < dg::kNumParams; ++i) ms.read(&v[i], 4, &n);
    return v;
}

static void sendParam(DeathAxeProcessor& p, ParamID id, double norm) {
    static float L[256], R[256];  static float* ch[2] = {L, R};
    Vst::ParameterChanges pc(2);  int32 qi, pi;
    pc.addParameterData(id, qi)->addPoint(0, norm, pi);
    AudioBusBuffers out{};  out.numChannels = 2;  out.channelBuffers32 = ch;
    ProcessData pd{};  pd.numSamples = 128;  pd.numOutputs = 1;  pd.outputs = &out;  pd.inputParameterChanges = &pc;
    pd.symbolicSampleSize = kSample32;
    p.process(pd);
}

int main() {
    const int np = dg::numPresets();
    std::printf("presets=%d\n", np);
    DeathAxeController* c = new DeathAxeController();  c->initialize(nullptr);
    String128 nm;
    c->getProgramName(dg::kPidProgram, np - 1, nm);  CHECK(ascii(nm) == dg::presetName(np - 1));

    // host program change -> controller loads every parameter and updates the editor's preset state
    c->setParamNormalized(dg::kPidProgram, 0.0);
    CHECK(std::fabs(c->get(dg::pGain) - 0.72f) < 1e-4f);
    CHECK(c->presetState().valid && c->presetState().factory == 0 && c->presetState().name == dg::presetName(0));
    c->setParamNormalized(dg::kPidProgram, 1.0 / (np - 1));
    CHECK(std::fabs(c->get(dg::pGain) - 0.85f) < 1e-4f);
    // editor-style edit through the Host interface
    c->begin(dg::pGain);  c->set(dg::pGain, 0.2f);  c->end(dg::pGain);
    CHECK(std::fabs(c->get(dg::pGain) - 0.2f) < 1e-4f);
    // the Save To Slot parameter is gone
    ParameterInfo pi;
    for (int i = 0; i < c->getParameterCount(); ++i) { c->getParameterInfo(i, pi); CHECK(pi.id != 302); }

    // processor: program change first, then individual values in the same block
    DeathAxeProcessor* p = new DeathAxeProcessor();  p->initialize(nullptr);
    ProcessSetup su{kRealtime, kSample32, 128, 44100.0};  p->setupProcessing(su);  p->setActive(true);
    sendParam(*p, dg::kPidProgram, 0.0);
    CHECK(std::fabs(stateOf(*p)[dg::pBass] - 2.0f) < 1e-4f);   // factory preset 1 has Bass +2
    sendParam(*p, dg::pBass, dg::toNorm(dg::pBass, 7.0f));
    CHECK(std::fabs(stateOf(*p)[dg::pBass] - 7.0f) < 1e-3f);

    MemoryStream ms;  p->getState(&ms);  ms.seek(0, IBStream::kIBSeekSet, nullptr);
    DeathAxeProcessor* p2 = new DeathAxeProcessor();  p2->initialize(nullptr);
    CHECK(p2->setState(&ms) == kResultOk);
    CHECK(std::fabs(stateOf(*p2)[dg::pBass] - 7.0f) < 1e-3f);
    ms.seek(0, IBStream::kIBSeekSet, nullptr);
    CHECK(c->setComponentState(&ms) == kResultOk);
    CHECK(std::fabs(c->get(dg::pBass) - 7.0f) < 1e-3f);

    std::printf(fails ? "plugin logic test FAILED (%d)\n" : "plugin logic test passed\n", fails);
    return fails ? 1 : 0;
}

// DeathAxe - factory presets. Each preset lists only the parameters that differ from the defaults.
#pragma once
#include "params.h"
#include <string>
#include <vector>
#include <initializer_list>

namespace dg {

namespace pre {
enum { V_MODERN = 0, V_BRUTAL, V_OLD, V_CHAINSAW, V_TECH };
enum { C_SINGLE = 0, C_POWER, C_POWER_OCT, C_OCT, C_POWER4, C_TRITONE, C_MINOR, C_SUBPOWER };
enum { CAB_V30 = 0, CAB_GREEN, CAB_2X12, CAB_1X12, CAB_8X10, CAB_DI };
enum { M_POLY = 0, M_MONO = 1 };
}

struct PO { int id; float v; };
struct PresetDef { std::string name, desc; std::vector<PO> o; };

inline const std::vector<PresetDef>& presetTable() {
    using namespace pre;
    // heap-allocated on purpose: no static destructor runs at module unload
    static const std::vector<PresetDef>& t = *new std::vector<PresetDef>{
    {"Death Metal Standard", "All-rounder: tight modern scooped rhythm tone, V30 cab.", {
        {pVoicing, V_MODERN}, {pGain, 0.72f}, {pBass, 2}, {pMid, -3}, {pMidFreq, 600}, {pTreble, 2},
        {pBoostDrive, 0.35f}, {pBoostTight, 190}}},

    {"Stockholm Buzzsaw", "Hard-clipped pedal-into-amp chainsaw: bass-heavy, razor top, scooped mids.", {
        {pVoicing, V_CHAINSAW}, {pGain, 0.85f}, {pBright, 0.25f}, {pBias, 0.35f}, {pSag, 0.15f},
        {pBass, 4}, {pMid, -3}, {pMidFreq, 500}, {pTreble, 4}, {pPresence, 2},
        {pCabType, CAB_GREEN}, {pMicPos, 0.4f}, {pMicDist, 0.15f}, {pBoostOn, 0},
        {pGateThresh, -52}, {pGateRelease, 40}, {pSustain, 3.0f}, {pDamping, 0.3f}, {pPickAttack, 0.8f},
        {pPickPos, 0.25f}, {pPickNoise, 0.3f}, {pWidth, 0.9f}, {pRevMix, 0.05f}}},

    {"Tampa Rot", "Dynamic old-school crunch with a boost in front: rotten, mid-heavy, alive under the pick.", {
        {pVoicing, V_OLD}, {pGain, 0.62f}, {pBright, 0.35f}, {pBias, 0.6f}, {pSag, 0.45f},
        {pBass, 2}, {pMid, 2}, {pMidFreq, 800}, {pPresence, 1}, {pBoostDrive, 0.4f}, {pBoostTight, 120},
        {pCabType, CAB_GREEN}, {pMicPos, 0.5f}, {pMicDist, 0.3f}, {pGateRelease, 80}, {pSustain, 5},
        {pDamping, 0.4f}, {pPickAttack, 0.6f}, {pWidth, 0.75f}, {pRevMix, 0.12f}, {pRevSize, 0.35f}}},

    {"Gothenburg Twin", "Melodic-death lead/rhythm tone: singing mids, neck pickup, legato glide, dotted delay.", {
        {pVoicing, V_BRUTAL}, {pGain, 0.5f}, {pBright, 0.45f}, {pBass, 0}, {pMid, 4}, {pMidFreq, 1200},
        {pTreble, 2}, {pPresence, 2}, {pVoiceMode, M_MONO}, {pGlide, 25}, {pSustain, 9}, {pDamping, 0.25f},
        {pPickPos, 0.6f}, {pPickAttack, 0.5f}, {pBoostDrive, 0.25f}, {pBoostTight, 120}, {pBoostLevel, 2},
        {pCabType, CAB_V30}, {pMicPos, 0.3f}, {pMicDist, 0.3f}, {pDelayMix, 0.18f}, {pDelayTime, 375},
        {pDelayFb, 0.35f}, {pRevMix, 0.2f}, {pRevSize, 0.5f}, {pWidth, 0.85f}, {pOutput, -7}}},

    {"Brutal Slam", "Huge, mid-scooped slam rhythm: power-chord dyads, sub support, 8x10 weight.", {
        {pVoicing, V_BRUTAL}, {pGain, 0.9f}, {pBright, 0.2f}, {pBias, 0.55f}, {pSag, 0.5f},
        {pBass, 4}, {pMid, -6}, {pMidFreq, 450}, {pTreble, 2}, {pResonance, 3}, {pCabType, CAB_8X10},
        {pMicPos, 0.3f}, {pMicDist, 0.2f}, {pPalmMute, 0.5f}, {pSustain, 2.2f}, {pGateThresh, -48},
        {pGateRelease, 25}, {pChord, C_POWER}, {pStrum, 0.1f}, {pSub, 0.3f}, {pBoostDrive, 0.3f},
        {pBoostTight, 200}, {pBoostLevel, -1}, {pLowCut, 40}, {pHighCut, 8000}, {pRevMix, 0.06f}, {pWidth, 0.7f}, {pOutput, -4}}},

    {"Technical Precision", "Surgical tech-death rhythm: very tight low end, fast gate, clear note definition.", {
        {pVoicing, V_TECH}, {pGain, 0.72f}, {pBright, 0.35f}, {pBias, 0.3f}, {pSag, 0.05f}, {pBass, -1},
        {pMid, 1}, {pMidFreq, 900}, {pTreble, 3}, {pPresence, 2}, {pCabType, CAB_2X12}, {pMicPos, 0.15f},
        {pMicDist, 0.1f}, {pBoostDrive, 0.3f}, {pBoostTight, 280}, {pGateThresh, -50}, {pGateRelease, 15},
        {pSustain, 3}, {pDamping, 0.3f}, {pPickAttack, 0.85f}, {pPickPos, 0.2f}, {pPickNoise, 0.15f},
        {pWidth, 0.6f}, {pLowCut, 60}, {pRevMix, 0.04f}}},

    {"Chug Machine", "Palm-muted machine-gun chugs on power chords. Stays tight at any tempo.", {
        {pVoicing, V_MODERN}, {pGain, 0.8f}, {pBright, 0.25f}, {pBias, 0.4f}, {pBass, 3}, {pMid, -4},
        {pMidFreq, 500}, {pTreble, 3}, {pPresence, 1}, {pPalmMute, 0.8f}, {pSustain, 1.2f}, {pChord, C_POWER},
        {pStrum, 0}, {pMicPos, 0.3f}, {pMicDist, 0.15f}, {pBoostDrive, 0.35f}, {pBoostTight, 220},
        {pGateThresh, -50}, {pGateRelease, 18}, {pWidth, 0.75f}, {pPickNoise, 0.35f}, {pPickAttack, 0.6f},
        {pRevMix, 0.03f}}},

    {"Tremolo Torrent", "Built for tremolo picking: hold a note and the strings re-pick for you.", {
        {pVoicing, V_BRUTAL}, {pGain, 0.7f}, {pBright, 0.4f}, {pTremRate, 0.65f}, {pSustain, 6}, {pMid, 2},
        {pMidFreq, 1000}, {pTreble, 2}, {pBoostDrive, 0.3f}, {pBoostTight, 150}, {pMicPos, 0.35f},
        {pDelayMix, 0.12f}, {pDelayTime, 250}, {pDelayFb, 0.25f}, {pRevMix, 0.15f}, {pGateRelease, 60},
        {pWidth, 0.9f}}},

    {"Dropped Abyss", "Seven semitones down plus a sub-octave string: seismic low end, dark top.", {
        {pVoicing, V_MODERN}, {pGain, 0.85f}, {pBias, 0.6f}, {pSag, 0.55f}, {pTranspose, -5}, {pSub, 0.55f},
        {pBass, 5}, {pMid, -3}, {pMidFreq, 600}, {pTreble, 1}, {pResonance, 6}, {pCabType, CAB_8X10},
        {pMicPos, 0.25f}, {pMicDist, 0.3f}, {pHighCut, 6500}, {pLowCut, 30}, {pSustain, 3}, {pDamping, 0.4f},
        {pGateThresh, -52}, {pGateRelease, 35}, {pPalmMute, 0.3f}, {pBoostDrive, 0.25f}, {pBoostTight, 140},
        {pBoostLevel, -1}, {pRevMix, 0.1f}, {pRevSize, 0.6f}, {pWidth, 0.8f}, {pOutput, -4}}},

    {"Pinch Squeal Lead", "Mono legato lead. Hit a note at velocity 120+ for a pinch-harmonic squeal.", {
        {pVoicing, V_BRUTAL}, {pGain, 0.62f}, {pBright, 0.5f}, {pVoiceMode, M_MONO}, {pGlide, 35}, {pPinch, 1.0f},
        {pSustain, 12}, {pDamping, 0.2f}, {pPickPos, 0.65f}, {pPickAttack, 0.55f}, {pPickNoise, 0.1f},
        {pMid, 4}, {pMidFreq, 1500}, {pTreble, 1}, {pPresence, 2}, {pMicPos, 0.25f}, {pMicDist, 0.3f},
        {pBoostDrive, 0.3f}, {pBoostTight, 130}, {pBoostLevel, 3}, {pDelayMix, 0.22f}, {pDelayTime, 410},
        {pDelayFb, 0.4f}, {pRevMix, 0.22f}, {pRevSize, 0.55f}, {pGateThresh, -62}, {pGateRelease, 150},
        {pSag, 0.2f}, {pWidth, 0.5f}, {pOutput, -8}}},

    {"Doom Funeral", "Slow, huge death-doom: strummed octave power chords, endless sustain, cavernous space.", {
        {pVoicing, V_OLD}, {pGain, 0.55f}, {pBright, 0.15f}, {pSag, 0.7f}, {pChord, C_POWER_OCT},
        {pStrum, 0.55f}, {pBass, 4}, {pMid, 1}, {pMidFreq, 700}, {pTreble, -2}, {pPresence, -2}, {pResonance, 4},
        {pCabType, CAB_GREEN}, {pMicPos, 0.6f}, {pMicDist, 0.5f}, {pBoostOn, 0}, {pSustain, 14}, {pDamping, 0.5f},
        {pPickPos, 0.5f}, {pPickAttack, 0.4f}, {pPickNoise, 0.1f}, {pGateThresh, -65}, {pGateRelease, 300},
        {pDelayMix, 0.22f}, {pDelayTime, 560}, {pDelayFb, 0.5f}, {pRevMix, 0.35f}, {pRevSize, 0.8f},
        {pRevDamp, 0.6f}, {pWidth, 1.0f}, {pLowCut, 35}, {pHighCut, 7000}, {pOutput, -7}}},

    {"Blackened Frost", "Icy blackened-death wall: tremolo-picked power chords, thin bass, huge reverb.", {
        {pVoicing, V_CHAINSAW}, {pGain, 0.95f}, {pBright, 0.7f}, {pBias, 0.25f}, {pSag, 0.1f}, {pTremRate, 0.85f},
        {pChord, C_POWER}, {pStrum, 0}, {pBass, -4}, {pMid, 2}, {pMidFreq, 1800}, {pTreble, 6}, {pPresence, 4},
        {pMicPos, 0.1f}, {pMicDist, 0.5f}, {pBoostDrive, 0.6f}, {pBoostTight, 300}, {pRevMix, 0.3f},
        {pRevSize, 0.85f}, {pRevDamp, 0.35f}, {pLowCut, 120}, {pHighCut, 14000}, {pSustain, 8},
        {pGateThresh, -58}, {pGateRelease, 120}, {pPickAttack, 0.9f}, {pPickNoise, 0.5f}, {pWidth, 1.0f}, {pOutput, -9}}},

    {"Cavernous Gore", "Low, wet and rotten: dropped tuning, sub string and a very distant mic in a big room.", {
        {pVoicing, V_BRUTAL}, {pGain, 0.85f}, {pSag, 0.6f}, {pBass, 3}, {pMid, -2}, {pMidFreq, 400},
        {pTranspose, -7}, {pSub, 0.3f}, {pCabType, CAB_8X10}, {pMicPos, 0.4f}, {pMicDist, 0.85f},
        {pRevMix, 0.32f}, {pRevSize, 0.9f}, {pRevDamp, 0.55f}, {pSustain, 5}, {pGateThresh, -52}, {pGateRelease, 70},
        {pPalmMute, 0.2f}, {pBoostDrive, 0.3f}, {pBoostTight, 160}, {pBoostLevel, -1}, {pHighCut, 7000}, {pWidth, 0.9f}, {pOutput, -6}}},

    {"Industrial Grind", "Cabless, digital-sounding grind: fizzy, upper-mid forward, almost mono.", {
        {pVoicing, V_TECH}, {pGain, 0.6f}, {pBright, 0.5f}, {pBias, 0.2f}, {pSag, 0}, {pCabType, CAB_DI},
        {pHighCut, 6000}, {pBass, 2}, {pMid, 6}, {pMidFreq, 1800}, {pTreble, 2}, {pBoostDrive, 0.7f},
        {pBoostTight, 320}, {pBoostLevel, 1}, {pPalmMute, 0.3f}, {pSustain, 2}, {pGateThresh, -45}, {pGateRelease, 12},
        {pPickAttack, 0.95f}, {pPickNoise, 0.6f}, {pPickPos, 0.1f}, {pChord, C_POWER}, {pStrum, 0}, {pRevMix, 0},
        {pWidth, 0.3f}, {pLowCut, 80}}},

    {"Old Flesh Crunch", "Early-days rhythm tone: low gain, big dynamics, loose and open 1x12 sound.", {
        {pVoicing, V_OLD}, {pGain, 0.38f}, {pBright, 0.5f}, {pBias, 0.7f}, {pSag, 0.65f}, {pBoostOn, 0},
        {pBass, 1}, {pMid, 3}, {pMidFreq, 900}, {pTreble, 1}, {pPresence, 2}, {pCabType, CAB_1X12},
        {pMicPos, 0.5f}, {pMicDist, 0.4f}, {pSustain, 4}, {pDamping, 0.45f}, {pPickAttack, 0.55f},
        {pPickNoise, 0.35f}, {pGateThresh, -60}, {pGateRelease, 90}, {pPalmMute, 0.2f}, {pRevMix, 0.1f}, {pWidth, 0.6f}, {pOutput, -3}}},

    {"Wall of Twin Tracks", "Maximum width: octave power chords, fully double-tracked, strummed.", {
        {pVoicing, V_MODERN}, {pGain, 0.75f}, {pSag, 0.35f}, {pChord, C_POWER_OCT}, {pStrum, 0.25f}, {pWidth, 1.0f},
        {pBass, 2}, {pMid, -2}, {pMidFreq, 600}, {pTreble, 2}, {pMicPos, 0.5f}, {pMicDist, 0.35f},
        {pBoostDrive, 0.3f}, {pBoostTight, 170}, {pSustain, 5}, {pRevMix, 0.1f}}},

    {"Death'n'Roll Swagger", "Mid-tempo groove tone: bright, bitey, a little slap-back for attitude.", {
        {pVoicing, V_OLD}, {pGain, 0.5f}, {pBright, 0.6f}, {pBias, 0.65f}, {pSag, 0.5f}, {pCabType, CAB_GREEN},
        {pMicPos, 0.6f}, {pMicDist, 0.35f}, {pBass, 2}, {pMid, 4}, {pMidFreq, 700}, {pTreble, 2}, {pBoostOn, 0},
        {pSustain, 4}, {pPickPos, 0.45f}, {pPickAttack, 0.6f}, {pPickNoise, 0.45f}, {pPalmMute, 0.15f},
        {pRevMix, 0.12f}, {pRevSize, 0.3f}, {pDelayMix, 0.1f}, {pDelayTime, 140}, {pDelayFb, 0.2f},
        {pWidth, 0.7f}, {pStrum, 0.2f}, {pOutput, -4}}},

    {"Goregrind Mud", "Twelve semitones down, saturated to the limit, gated to a chop.", {
        {pVoicing, V_MODERN}, {pGain, 1.0f}, {pBias, 0.7f}, {pSag, 0.6f}, {pTranspose, -12}, {pBass, 6},
        {pResonance, 8}, {pMid, -5}, {pMidFreq, 500}, {pTreble, -1}, {pCabType, CAB_8X10}, {pMicPos, 0.3f},
        {pMicDist, 0.2f}, {pHighCut, 5000}, {pLowCut, 28}, {pSustain, 2}, {pDamping, 0.5f}, {pGateThresh, -40},
        {pGateRelease, 12}, {pPalmMute, 0.6f}, {pBoostDrive, 0.5f}, {pBoostTight, 100}, {pBoostLevel, -2},
        {pRevMix, 0.04f}, {pWidth, 0.6f}, {pChord, C_POWER}, {pStrum, 0}, {pOutput, -3}}},

    {"Slam Djent-Death", "Staccato, tightly gated, trebly power-chord stabs on a tight 2x12.", {
        {pVoicing, V_TECH}, {pGain, 0.8f}, {pSag, 0.1f}, {pChord, C_POWER}, {pPalmMute, 0.6f}, {pSustain, 1.5f},
        {pBass, 1}, {pMid, -1}, {pMidFreq, 700}, {pTreble, 4}, {pPresence, 2}, {pCabType, CAB_2X12},
        {pMicPos, 0.1f}, {pMicDist, 0.1f}, {pGateThresh, -48}, {pGateRelease, 10}, {pBoostDrive, 0.4f},
        {pBoostTight, 260}, {pBoostLevel, 1}, {pLowCut, 80}, {pPickAttack, 0.8f}, {pPickNoise, 0.3f},
        {pWidth, 0.7f}, {pRevMix, 0.02f}, {pStrum, 0}}},

    {"Necro Basement", "Lo-fi demo-tape rot: no bottom, hissy top, cheap reverb, distant cab.", {
        {pVoicing, V_CHAINSAW}, {pGain, 1.0f}, {pBright, 0.8f}, {pBias, 0.15f}, {pSag, 0}, {pBass, -6}, {pMid, 3},
        {pMidFreq, 900}, {pTreble, 4}, {pCabType, CAB_GREEN}, {pMicPos, 0.8f}, {pMicDist, 0.9f}, {pLowCut, 130},
        {pHighCut, 5000}, {pPickNoise, 0.8f}, {pPickAttack, 0.7f}, {pSustain, 3}, {pGateThresh, -50},
        {pGateRelease, 50}, {pPalmMute, 0.1f}, {pRevMix, 0.25f}, {pRevSize, 0.45f}, {pRevDamp, 0.8f},
        {pBoostOn, 0}, {pWidth, 0.4f}}},

    {"Sweep Lead", "Clear, sustaining solo tone with a touch of delay. Mono legato, no glide.", {
        {pVoicing, V_TECH}, {pGain, 0.68f}, {pBright, 0.4f}, {pVoiceMode, M_MONO}, {pGlide, 0}, {pSustain, 10},
        {pDamping, 0.2f}, {pPickPos, 0.6f}, {pPickAttack, 0.6f}, {pPickNoise, 0.1f}, {pMid, 4}, {pMidFreq, 1400},
        {pTreble, 1}, {pPresence, 2}, {pMicPos, 0.3f}, {pMicDist, 0.3f}, {pBoostDrive, 0.35f}, {pBoostTight, 140},
        {pBoostLevel, 3}, {pGateThresh, -64}, {pGateRelease, 200}, {pDelayMix, 0.2f}, {pDelayTime, 340},
        {pDelayFb, 0.35f}, {pRevMix, 0.18f}, {pSag, 0.15f}, {pWidth, 0.4f}, {pOutput, -7}}},

    {"Crypt Clean", "Near-clean, eerie intro tone: neck pickup, long delay and reverb. Play it soft.", {
        {pVoicing, V_OLD}, {pGain, 0}, {pBright, 0.6f}, {pSag, 0}, {pBoostDrive, 0}, {pBoostTight, 100},
        {pBoostLevel, -12}, {pTreble, 2}, {pCabType, CAB_1X12}, {pMicPos, 0.4f}, {pMicDist, 0.4f}, {pSustain, 7},
        {pDamping, 0.55f}, {pPickAttack, 0.35f}, {pPickNoise, 0.1f}, {pPickPos, 0.7f}, {pDelayMix, 0.3f},
        {pDelayTime, 420}, {pDelayFb, 0.45f}, {pRevMix, 0.3f}, {pRevSize, 0.7f}, {pGateThresh, -80},
        {pGateRelease, 300}, {pWidth, 0.9f}, {pOutput, 4}}},

    {"Thrash Pit", "Death-thrash crossover: pushed mids, tremolo-picked power chords, snappy gate.", {
        {pVoicing, V_BRUTAL}, {pGain, 0.6f}, {pBright, 0.45f}, {pBias, 0.55f}, {pBoostDrive, 0.4f}, {pBoostTight, 200},
        {pBoostLevel, 1}, {pTremRate, 0.5f}, {pChord, C_POWER}, {pStrum, 0.05f}, {pBass, 1}, {pMid, -3},
        {pMidFreq, 600}, {pTreble, 3}, {pMicPos, 0.3f}, {pMicDist, 0.2f}, {pSustain, 3}, {pGateThresh, -52},
        {pGateRelease, 35}, {pPalmMute, 0.3f}, {pPickAttack, 0.75f}, {pPickNoise, 0.35f}, {pRevMix, 0.05f},
        {pWidth, 0.8f}}},

    {"Atmospheric Dirge", "Slow ambient death: 18-second strings, huge delay and reverb, soft octave chords.", {
        {pVoicing, V_OLD}, {pGain, 0.45f}, {pSag, 0.6f}, {pChord, C_POWER_OCT}, {pStrum, 0.6f}, {pSustain, 18},
        {pDamping, 0.55f}, {pBass, 2}, {pTreble, -1}, {pCabType, CAB_GREEN}, {pMicPos, 0.5f}, {pMicDist, 0.6f},
        {pPickPos, 0.55f}, {pPickAttack, 0.3f}, {pDelayMix, 0.3f}, {pDelayTime, 700}, {pDelayFb, 0.6f},
        {pRevMix, 0.45f}, {pRevSize, 0.9f}, {pRevDamp, 0.65f}, {pGateThresh, -70}, {pGateRelease, 400},
        {pBoostOn, 0}, {pLowCut, 35}, {pHighCut, 6500}, {pWidth, 1.0f}, {pOutput, -9}}},

    {"Cathedral of Bones", "Melodic death-doom cathedral: neck-pickup octave chords, long dotted delay, huge hall.", {
        {pVoicing, V_OLD}, {pGain, 0.5f}, {pBright, 0.3f}, {pSag, 0.5f}, {pChord, C_POWER_OCT}, {pStrum, 0.4f},
        {pBass, 2}, {pMid, 2}, {pMidFreq, 900}, {pTreble, 1}, {pCabType, CAB_V30}, {pMicPos, 0.4f}, {pMicDist, 0.5f},
        {pPickPos, 0.7f}, {pPickAttack, 0.4f}, {pSustain, 12}, {pDamping, 0.4f}, {pBoostDrive, 0.2f},
        {pBoostTight, 110}, {pGateThresh, -68}, {pGateRelease, 300}, {pDelayMix, 0.25f}, {pDelayTime, 520},
        {pDelayFb, 0.5f}, {pRevMix, 0.4f}, {pRevSize, 0.95f}, {pRevDamp, 0.5f}, {pWidth, 1.0f}, {pOutput, -9.4}}},

    {"Minor Key Dirge", "Every key plays a full minor triad, slowly strummed. Dark, sad, enormous.", {
        {pVoicing, V_OLD}, {pGain, 0.5f}, {pBright, 0.2f}, {pSag, 0.6f}, {pChord, C_MINOR}, {pStrum, 0.7f},
        {pBass, 3}, {pTreble, -1}, {pCabType, CAB_GREEN}, {pMicPos, 0.5f}, {pMicDist, 0.5f}, {pSustain, 15},
        {pDamping, 0.5f}, {pPickPos, 0.5f}, {pPickAttack, 0.35f}, {pBoostOn, 0}, {pGateThresh, -70}, {pGateRelease, 350},
        {pDelayMix, 0.2f}, {pDelayTime, 640}, {pDelayFb, 0.45f}, {pRevMix, 0.35f}, {pRevSize, 0.85f}, {pRevDamp, 0.6f},
        {pWidth, 1.0f}, {pHighCut, 7500}, {pOutput, -7.8}}},

    {"Tritone Terror", "The devil's interval on every note: tremolo-picked horror-movie dissonance.", {
        {pVoicing, V_BRUTAL}, {pGain, 0.78f}, {pBright, 0.45f}, {pChord, C_TRITONE}, {pStrum, 0}, {pTremRate, 0.6f},
        {pBass, 0}, {pMid, 3}, {pMidFreq, 1100}, {pTreble, 3}, {pPresence, 2}, {pCabType, CAB_V30}, {pMicPos, 0.2f},
        {pMicDist, 0.3f}, {pBoostDrive, 0.4f}, {pBoostTight, 200}, {pSustain, 6}, {pGateThresh, -55},
        {pGateRelease, 60}, {pPickAttack, 0.8f}, {pRevMix, 0.18f}, {pRevSize, 0.6f}, {pWidth, 0.9f}, {pOutput, -8.8}}},

    {"Bowel Shaker", "Two octaves down with a sub string. Mostly felt, not heard: clear the room.", {
        {pVoicing, V_MODERN}, {pGain, 0.9f}, {pBias, 0.6f}, {pSag, 0.7f}, {pTranspose, -24}, {pSub, 0.8f},
        {pBass, 6}, {pResonance, 9}, {pMid, -4}, {pMidFreq, 500}, {pTreble, -3}, {pCabType, CAB_8X10},
        {pMicPos, 0.3f}, {pMicDist, 0.2f}, {pHighCut, 4000}, {pLowCut, 20}, {pSustain, 3}, {pDamping, 0.55f},
        {pGateThresh, -50}, {pGateRelease, 40}, {pPalmMute, 0.3f}, {pBoostDrive, 0.3f}, {pBoostTight, 90},
        {pRevMix, 0.05f}, {pWidth, 0.5f}, {pOutput, -4.4}}},

    {"Machine Gun Blast", "Maximum-speed tremolo picking on power chords, razor gate, every pick audible.", {
        {pVoicing, V_TECH}, {pGain, 0.85f}, {pSag, 0.05f}, {pChord, C_POWER}, {pStrum, 0}, {pTremRate, 1.0f},
        {pPalmMute, 0.45f}, {pSustain, 2}, {pBass, 0}, {pMid, 2}, {pMidFreq, 1000}, {pTreble, 4}, {pPresence, 3},
        {pCabType, CAB_2X12}, {pMicPos, 0.1f}, {pMicDist, 0.1f}, {pBoostDrive, 0.45f}, {pBoostTight, 300},
        {pBoostLevel, 1}, {pGateThresh, -46}, {pGateRelease, 8}, {pPickAttack, 0.95f}, {pPickNoise, 0.4f},
        {pLowCut, 70}, {pWidth, 0.8f}, {pRevMix, 0.02f}, {pOutput, -8.5}}},

    {"Haunted Music Box", "Almost-clean octave-up shimmer with long echoes. Creepy intros and outros.", {
        {pVoicing, V_OLD}, {pGain, 0.05f}, {pBright, 0.7f}, {pSag, 0}, {pTranspose, 12}, {pChord, C_OCT}, {pStrum, 0.3f},
        {pBoostDrive, 0}, {pBoostTight, 150}, {pBoostLevel, -10}, {pTreble, 3}, {pCabType, CAB_1X12}, {pMicPos, 0.2f},
        {pMicDist, 0.5f}, {pSustain, 3.5f}, {pDamping, 0.6f}, {pPickAttack, 0.2f}, {pPickNoise, 0.05f}, {pPickPos, 0.8f},
        {pGateThresh, -80}, {pGateRelease, 400}, {pDelayMix, 0.4f}, {pDelayTime, 300}, {pDelayFb, 0.55f},
        {pRevMix, 0.5f}, {pRevSize, 0.8f}, {pRevDamp, 0.3f}, {pWidth, 1.0f}, {pOutput, -7.3}}},

    {"Megaphone Mortuary", "Telephone-through-a-bullhorn distortion: no lows, no highs, all attitude.", {
        {pVoicing, V_CHAINSAW}, {pGain, 1.0f}, {pBright, 0.5f}, {pBias, 0.4f}, {pSag, 0}, {pCabType, CAB_DI},
        {pLowCut, 260}, {pHighCut, 3200}, {pBass, -8}, {pMid, 8}, {pMidFreq, 1400}, {pTreble, 0}, {pBoostDrive, 0.5f},
        {pBoostTight, 400}, {pSustain, 3}, {pGateThresh, -45}, {pGateRelease, 25}, {pPickNoise, 0.5f},
        {pRevMix, 0.1f}, {pRevSize, 0.3f}, {pWidth, 0.2f}, {pOutput, -5.5}}},

    {"Throat Cut", "Nasal, vocal-like mid peak. Sounds like the guitar is screaming along with the singer.", {
        {pVoicing, V_BRUTAL}, {pGain, 0.75f}, {pBright, 0.2f}, {pBass, -2}, {pMid, 12}, {pMidFreq, 1600}, {pTreble, -2},
        {pPresence, 0}, {pCabType, CAB_1X12}, {pMicPos, 0.6f}, {pMicDist, 0.2f}, {pBoostDrive, 0.35f}, {pBoostTight, 220},
        {pSustain, 5}, {pGateThresh, -55}, {pGateRelease, 50}, {pPickAttack, 0.6f}, {pWidth, 0.6f}, {pRevMix, 0.08f}, {pOutput, -8.5}}},

    {"Frostbitten Cathedral", "Blackened-death tremolo power chords washed in an icy hall of reverb.", {
        {pVoicing, V_CHAINSAW}, {pGain, 0.88f}, {pBright, 0.8f}, {pBias, 0.3f}, {pSag, 0.1f}, {pChord, C_POWER_OCT},
        {pStrum, 0.1f}, {pTremRate, 0.75f}, {pBass, -3}, {pMid, 3}, {pMidFreq, 2000}, {pTreble, 5}, {pPresence, 4},
        {pCabType, CAB_V30}, {pMicPos, 0.1f}, {pMicDist, 0.6f}, {pBoostDrive, 0.55f}, {pBoostTight, 280},
        {pSustain, 9}, {pGateThresh, -58}, {pGateRelease, 150}, {pDelayMix, 0.15f}, {pDelayTime, 480}, {pDelayFb, 0.4f},
        {pRevMix, 0.5f}, {pRevSize, 0.95f}, {pRevDamp, 0.25f}, {pLowCut, 110}, {pHighCut, 15000}, {pPickAttack, 0.9f},
        {pWidth, 1.0f}, {pOutput, -13.3}}},

    {"Slide Guitar From Hell", "Mono slides between notes with feedback echoes and a squeal. Overlap notes to slur.", {
        {pVoicing, V_BRUTAL}, {pGain, 0.6f}, {pBright, 0.5f}, {pVoiceMode, M_MONO}, {pGlide, 180}, {pPinch, 0.5f},
        {pSustain, 14}, {pDamping, 0.2f}, {pPickPos, 0.6f}, {pPickAttack, 0.5f}, {pPickNoise, 0.05f}, {pMid, 4},
        {pMidFreq, 1300}, {pTreble, 1}, {pBendRange, 12}, {pBoostDrive, 0.3f}, {pBoostTight, 120}, {pBoostLevel, 3},
        {pGateThresh, -66}, {pGateRelease, 250}, {pDelayMix, 0.35f}, {pDelayTime, 450}, {pDelayFb, 0.6f},
        {pRevMix, 0.3f}, {pRevSize, 0.7f}, {pSag, 0.3f}, {pWidth, 0.5f}, {pOutput, -9.3}}},

    {"Pit of Fuzz", "Broken, spitting asymmetric fuzz: maximum bias, heavy sag, boost cooked to the limit.", {
        {pVoicing, V_CHAINSAW}, {pGain, 1.0f}, {pBright, 0.4f}, {pBias, 1.0f}, {pSag, 0.9f}, {pBoostDrive, 0.9f},
        {pBoostTight, 100}, {pBoostLevel, -3}, {pBass, 2}, {pMid, -2}, {pMidFreq, 700}, {pTreble, 2}, {pCabType, CAB_GREEN},
        {pMicPos, 0.5f}, {pMicDist, 0.3f}, {pSustain, 7}, {pGateThresh, -55}, {pGateRelease, 90}, {pPickNoise, 0.5f},
        {pWidth, 0.7f}, {pRevMix, 0.1f}, {pOutput, -5.7}}},

    {"Crushed Cassette", "Worn-out four-track demo tape: thin, dark, noisy picks, everything pressed together.", {
        {pVoicing, V_OLD}, {pGain, 0.8f}, {pBright, 0.6f}, {pBias, 0.8f}, {pSag, 0.8f}, {pCabType, CAB_DI},
        {pLowCut, 250}, {pHighCut, 3500}, {pBass, -3}, {pMid, 5}, {pMidFreq, 1000}, {pPickNoise, 1.0f}, {pPickAttack, 0.5f},
        {pSustain, 3}, {pGateThresh, -52}, {pGateRelease, 70}, {pBoostDrive, 0.5f}, {pBoostTight, 250},
        {pDelayMix, 0.1f}, {pDelayTime, 90}, {pDelayFb, 0.5f}, {pRevMix, 0.2f}, {pRevSize, 0.3f}, {pRevDamp, 0.9f},
        {pWidth, 0.3f}, {pOutput, -2.6}}},

    {"Fourths of Doom", "Parallel fourths instead of fifths: open, ominous, slightly Eastern. Wide and strummed.", {
        {pVoicing, V_MODERN}, {pGain, 0.7f}, {pSag, 0.4f}, {pChord, C_POWER4}, {pStrum, 0.35f}, {pBass, 2}, {pMid, -1},
        {pMidFreq, 700}, {pTreble, 2}, {pCabType, CAB_V30}, {pMicPos, 0.4f}, {pMicDist, 0.4f}, {pSustain, 7},
        {pBoostDrive, 0.3f}, {pBoostTight, 150}, {pGateThresh, -60}, {pGateRelease, 120}, {pDelayMix, 0.12f},
        {pDelayTime, 380}, {pDelayFb, 0.35f}, {pRevMix, 0.25f}, {pRevSize, 0.7f}, {pWidth, 1.0f}, {pOutput, -7.2}}},

    {"Ritual Drone", "Twenty-second strings, endless echo and a held sub: hold one chord and let it breathe.", {
        {pVoicing, V_OLD}, {pGain, 0.35f}, {pBright, 0.1f}, {pSag, 0.5f}, {pChord, C_POWER_OCT}, {pStrum, 0.8f},
        {pSub, 0.5f}, {pBass, 3}, {pTreble, -3}, {pCabType, CAB_GREEN}, {pMicPos, 0.6f}, {pMicDist, 0.7f},
        {pSustain, 20}, {pDamping, 0.6f}, {pPickAttack, 0.2f}, {pPickPos, 0.6f}, {pBoostOn, 0}, {pGateThresh, -80},
        {pGateRelease, 500}, {pDelayMix, 0.45f}, {pDelayTime, 900}, {pDelayFb, 0.7f}, {pRevMix, 0.6f}, {pRevSize, 1.0f},
        {pRevDamp, 0.7f}, {pHighCut, 5500}, {pLowCut, 30}, {pWidth, 1.0f}, {pOutput, -12.0}}},

    {"Shattered Glass", "Ice-thin, bridge-pickup lead with a razor edge. Cuts through any mix.", {
        {pVoicing, V_TECH}, {pGain, 0.6f}, {pBright, 0.8f}, {pBias, 0.3f}, {pSag, 0}, {pVoiceMode, M_MONO}, {pGlide, 10},
        {pBass, -4}, {pMid, 2}, {pMidFreq, 2200}, {pTreble, 8}, {pPresence, 5}, {pCabType, CAB_2X12}, {pMicPos, 0.0f},
        {pMicDist, 0.1f}, {pPickPos, 0.0f}, {pPickAttack, 1.0f}, {pPickNoise, 0.15f}, {pSustain, 9}, {pDamping, 0.15f},
        {pBoostDrive, 0.35f}, {pBoostTight, 320}, {pBoostLevel, 3}, {pGateThresh, -62}, {pGateRelease, 180},
        {pLowCut, 140}, {pHighCut, 18000}, {pDelayMix, 0.18f}, {pDelayTime, 330}, {pDelayFb, 0.3f}, {pRevMix, 0.2f},
        {pRevSize, 0.5f}, {pWidth, 0.3f}, {pOutput, -11.5}}},

    {"Grave Robber Slapback", "Short slap-back echo on a loose old-school crunch: the sound of digging by lantern.", {
        {pVoicing, V_OLD}, {pGain, 0.45f}, {pBright, 0.5f}, {pBias, 0.7f}, {pSag, 0.6f}, {pBoostOn, 0}, {pBass, 1},
        {pMid, 3}, {pMidFreq, 800}, {pTreble, 1}, {pCabType, CAB_1X12}, {pMicPos, 0.5f}, {pMicDist, 0.5f},
        {pSustain, 4}, {pPickNoise, 0.4f}, {pPalmMute, 0.2f}, {pGateThresh, -58}, {pGateRelease, 80}, {pDelayMix, 0.4f},
        {pDelayTime, 95}, {pDelayFb, 0.12f}, {pRevMix, 0.15f}, {pRevSize, 0.3f}, {pWidth, 0.6f}, {pOutput, -2.9}}},
    };
    return t;
}

inline int numPresets() { return (int)presetTable().size(); }
inline const char* presetName(int i) { return presetTable()[i].name.c_str(); }
inline const char* presetDesc(int i) { return presetTable()[i].desc.c_str(); }
inline void presetValues(int i, float* out) {
    for (int p = 0; p < kNumParams; ++p) out[p] = kParams[p].defV;
    for (const PO& o : presetTable()[i].o) out[o.id] = o.v;
}

} // namespace dg

/**
    bespoke synth, a software modular synthesizer
    Copyright (C) 2021 Ryan Challinor (contact: awwbees@gmail.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/
//
//  PolyphonyMgr.h
//  additiveSynth
//
//  Created by Ryan Challinor on 11/20/12.
//
//

#ifndef __additiveSynth__PolyphonyMgr__
#define __additiveSynth__PolyphonyMgr__

#include <memory>
#include "SynthGlobals.h"
#include "ChannelBuffer.h"

const int kVoiceFadeSamples = 50;

extern ChannelBuffer gMidiVoiceWorkChannelBuffer;

class IMidiVoice;
class IVoiceParams;
class IDrawableModule;
struct ModulationParameters;

struct VoiceInfo
{
   float mPitch{ -1 };
   std::unique_ptr<IMidiVoice> mVoice{ nullptr };
   double mTime{ 0 };
   bool mNoteOn{ false };
   float mActivity{ 0 };
};

using VoiceConstructor = std::unique_ptr<IMidiVoice> (*)(IDrawableModule* owner);

class PolyphonicVoiceScheduler
{
public:
   PolyphonicVoiceScheduler(IDrawableModule* owner);

   void Init(VoiceConstructor type,
             IVoiceParams* mVoiceParams);

   void Start(double time, int pitch, float amount, int voiceIdx, ModulationParameters modulation);
   void Stop(double time, int pitch, int voiceIdx);
   void Process(double time, ChannelBuffer* out, int bufferSize);
   void DrawDebug(float x, float y);
   void SetVoiceLimit(int limit) { mVoiceLimit = limit; }
   void KillAll();
   void SetOversampling(int oversampling) { mOversampling = oversampling; }

private:
   VoiceInfo mVoices[kNumVoices];
   bool mAllowStealing{ true };
   int mLastVoice{ -1 };
   ChannelBuffer mFadeOutBuffer{ kVoiceFadeSamples };
   ChannelBuffer mFadeOutWorkBuffer{ kVoiceFadeSamples };
   int mFadeOutBufferPos{ 0 };
   IDrawableModule* mOwner;
   int mVoiceLimit{ kNumVoices };
   int mOversampling{ 1 };
};

#endif /* defined(__additiveSynth__PolyphonyMgr__) */

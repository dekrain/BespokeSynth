/**
    bespoke synth, a software modular synthesizer
    Copyright (C) 2024 Ryan Challinor (contact: awwbees@gmail.com)

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
//  PolyphonyScheduler.h
//
//  Created by Dawid Kraiński on 24 Mar 2024.
//  Based on PolyphonyMgr.h by Ryan Challinor.
//

#pragma once

#include "SynthGlobals.h"

struct ModulationParameters;

struct SchedVoiceInfo
{
   float mPitch{ -1 };
   double mTime{ 0 };
   bool mNoteOn{ false };
};

// TODO: Delet
class IPolyphonyReceiver
{
public:
   virtual void StartVoice(size_t voiceId, double time, int pitch, float amount, ModulationParameters modulations) = 0;
   virtual void StopVoice(size_t voiceId, float pitch, double time) = 0;
};

class PolyphonyScheduler
{
public:
   PolyphonyScheduler(IPolyphonyReceiver* receiver) : mReceiver(receiver) {}

   void Start(double time, int pitch, float amount, int voiceIdx, ModulationParameters modulation);
   void Stop(double time, int pitch, int voiceIdx);
   void DrawDebug(float x, float y);
   void SetVoiceLimit(int limit) { mVoiceLimit = limit; }
   void KillAll();

private:
   SchedVoiceInfo mVoices[kNumVoices];
   bool mAllowStealing{ true };
   int mLastVoice{ -1 };
   int mVoiceLimit{ kNumVoices };
   IPolyphonyReceiver* mReceiver;
};

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
//  PolyphonyScheduler.cpp
//
//  Created by Dawid Kraiński on 24 Mar 2024.
//  Based on PolyphonyMgr.cpp by Ryan Challinor.
//

#include "PolyphonyScheduler.h"

#include "ModulationChain.h"

void PolyphonyScheduler::Start(double time, int pitch, float amount, int voiceIdx, ModulationParameters modulation)
{
   assert(voiceIdx < kNumVoices);

   bool preserveVoice = voiceIdx != -1 && //we specified a voice
                        mVoices[voiceIdx].mPitch != -1; //there is a note playing from that voice

   if (voiceIdx == -1) //need a new voice
   {
      for (int i = 0; i < mVoiceLimit; ++i)
      {
         int check = (i + mLastVoice + 1) % mVoiceLimit; //try to keep incrementing through list to allow old voices to finish
         if (mVoices[check].mPitch == -1)
         {
            voiceIdx = check;
            break;
         }
      }
   }

   if (voiceIdx == -1) //all used
   {
      if (mAllowStealing)
      {
         double oldest = mVoices[0].mTime;
         int oldestIndex = 0;
         for (int i = 1; i < mVoiceLimit; ++i)
         {
            if (mVoices[i].mTime < oldest)
            {
               oldest = mVoices[i].mTime;
               oldestIndex = i;
            }
         }
         voiceIdx = oldestIndex;
      }
      else
      {
         return;
      }
   }

   /*if (!voice.IsDone(time) && (!preserveVoice || modulation.pan != voice.GetPan()))
   {
      //ofLog() << "fading stolen voice " << voiceIdx << " at " << time;
   }
   if (!preserveVoice)
      voice.ClearVoice();*/
   /*voice.SetPitch(pitch);
   voice.SetModulators(modulation);
   voice.Start(time, amount);
   voice.SetPan(modulation.pan);*/
   mLastVoice = voiceIdx;

   mVoices[voiceIdx].mPitch = pitch;
   mVoices[voiceIdx].mTime = time;
   mVoices[voiceIdx].mNoteOn = true;

   mReceiver->StartVoice(voiceIdx, time, pitch, amount, modulation);
}

void PolyphonyScheduler::Stop(double time, int pitch, int voiceIdx)
{
   if (voiceIdx == -1)
   {
      double oldest = std::numeric_limits<double>::max();
      for (int i = 0; i < mVoiceLimit; ++i)
      {
         if (mVoices[i].mPitch == pitch && mVoices[i].mNoteOn && mVoices[i].mTime < oldest)
         {
            oldest = mVoices[i].mTime;
            voiceIdx = i;
         }
      }
   }
   if (voiceIdx > -1 && mVoices[voiceIdx].mPitch == pitch && mVoices[voiceIdx].mNoteOn)
   {
      //mVoices[voiceIdx].mVoice->Stop(time);
      mVoices[voiceIdx].mNoteOn = false;
      mVoices[voiceIdx].mPitch = -1;

      mReceiver->StopVoice(voiceIdx, mVoices[voiceIdx].mPitch, mVoices[voiceIdx].mTime);
   }
}

void PolyphonyScheduler::KillAll()
{
   for (int i = 0; i < kNumVoices; ++i)
   {
      //mVoices[i].mVoice->ClearVoice();
      if (mVoices[i].mNoteOn)
      {
         mVoices[i].mNoteOn = false;
         mVoices[i].mPitch = -1;

         mReceiver->StopVoice(i, mVoices[i].mPitch, mVoices[i].mTime);
      }
   }
}

void PolyphonyScheduler::DrawDebug(float x, float y)
{
   ofPushMatrix();
   ofPushStyle();
   ofTranslate(x, y);
   for (int i = 0; i < kNumVoices; ++i)
   {
      if (mVoices[i].mPitch == -1)
         ofSetColor(100, 100, 100);
      else if (mVoices[i].mNoteOn)
         ofSetColor(0, 255, 0);
      else
         ofSetColor(255, 0, 0);
      std::string outputLine = "voice " + ofToString(i);
      if (mVoices[i].mPitch == -1)
         outputLine += " unused";
      else
         outputLine += " used: " + ofToString(mVoices[i].mPitch) + (mVoices[i].mNoteOn ? " (note on)" : " (note off)");
      DrawTextNormal(outputLine, 0, i * 18);
   }
   ofPopStyle();
   ofPopMatrix();
}

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
/*
   ==============================================================================

   PolyphonicContainer.h
   Created: 25 Mar 2024
   Author:  Dawid Kraiński

   ==============================================================================
*/

#pragma once

#include "ClickButton.h"
#include "IDrawableModule.h"
#include "INoteReceiver.h"
#include "ModuleContainer.h"
#include "PolyphonyScheduler.h"
#include "SynthGlobals.h"

struct PolyphonicVoiceContainer;

class PolyphonicContainer : public IDrawableModule, public INoteReceiver, public IButtonListener, public IPolyphonyReceiver
{
public:
   PolyphonicContainer();
   ~PolyphonicContainer();
   static IDrawableModule* Create() { return new PolyphonicContainer(); }

   static bool AcceptsNotes() { return true; }

   void CreateUIControls() override;
   void SetEnabled(bool enabled) override;

   ModuleContainer* GetContainer() override { return &mModuleContainer; }

   void Poll() override;
   bool ShouldClipContents() override { return false; }

   int GetModuleSaveStateRev() const override { return 0; }

   bool HasDebugDraw() const override { return true; }

   //IPatchable
   void PostRepatch(PatchCableSource* cableSource, bool fromUserClick) override;

   bool IsEnabled() const override { return mEnabled; }

   //INoteReceiver
   void PlayNote(double time, int pitch, int velocity, int voiceIdx = -1, ModulationParameters modulation = ModulationParameters()) override;
   void SendPressure(int pitch, int pressure) override;
   void SendCC(int control, int value, int voiceIdx = -1) override;
   void SendMidi(const juce::MidiMessage &message) override;

   //IPolyphonyReceiver
   void StartVoice(size_t voiceId, double time, int pitch, float amount, ModulationParameters modulations) override;
   void StopVoice(size_t voiceId, float pitch, double time) override;

   //IButtonListener
   void ButtonClicked(ClickButton *button, double time) override;

private:
   //IDrawableModule
   void PreDrawModule() override;
   void DrawModule() override;
   void DrawModuleUnclipped() override;
   void GetModuleDimensions(float& width, float& height) override;
   void OnClicked(float x, float y, bool right) override;
   void MouseReleased() override;

   bool CanAddDropModules();
   bool IsAddableModule(IDrawableModule* module);
   bool IsMouseHovered();

   void TakeModule(IDrawableModule* module);
   void ReleaseModule(IDrawableModule* module);

   PolyphonyScheduler mNoteScheduler;
   PatchCableSource* mRemoveModuleCable{ nullptr };
   PatchCableSource* mVoiceNoteCable{ nullptr };
   ClickButton* mDisbandButton{ nullptr };
   ModuleContainer mModuleContainer;
   std::unique_ptr<PolyphonicVoiceContainer> mVoices[kNumVoices];
};

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

   PolyphonicContainer.cpp
   Created: 26 Mar 2024
   Author:  Dawid Kraiński

  ==============================================================================
*/

#include "PolyphonicContainer.h"

#include <unordered_map>

#include "ClickButton.h"
#include "IDrawableModule.h"
#include "INoteSource.h"
#include "OpenFrameworksPort.h"
#include "PatchCable.h"
#include "PatchCableSource.h"
#include "Prefab.h"
#include "SynthGlobals.h"
#include "ModularSynth.h"

static constexpr float paddingX = 10;
static constexpr float paddingY = 10;

struct PolyphonicVoiceContainer : public IDrawableModule {
   ModuleContainer container;
   PatchCableSource source;
   AdditionalNoteCable voiceCable;
   std::unordered_map<IDrawableModule*, IDrawableModule*> module_map;

   PolyphonicVoiceContainer(PolyphonicContainer* container, const char* name)
      : source(this, kConnectionType_Note)
   {
      container->AddChild(this);
      //SetParent(container);
      SetName(name);
      this->container.SetOwner(this);
      voiceCable.SetPatchCableSource(&source);
   }

   //IDrawableModule
   void DrawModule() override {}
};

PolyphonicContainer::PolyphonicContainer()
   : mNoteScheduler(this)
{
   mModuleContainer.SetOwner(this);
}

PolyphonicContainer::~PolyphonicContainer() {}

void PolyphonicContainer::CreateUIControls()
{
   IDrawableModule::CreateUIControls();

   mDisbandButton = new ClickButton(this, "disband", -1, -1);

   mRemoveModuleCable = new PatchCableSource(this, kConnectionType_Special);
   mRemoveModuleCable->SetManualPosition(10, 10);
   AddPatchCableSource(mRemoveModuleCable);

   mVoiceNoteCable = new PatchCableSource(this, kConnectionType_Note);
   mVoiceNoteCable->SetManualSide(PatchCableSource::Side::kBottom);
   AddPatchCableSource(mVoiceNoteCable);
}

void PolyphonicContainer::SetEnabled(bool enabled) {
   mEnabled = enabled;
   //mModuleContainer.GetAllModules()
}

void PolyphonicContainer::Poll()
{
   float xMin, yMin;
   GetPosition(xMin, yMin);
   for (auto* module : mModuleContainer.GetModules())
   {
      xMin = MIN(xMin, module->GetPosition().x - paddingX);
      yMin = MIN(yMin, module->GetPosition().y - 30);
   }

   int xOffset = GetPosition().x - xMin;
   int yOffset = GetPosition().y - yMin;
   for (auto* module : mModuleContainer.GetModules())
      module->SetPosition(module->GetPosition(K(local)).x + xOffset, module->GetPosition(K(local)).y + yOffset);

   if (abs(GetPosition().x - xMin) >= 1 || abs(GetPosition().y - yMin) >= 1)
      SetPosition(xMin, yMin);

   for (auto& voice : mVoices)
   {
      if (!voice)
         continue;

      voice->BasePoll();
   }
}


bool PolyphonicContainer::IsMouseHovered()
{
   return GetRect(!K(local)).contains(TheSynth->GetMouseX(GetOwningContainer()), TheSynth->GetMouseY(GetOwningContainer()));
}

bool PolyphonicContainer::CanAddDropModules()
{
   if (!IsMouseHovered() || TheSynth->IsGroupSelecting())
      return false;

   if (IsAddableModule(TheSynth->GetMoveModule()))
      return true;

   if (IsAddableModule(Prefab::sJustReleasedModule))
      return true;

   if (!TheSynth->GetGroupSelectedModules().empty())
   {
      for (auto* module : TheSynth->GetGroupSelectedModules())
      {
         if (module == this)
            return false;
         if (IsAddableModule(module))
            return true;
      }
   }

   return false;
}

bool PolyphonicContainer::IsAddableModule(IDrawableModule* module)
{
   if (!module || module == this)
      return false;

   if (module->IsSingleton())
      return false;

   for (auto* parent = module->GetParent(); parent; parent = parent->GetParent())
   {
      if (parent == this)
         return false;
   }

   return true;
}


void PolyphonicContainer::TakeModule(IDrawableModule* module)
{
   mModuleContainer.TakeModule(module);
   for (auto* source : module->GetPatchCableSources())
   {
      // TODO: Maybe store enabled cables in a vector somewhere?
      //source->SetEnabled(false);
   }
}

void PolyphonicContainer::ReleaseModule(IDrawableModule* module)
{
   GetOwningContainer()->TakeModule(module);
   for (auto* source : module->GetPatchCableSources())
   {
      // TODO: Figure out what the value could be
      //source->SetEnabled(true);
   }
}


void PolyphonicContainer::OnClicked(float x, float y, bool right)
{
   IDrawableModule::OnClicked(x, y, right);

   if (y > 0 && !right)
      TheSynth->SetGroupSelectContext(&mModuleContainer);
}

void PolyphonicContainer::MouseReleased()
{
   IDrawableModule::MouseReleased();

   if (CanAddDropModules())
   {
      if (IsAddableModule(Prefab::sJustReleasedModule))
         TakeModule(Prefab::sJustReleasedModule);

      for (auto* module : TheSynth->GetGroupSelectedModules())
      {
         if (IsAddableModule(module))
            TakeModule(module);
      }
   }
}

void PolyphonicContainer::PreDrawModule()
{
   float width, height;
   //GetPosition(x, y);
   GetDimensions(width, height);
   mVoiceNoteCable->SetManualPosition(width/2, 10.f);
   float disbandWidth, disbandHeight;
   mDisbandButton->GetDimensions(disbandWidth, disbandHeight);
   mDisbandButton->SetPosition(width - disbandWidth - paddingX, 2);

   if (!mDrawDebug)
      return;

   constexpr static float sLabelHeight = 0.0; //10.0;
   float baseX = 0, baseY = height + paddingY;
   for (size_t voice = 0; voice != kNumVoices; ++voice)
   {
      if (!mVoices[voice])
         continue;

      mVoices[voice]->SetPosition(baseX / 3., (baseY + sLabelHeight) / 3.);
      //mVoices[voice]->container.DrawContents();
      mVoices[voice]->container.DrawPatchCables(!K(parentMinimized), !K(inFront));
      ofPushMatrix();
      ofTranslate(baseX + mX, baseY + sLabelHeight + mY);
      mVoices[voice]->container.DrawModules();
      ofPopMatrix();
      mVoices[voice]->container.DrawPatchCables(!K(parentMinimized), K(inFront));
      ofPushMatrix();
      ofTranslate(baseX + mX, baseY + sLabelHeight + mY);
      mVoices[voice]->container.DrawUnclipped();
      ofPopMatrix();

      baseX += width + paddingY;
   }
}

void PolyphonicContainer::DrawModule()
{
   if (Minimized() || !IsVisible())
      return;

   if (TheSynth->IsMouseButtonHeld(1) && CanAddDropModules())
   {
      ofPushStyle();
      ofSetColor(255, 255, 255, 80);
      ofFill();
      ofRect(0, 0, GetRect().width, GetRect().height);
      ofPopStyle();
   }

   mDisbandButton->Draw();
   DrawTextNormal("remove", 18, 14);

   mModuleContainer.DrawModules();
}

void PolyphonicContainer::DrawModuleUnclipped()
{
   mModuleContainer.DrawUnclipped();

   if (mDrawDebug)
   {
      float width, height;
      GetDimensions(width, height);
      mNoteScheduler.DrawDebug(width + paddingX, 0);

      constexpr static float sLabelHeight = 0.0; //10.0;
      float baseX = 0, baseY = height + paddingY;
      for (size_t voice = 0; voice != kNumVoices; ++voice)
      {
         if (!mVoices[voice])
            continue;

         ofPushStyle();
         ofSetColor(0.7 * 255.f);
         juce::String voiceLabel("Voice ");
         voiceLabel << voice;
         DrawTextNormal(voiceLabel.toStdString(), baseX, baseY);
         ofPopStyle();

         //if (mVoices[voice])
         /*{
            mVoices[voice]->SetPosition(baseX / 3., (baseY + sLabelHeight) / 3.);
            //mVoices[voice]->container.DrawContents();
            mVoices[voice]->container.DrawPatchCables(!K(parentMinimized), !K(inFront));
            ofPushMatrix();
            ofTranslate(baseX, baseY + sLabelHeight);
            mVoices[voice]->container.DrawModules();
            ofPopMatrix();
            mVoices[voice]->container.DrawPatchCables(!K(parentMinimized), K(inFront));
            ofPushMatrix();
            ofTranslate(baseX, baseY + sLabelHeight);
            mVoices[voice]->container.DrawUnclipped();
            ofPopMatrix();
         }*/

         baseX += width + paddingY;
      }
   }
}

void PolyphonicContainer::PostRepatch(PatchCableSource* cableSource, bool fromUserClick)
{
   if (cableSource == mRemoveModuleCable)
   {
      IDrawableModule* module = dynamic_cast<IDrawableModule*>(cableSource->GetTarget());
      cableSource->Clear();

      if (!module)
         return;

      if (VectorContains(module, mModuleContainer.GetModules()))
      {
         ReleaseModule(module);
         GetOwningContainer()->MoveToFront(this);
      }

      return;
   }

   //cableSource->SetEnabled(false);
}

void PolyphonicContainer::GetModuleDimensions(float& width, float& height)
{
   float x, y;
   GetPosition(x, y);
   width = 215;
   height = 40;

   for (auto* module : mModuleContainer.GetModules())
   {
      ofRectangle rect = module->GetRect();
      if (rect.x - x + rect.width + paddingX > width)
         width = rect.x - x + rect.width + paddingX;
      if (rect.y - y + rect.height + paddingY > height)
         height = rect.y - y + rect.height + paddingY;
   }
}

void PolyphonicContainer::ButtonClicked(ClickButton* button, double time)
{
   if (button == mDisbandButton)
   {
      for (auto* module : mModuleContainer.GetModules())
      {
         ReleaseModule(module);
      }
      GetOwningContainer()->DeleteModule(this);
   }
}


void PolyphonicContainer::PlayNote(double time, int pitch, int velocity, int voiceIdx, ModulationParameters modulation) {
   if (!mEnabled)
      return;

   // We can pass velocity raw, as we don't need need it directly.
   if (velocity > 0)
      mNoteScheduler.Start(time, pitch, velocity, voiceIdx, modulation);
   else
      mNoteScheduler.Stop(time, pitch, voiceIdx);
}

void PolyphonicContainer::SendPressure(int pitch, int pressure) {
   for (auto& voice : mVoices)
   {
      if (!voice)
         continue;

      for (auto noteReceiver : voice->source.GetNoteReceivers())
         noteReceiver->SendPressure(pitch, pressure);
   }
}

void PolyphonicContainer::SendCC(int control, int value, int voiceIdx) {
   for (auto& voice : mVoices)
   {
      if (!voice)
         continue;

      for (auto noteReceiver : voice->source.GetNoteReceivers())
         noteReceiver->SendCC(control, value);
   }
}

void PolyphonicContainer::SendMidi(const juce::MidiMessage &message) {
   for (auto& voice : mVoices)
   {
      if (!voice)
         continue;

      for (auto noteReceiver : voice->source.GetNoteReceivers())
         noteReceiver->SendMidi(message);
   }
}

void PolyphonicContainer::StartVoice(size_t voiceId, double time, int pitch, float amount, ModulationParameters modulations)
{
   juce::String name = "voice";
   name << voiceId;
   auto* voice = new PolyphonicVoiceContainer(this, name.getCharPointer());
   mVoices[voiceId].reset(voice);

   // Duplicate modules
   juce::MemoryBlock state;
   auto modulesLayout = mModuleContainer.WriteModules();
   {
      FileStreamOut out(state);
      mModuleContainer.SaveState(out);
   }

   //IClickable::sPathLoadContext = Path() + "~";
   voice->container.LoadModules(modulesLayout);
   {
      FileStreamIn in(state);
      voice->container.LoadState(in);
   }

   // Only now set the owner
   //voice->container.SetOwner(voice);

   {
      std::vector<IDrawableModule*> allModules;
      voice->container.GetAllModules(allModules);

      // Map all modules
      IClickable::SetSaveContext(voice);
      //IClickable::SetSaveContext(this);
      //IClickable::SetLoadContext(voice);
      for (auto module : allModules)
      {
         auto path = module->Path();
         IDrawableModule* original = mModuleContainer.FindModule(path);
         if (!original)
         {
            TheSynth->LogEvent("Couldn't find module \"" + path + "\" in voice", kLogEventType_Error);
         }
         else
         {
            auto res = voice->module_map.try_emplace(original, module);
            if (!res.second)
            {
               TheSynth->LogEvent("Found duplicate of module \"" + path + "\" in voice", kLogEventType_Error);
            }
         }
      }
      IClickable::ClearSaveContext();
      //IClickable::ClearLoadContext();

      // Patch the cables
      #if 0
      for (auto* module : allModules)
      {
         for (auto* cable : mVoiceNoteCable->GetPatchCables())
         {
            if (cable->GetTarget() == module)
            {
               voice->source.AddPatchCable(cable->GetTarget());
               mVoiceNoteCable->RemovePatchCable(cable);
               break;
            }
         }

         // Right now, only modules can be targets of note cables
         //for (auto* source : module->GetUIControls())
         //{
         //}
      }
      #endif

      // Repatch note source to point to remapped voice module
      for (auto* cable : mVoiceNoteCable->GetPatchCables())
      {
         if (auto it = voice->module_map.find(dynamic_cast<IDrawableModule*>(cable->GetTarget())); it != voice->module_map.cend())
         {
            voice->source.AddPatchCable(it->second);
         }
      }
   }

   // Play the note
   voice->voiceCable.PlayNoteOutput(time, pitch, (int)amount, voiceId, modulations);
}

void PolyphonicContainer::StopVoice(size_t voiceId, float pitch, double time)
{
   auto voice = std::move(mVoices[voiceId]);
   if (!voice)
      return;

   RemoveChild(voice.get());
   voice->container.Clear();
}

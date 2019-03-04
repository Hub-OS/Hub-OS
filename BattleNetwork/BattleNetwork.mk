##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=BattleNetwork
ConfigurationName      :=Debug
WorkspacePath          :=$(CURDIR)
ProjectPath            :=$(CURDIR)
IntermediateDirectory  :=./Debug
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=mav
Date                   :=03/03/19
CodeLitePath           :=/home/mav/.codelite
LinkerName             :=g++
SharedObjectLinkerName :=g++
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputFile             :=$(IntermediateDirectory)/$(ProjectName)
Preprocessors          :=
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="BattleNetwork.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  -O0
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). $(IncludeSwitch)./../extern/includes 
IncludePCH             := 
RcIncludePath          := 
Libs                   := $(LibrarySwitch)sfml-window $(LibrarySwitch)sfml-graphics $(LibrarySwitch)sfml-audio $(LibrarySwitch)sfml-system 
ArLibs                 := "sfml-window" "sfml-graphics" "sfml-audio" "sfml-system"
LibPath                := $(LibraryPathSwitch). $(LibraryPathSwitch). $(LibraryPathSwitch)Debug $(LibraryPathSwitch)../extern/libs 


##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := /usr/bin/x86_64-linux-gnu-ar rcu
CXX      := g++
CC       := gcc
CXXFLAGS := -fPIC -shared -g -Wall -Wno-sign-compare -Wno-unused-variable -Wno-unused-value -Wno-unused-function -Wno-reorder -Wno-switch -std=c++11 $(Preprocessors) -D_GLIBCXX_USE_CXX11_ABI=0
CFLAGS   :=   $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/x86_64-linux-gnu-as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/bnObstacle.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnHitBox.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnCanodumb.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnGear.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnAuraHealthUI.cpp$(ObjectSuffix) $(IntermediateDirectory)/main.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnRollHeart.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnLanBackground.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnChipDescriptionTextbox.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnOverworldMap.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/bnStarman.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnCamera.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnAudioResourceManager.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnExplosion.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnRollHeal.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnStarfishIdleState.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnMetalManBossFight2.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnElecpulse.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnFakeScene.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnStarfishAttackState.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/bnRowHit.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnMettaurIdleState.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnSelectMobScene.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnMainMenuScene.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnCounterHitListener.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnCounterHitPublisher.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnGuardHit.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnCharacter.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnMettaurAttackState.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnThunder.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/bnMetalMan.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnElementalDamage.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnTextureResourceManager.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnPlayer.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnMettaur.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnCannon.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnCanodumbAttackState.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnChargedBusterHit.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnComponent.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnEnemyChipsUI.cpp$(ObjectSuffix) \
	

Objects1=$(IntermediateDirectory)/bnInfiniteMap.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnLibraryScene.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnPanelGrab.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnFolderEditScene.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnDefenseAura.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnParticlePoof.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnMobHealthUI.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnWave.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnDefenseGuard.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnCanodumbMob.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/bnProgsManIdleState.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnMetalManPunchState.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnProgsMan.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnStarfishMob.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnGameOverScene.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnNinjaStar.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnUndernetBackground.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnArtifact.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnHideUntil.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnEntity.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/bnShineExplosion.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnGraveyardBackground.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnPlayerHitState.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnMysteryData.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnMettaurMoveState.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnChipSelectionCust.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnBuster.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnPlayerHealthUI.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnInputManager.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnRoll.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/bnChipUseListener.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnSmartShader.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnCanonSmoke.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnAnimationComponent.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnOverworldLight.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnDefenseAntiDamage.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnHideTimer.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnProgsManPunchState.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnBubble.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnChipLibrary.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/bnChipUsePublisher.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnStarfish.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnBasicSword.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnRandomMettaurMob.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnProgsManShootState.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnAirShot.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnMetalManMoveState.cpp$(ObjectSuffix) 

Objects2=$(IntermediateDirectory)/bnVirusBackground.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnProgsManBossFight.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnProgBomb.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/bnTwoMettaurMob.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnMetalManIdleState.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnSelectedChipsUI.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnNaviRegistration.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnChipFolder.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnCustEmblem.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnShaderResourceManager.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnDefenseBubbleWrap.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnChip.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnAura.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/bnProgsManThrowState.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnField.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnPlayerControlledState.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnFishy.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnChargeComponent.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnGridBackground.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnMetalManBossFight.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnProgsManHitState.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnAnimatedCharacter.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnAnimate.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/bnEngine.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnSpell.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnSelectNaviScene.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnFolderScene.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnMetalBlade.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnBattleScene.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnCube.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnPlayerIdleState.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnReflectShield.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnDefenseRule.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/bnAnimation.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnCanodumbCursor.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnRockDebris.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnBubbleTrap.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnAnimatedTextBox.cpp$(ObjectSuffix) 

Objects3=$(IntermediateDirectory)/bnMetalManThrowState.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnLogger.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnCanodumbIdleState.cpp$(ObjectSuffix) $(IntermediateDirectory)/mmbn.ico.c$(ObjectSuffix) $(IntermediateDirectory)/bnTile.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/bnNinjaAntiDamage.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnInvis.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnProgsManMoveState.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnBattleResults.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnPA.cpp$(ObjectSuffix) $(IntermediateDirectory)/bnMobRegistration.cpp$(ObjectSuffix) 



Objects=$(Objects0) $(Objects1) $(Objects2) $(Objects3) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	@echo $(Objects1) >> $(ObjectsFileList)
	@echo $(Objects2) >> $(ObjectsFileList)
	@echo $(Objects3) >> $(ObjectsFileList)
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

MakeIntermediateDirs:
	@test -d ./Debug || $(MakeDirCommand) ./Debug


$(IntermediateDirectory)/.d:
	@test -d ./Debug || $(MakeDirCommand) ./Debug

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/bnObstacle.cpp$(ObjectSuffix): bnObstacle.cpp $(IntermediateDirectory)/bnObstacle.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnObstacle.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnObstacle.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnObstacle.cpp$(DependSuffix): bnObstacle.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnObstacle.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnObstacle.cpp$(DependSuffix) -MM bnObstacle.cpp

$(IntermediateDirectory)/bnObstacle.cpp$(PreprocessSuffix): bnObstacle.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnObstacle.cpp$(PreprocessSuffix) bnObstacle.cpp

$(IntermediateDirectory)/bnHitBox.cpp$(ObjectSuffix): bnHitBox.cpp $(IntermediateDirectory)/bnHitBox.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnHitBox.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnHitBox.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnHitBox.cpp$(DependSuffix): bnHitBox.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnHitBox.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnHitBox.cpp$(DependSuffix) -MM bnHitBox.cpp

$(IntermediateDirectory)/bnHitBox.cpp$(PreprocessSuffix): bnHitBox.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnHitBox.cpp$(PreprocessSuffix) bnHitBox.cpp

$(IntermediateDirectory)/bnCanodumb.cpp$(ObjectSuffix): bnCanodumb.cpp $(IntermediateDirectory)/bnCanodumb.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnCanodumb.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnCanodumb.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnCanodumb.cpp$(DependSuffix): bnCanodumb.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnCanodumb.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnCanodumb.cpp$(DependSuffix) -MM bnCanodumb.cpp

$(IntermediateDirectory)/bnCanodumb.cpp$(PreprocessSuffix): bnCanodumb.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnCanodumb.cpp$(PreprocessSuffix) bnCanodumb.cpp

$(IntermediateDirectory)/bnGear.cpp$(ObjectSuffix): bnGear.cpp $(IntermediateDirectory)/bnGear.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnGear.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnGear.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnGear.cpp$(DependSuffix): bnGear.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnGear.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnGear.cpp$(DependSuffix) -MM bnGear.cpp

$(IntermediateDirectory)/bnGear.cpp$(PreprocessSuffix): bnGear.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnGear.cpp$(PreprocessSuffix) bnGear.cpp

$(IntermediateDirectory)/bnAuraHealthUI.cpp$(ObjectSuffix): bnAuraHealthUI.cpp $(IntermediateDirectory)/bnAuraHealthUI.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnAuraHealthUI.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnAuraHealthUI.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnAuraHealthUI.cpp$(DependSuffix): bnAuraHealthUI.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnAuraHealthUI.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnAuraHealthUI.cpp$(DependSuffix) -MM bnAuraHealthUI.cpp

$(IntermediateDirectory)/bnAuraHealthUI.cpp$(PreprocessSuffix): bnAuraHealthUI.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnAuraHealthUI.cpp$(PreprocessSuffix) bnAuraHealthUI.cpp

$(IntermediateDirectory)/main.cpp$(ObjectSuffix): main.cpp $(IntermediateDirectory)/main.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/main.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/main.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/main.cpp$(DependSuffix): main.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/main.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/main.cpp$(DependSuffix) -MM main.cpp

$(IntermediateDirectory)/main.cpp$(PreprocessSuffix): main.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/main.cpp$(PreprocessSuffix) main.cpp

$(IntermediateDirectory)/bnRollHeart.cpp$(ObjectSuffix): bnRollHeart.cpp $(IntermediateDirectory)/bnRollHeart.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnRollHeart.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnRollHeart.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnRollHeart.cpp$(DependSuffix): bnRollHeart.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnRollHeart.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnRollHeart.cpp$(DependSuffix) -MM bnRollHeart.cpp

$(IntermediateDirectory)/bnRollHeart.cpp$(PreprocessSuffix): bnRollHeart.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnRollHeart.cpp$(PreprocessSuffix) bnRollHeart.cpp

$(IntermediateDirectory)/bnLanBackground.cpp$(ObjectSuffix): bnLanBackground.cpp $(IntermediateDirectory)/bnLanBackground.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnLanBackground.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnLanBackground.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnLanBackground.cpp$(DependSuffix): bnLanBackground.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnLanBackground.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnLanBackground.cpp$(DependSuffix) -MM bnLanBackground.cpp

$(IntermediateDirectory)/bnLanBackground.cpp$(PreprocessSuffix): bnLanBackground.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnLanBackground.cpp$(PreprocessSuffix) bnLanBackground.cpp

$(IntermediateDirectory)/bnChipDescriptionTextbox.cpp$(ObjectSuffix): bnChipDescriptionTextbox.cpp $(IntermediateDirectory)/bnChipDescriptionTextbox.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnChipDescriptionTextbox.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnChipDescriptionTextbox.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnChipDescriptionTextbox.cpp$(DependSuffix): bnChipDescriptionTextbox.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnChipDescriptionTextbox.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnChipDescriptionTextbox.cpp$(DependSuffix) -MM bnChipDescriptionTextbox.cpp

$(IntermediateDirectory)/bnChipDescriptionTextbox.cpp$(PreprocessSuffix): bnChipDescriptionTextbox.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnChipDescriptionTextbox.cpp$(PreprocessSuffix) bnChipDescriptionTextbox.cpp

$(IntermediateDirectory)/bnOverworldMap.cpp$(ObjectSuffix): bnOverworldMap.cpp $(IntermediateDirectory)/bnOverworldMap.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnOverworldMap.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnOverworldMap.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnOverworldMap.cpp$(DependSuffix): bnOverworldMap.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnOverworldMap.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnOverworldMap.cpp$(DependSuffix) -MM bnOverworldMap.cpp

$(IntermediateDirectory)/bnOverworldMap.cpp$(PreprocessSuffix): bnOverworldMap.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnOverworldMap.cpp$(PreprocessSuffix) bnOverworldMap.cpp

$(IntermediateDirectory)/bnStarman.cpp$(ObjectSuffix): bnStarman.cpp $(IntermediateDirectory)/bnStarman.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnStarman.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnStarman.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnStarman.cpp$(DependSuffix): bnStarman.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnStarman.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnStarman.cpp$(DependSuffix) -MM bnStarman.cpp

$(IntermediateDirectory)/bnStarman.cpp$(PreprocessSuffix): bnStarman.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnStarman.cpp$(PreprocessSuffix) bnStarman.cpp

$(IntermediateDirectory)/bnCamera.cpp$(ObjectSuffix): bnCamera.cpp $(IntermediateDirectory)/bnCamera.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnCamera.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnCamera.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnCamera.cpp$(DependSuffix): bnCamera.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnCamera.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnCamera.cpp$(DependSuffix) -MM bnCamera.cpp

$(IntermediateDirectory)/bnCamera.cpp$(PreprocessSuffix): bnCamera.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnCamera.cpp$(PreprocessSuffix) bnCamera.cpp

$(IntermediateDirectory)/bnAudioResourceManager.cpp$(ObjectSuffix): bnAudioResourceManager.cpp $(IntermediateDirectory)/bnAudioResourceManager.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnAudioResourceManager.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnAudioResourceManager.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnAudioResourceManager.cpp$(DependSuffix): bnAudioResourceManager.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnAudioResourceManager.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnAudioResourceManager.cpp$(DependSuffix) -MM bnAudioResourceManager.cpp

$(IntermediateDirectory)/bnAudioResourceManager.cpp$(PreprocessSuffix): bnAudioResourceManager.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnAudioResourceManager.cpp$(PreprocessSuffix) bnAudioResourceManager.cpp

$(IntermediateDirectory)/bnExplosion.cpp$(ObjectSuffix): bnExplosion.cpp $(IntermediateDirectory)/bnExplosion.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnExplosion.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnExplosion.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnExplosion.cpp$(DependSuffix): bnExplosion.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnExplosion.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnExplosion.cpp$(DependSuffix) -MM bnExplosion.cpp

$(IntermediateDirectory)/bnExplosion.cpp$(PreprocessSuffix): bnExplosion.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnExplosion.cpp$(PreprocessSuffix) bnExplosion.cpp

$(IntermediateDirectory)/bnRollHeal.cpp$(ObjectSuffix): bnRollHeal.cpp $(IntermediateDirectory)/bnRollHeal.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnRollHeal.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnRollHeal.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnRollHeal.cpp$(DependSuffix): bnRollHeal.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnRollHeal.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnRollHeal.cpp$(DependSuffix) -MM bnRollHeal.cpp

$(IntermediateDirectory)/bnRollHeal.cpp$(PreprocessSuffix): bnRollHeal.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnRollHeal.cpp$(PreprocessSuffix) bnRollHeal.cpp

$(IntermediateDirectory)/bnStarfishIdleState.cpp$(ObjectSuffix): bnStarfishIdleState.cpp $(IntermediateDirectory)/bnStarfishIdleState.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnStarfishIdleState.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnStarfishIdleState.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnStarfishIdleState.cpp$(DependSuffix): bnStarfishIdleState.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnStarfishIdleState.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnStarfishIdleState.cpp$(DependSuffix) -MM bnStarfishIdleState.cpp

$(IntermediateDirectory)/bnStarfishIdleState.cpp$(PreprocessSuffix): bnStarfishIdleState.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnStarfishIdleState.cpp$(PreprocessSuffix) bnStarfishIdleState.cpp

$(IntermediateDirectory)/bnMetalManBossFight2.cpp$(ObjectSuffix): bnMetalManBossFight2.cpp $(IntermediateDirectory)/bnMetalManBossFight2.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnMetalManBossFight2.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnMetalManBossFight2.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnMetalManBossFight2.cpp$(DependSuffix): bnMetalManBossFight2.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnMetalManBossFight2.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnMetalManBossFight2.cpp$(DependSuffix) -MM bnMetalManBossFight2.cpp

$(IntermediateDirectory)/bnMetalManBossFight2.cpp$(PreprocessSuffix): bnMetalManBossFight2.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnMetalManBossFight2.cpp$(PreprocessSuffix) bnMetalManBossFight2.cpp

$(IntermediateDirectory)/bnElecpulse.cpp$(ObjectSuffix): bnElecpulse.cpp $(IntermediateDirectory)/bnElecpulse.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnElecpulse.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnElecpulse.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnElecpulse.cpp$(DependSuffix): bnElecpulse.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnElecpulse.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnElecpulse.cpp$(DependSuffix) -MM bnElecpulse.cpp

$(IntermediateDirectory)/bnElecpulse.cpp$(PreprocessSuffix): bnElecpulse.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnElecpulse.cpp$(PreprocessSuffix) bnElecpulse.cpp

$(IntermediateDirectory)/bnFakeScene.cpp$(ObjectSuffix): bnFakeScene.cpp $(IntermediateDirectory)/bnFakeScene.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnFakeScene.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnFakeScene.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnFakeScene.cpp$(DependSuffix): bnFakeScene.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnFakeScene.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnFakeScene.cpp$(DependSuffix) -MM bnFakeScene.cpp

$(IntermediateDirectory)/bnFakeScene.cpp$(PreprocessSuffix): bnFakeScene.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnFakeScene.cpp$(PreprocessSuffix) bnFakeScene.cpp

$(IntermediateDirectory)/bnStarfishAttackState.cpp$(ObjectSuffix): bnStarfishAttackState.cpp $(IntermediateDirectory)/bnStarfishAttackState.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnStarfishAttackState.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnStarfishAttackState.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnStarfishAttackState.cpp$(DependSuffix): bnStarfishAttackState.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnStarfishAttackState.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnStarfishAttackState.cpp$(DependSuffix) -MM bnStarfishAttackState.cpp

$(IntermediateDirectory)/bnStarfishAttackState.cpp$(PreprocessSuffix): bnStarfishAttackState.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnStarfishAttackState.cpp$(PreprocessSuffix) bnStarfishAttackState.cpp

$(IntermediateDirectory)/bnRowHit.cpp$(ObjectSuffix): bnRowHit.cpp $(IntermediateDirectory)/bnRowHit.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnRowHit.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnRowHit.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnRowHit.cpp$(DependSuffix): bnRowHit.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnRowHit.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnRowHit.cpp$(DependSuffix) -MM bnRowHit.cpp

$(IntermediateDirectory)/bnRowHit.cpp$(PreprocessSuffix): bnRowHit.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnRowHit.cpp$(PreprocessSuffix) bnRowHit.cpp

$(IntermediateDirectory)/bnMettaurIdleState.cpp$(ObjectSuffix): bnMettaurIdleState.cpp $(IntermediateDirectory)/bnMettaurIdleState.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnMettaurIdleState.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnMettaurIdleState.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnMettaurIdleState.cpp$(DependSuffix): bnMettaurIdleState.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnMettaurIdleState.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnMettaurIdleState.cpp$(DependSuffix) -MM bnMettaurIdleState.cpp

$(IntermediateDirectory)/bnMettaurIdleState.cpp$(PreprocessSuffix): bnMettaurIdleState.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnMettaurIdleState.cpp$(PreprocessSuffix) bnMettaurIdleState.cpp

$(IntermediateDirectory)/bnSelectMobScene.cpp$(ObjectSuffix): bnSelectMobScene.cpp $(IntermediateDirectory)/bnSelectMobScene.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnSelectMobScene.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnSelectMobScene.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnSelectMobScene.cpp$(DependSuffix): bnSelectMobScene.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnSelectMobScene.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnSelectMobScene.cpp$(DependSuffix) -MM bnSelectMobScene.cpp

$(IntermediateDirectory)/bnSelectMobScene.cpp$(PreprocessSuffix): bnSelectMobScene.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnSelectMobScene.cpp$(PreprocessSuffix) bnSelectMobScene.cpp

$(IntermediateDirectory)/bnMainMenuScene.cpp$(ObjectSuffix): bnMainMenuScene.cpp $(IntermediateDirectory)/bnMainMenuScene.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnMainMenuScene.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnMainMenuScene.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnMainMenuScene.cpp$(DependSuffix): bnMainMenuScene.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnMainMenuScene.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnMainMenuScene.cpp$(DependSuffix) -MM bnMainMenuScene.cpp

$(IntermediateDirectory)/bnMainMenuScene.cpp$(PreprocessSuffix): bnMainMenuScene.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnMainMenuScene.cpp$(PreprocessSuffix) bnMainMenuScene.cpp

$(IntermediateDirectory)/bnCounterHitListener.cpp$(ObjectSuffix): bnCounterHitListener.cpp $(IntermediateDirectory)/bnCounterHitListener.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnCounterHitListener.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnCounterHitListener.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnCounterHitListener.cpp$(DependSuffix): bnCounterHitListener.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnCounterHitListener.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnCounterHitListener.cpp$(DependSuffix) -MM bnCounterHitListener.cpp

$(IntermediateDirectory)/bnCounterHitListener.cpp$(PreprocessSuffix): bnCounterHitListener.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnCounterHitListener.cpp$(PreprocessSuffix) bnCounterHitListener.cpp

$(IntermediateDirectory)/bnCounterHitPublisher.cpp$(ObjectSuffix): bnCounterHitPublisher.cpp $(IntermediateDirectory)/bnCounterHitPublisher.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnCounterHitPublisher.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnCounterHitPublisher.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnCounterHitPublisher.cpp$(DependSuffix): bnCounterHitPublisher.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnCounterHitPublisher.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnCounterHitPublisher.cpp$(DependSuffix) -MM bnCounterHitPublisher.cpp

$(IntermediateDirectory)/bnCounterHitPublisher.cpp$(PreprocessSuffix): bnCounterHitPublisher.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnCounterHitPublisher.cpp$(PreprocessSuffix) bnCounterHitPublisher.cpp

$(IntermediateDirectory)/bnGuardHit.cpp$(ObjectSuffix): bnGuardHit.cpp $(IntermediateDirectory)/bnGuardHit.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnGuardHit.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnGuardHit.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnGuardHit.cpp$(DependSuffix): bnGuardHit.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnGuardHit.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnGuardHit.cpp$(DependSuffix) -MM bnGuardHit.cpp

$(IntermediateDirectory)/bnGuardHit.cpp$(PreprocessSuffix): bnGuardHit.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnGuardHit.cpp$(PreprocessSuffix) bnGuardHit.cpp

$(IntermediateDirectory)/bnCharacter.cpp$(ObjectSuffix): bnCharacter.cpp $(IntermediateDirectory)/bnCharacter.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnCharacter.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnCharacter.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnCharacter.cpp$(DependSuffix): bnCharacter.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnCharacter.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnCharacter.cpp$(DependSuffix) -MM bnCharacter.cpp

$(IntermediateDirectory)/bnCharacter.cpp$(PreprocessSuffix): bnCharacter.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnCharacter.cpp$(PreprocessSuffix) bnCharacter.cpp

$(IntermediateDirectory)/bnMettaurAttackState.cpp$(ObjectSuffix): bnMettaurAttackState.cpp $(IntermediateDirectory)/bnMettaurAttackState.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnMettaurAttackState.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnMettaurAttackState.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnMettaurAttackState.cpp$(DependSuffix): bnMettaurAttackState.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnMettaurAttackState.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnMettaurAttackState.cpp$(DependSuffix) -MM bnMettaurAttackState.cpp

$(IntermediateDirectory)/bnMettaurAttackState.cpp$(PreprocessSuffix): bnMettaurAttackState.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnMettaurAttackState.cpp$(PreprocessSuffix) bnMettaurAttackState.cpp

$(IntermediateDirectory)/bnThunder.cpp$(ObjectSuffix): bnThunder.cpp $(IntermediateDirectory)/bnThunder.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnThunder.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnThunder.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnThunder.cpp$(DependSuffix): bnThunder.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnThunder.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnThunder.cpp$(DependSuffix) -MM bnThunder.cpp

$(IntermediateDirectory)/bnThunder.cpp$(PreprocessSuffix): bnThunder.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnThunder.cpp$(PreprocessSuffix) bnThunder.cpp

$(IntermediateDirectory)/bnMetalMan.cpp$(ObjectSuffix): bnMetalMan.cpp $(IntermediateDirectory)/bnMetalMan.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnMetalMan.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnMetalMan.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnMetalMan.cpp$(DependSuffix): bnMetalMan.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnMetalMan.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnMetalMan.cpp$(DependSuffix) -MM bnMetalMan.cpp

$(IntermediateDirectory)/bnMetalMan.cpp$(PreprocessSuffix): bnMetalMan.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnMetalMan.cpp$(PreprocessSuffix) bnMetalMan.cpp

$(IntermediateDirectory)/bnElementalDamage.cpp$(ObjectSuffix): bnElementalDamage.cpp $(IntermediateDirectory)/bnElementalDamage.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnElementalDamage.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnElementalDamage.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnElementalDamage.cpp$(DependSuffix): bnElementalDamage.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnElementalDamage.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnElementalDamage.cpp$(DependSuffix) -MM bnElementalDamage.cpp

$(IntermediateDirectory)/bnElementalDamage.cpp$(PreprocessSuffix): bnElementalDamage.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnElementalDamage.cpp$(PreprocessSuffix) bnElementalDamage.cpp

$(IntermediateDirectory)/bnTextureResourceManager.cpp$(ObjectSuffix): bnTextureResourceManager.cpp $(IntermediateDirectory)/bnTextureResourceManager.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnTextureResourceManager.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnTextureResourceManager.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnTextureResourceManager.cpp$(DependSuffix): bnTextureResourceManager.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnTextureResourceManager.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnTextureResourceManager.cpp$(DependSuffix) -MM bnTextureResourceManager.cpp

$(IntermediateDirectory)/bnTextureResourceManager.cpp$(PreprocessSuffix): bnTextureResourceManager.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnTextureResourceManager.cpp$(PreprocessSuffix) bnTextureResourceManager.cpp

$(IntermediateDirectory)/bnPlayer.cpp$(ObjectSuffix): bnPlayer.cpp $(IntermediateDirectory)/bnPlayer.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnPlayer.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnPlayer.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnPlayer.cpp$(DependSuffix): bnPlayer.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnPlayer.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnPlayer.cpp$(DependSuffix) -MM bnPlayer.cpp

$(IntermediateDirectory)/bnPlayer.cpp$(PreprocessSuffix): bnPlayer.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnPlayer.cpp$(PreprocessSuffix) bnPlayer.cpp

$(IntermediateDirectory)/bnMettaur.cpp$(ObjectSuffix): bnMettaur.cpp $(IntermediateDirectory)/bnMettaur.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnMettaur.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnMettaur.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnMettaur.cpp$(DependSuffix): bnMettaur.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnMettaur.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnMettaur.cpp$(DependSuffix) -MM bnMettaur.cpp

$(IntermediateDirectory)/bnMettaur.cpp$(PreprocessSuffix): bnMettaur.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnMettaur.cpp$(PreprocessSuffix) bnMettaur.cpp

$(IntermediateDirectory)/bnCannon.cpp$(ObjectSuffix): bnCannon.cpp $(IntermediateDirectory)/bnCannon.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnCannon.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnCannon.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnCannon.cpp$(DependSuffix): bnCannon.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnCannon.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnCannon.cpp$(DependSuffix) -MM bnCannon.cpp

$(IntermediateDirectory)/bnCannon.cpp$(PreprocessSuffix): bnCannon.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnCannon.cpp$(PreprocessSuffix) bnCannon.cpp

$(IntermediateDirectory)/bnCanodumbAttackState.cpp$(ObjectSuffix): bnCanodumbAttackState.cpp $(IntermediateDirectory)/bnCanodumbAttackState.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnCanodumbAttackState.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnCanodumbAttackState.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnCanodumbAttackState.cpp$(DependSuffix): bnCanodumbAttackState.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnCanodumbAttackState.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnCanodumbAttackState.cpp$(DependSuffix) -MM bnCanodumbAttackState.cpp

$(IntermediateDirectory)/bnCanodumbAttackState.cpp$(PreprocessSuffix): bnCanodumbAttackState.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnCanodumbAttackState.cpp$(PreprocessSuffix) bnCanodumbAttackState.cpp

$(IntermediateDirectory)/bnChargedBusterHit.cpp$(ObjectSuffix): bnChargedBusterHit.cpp $(IntermediateDirectory)/bnChargedBusterHit.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnChargedBusterHit.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnChargedBusterHit.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnChargedBusterHit.cpp$(DependSuffix): bnChargedBusterHit.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnChargedBusterHit.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnChargedBusterHit.cpp$(DependSuffix) -MM bnChargedBusterHit.cpp

$(IntermediateDirectory)/bnChargedBusterHit.cpp$(PreprocessSuffix): bnChargedBusterHit.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnChargedBusterHit.cpp$(PreprocessSuffix) bnChargedBusterHit.cpp

$(IntermediateDirectory)/bnComponent.cpp$(ObjectSuffix): bnComponent.cpp $(IntermediateDirectory)/bnComponent.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnComponent.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnComponent.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnComponent.cpp$(DependSuffix): bnComponent.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnComponent.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnComponent.cpp$(DependSuffix) -MM bnComponent.cpp

$(IntermediateDirectory)/bnComponent.cpp$(PreprocessSuffix): bnComponent.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnComponent.cpp$(PreprocessSuffix) bnComponent.cpp

$(IntermediateDirectory)/bnEnemyChipsUI.cpp$(ObjectSuffix): bnEnemyChipsUI.cpp $(IntermediateDirectory)/bnEnemyChipsUI.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnEnemyChipsUI.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnEnemyChipsUI.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnEnemyChipsUI.cpp$(DependSuffix): bnEnemyChipsUI.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnEnemyChipsUI.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnEnemyChipsUI.cpp$(DependSuffix) -MM bnEnemyChipsUI.cpp

$(IntermediateDirectory)/bnEnemyChipsUI.cpp$(PreprocessSuffix): bnEnemyChipsUI.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnEnemyChipsUI.cpp$(PreprocessSuffix) bnEnemyChipsUI.cpp

$(IntermediateDirectory)/bnInfiniteMap.cpp$(ObjectSuffix): bnInfiniteMap.cpp $(IntermediateDirectory)/bnInfiniteMap.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnInfiniteMap.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnInfiniteMap.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnInfiniteMap.cpp$(DependSuffix): bnInfiniteMap.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnInfiniteMap.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnInfiniteMap.cpp$(DependSuffix) -MM bnInfiniteMap.cpp

$(IntermediateDirectory)/bnInfiniteMap.cpp$(PreprocessSuffix): bnInfiniteMap.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnInfiniteMap.cpp$(PreprocessSuffix) bnInfiniteMap.cpp

$(IntermediateDirectory)/bnLibraryScene.cpp$(ObjectSuffix): bnLibraryScene.cpp $(IntermediateDirectory)/bnLibraryScene.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnLibraryScene.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnLibraryScene.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnLibraryScene.cpp$(DependSuffix): bnLibraryScene.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnLibraryScene.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnLibraryScene.cpp$(DependSuffix) -MM bnLibraryScene.cpp

$(IntermediateDirectory)/bnLibraryScene.cpp$(PreprocessSuffix): bnLibraryScene.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnLibraryScene.cpp$(PreprocessSuffix) bnLibraryScene.cpp

$(IntermediateDirectory)/bnPanelGrab.cpp$(ObjectSuffix): bnPanelGrab.cpp $(IntermediateDirectory)/bnPanelGrab.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnPanelGrab.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnPanelGrab.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnPanelGrab.cpp$(DependSuffix): bnPanelGrab.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnPanelGrab.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnPanelGrab.cpp$(DependSuffix) -MM bnPanelGrab.cpp

$(IntermediateDirectory)/bnPanelGrab.cpp$(PreprocessSuffix): bnPanelGrab.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnPanelGrab.cpp$(PreprocessSuffix) bnPanelGrab.cpp

$(IntermediateDirectory)/bnFolderEditScene.cpp$(ObjectSuffix): bnFolderEditScene.cpp $(IntermediateDirectory)/bnFolderEditScene.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnFolderEditScene.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnFolderEditScene.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnFolderEditScene.cpp$(DependSuffix): bnFolderEditScene.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnFolderEditScene.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnFolderEditScene.cpp$(DependSuffix) -MM bnFolderEditScene.cpp

$(IntermediateDirectory)/bnFolderEditScene.cpp$(PreprocessSuffix): bnFolderEditScene.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnFolderEditScene.cpp$(PreprocessSuffix) bnFolderEditScene.cpp

$(IntermediateDirectory)/bnDefenseAura.cpp$(ObjectSuffix): bnDefenseAura.cpp $(IntermediateDirectory)/bnDefenseAura.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnDefenseAura.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnDefenseAura.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnDefenseAura.cpp$(DependSuffix): bnDefenseAura.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnDefenseAura.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnDefenseAura.cpp$(DependSuffix) -MM bnDefenseAura.cpp

$(IntermediateDirectory)/bnDefenseAura.cpp$(PreprocessSuffix): bnDefenseAura.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnDefenseAura.cpp$(PreprocessSuffix) bnDefenseAura.cpp

$(IntermediateDirectory)/bnParticlePoof.cpp$(ObjectSuffix): bnParticlePoof.cpp $(IntermediateDirectory)/bnParticlePoof.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnParticlePoof.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnParticlePoof.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnParticlePoof.cpp$(DependSuffix): bnParticlePoof.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnParticlePoof.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnParticlePoof.cpp$(DependSuffix) -MM bnParticlePoof.cpp

$(IntermediateDirectory)/bnParticlePoof.cpp$(PreprocessSuffix): bnParticlePoof.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnParticlePoof.cpp$(PreprocessSuffix) bnParticlePoof.cpp

$(IntermediateDirectory)/bnMobHealthUI.cpp$(ObjectSuffix): bnMobHealthUI.cpp $(IntermediateDirectory)/bnMobHealthUI.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnMobHealthUI.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnMobHealthUI.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnMobHealthUI.cpp$(DependSuffix): bnMobHealthUI.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnMobHealthUI.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnMobHealthUI.cpp$(DependSuffix) -MM bnMobHealthUI.cpp

$(IntermediateDirectory)/bnMobHealthUI.cpp$(PreprocessSuffix): bnMobHealthUI.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnMobHealthUI.cpp$(PreprocessSuffix) bnMobHealthUI.cpp

$(IntermediateDirectory)/bnWave.cpp$(ObjectSuffix): bnWave.cpp $(IntermediateDirectory)/bnWave.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnWave.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnWave.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnWave.cpp$(DependSuffix): bnWave.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnWave.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnWave.cpp$(DependSuffix) -MM bnWave.cpp

$(IntermediateDirectory)/bnWave.cpp$(PreprocessSuffix): bnWave.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnWave.cpp$(PreprocessSuffix) bnWave.cpp

$(IntermediateDirectory)/bnDefenseGuard.cpp$(ObjectSuffix): bnDefenseGuard.cpp $(IntermediateDirectory)/bnDefenseGuard.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnDefenseGuard.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnDefenseGuard.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnDefenseGuard.cpp$(DependSuffix): bnDefenseGuard.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnDefenseGuard.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnDefenseGuard.cpp$(DependSuffix) -MM bnDefenseGuard.cpp

$(IntermediateDirectory)/bnDefenseGuard.cpp$(PreprocessSuffix): bnDefenseGuard.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnDefenseGuard.cpp$(PreprocessSuffix) bnDefenseGuard.cpp

$(IntermediateDirectory)/bnCanodumbMob.cpp$(ObjectSuffix): bnCanodumbMob.cpp $(IntermediateDirectory)/bnCanodumbMob.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnCanodumbMob.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnCanodumbMob.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnCanodumbMob.cpp$(DependSuffix): bnCanodumbMob.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnCanodumbMob.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnCanodumbMob.cpp$(DependSuffix) -MM bnCanodumbMob.cpp

$(IntermediateDirectory)/bnCanodumbMob.cpp$(PreprocessSuffix): bnCanodumbMob.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnCanodumbMob.cpp$(PreprocessSuffix) bnCanodumbMob.cpp

$(IntermediateDirectory)/bnProgsManIdleState.cpp$(ObjectSuffix): bnProgsManIdleState.cpp $(IntermediateDirectory)/bnProgsManIdleState.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnProgsManIdleState.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnProgsManIdleState.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnProgsManIdleState.cpp$(DependSuffix): bnProgsManIdleState.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnProgsManIdleState.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnProgsManIdleState.cpp$(DependSuffix) -MM bnProgsManIdleState.cpp

$(IntermediateDirectory)/bnProgsManIdleState.cpp$(PreprocessSuffix): bnProgsManIdleState.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnProgsManIdleState.cpp$(PreprocessSuffix) bnProgsManIdleState.cpp

$(IntermediateDirectory)/bnMetalManPunchState.cpp$(ObjectSuffix): bnMetalManPunchState.cpp $(IntermediateDirectory)/bnMetalManPunchState.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnMetalManPunchState.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnMetalManPunchState.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnMetalManPunchState.cpp$(DependSuffix): bnMetalManPunchState.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnMetalManPunchState.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnMetalManPunchState.cpp$(DependSuffix) -MM bnMetalManPunchState.cpp

$(IntermediateDirectory)/bnMetalManPunchState.cpp$(PreprocessSuffix): bnMetalManPunchState.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnMetalManPunchState.cpp$(PreprocessSuffix) bnMetalManPunchState.cpp

$(IntermediateDirectory)/bnProgsMan.cpp$(ObjectSuffix): bnProgsMan.cpp $(IntermediateDirectory)/bnProgsMan.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnProgsMan.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnProgsMan.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnProgsMan.cpp$(DependSuffix): bnProgsMan.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnProgsMan.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnProgsMan.cpp$(DependSuffix) -MM bnProgsMan.cpp

$(IntermediateDirectory)/bnProgsMan.cpp$(PreprocessSuffix): bnProgsMan.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnProgsMan.cpp$(PreprocessSuffix) bnProgsMan.cpp

$(IntermediateDirectory)/bnStarfishMob.cpp$(ObjectSuffix): bnStarfishMob.cpp $(IntermediateDirectory)/bnStarfishMob.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnStarfishMob.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnStarfishMob.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnStarfishMob.cpp$(DependSuffix): bnStarfishMob.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnStarfishMob.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnStarfishMob.cpp$(DependSuffix) -MM bnStarfishMob.cpp

$(IntermediateDirectory)/bnStarfishMob.cpp$(PreprocessSuffix): bnStarfishMob.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnStarfishMob.cpp$(PreprocessSuffix) bnStarfishMob.cpp

$(IntermediateDirectory)/bnGameOverScene.cpp$(ObjectSuffix): bnGameOverScene.cpp $(IntermediateDirectory)/bnGameOverScene.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnGameOverScene.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnGameOverScene.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnGameOverScene.cpp$(DependSuffix): bnGameOverScene.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnGameOverScene.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnGameOverScene.cpp$(DependSuffix) -MM bnGameOverScene.cpp

$(IntermediateDirectory)/bnGameOverScene.cpp$(PreprocessSuffix): bnGameOverScene.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnGameOverScene.cpp$(PreprocessSuffix) bnGameOverScene.cpp

$(IntermediateDirectory)/bnNinjaStar.cpp$(ObjectSuffix): bnNinjaStar.cpp $(IntermediateDirectory)/bnNinjaStar.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnNinjaStar.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnNinjaStar.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnNinjaStar.cpp$(DependSuffix): bnNinjaStar.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnNinjaStar.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnNinjaStar.cpp$(DependSuffix) -MM bnNinjaStar.cpp

$(IntermediateDirectory)/bnNinjaStar.cpp$(PreprocessSuffix): bnNinjaStar.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnNinjaStar.cpp$(PreprocessSuffix) bnNinjaStar.cpp

$(IntermediateDirectory)/bnUndernetBackground.cpp$(ObjectSuffix): bnUndernetBackground.cpp $(IntermediateDirectory)/bnUndernetBackground.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnUndernetBackground.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnUndernetBackground.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnUndernetBackground.cpp$(DependSuffix): bnUndernetBackground.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnUndernetBackground.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnUndernetBackground.cpp$(DependSuffix) -MM bnUndernetBackground.cpp

$(IntermediateDirectory)/bnUndernetBackground.cpp$(PreprocessSuffix): bnUndernetBackground.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnUndernetBackground.cpp$(PreprocessSuffix) bnUndernetBackground.cpp

$(IntermediateDirectory)/bnArtifact.cpp$(ObjectSuffix): bnArtifact.cpp $(IntermediateDirectory)/bnArtifact.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnArtifact.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnArtifact.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnArtifact.cpp$(DependSuffix): bnArtifact.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnArtifact.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnArtifact.cpp$(DependSuffix) -MM bnArtifact.cpp

$(IntermediateDirectory)/bnArtifact.cpp$(PreprocessSuffix): bnArtifact.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnArtifact.cpp$(PreprocessSuffix) bnArtifact.cpp

$(IntermediateDirectory)/bnHideUntil.cpp$(ObjectSuffix): bnHideUntil.cpp $(IntermediateDirectory)/bnHideUntil.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnHideUntil.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnHideUntil.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnHideUntil.cpp$(DependSuffix): bnHideUntil.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnHideUntil.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnHideUntil.cpp$(DependSuffix) -MM bnHideUntil.cpp

$(IntermediateDirectory)/bnHideUntil.cpp$(PreprocessSuffix): bnHideUntil.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnHideUntil.cpp$(PreprocessSuffix) bnHideUntil.cpp

$(IntermediateDirectory)/bnEntity.cpp$(ObjectSuffix): bnEntity.cpp $(IntermediateDirectory)/bnEntity.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnEntity.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnEntity.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnEntity.cpp$(DependSuffix): bnEntity.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnEntity.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnEntity.cpp$(DependSuffix) -MM bnEntity.cpp

$(IntermediateDirectory)/bnEntity.cpp$(PreprocessSuffix): bnEntity.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnEntity.cpp$(PreprocessSuffix) bnEntity.cpp

$(IntermediateDirectory)/bnShineExplosion.cpp$(ObjectSuffix): bnShineExplosion.cpp $(IntermediateDirectory)/bnShineExplosion.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnShineExplosion.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnShineExplosion.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnShineExplosion.cpp$(DependSuffix): bnShineExplosion.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnShineExplosion.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnShineExplosion.cpp$(DependSuffix) -MM bnShineExplosion.cpp

$(IntermediateDirectory)/bnShineExplosion.cpp$(PreprocessSuffix): bnShineExplosion.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnShineExplosion.cpp$(PreprocessSuffix) bnShineExplosion.cpp

$(IntermediateDirectory)/bnGraveyardBackground.cpp$(ObjectSuffix): bnGraveyardBackground.cpp $(IntermediateDirectory)/bnGraveyardBackground.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnGraveyardBackground.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnGraveyardBackground.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnGraveyardBackground.cpp$(DependSuffix): bnGraveyardBackground.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnGraveyardBackground.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnGraveyardBackground.cpp$(DependSuffix) -MM bnGraveyardBackground.cpp

$(IntermediateDirectory)/bnGraveyardBackground.cpp$(PreprocessSuffix): bnGraveyardBackground.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnGraveyardBackground.cpp$(PreprocessSuffix) bnGraveyardBackground.cpp

$(IntermediateDirectory)/bnPlayerHitState.cpp$(ObjectSuffix): bnPlayerHitState.cpp $(IntermediateDirectory)/bnPlayerHitState.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnPlayerHitState.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnPlayerHitState.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnPlayerHitState.cpp$(DependSuffix): bnPlayerHitState.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnPlayerHitState.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnPlayerHitState.cpp$(DependSuffix) -MM bnPlayerHitState.cpp

$(IntermediateDirectory)/bnPlayerHitState.cpp$(PreprocessSuffix): bnPlayerHitState.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnPlayerHitState.cpp$(PreprocessSuffix) bnPlayerHitState.cpp

$(IntermediateDirectory)/bnMysteryData.cpp$(ObjectSuffix): bnMysteryData.cpp $(IntermediateDirectory)/bnMysteryData.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnMysteryData.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnMysteryData.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnMysteryData.cpp$(DependSuffix): bnMysteryData.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnMysteryData.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnMysteryData.cpp$(DependSuffix) -MM bnMysteryData.cpp

$(IntermediateDirectory)/bnMysteryData.cpp$(PreprocessSuffix): bnMysteryData.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnMysteryData.cpp$(PreprocessSuffix) bnMysteryData.cpp

$(IntermediateDirectory)/bnMettaurMoveState.cpp$(ObjectSuffix): bnMettaurMoveState.cpp $(IntermediateDirectory)/bnMettaurMoveState.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnMettaurMoveState.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnMettaurMoveState.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnMettaurMoveState.cpp$(DependSuffix): bnMettaurMoveState.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnMettaurMoveState.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnMettaurMoveState.cpp$(DependSuffix) -MM bnMettaurMoveState.cpp

$(IntermediateDirectory)/bnMettaurMoveState.cpp$(PreprocessSuffix): bnMettaurMoveState.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnMettaurMoveState.cpp$(PreprocessSuffix) bnMettaurMoveState.cpp

$(IntermediateDirectory)/bnChipSelectionCust.cpp$(ObjectSuffix): bnChipSelectionCust.cpp $(IntermediateDirectory)/bnChipSelectionCust.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnChipSelectionCust.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnChipSelectionCust.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnChipSelectionCust.cpp$(DependSuffix): bnChipSelectionCust.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnChipSelectionCust.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnChipSelectionCust.cpp$(DependSuffix) -MM bnChipSelectionCust.cpp

$(IntermediateDirectory)/bnChipSelectionCust.cpp$(PreprocessSuffix): bnChipSelectionCust.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnChipSelectionCust.cpp$(PreprocessSuffix) bnChipSelectionCust.cpp

$(IntermediateDirectory)/bnBuster.cpp$(ObjectSuffix): bnBuster.cpp $(IntermediateDirectory)/bnBuster.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnBuster.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnBuster.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnBuster.cpp$(DependSuffix): bnBuster.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnBuster.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnBuster.cpp$(DependSuffix) -MM bnBuster.cpp

$(IntermediateDirectory)/bnBuster.cpp$(PreprocessSuffix): bnBuster.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnBuster.cpp$(PreprocessSuffix) bnBuster.cpp

$(IntermediateDirectory)/bnPlayerHealthUI.cpp$(ObjectSuffix): bnPlayerHealthUI.cpp $(IntermediateDirectory)/bnPlayerHealthUI.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnPlayerHealthUI.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnPlayerHealthUI.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnPlayerHealthUI.cpp$(DependSuffix): bnPlayerHealthUI.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnPlayerHealthUI.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnPlayerHealthUI.cpp$(DependSuffix) -MM bnPlayerHealthUI.cpp

$(IntermediateDirectory)/bnPlayerHealthUI.cpp$(PreprocessSuffix): bnPlayerHealthUI.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnPlayerHealthUI.cpp$(PreprocessSuffix) bnPlayerHealthUI.cpp

$(IntermediateDirectory)/bnInputManager.cpp$(ObjectSuffix): bnInputManager.cpp $(IntermediateDirectory)/bnInputManager.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnInputManager.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnInputManager.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnInputManager.cpp$(DependSuffix): bnInputManager.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnInputManager.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnInputManager.cpp$(DependSuffix) -MM bnInputManager.cpp

$(IntermediateDirectory)/bnInputManager.cpp$(PreprocessSuffix): bnInputManager.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnInputManager.cpp$(PreprocessSuffix) bnInputManager.cpp

$(IntermediateDirectory)/bnRoll.cpp$(ObjectSuffix): bnRoll.cpp $(IntermediateDirectory)/bnRoll.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnRoll.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnRoll.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnRoll.cpp$(DependSuffix): bnRoll.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnRoll.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnRoll.cpp$(DependSuffix) -MM bnRoll.cpp

$(IntermediateDirectory)/bnRoll.cpp$(PreprocessSuffix): bnRoll.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnRoll.cpp$(PreprocessSuffix) bnRoll.cpp

$(IntermediateDirectory)/bnChipUseListener.cpp$(ObjectSuffix): bnChipUseListener.cpp $(IntermediateDirectory)/bnChipUseListener.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnChipUseListener.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnChipUseListener.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnChipUseListener.cpp$(DependSuffix): bnChipUseListener.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnChipUseListener.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnChipUseListener.cpp$(DependSuffix) -MM bnChipUseListener.cpp

$(IntermediateDirectory)/bnChipUseListener.cpp$(PreprocessSuffix): bnChipUseListener.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnChipUseListener.cpp$(PreprocessSuffix) bnChipUseListener.cpp

$(IntermediateDirectory)/bnSmartShader.cpp$(ObjectSuffix): bnSmartShader.cpp $(IntermediateDirectory)/bnSmartShader.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnSmartShader.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnSmartShader.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnSmartShader.cpp$(DependSuffix): bnSmartShader.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnSmartShader.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnSmartShader.cpp$(DependSuffix) -MM bnSmartShader.cpp

$(IntermediateDirectory)/bnSmartShader.cpp$(PreprocessSuffix): bnSmartShader.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnSmartShader.cpp$(PreprocessSuffix) bnSmartShader.cpp

$(IntermediateDirectory)/bnCanonSmoke.cpp$(ObjectSuffix): bnCanonSmoke.cpp $(IntermediateDirectory)/bnCanonSmoke.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnCanonSmoke.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnCanonSmoke.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnCanonSmoke.cpp$(DependSuffix): bnCanonSmoke.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnCanonSmoke.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnCanonSmoke.cpp$(DependSuffix) -MM bnCanonSmoke.cpp

$(IntermediateDirectory)/bnCanonSmoke.cpp$(PreprocessSuffix): bnCanonSmoke.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnCanonSmoke.cpp$(PreprocessSuffix) bnCanonSmoke.cpp

$(IntermediateDirectory)/bnAnimationComponent.cpp$(ObjectSuffix): bnAnimationComponent.cpp $(IntermediateDirectory)/bnAnimationComponent.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnAnimationComponent.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnAnimationComponent.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnAnimationComponent.cpp$(DependSuffix): bnAnimationComponent.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnAnimationComponent.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnAnimationComponent.cpp$(DependSuffix) -MM bnAnimationComponent.cpp

$(IntermediateDirectory)/bnAnimationComponent.cpp$(PreprocessSuffix): bnAnimationComponent.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnAnimationComponent.cpp$(PreprocessSuffix) bnAnimationComponent.cpp

$(IntermediateDirectory)/bnOverworldLight.cpp$(ObjectSuffix): bnOverworldLight.cpp $(IntermediateDirectory)/bnOverworldLight.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnOverworldLight.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnOverworldLight.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnOverworldLight.cpp$(DependSuffix): bnOverworldLight.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnOverworldLight.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnOverworldLight.cpp$(DependSuffix) -MM bnOverworldLight.cpp

$(IntermediateDirectory)/bnOverworldLight.cpp$(PreprocessSuffix): bnOverworldLight.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnOverworldLight.cpp$(PreprocessSuffix) bnOverworldLight.cpp

$(IntermediateDirectory)/bnDefenseAntiDamage.cpp$(ObjectSuffix): bnDefenseAntiDamage.cpp $(IntermediateDirectory)/bnDefenseAntiDamage.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnDefenseAntiDamage.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnDefenseAntiDamage.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnDefenseAntiDamage.cpp$(DependSuffix): bnDefenseAntiDamage.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnDefenseAntiDamage.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnDefenseAntiDamage.cpp$(DependSuffix) -MM bnDefenseAntiDamage.cpp

$(IntermediateDirectory)/bnDefenseAntiDamage.cpp$(PreprocessSuffix): bnDefenseAntiDamage.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnDefenseAntiDamage.cpp$(PreprocessSuffix) bnDefenseAntiDamage.cpp

$(IntermediateDirectory)/bnHideTimer.cpp$(ObjectSuffix): bnHideTimer.cpp $(IntermediateDirectory)/bnHideTimer.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnHideTimer.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnHideTimer.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnHideTimer.cpp$(DependSuffix): bnHideTimer.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnHideTimer.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnHideTimer.cpp$(DependSuffix) -MM bnHideTimer.cpp

$(IntermediateDirectory)/bnHideTimer.cpp$(PreprocessSuffix): bnHideTimer.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnHideTimer.cpp$(PreprocessSuffix) bnHideTimer.cpp

$(IntermediateDirectory)/bnProgsManPunchState.cpp$(ObjectSuffix): bnProgsManPunchState.cpp $(IntermediateDirectory)/bnProgsManPunchState.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnProgsManPunchState.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnProgsManPunchState.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnProgsManPunchState.cpp$(DependSuffix): bnProgsManPunchState.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnProgsManPunchState.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnProgsManPunchState.cpp$(DependSuffix) -MM bnProgsManPunchState.cpp

$(IntermediateDirectory)/bnProgsManPunchState.cpp$(PreprocessSuffix): bnProgsManPunchState.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnProgsManPunchState.cpp$(PreprocessSuffix) bnProgsManPunchState.cpp

$(IntermediateDirectory)/bnBubble.cpp$(ObjectSuffix): bnBubble.cpp $(IntermediateDirectory)/bnBubble.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnBubble.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnBubble.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnBubble.cpp$(DependSuffix): bnBubble.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnBubble.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnBubble.cpp$(DependSuffix) -MM bnBubble.cpp

$(IntermediateDirectory)/bnBubble.cpp$(PreprocessSuffix): bnBubble.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnBubble.cpp$(PreprocessSuffix) bnBubble.cpp

$(IntermediateDirectory)/bnChipLibrary.cpp$(ObjectSuffix): bnChipLibrary.cpp $(IntermediateDirectory)/bnChipLibrary.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnChipLibrary.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnChipLibrary.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnChipLibrary.cpp$(DependSuffix): bnChipLibrary.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnChipLibrary.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnChipLibrary.cpp$(DependSuffix) -MM bnChipLibrary.cpp

$(IntermediateDirectory)/bnChipLibrary.cpp$(PreprocessSuffix): bnChipLibrary.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnChipLibrary.cpp$(PreprocessSuffix) bnChipLibrary.cpp

$(IntermediateDirectory)/bnChipUsePublisher.cpp$(ObjectSuffix): bnChipUsePublisher.cpp $(IntermediateDirectory)/bnChipUsePublisher.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnChipUsePublisher.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnChipUsePublisher.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnChipUsePublisher.cpp$(DependSuffix): bnChipUsePublisher.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnChipUsePublisher.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnChipUsePublisher.cpp$(DependSuffix) -MM bnChipUsePublisher.cpp

$(IntermediateDirectory)/bnChipUsePublisher.cpp$(PreprocessSuffix): bnChipUsePublisher.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnChipUsePublisher.cpp$(PreprocessSuffix) bnChipUsePublisher.cpp

$(IntermediateDirectory)/bnStarfish.cpp$(ObjectSuffix): bnStarfish.cpp $(IntermediateDirectory)/bnStarfish.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnStarfish.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnStarfish.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnStarfish.cpp$(DependSuffix): bnStarfish.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnStarfish.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnStarfish.cpp$(DependSuffix) -MM bnStarfish.cpp

$(IntermediateDirectory)/bnStarfish.cpp$(PreprocessSuffix): bnStarfish.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnStarfish.cpp$(PreprocessSuffix) bnStarfish.cpp

$(IntermediateDirectory)/bnBasicSword.cpp$(ObjectSuffix): bnBasicSword.cpp $(IntermediateDirectory)/bnBasicSword.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnBasicSword.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnBasicSword.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnBasicSword.cpp$(DependSuffix): bnBasicSword.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnBasicSword.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnBasicSword.cpp$(DependSuffix) -MM bnBasicSword.cpp

$(IntermediateDirectory)/bnBasicSword.cpp$(PreprocessSuffix): bnBasicSword.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnBasicSword.cpp$(PreprocessSuffix) bnBasicSword.cpp

$(IntermediateDirectory)/bnRandomMettaurMob.cpp$(ObjectSuffix): bnRandomMettaurMob.cpp $(IntermediateDirectory)/bnRandomMettaurMob.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnRandomMettaurMob.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnRandomMettaurMob.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnRandomMettaurMob.cpp$(DependSuffix): bnRandomMettaurMob.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnRandomMettaurMob.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnRandomMettaurMob.cpp$(DependSuffix) -MM bnRandomMettaurMob.cpp

$(IntermediateDirectory)/bnRandomMettaurMob.cpp$(PreprocessSuffix): bnRandomMettaurMob.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnRandomMettaurMob.cpp$(PreprocessSuffix) bnRandomMettaurMob.cpp

$(IntermediateDirectory)/bnProgsManShootState.cpp$(ObjectSuffix): bnProgsManShootState.cpp $(IntermediateDirectory)/bnProgsManShootState.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnProgsManShootState.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnProgsManShootState.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnProgsManShootState.cpp$(DependSuffix): bnProgsManShootState.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnProgsManShootState.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnProgsManShootState.cpp$(DependSuffix) -MM bnProgsManShootState.cpp

$(IntermediateDirectory)/bnProgsManShootState.cpp$(PreprocessSuffix): bnProgsManShootState.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnProgsManShootState.cpp$(PreprocessSuffix) bnProgsManShootState.cpp

$(IntermediateDirectory)/bnAirShot.cpp$(ObjectSuffix): bnAirShot.cpp $(IntermediateDirectory)/bnAirShot.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnAirShot.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnAirShot.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnAirShot.cpp$(DependSuffix): bnAirShot.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnAirShot.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnAirShot.cpp$(DependSuffix) -MM bnAirShot.cpp

$(IntermediateDirectory)/bnAirShot.cpp$(PreprocessSuffix): bnAirShot.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnAirShot.cpp$(PreprocessSuffix) bnAirShot.cpp

$(IntermediateDirectory)/bnMetalManMoveState.cpp$(ObjectSuffix): bnMetalManMoveState.cpp $(IntermediateDirectory)/bnMetalManMoveState.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnMetalManMoveState.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnMetalManMoveState.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnMetalManMoveState.cpp$(DependSuffix): bnMetalManMoveState.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnMetalManMoveState.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnMetalManMoveState.cpp$(DependSuffix) -MM bnMetalManMoveState.cpp

$(IntermediateDirectory)/bnMetalManMoveState.cpp$(PreprocessSuffix): bnMetalManMoveState.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnMetalManMoveState.cpp$(PreprocessSuffix) bnMetalManMoveState.cpp

$(IntermediateDirectory)/bnVirusBackground.cpp$(ObjectSuffix): bnVirusBackground.cpp $(IntermediateDirectory)/bnVirusBackground.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnVirusBackground.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnVirusBackground.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnVirusBackground.cpp$(DependSuffix): bnVirusBackground.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnVirusBackground.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnVirusBackground.cpp$(DependSuffix) -MM bnVirusBackground.cpp

$(IntermediateDirectory)/bnVirusBackground.cpp$(PreprocessSuffix): bnVirusBackground.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnVirusBackground.cpp$(PreprocessSuffix) bnVirusBackground.cpp

$(IntermediateDirectory)/bnProgsManBossFight.cpp$(ObjectSuffix): bnProgsManBossFight.cpp $(IntermediateDirectory)/bnProgsManBossFight.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnProgsManBossFight.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnProgsManBossFight.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnProgsManBossFight.cpp$(DependSuffix): bnProgsManBossFight.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnProgsManBossFight.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnProgsManBossFight.cpp$(DependSuffix) -MM bnProgsManBossFight.cpp

$(IntermediateDirectory)/bnProgsManBossFight.cpp$(PreprocessSuffix): bnProgsManBossFight.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnProgsManBossFight.cpp$(PreprocessSuffix) bnProgsManBossFight.cpp

$(IntermediateDirectory)/bnProgBomb.cpp$(ObjectSuffix): bnProgBomb.cpp $(IntermediateDirectory)/bnProgBomb.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnProgBomb.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnProgBomb.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnProgBomb.cpp$(DependSuffix): bnProgBomb.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnProgBomb.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnProgBomb.cpp$(DependSuffix) -MM bnProgBomb.cpp

$(IntermediateDirectory)/bnProgBomb.cpp$(PreprocessSuffix): bnProgBomb.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnProgBomb.cpp$(PreprocessSuffix) bnProgBomb.cpp

$(IntermediateDirectory)/bnTwoMettaurMob.cpp$(ObjectSuffix): bnTwoMettaurMob.cpp $(IntermediateDirectory)/bnTwoMettaurMob.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnTwoMettaurMob.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnTwoMettaurMob.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnTwoMettaurMob.cpp$(DependSuffix): bnTwoMettaurMob.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnTwoMettaurMob.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnTwoMettaurMob.cpp$(DependSuffix) -MM bnTwoMettaurMob.cpp

$(IntermediateDirectory)/bnTwoMettaurMob.cpp$(PreprocessSuffix): bnTwoMettaurMob.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnTwoMettaurMob.cpp$(PreprocessSuffix) bnTwoMettaurMob.cpp

$(IntermediateDirectory)/bnMetalManIdleState.cpp$(ObjectSuffix): bnMetalManIdleState.cpp $(IntermediateDirectory)/bnMetalManIdleState.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnMetalManIdleState.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnMetalManIdleState.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnMetalManIdleState.cpp$(DependSuffix): bnMetalManIdleState.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnMetalManIdleState.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnMetalManIdleState.cpp$(DependSuffix) -MM bnMetalManIdleState.cpp

$(IntermediateDirectory)/bnMetalManIdleState.cpp$(PreprocessSuffix): bnMetalManIdleState.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnMetalManIdleState.cpp$(PreprocessSuffix) bnMetalManIdleState.cpp

$(IntermediateDirectory)/bnSelectedChipsUI.cpp$(ObjectSuffix): bnSelectedChipsUI.cpp $(IntermediateDirectory)/bnSelectedChipsUI.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnSelectedChipsUI.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnSelectedChipsUI.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnSelectedChipsUI.cpp$(DependSuffix): bnSelectedChipsUI.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnSelectedChipsUI.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnSelectedChipsUI.cpp$(DependSuffix) -MM bnSelectedChipsUI.cpp

$(IntermediateDirectory)/bnSelectedChipsUI.cpp$(PreprocessSuffix): bnSelectedChipsUI.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnSelectedChipsUI.cpp$(PreprocessSuffix) bnSelectedChipsUI.cpp

$(IntermediateDirectory)/bnNaviRegistration.cpp$(ObjectSuffix): bnNaviRegistration.cpp $(IntermediateDirectory)/bnNaviRegistration.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnNaviRegistration.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnNaviRegistration.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnNaviRegistration.cpp$(DependSuffix): bnNaviRegistration.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnNaviRegistration.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnNaviRegistration.cpp$(DependSuffix) -MM bnNaviRegistration.cpp

$(IntermediateDirectory)/bnNaviRegistration.cpp$(PreprocessSuffix): bnNaviRegistration.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnNaviRegistration.cpp$(PreprocessSuffix) bnNaviRegistration.cpp

$(IntermediateDirectory)/bnChipFolder.cpp$(ObjectSuffix): bnChipFolder.cpp $(IntermediateDirectory)/bnChipFolder.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnChipFolder.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnChipFolder.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnChipFolder.cpp$(DependSuffix): bnChipFolder.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnChipFolder.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnChipFolder.cpp$(DependSuffix) -MM bnChipFolder.cpp

$(IntermediateDirectory)/bnChipFolder.cpp$(PreprocessSuffix): bnChipFolder.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnChipFolder.cpp$(PreprocessSuffix) bnChipFolder.cpp

$(IntermediateDirectory)/bnCustEmblem.cpp$(ObjectSuffix): bnCustEmblem.cpp $(IntermediateDirectory)/bnCustEmblem.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnCustEmblem.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnCustEmblem.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnCustEmblem.cpp$(DependSuffix): bnCustEmblem.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnCustEmblem.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnCustEmblem.cpp$(DependSuffix) -MM bnCustEmblem.cpp

$(IntermediateDirectory)/bnCustEmblem.cpp$(PreprocessSuffix): bnCustEmblem.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnCustEmblem.cpp$(PreprocessSuffix) bnCustEmblem.cpp

$(IntermediateDirectory)/bnShaderResourceManager.cpp$(ObjectSuffix): bnShaderResourceManager.cpp $(IntermediateDirectory)/bnShaderResourceManager.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnShaderResourceManager.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnShaderResourceManager.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnShaderResourceManager.cpp$(DependSuffix): bnShaderResourceManager.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnShaderResourceManager.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnShaderResourceManager.cpp$(DependSuffix) -MM bnShaderResourceManager.cpp

$(IntermediateDirectory)/bnShaderResourceManager.cpp$(PreprocessSuffix): bnShaderResourceManager.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnShaderResourceManager.cpp$(PreprocessSuffix) bnShaderResourceManager.cpp

$(IntermediateDirectory)/bnDefenseBubbleWrap.cpp$(ObjectSuffix): bnDefenseBubbleWrap.cpp $(IntermediateDirectory)/bnDefenseBubbleWrap.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnDefenseBubbleWrap.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnDefenseBubbleWrap.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnDefenseBubbleWrap.cpp$(DependSuffix): bnDefenseBubbleWrap.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnDefenseBubbleWrap.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnDefenseBubbleWrap.cpp$(DependSuffix) -MM bnDefenseBubbleWrap.cpp

$(IntermediateDirectory)/bnDefenseBubbleWrap.cpp$(PreprocessSuffix): bnDefenseBubbleWrap.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnDefenseBubbleWrap.cpp$(PreprocessSuffix) bnDefenseBubbleWrap.cpp

$(IntermediateDirectory)/bnChip.cpp$(ObjectSuffix): bnChip.cpp $(IntermediateDirectory)/bnChip.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnChip.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnChip.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnChip.cpp$(DependSuffix): bnChip.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnChip.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnChip.cpp$(DependSuffix) -MM bnChip.cpp

$(IntermediateDirectory)/bnChip.cpp$(PreprocessSuffix): bnChip.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnChip.cpp$(PreprocessSuffix) bnChip.cpp

$(IntermediateDirectory)/bnAura.cpp$(ObjectSuffix): bnAura.cpp $(IntermediateDirectory)/bnAura.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnAura.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnAura.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnAura.cpp$(DependSuffix): bnAura.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnAura.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnAura.cpp$(DependSuffix) -MM bnAura.cpp

$(IntermediateDirectory)/bnAura.cpp$(PreprocessSuffix): bnAura.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnAura.cpp$(PreprocessSuffix) bnAura.cpp

$(IntermediateDirectory)/bnProgsManThrowState.cpp$(ObjectSuffix): bnProgsManThrowState.cpp $(IntermediateDirectory)/bnProgsManThrowState.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnProgsManThrowState.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnProgsManThrowState.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnProgsManThrowState.cpp$(DependSuffix): bnProgsManThrowState.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnProgsManThrowState.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnProgsManThrowState.cpp$(DependSuffix) -MM bnProgsManThrowState.cpp

$(IntermediateDirectory)/bnProgsManThrowState.cpp$(PreprocessSuffix): bnProgsManThrowState.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnProgsManThrowState.cpp$(PreprocessSuffix) bnProgsManThrowState.cpp

$(IntermediateDirectory)/bnField.cpp$(ObjectSuffix): bnField.cpp $(IntermediateDirectory)/bnField.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnField.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnField.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnField.cpp$(DependSuffix): bnField.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnField.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnField.cpp$(DependSuffix) -MM bnField.cpp

$(IntermediateDirectory)/bnField.cpp$(PreprocessSuffix): bnField.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnField.cpp$(PreprocessSuffix) bnField.cpp

$(IntermediateDirectory)/bnPlayerControlledState.cpp$(ObjectSuffix): bnPlayerControlledState.cpp $(IntermediateDirectory)/bnPlayerControlledState.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnPlayerControlledState.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnPlayerControlledState.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnPlayerControlledState.cpp$(DependSuffix): bnPlayerControlledState.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnPlayerControlledState.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnPlayerControlledState.cpp$(DependSuffix) -MM bnPlayerControlledState.cpp

$(IntermediateDirectory)/bnPlayerControlledState.cpp$(PreprocessSuffix): bnPlayerControlledState.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnPlayerControlledState.cpp$(PreprocessSuffix) bnPlayerControlledState.cpp

$(IntermediateDirectory)/bnFishy.cpp$(ObjectSuffix): bnFishy.cpp $(IntermediateDirectory)/bnFishy.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnFishy.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnFishy.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnFishy.cpp$(DependSuffix): bnFishy.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnFishy.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnFishy.cpp$(DependSuffix) -MM bnFishy.cpp

$(IntermediateDirectory)/bnFishy.cpp$(PreprocessSuffix): bnFishy.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnFishy.cpp$(PreprocessSuffix) bnFishy.cpp

$(IntermediateDirectory)/bnChargeComponent.cpp$(ObjectSuffix): bnChargeComponent.cpp $(IntermediateDirectory)/bnChargeComponent.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnChargeComponent.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnChargeComponent.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnChargeComponent.cpp$(DependSuffix): bnChargeComponent.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnChargeComponent.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnChargeComponent.cpp$(DependSuffix) -MM bnChargeComponent.cpp

$(IntermediateDirectory)/bnChargeComponent.cpp$(PreprocessSuffix): bnChargeComponent.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnChargeComponent.cpp$(PreprocessSuffix) bnChargeComponent.cpp

$(IntermediateDirectory)/bnGridBackground.cpp$(ObjectSuffix): bnGridBackground.cpp $(IntermediateDirectory)/bnGridBackground.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnGridBackground.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnGridBackground.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnGridBackground.cpp$(DependSuffix): bnGridBackground.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnGridBackground.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnGridBackground.cpp$(DependSuffix) -MM bnGridBackground.cpp

$(IntermediateDirectory)/bnGridBackground.cpp$(PreprocessSuffix): bnGridBackground.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnGridBackground.cpp$(PreprocessSuffix) bnGridBackground.cpp

$(IntermediateDirectory)/bnMetalManBossFight.cpp$(ObjectSuffix): bnMetalManBossFight.cpp $(IntermediateDirectory)/bnMetalManBossFight.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnMetalManBossFight.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnMetalManBossFight.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnMetalManBossFight.cpp$(DependSuffix): bnMetalManBossFight.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnMetalManBossFight.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnMetalManBossFight.cpp$(DependSuffix) -MM bnMetalManBossFight.cpp

$(IntermediateDirectory)/bnMetalManBossFight.cpp$(PreprocessSuffix): bnMetalManBossFight.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnMetalManBossFight.cpp$(PreprocessSuffix) bnMetalManBossFight.cpp

$(IntermediateDirectory)/bnProgsManHitState.cpp$(ObjectSuffix): bnProgsManHitState.cpp $(IntermediateDirectory)/bnProgsManHitState.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnProgsManHitState.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnProgsManHitState.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnProgsManHitState.cpp$(DependSuffix): bnProgsManHitState.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnProgsManHitState.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnProgsManHitState.cpp$(DependSuffix) -MM bnProgsManHitState.cpp

$(IntermediateDirectory)/bnProgsManHitState.cpp$(PreprocessSuffix): bnProgsManHitState.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnProgsManHitState.cpp$(PreprocessSuffix) bnProgsManHitState.cpp

$(IntermediateDirectory)/bnAnimatedCharacter.cpp$(ObjectSuffix): bnAnimatedCharacter.cpp $(IntermediateDirectory)/bnAnimatedCharacter.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnAnimatedCharacter.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnAnimatedCharacter.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnAnimatedCharacter.cpp$(DependSuffix): bnAnimatedCharacter.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnAnimatedCharacter.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnAnimatedCharacter.cpp$(DependSuffix) -MM bnAnimatedCharacter.cpp

$(IntermediateDirectory)/bnAnimatedCharacter.cpp$(PreprocessSuffix): bnAnimatedCharacter.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnAnimatedCharacter.cpp$(PreprocessSuffix) bnAnimatedCharacter.cpp

$(IntermediateDirectory)/bnAnimate.cpp$(ObjectSuffix): bnAnimate.cpp $(IntermediateDirectory)/bnAnimate.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnAnimate.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnAnimate.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnAnimate.cpp$(DependSuffix): bnAnimate.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnAnimate.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnAnimate.cpp$(DependSuffix) -MM bnAnimate.cpp

$(IntermediateDirectory)/bnAnimate.cpp$(PreprocessSuffix): bnAnimate.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnAnimate.cpp$(PreprocessSuffix) bnAnimate.cpp

$(IntermediateDirectory)/bnEngine.cpp$(ObjectSuffix): bnEngine.cpp $(IntermediateDirectory)/bnEngine.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnEngine.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnEngine.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnEngine.cpp$(DependSuffix): bnEngine.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnEngine.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnEngine.cpp$(DependSuffix) -MM bnEngine.cpp

$(IntermediateDirectory)/bnEngine.cpp$(PreprocessSuffix): bnEngine.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnEngine.cpp$(PreprocessSuffix) bnEngine.cpp

$(IntermediateDirectory)/bnSpell.cpp$(ObjectSuffix): bnSpell.cpp $(IntermediateDirectory)/bnSpell.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnSpell.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnSpell.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnSpell.cpp$(DependSuffix): bnSpell.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnSpell.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnSpell.cpp$(DependSuffix) -MM bnSpell.cpp

$(IntermediateDirectory)/bnSpell.cpp$(PreprocessSuffix): bnSpell.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnSpell.cpp$(PreprocessSuffix) bnSpell.cpp

$(IntermediateDirectory)/bnSelectNaviScene.cpp$(ObjectSuffix): bnSelectNaviScene.cpp $(IntermediateDirectory)/bnSelectNaviScene.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnSelectNaviScene.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnSelectNaviScene.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnSelectNaviScene.cpp$(DependSuffix): bnSelectNaviScene.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnSelectNaviScene.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnSelectNaviScene.cpp$(DependSuffix) -MM bnSelectNaviScene.cpp

$(IntermediateDirectory)/bnSelectNaviScene.cpp$(PreprocessSuffix): bnSelectNaviScene.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnSelectNaviScene.cpp$(PreprocessSuffix) bnSelectNaviScene.cpp

$(IntermediateDirectory)/bnFolderScene.cpp$(ObjectSuffix): bnFolderScene.cpp $(IntermediateDirectory)/bnFolderScene.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnFolderScene.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnFolderScene.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnFolderScene.cpp$(DependSuffix): bnFolderScene.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnFolderScene.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnFolderScene.cpp$(DependSuffix) -MM bnFolderScene.cpp

$(IntermediateDirectory)/bnFolderScene.cpp$(PreprocessSuffix): bnFolderScene.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnFolderScene.cpp$(PreprocessSuffix) bnFolderScene.cpp

$(IntermediateDirectory)/bnMetalBlade.cpp$(ObjectSuffix): bnMetalBlade.cpp $(IntermediateDirectory)/bnMetalBlade.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnMetalBlade.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnMetalBlade.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnMetalBlade.cpp$(DependSuffix): bnMetalBlade.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnMetalBlade.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnMetalBlade.cpp$(DependSuffix) -MM bnMetalBlade.cpp

$(IntermediateDirectory)/bnMetalBlade.cpp$(PreprocessSuffix): bnMetalBlade.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnMetalBlade.cpp$(PreprocessSuffix) bnMetalBlade.cpp

$(IntermediateDirectory)/bnBattleScene.cpp$(ObjectSuffix): bnBattleScene.cpp $(IntermediateDirectory)/bnBattleScene.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnBattleScene.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnBattleScene.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnBattleScene.cpp$(DependSuffix): bnBattleScene.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnBattleScene.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnBattleScene.cpp$(DependSuffix) -MM bnBattleScene.cpp

$(IntermediateDirectory)/bnBattleScene.cpp$(PreprocessSuffix): bnBattleScene.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnBattleScene.cpp$(PreprocessSuffix) bnBattleScene.cpp

$(IntermediateDirectory)/bnCube.cpp$(ObjectSuffix): bnCube.cpp $(IntermediateDirectory)/bnCube.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnCube.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnCube.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnCube.cpp$(DependSuffix): bnCube.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnCube.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnCube.cpp$(DependSuffix) -MM bnCube.cpp

$(IntermediateDirectory)/bnCube.cpp$(PreprocessSuffix): bnCube.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnCube.cpp$(PreprocessSuffix) bnCube.cpp

$(IntermediateDirectory)/bnPlayerIdleState.cpp$(ObjectSuffix): bnPlayerIdleState.cpp $(IntermediateDirectory)/bnPlayerIdleState.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnPlayerIdleState.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnPlayerIdleState.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnPlayerIdleState.cpp$(DependSuffix): bnPlayerIdleState.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnPlayerIdleState.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnPlayerIdleState.cpp$(DependSuffix) -MM bnPlayerIdleState.cpp

$(IntermediateDirectory)/bnPlayerIdleState.cpp$(PreprocessSuffix): bnPlayerIdleState.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnPlayerIdleState.cpp$(PreprocessSuffix) bnPlayerIdleState.cpp

$(IntermediateDirectory)/bnReflectShield.cpp$(ObjectSuffix): bnReflectShield.cpp $(IntermediateDirectory)/bnReflectShield.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnReflectShield.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnReflectShield.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnReflectShield.cpp$(DependSuffix): bnReflectShield.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnReflectShield.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnReflectShield.cpp$(DependSuffix) -MM bnReflectShield.cpp

$(IntermediateDirectory)/bnReflectShield.cpp$(PreprocessSuffix): bnReflectShield.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnReflectShield.cpp$(PreprocessSuffix) bnReflectShield.cpp

$(IntermediateDirectory)/bnDefenseRule.cpp$(ObjectSuffix): bnDefenseRule.cpp $(IntermediateDirectory)/bnDefenseRule.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnDefenseRule.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnDefenseRule.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnDefenseRule.cpp$(DependSuffix): bnDefenseRule.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnDefenseRule.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnDefenseRule.cpp$(DependSuffix) -MM bnDefenseRule.cpp

$(IntermediateDirectory)/bnDefenseRule.cpp$(PreprocessSuffix): bnDefenseRule.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnDefenseRule.cpp$(PreprocessSuffix) bnDefenseRule.cpp

$(IntermediateDirectory)/bnAnimation.cpp$(ObjectSuffix): bnAnimation.cpp $(IntermediateDirectory)/bnAnimation.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnAnimation.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnAnimation.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnAnimation.cpp$(DependSuffix): bnAnimation.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnAnimation.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnAnimation.cpp$(DependSuffix) -MM bnAnimation.cpp

$(IntermediateDirectory)/bnAnimation.cpp$(PreprocessSuffix): bnAnimation.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnAnimation.cpp$(PreprocessSuffix) bnAnimation.cpp

$(IntermediateDirectory)/bnCanodumbCursor.cpp$(ObjectSuffix): bnCanodumbCursor.cpp $(IntermediateDirectory)/bnCanodumbCursor.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnCanodumbCursor.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnCanodumbCursor.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnCanodumbCursor.cpp$(DependSuffix): bnCanodumbCursor.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnCanodumbCursor.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnCanodumbCursor.cpp$(DependSuffix) -MM bnCanodumbCursor.cpp

$(IntermediateDirectory)/bnCanodumbCursor.cpp$(PreprocessSuffix): bnCanodumbCursor.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnCanodumbCursor.cpp$(PreprocessSuffix) bnCanodumbCursor.cpp

$(IntermediateDirectory)/bnRockDebris.cpp$(ObjectSuffix): bnRockDebris.cpp $(IntermediateDirectory)/bnRockDebris.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnRockDebris.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnRockDebris.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnRockDebris.cpp$(DependSuffix): bnRockDebris.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnRockDebris.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnRockDebris.cpp$(DependSuffix) -MM bnRockDebris.cpp

$(IntermediateDirectory)/bnRockDebris.cpp$(PreprocessSuffix): bnRockDebris.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnRockDebris.cpp$(PreprocessSuffix) bnRockDebris.cpp

$(IntermediateDirectory)/bnBubbleTrap.cpp$(ObjectSuffix): bnBubbleTrap.cpp $(IntermediateDirectory)/bnBubbleTrap.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnBubbleTrap.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnBubbleTrap.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnBubbleTrap.cpp$(DependSuffix): bnBubbleTrap.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnBubbleTrap.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnBubbleTrap.cpp$(DependSuffix) -MM bnBubbleTrap.cpp

$(IntermediateDirectory)/bnBubbleTrap.cpp$(PreprocessSuffix): bnBubbleTrap.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnBubbleTrap.cpp$(PreprocessSuffix) bnBubbleTrap.cpp

$(IntermediateDirectory)/bnAnimatedTextBox.cpp$(ObjectSuffix): bnAnimatedTextBox.cpp $(IntermediateDirectory)/bnAnimatedTextBox.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnAnimatedTextBox.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnAnimatedTextBox.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnAnimatedTextBox.cpp$(DependSuffix): bnAnimatedTextBox.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnAnimatedTextBox.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnAnimatedTextBox.cpp$(DependSuffix) -MM bnAnimatedTextBox.cpp

$(IntermediateDirectory)/bnAnimatedTextBox.cpp$(PreprocessSuffix): bnAnimatedTextBox.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnAnimatedTextBox.cpp$(PreprocessSuffix) bnAnimatedTextBox.cpp

$(IntermediateDirectory)/bnMetalManThrowState.cpp$(ObjectSuffix): bnMetalManThrowState.cpp $(IntermediateDirectory)/bnMetalManThrowState.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnMetalManThrowState.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnMetalManThrowState.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnMetalManThrowState.cpp$(DependSuffix): bnMetalManThrowState.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnMetalManThrowState.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnMetalManThrowState.cpp$(DependSuffix) -MM bnMetalManThrowState.cpp

$(IntermediateDirectory)/bnMetalManThrowState.cpp$(PreprocessSuffix): bnMetalManThrowState.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnMetalManThrowState.cpp$(PreprocessSuffix) bnMetalManThrowState.cpp

$(IntermediateDirectory)/bnLogger.cpp$(ObjectSuffix): bnLogger.cpp $(IntermediateDirectory)/bnLogger.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnLogger.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnLogger.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnLogger.cpp$(DependSuffix): bnLogger.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnLogger.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnLogger.cpp$(DependSuffix) -MM bnLogger.cpp

$(IntermediateDirectory)/bnLogger.cpp$(PreprocessSuffix): bnLogger.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnLogger.cpp$(PreprocessSuffix) bnLogger.cpp

$(IntermediateDirectory)/bnCanodumbIdleState.cpp$(ObjectSuffix): bnCanodumbIdleState.cpp $(IntermediateDirectory)/bnCanodumbIdleState.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnCanodumbIdleState.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnCanodumbIdleState.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnCanodumbIdleState.cpp$(DependSuffix): bnCanodumbIdleState.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnCanodumbIdleState.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnCanodumbIdleState.cpp$(DependSuffix) -MM bnCanodumbIdleState.cpp

$(IntermediateDirectory)/bnCanodumbIdleState.cpp$(PreprocessSuffix): bnCanodumbIdleState.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnCanodumbIdleState.cpp$(PreprocessSuffix) bnCanodumbIdleState.cpp

$(IntermediateDirectory)/mmbn.ico.c$(ObjectSuffix): mmbn.ico.c $(IntermediateDirectory)/mmbn.ico.c$(DependSuffix)
	$(CC) $(SourceSwitch) $(ProjectPath)/mmbn.ico.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/mmbn.ico.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/mmbn.ico.c$(DependSuffix): mmbn.ico.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/mmbn.ico.c$(ObjectSuffix) -MF$(IntermediateDirectory)/mmbn.ico.c$(DependSuffix) -MM mmbn.ico.c

$(IntermediateDirectory)/mmbn.ico.c$(PreprocessSuffix): mmbn.ico.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/mmbn.ico.c$(PreprocessSuffix) mmbn.ico.c

$(IntermediateDirectory)/bnTile.cpp$(ObjectSuffix): bnTile.cpp $(IntermediateDirectory)/bnTile.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnTile.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnTile.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnTile.cpp$(DependSuffix): bnTile.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnTile.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnTile.cpp$(DependSuffix) -MM bnTile.cpp

$(IntermediateDirectory)/bnTile.cpp$(PreprocessSuffix): bnTile.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnTile.cpp$(PreprocessSuffix) bnTile.cpp

$(IntermediateDirectory)/bnNinjaAntiDamage.cpp$(ObjectSuffix): bnNinjaAntiDamage.cpp $(IntermediateDirectory)/bnNinjaAntiDamage.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnNinjaAntiDamage.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnNinjaAntiDamage.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnNinjaAntiDamage.cpp$(DependSuffix): bnNinjaAntiDamage.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnNinjaAntiDamage.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnNinjaAntiDamage.cpp$(DependSuffix) -MM bnNinjaAntiDamage.cpp

$(IntermediateDirectory)/bnNinjaAntiDamage.cpp$(PreprocessSuffix): bnNinjaAntiDamage.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnNinjaAntiDamage.cpp$(PreprocessSuffix) bnNinjaAntiDamage.cpp

$(IntermediateDirectory)/bnInvis.cpp$(ObjectSuffix): bnInvis.cpp $(IntermediateDirectory)/bnInvis.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnInvis.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnInvis.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnInvis.cpp$(DependSuffix): bnInvis.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnInvis.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnInvis.cpp$(DependSuffix) -MM bnInvis.cpp

$(IntermediateDirectory)/bnInvis.cpp$(PreprocessSuffix): bnInvis.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnInvis.cpp$(PreprocessSuffix) bnInvis.cpp

$(IntermediateDirectory)/bnProgsManMoveState.cpp$(ObjectSuffix): bnProgsManMoveState.cpp $(IntermediateDirectory)/bnProgsManMoveState.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnProgsManMoveState.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnProgsManMoveState.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnProgsManMoveState.cpp$(DependSuffix): bnProgsManMoveState.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnProgsManMoveState.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnProgsManMoveState.cpp$(DependSuffix) -MM bnProgsManMoveState.cpp

$(IntermediateDirectory)/bnProgsManMoveState.cpp$(PreprocessSuffix): bnProgsManMoveState.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnProgsManMoveState.cpp$(PreprocessSuffix) bnProgsManMoveState.cpp

$(IntermediateDirectory)/bnBattleResults.cpp$(ObjectSuffix): bnBattleResults.cpp $(IntermediateDirectory)/bnBattleResults.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnBattleResults.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnBattleResults.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnBattleResults.cpp$(DependSuffix): bnBattleResults.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnBattleResults.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnBattleResults.cpp$(DependSuffix) -MM bnBattleResults.cpp

$(IntermediateDirectory)/bnBattleResults.cpp$(PreprocessSuffix): bnBattleResults.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnBattleResults.cpp$(PreprocessSuffix) bnBattleResults.cpp

$(IntermediateDirectory)/bnPA.cpp$(ObjectSuffix): bnPA.cpp $(IntermediateDirectory)/bnPA.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnPA.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnPA.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnPA.cpp$(DependSuffix): bnPA.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnPA.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnPA.cpp$(DependSuffix) -MM bnPA.cpp

$(IntermediateDirectory)/bnPA.cpp$(PreprocessSuffix): bnPA.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnPA.cpp$(PreprocessSuffix) bnPA.cpp

$(IntermediateDirectory)/bnMobRegistration.cpp$(ObjectSuffix): bnMobRegistration.cpp $(IntermediateDirectory)/bnMobRegistration.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) $(ProjectPath)/bnMobRegistration.cpp $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bnMobRegistration.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bnMobRegistration.cpp$(DependSuffix): bnMobRegistration.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bnMobRegistration.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bnMobRegistration.cpp$(DependSuffix) -MM bnMobRegistration.cpp

$(IntermediateDirectory)/bnMobRegistration.cpp$(PreprocessSuffix): bnMobRegistration.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bnMobRegistration.cpp$(PreprocessSuffix) bnMobRegistration.cpp


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r ./Debug/



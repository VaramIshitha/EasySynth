// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#include "RendererTargets/RendererTarget.h"

#include "ILevelSequenceEditorToolkit.h"
#include "ISequencer.h"
#include "LevelSequence.h"
#include "Sections/MovieSceneCameraCutSection.h"
#include "Subsystems/AssetEditorSubsystem.h"


TArray<UCameraComponent*> FRendererTarget::GetCameras(ULevelSequence* LevelSequence)
{
	TArray<UCameraComponent*> Cameras;

	UMovieScene* MovieScene = LevelSequence->GetMovieScene();
	if (MovieScene == nullptr)
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not get the movie scene from the level sequence"),
			*FString(__FUNCTION__))
		return Cameras;
	}

	UMovieSceneTrack* CameraCutTrack = MovieScene->GetCameraCutTrack();
	if (CameraCutTrack == nullptr)
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not get the camera cut track from the movie scene"),
			*FString(__FUNCTION__))
		return Cameras;
	}

	TArray<UMovieSceneSection*> MovieSceneSections = CameraCutTrack->GetAllSections();
	if (MovieSceneSections.Num() == 0)
	{
		UE_LOG(LogEasySynth, Warning, TEXT("%s: No sections inside the camera cut track"), *FString(__FUNCTION__))
		return Cameras;
	}

	// Open sequencer editor for the level sequence asset
	TArray<UObject*> Assets;
	Assets.Add(LevelSequence);
	if (!GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(Assets))
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not open the level sequence editor"), *FString(__FUNCTION__))
		return Cameras;
	}

	// Get the LevelSequenceEditor
	IAssetEditorInstance* AssetEditor =
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(LevelSequence, false);
	if (AssetEditor == nullptr)
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not find the asset editor"), *FString(__FUNCTION__))
		return Cameras;
	}
	ILevelSequenceEditorToolkit* LevelSequenceEditor = static_cast<ILevelSequenceEditorToolkit*>(AssetEditor);
	if (LevelSequenceEditor == nullptr)
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not find the level sequence editor"), *FString(__FUNCTION__))
		return Cameras;
	}

	// Get the Sequencer
	TWeakPtr<ISequencer> WeakSequencer = LevelSequenceEditor ? LevelSequenceEditor->GetSequencer() : nullptr;
	if (!WeakSequencer.IsValid())
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not get the sequencer"), *FString(__FUNCTION__))
		return Cameras;
	}

	// Get the camera from each section
	for (UMovieSceneSection* MovieSceneSection : MovieSceneSections)
	{
		// Convert UMovieSceneSection to UMovieSceneCameraCutSection
		auto CutSection = Cast<UMovieSceneCameraCutSection>(MovieSceneSection);
		if (CutSection == nullptr)
		{
			UE_LOG(LogEasySynth, Error, TEXT("%s: Could not convert MovieSceneSection into a CutSection"),
				*FString(__FUNCTION__));
			Cameras.Empty();
			return Cameras;
		}

		// Get the camera componenet
		UCameraComponent* Camera =
			CutSection->GetFirstCamera(*WeakSequencer.Pin(), WeakSequencer.Pin()->GetFocusedTemplateID());
		if (Camera == nullptr)
		{
			UE_LOG(LogEasySynth, Error, TEXT("%s: Cut section camera component is null"), *FString(__FUNCTION__))
			Cameras.Empty();
			return Cameras;
		}
		Cameras.Add(Camera);
	}

	return Cameras;
}

bool FRendererTarget::ClearCameraPostProcess(ULevelSequence* LevelSequence)
{
	// Get all camera components bound to the level sequence
	TArray<UCameraComponent*> Cameras = GetCameras(LevelSequence);
	if (Cameras.Num() == 0)
	{
		UE_LOG(LogEasySynth, Warning, TEXT("%s: No cameras bound to the level sequence found"), *FString(__FUNCTION__))
		return false;
	}

	// Clear post process materials
	for (UCameraComponent* Camera : Cameras)
	{
		if (Camera == nullptr)
		{
			UE_LOG(LogEasySynth, Error, TEXT("%s: Found camera is null"), *FString(__FUNCTION__))
			return false;
		}
		Camera->PostProcessSettings.WeightedBlendables.Array.Empty();
	}

	return true;
}
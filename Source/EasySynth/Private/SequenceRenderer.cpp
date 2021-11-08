// Copyright Ydrive 2021

#include "SequenceRenderer.h"

#include "MoviePipelineQueueSubsystem.h"
#include "MovieRenderPipelineSettings.h"


const FString FSequenceRenderer::EasySynthMoviePipelineConfigPath("/EasySynth/EasySynthMoviePipelineConfig");

FSequenceRenderer::FSequenceRenderer() :
	bCurrentlyRendering(false),
	ErrorMessage("")
{}

bool FSequenceRenderer::RenderSequence(ULevelSequence* LevelSequence, FSequenceRendererTargets RenderingTargets)
{
	UE_LOG(LogEasySynth, Log, TEXT("%s"), *FString(__FUNCTION__))

	// Check if rendering is already in progress
	if (bCurrentlyRendering)
	{
		ErrorMessage = "Rendering already in progress";
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	// Check if LevelSequence is valid
	Sequence = LevelSequence;
	if (Sequence == nullptr)
	{
		ErrorMessage = "Provided level sequence is null";
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	// Check if any rendering target is selected
	RequestedSequenceRendererTargets = RenderingTargets;
	if (!RequestedSequenceRendererTargets.AnyOptionSelected())
	{
		ErrorMessage = "No rendering targets selected";
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
		return false;
	}

	// Starting the next target rendering will increase the CurrentTarget from -1 to 0
	CurrentTarget = -1;
	bCurrentlyRendering = true;

	UE_LOG(LogEasySynth, Log, TEXT("%s: Rendering..."), *FString(__FUNCTION__))

	// Find the next target and start rendering
	FindNextTarget();

	return true;
}

void FSequenceRenderer::OnExecutorFinished(UMoviePipelineExecutorBase* InPipelineExecutor, bool bSuccess)
{
	if (!bSuccess)
	{
		ErrorMessage = FString::Printf(
			TEXT("Failed while rendering the %s target"),
			*FSequenceRendererTargets::TargetName(CurrentTarget));
		return BroadcastRenderingFinished(false);
	}

	// Successful rendering, proceed to the next target
	FindNextTarget();
}

void FSequenceRenderer::FindNextTarget()
{
	// Find the next requested target
	while (++CurrentTarget != FSequenceRendererTargets::COUNT &&
		!RequestedSequenceRendererTargets.TargetSelected(CurrentTarget)) {}

	// Check if the end is reached
	if (CurrentTarget == FSequenceRendererTargets::COUNT)
	{
		return BroadcastRenderingFinished(true);
	}

	// TODO: Setup specifics of the current rendering target
	UE_LOG(LogEasySynth, Log, TEXT("%s: Rendering the %s target"),
		*FString(__FUNCTION__), *FSequenceRendererTargets::TargetName(CurrentTarget))

	// Start the rendering after a brief pause
	const float DelaySeconds = 2.0f;
	const bool bLoop = false;
	GEditor->GetEditorWorldContext().World()->GetTimerManager().SetTimer(
		RendererPauseTimerHandle,
		FTimerDelegate::CreateRaw(this, &FSequenceRenderer::StartRendering),
		DelaySeconds,
		bLoop);
}

void FSequenceRenderer::StartRendering()
{
	// Make sure the sequence is still sound
	if (Sequence == nullptr)
	{
		ErrorMessage = "Provided level sequence is null";
		return BroadcastRenderingFinished(false);
	}

	// Load the asset representing needed movie pipeline configuration
	UMoviePipelineMasterConfig* EasySynthMoviePipelineConfig =
		LoadObject<UMoviePipelineMasterConfig>(nullptr, *EasySynthMoviePipelineConfigPath);
	if (EasySynthMoviePipelineConfig == nullptr)
	{
		ErrorMessage = "Could not load the EasySynthMoviePipelineConfig";
		return BroadcastRenderingFinished(false);
	}

	// Get the movie rendering editor subsystem
	MoviePipelineQueueSubsystem = GEditor->GetEditorSubsystem<UMoviePipelineQueueSubsystem>();
	if (MoviePipelineQueueSubsystem == nullptr)
	{
		ErrorMessage = "Could not get the UMoviePipelineQueueSubsystem";
		return BroadcastRenderingFinished(false);
	}

	// Get the queue of sequences to be renderer
	UMoviePipelineQueue* MoviePipelineQueue = MoviePipelineQueueSubsystem->GetQueue();
	if (MoviePipelineQueue == nullptr)
	{
		ErrorMessage = "Could not get the UMoviePipelineQueue";
		return BroadcastRenderingFinished(false);
	}
	MoviePipelineQueue->Modify();

	// Clear the current queue
	TArray<UMoviePipelineExecutorJob*> ExistingJobs = MoviePipelineQueue->GetJobs();
	for (UMoviePipelineExecutorJob* MoviePipelineExecutorJob : ExistingJobs)
	{
		MoviePipelineQueue->DeleteJob(MoviePipelineExecutorJob);
	}
	if (MoviePipelineQueue->GetJobs().Num() > 0)
	{
		ErrorMessage = "Job queue not properly cleared";
		return BroadcastRenderingFinished(false);
	}

	// Add received level sequence to the queue as a new job
	UMoviePipelineExecutorJob* NewJob = MoviePipelineQueue->AllocateNewJob(UMoviePipelineExecutorJob::StaticClass());
	if (NewJob == nullptr)
	{
		ErrorMessage = "Failed to create new rendering job";
		return BroadcastRenderingFinished(false);
	}
	NewJob->Modify();
	NewJob->Map = FSoftObjectPath(GEditor->GetEditorWorldContext().World());
	NewJob->Author = FPlatformProcess::UserName(false);
	NewJob->SetSequence(Sequence);
	NewJob->JobName = NewJob->Sequence.GetAssetName();
	NewJob->SetConfiguration(EasySynthMoviePipelineConfig);

	// Get the default movie rendering settings
	const UMovieRenderPipelineProjectSettings* ProjectSettings = GetDefault<UMovieRenderPipelineProjectSettings>();
	if (ProjectSettings->DefaultLocalExecutor == nullptr)
	{
		ErrorMessage = "Could not get the UMovieRenderPipelineProjectSettings";
		return BroadcastRenderingFinished(false);;
	}

	// Run the rendering
	UMoviePipelineExecutorBase* ActiveExecutor =
		MoviePipelineQueueSubsystem->RenderQueueWithExecutor(ProjectSettings->DefaultLocalExecutor);
	if (ActiveExecutor == nullptr)
	{
		ErrorMessage = "Could not start the rendering";
		return BroadcastRenderingFinished(false);;
	}

	// Assign rendering finished callback
	ActiveExecutor->OnExecutorFinished().AddRaw(this, &FSequenceRenderer::OnExecutorFinished);
	// TODO: Bind other events, such as rendering canceled
}

void FSequenceRenderer::BroadcastRenderingFinished(bool bSuccess)
{
	if (!bSuccess)
	{
		UE_LOG(LogEasySynth, Warning, TEXT("%s: %s"), *FString(__FUNCTION__), *ErrorMessage)
	}
	bCurrentlyRendering = false;
	DelegateRenderingFinished.Broadcast(bSuccess);
}

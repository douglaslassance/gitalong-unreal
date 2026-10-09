// Copyright (c) 2014-2022 Sebastien Rombauts (sebastien.rombauts@gmail.com)
// Copyright (c) 2021-2026 Douglas Lassance
//
// Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
// or copy at http://opensource.org/licenses/MIT)

#include "GitSourceControlOperations.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "SourceControlOperations.h"
#include "ISourceControlModule.h"
#include "GitSourceControlModule.h"
#include "GitSourceControlCommand.h"
#include "GitSourceControlUtils.h"
#include "Logging/MessageLog.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "GitSourceControl"

FName FGitPush::GetName() const
{
	return "Push";
}

FText FGitPush::GetInProgressString() const
{
	// TODO Configure origin
	return LOCTEXT("SourceControl_Push", "Pushing local commits to remote origin...");
}

/** Tell if a Git hook installed by "gitalong setup --update-hooks" already runs "gitalong update" */
static bool HasGitalongHook(const FString& InRepositoryRoot, const TCHAR* InHookName)
{
	FString Hook;
	const FString HookPath = FPaths::Combine(InRepositoryRoot, TEXT(".git/hooks"), InHookName);
	return FFileHelper::LoadFileToString(Hook, *HookPath) && Hook.Contains(TEXT("gitalong update"));
}

/** Publish the local changes of this clone so that the rest of the team sees them */
static void UpdateGitalong(FGitSourceControlCommand& InCommand)
{
	TArray<FString> InfoMessages;
	TArray<FString> ErrorMessages;
	if (!GitSourceControlUtils::RunGitalongUpdate(InCommand.PathToGitalongBinary, InCommand.PathToGitBinary, InCommand.PathToRepositoryRoot, InfoMessages, ErrorMessages))
	{
		// The local operation already succeeded, only the team awareness is late
		for (const FString& ErrorMessage : ErrorMessages)
		{
			InCommand.InfoMessages.Add(FString::Printf(TEXT("gitalong update: %s"), *ErrorMessage));
		}
	}
}

/** Fetch the remote so that files changed there show as not current */
static void FetchRemote(FGitSourceControlCommand& InCommand)
{
	TArray<FString> Parameters;
	Parameters.Add(TEXT("--quiet"));
	TArray<FString> InfoMessages;
	TArray<FString> ErrorMessages;
	if (!GitSourceControlUtils::RunCommand(TEXT("fetch"), InCommand.PathToGitBinary, InCommand.PathToRepositoryRoot, Parameters, TArray<FString>(), InfoMessages, ErrorMessages))
	{
		// Working offline is fine, the status is only less up to date
		for (const FString& ErrorMessage : ErrorMessages)
		{
			InCommand.InfoMessages.Add(FString::Printf(TEXT("git fetch: %s"), *ErrorMessage));
		}
	}
}

FName FGitConnectWorker::GetName() const
{
	return "Connect";
}

bool FGitConnectWorker::Execute(FGitSourceControlCommand& InCommand)
{
	check(InCommand.Operation->GetName() == GetName());
	TSharedRef<FConnect, ESPMode::ThreadSafe> Operation = StaticCastSharedRef<FConnect>(InCommand.Operation);

	// Check Git Availability
	if((InCommand.PathToGitBinary.Len() == 0) || !GitSourceControlUtils::CheckGitAvailability(InCommand.PathToGitBinary))
	{
		Operation->SetErrorText(LOCTEXT("GitNotFound", "Failed to enable Gitalong revision control. You need to install Git and specify a valid path to git executable."));
		InCommand.bCommandSuccessful = false;
		return false;
	}

	// Check Gitalong Availability
	if((InCommand.PathToGitalongBinary.Len() == 0) || !GitSourceControlUtils::CheckGitalongAvailability(InCommand.PathToGitalongBinary))
	{
		Operation->SetErrorText(LOCTEXT("GitalongNotFound", "Failed to enable Gitalong revision control. You need to install Gitalong 1.1 or later and specify a valid path to gitalong executable."));
		InCommand.bCommandSuccessful = false;
		return false;
	}

	// The checkout workflow depends on the Gitalong configuration of the repository
	FGitalongConfig GitalongConfig;
	const bool bGitalongSetUp = GitSourceControlUtils::GetGitalongConfig(InCommand.PathToGitalongBinary, InCommand.PathToRepositoryRoot, GitalongConfig);
	InCommand.bUsesCheckout = GitalongConfig.bTrackUncommitted;
	if (!bGitalongSetUp)
	{
		InCommand.PathToGitalongBinary.Empty();
	}
	FetchRemote(InCommand);

	// Now update the status of assets in Content/ directory and also Config files
	TArray<FString> ProjectDirs;
	ProjectDirs.Add(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()));
	ProjectDirs.Add(FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir()));
	InCommand.bCommandSuccessful = GitSourceControlUtils::RunUpdateStatus(InCommand, ProjectDirs, InCommand.ErrorMessages, States);
	if(!InCommand.bCommandSuccessful || InCommand.ErrorMessages.Num() > 0)
	{
		Operation->SetErrorText(LOCTEXT("NotAGitRepository", "Failed to enable Gitalong revision control. You need to initialize the project as a Git repository first."));
		InCommand.bCommandSuccessful = false;
		return false;
	}
	if (!bGitalongSetUp)
	{
		Operation->SetErrorText(LOCTEXT("NotAGitalongRepository", "Failed to enable Gitalong revision control. You need to set up the repository by running 'gitalong setup' first."));
		InCommand.bCommandSuccessful = false;
		return false;
	}

	InCommand.GitalongConfig = GitalongConfig;
	GitSourceControlUtils::GetCommitInfo(InCommand.PathToGitBinary, InCommand.PathToRepositoryRoot, InCommand.CommitId, InCommand.CommitSummary);

	// Publish what changed locally while the editor was closed
	UpdateGitalong(InCommand);

	return InCommand.bCommandSuccessful;
}

bool FGitConnectWorker::UpdateStates() const
{
	return GitSourceControlUtils::UpdateCachedStates(States);
}

FName FGitCheckOutWorker::GetName() const
{
	return "CheckOut";
}

bool FGitCheckOutWorker::Execute(FGitSourceControlCommand& InCommand)
{
	check(InCommand.Operation->GetName() == GetName());

	TMap<FString, FGitalongStatus> Blockers;
	InCommand.bCommandSuccessful = GitSourceControlUtils::RunGitalongClaim(InCommand.PathToGitalongBinary, InCommand.PathToGitBinary, InCommand.PathToRepositoryRoot, InCommand.Files, Blockers, InCommand.ErrorMessages);
	for (const TPair<FString, FGitalongStatus>& Blocker : Blockers)
	{
		const FString Filename = FPaths::GetCleanFilename(Blocker.Key);
		if (Blocker.Value.Sha.IsEmpty())
		{
			InCommand.ErrorMessages.Add(FString::Printf(TEXT("%s is claimed by %s."), *Filename, *Blocker.Value.GetHolder()));
		}
		else
		{
			InCommand.ErrorMessages.Add(FString::Printf(TEXT("%s was changed by %s in commit %s on branch %s, which is missing from the active branch."), *Filename, *Blocker.Value.GetHolder(), *Blocker.Value.Sha.Left(8), *Blocker.Value.GetBranch()));
		}
	}
	InCommand.bCommandSuccessful &= Blockers.Num() == 0;

	// now update the status of our files
	GitSourceControlUtils::RunUpdateStatus(InCommand, InCommand.Files, InCommand.ErrorMessages, States);

	return InCommand.bCommandSuccessful;
}

bool FGitCheckOutWorker::UpdateStates() const
{
	return GitSourceControlUtils::UpdateCachedStates(States);
}

static FText ParseCommitResults(const TArray<FString>& InResults)
{
	if(InResults.Num() >= 1)
	{
		const FString& FirstLine = InResults[0];
		return FText::Format(LOCTEXT("CommitMessage", "Commited {0}."), FText::FromString(FirstLine));
	}
	return LOCTEXT("CommitMessageUnknown", "Submitted revision.");
}

FName FGitCheckInWorker::GetName() const
{
	return "CheckIn";
}

bool FGitCheckInWorker::Execute(FGitSourceControlCommand& InCommand)
{
	check(InCommand.Operation->GetName() == GetName());

	TSharedRef<FCheckIn, ESPMode::ThreadSafe> Operation = StaticCastSharedRef<FCheckIn>(InCommand.Operation);

	// make a temp file to place our commit message in
	FGitScopedTempFile CommitMsgFile(Operation->GetDescription());
	if(CommitMsgFile.GetFilename().Len() > 0)
	{
		TArray<FString> Parameters;
		FString ParamCommitMsgFilename = TEXT("--file=\"");
		ParamCommitMsgFilename += FPaths::ConvertRelativePathToFull(CommitMsgFile.GetFilename());
		ParamCommitMsgFilename += TEXT("\"");
		Parameters.Add(ParamCommitMsgFilename);

		InCommand.bCommandSuccessful = GitSourceControlUtils::RunCommit(InCommand.PathToGitBinary, InCommand.PathToRepositoryRoot, Parameters, InCommand.Files, InCommand.InfoMessages, InCommand.ErrorMessages);
		if(InCommand.bCommandSuccessful)
		{
			// Remove any deleted files from status cache
			FGitSourceControlModule& GitSourceControl = FModuleManager::GetModuleChecked<FGitSourceControlModule>("GitSourceControl");
			FGitSourceControlProvider& Provider = GitSourceControl.GetProvider();

			TArray<TSharedRef<ISourceControlState, ESPMode::ThreadSafe>> LocalStates;
			Provider.GetState(InCommand.Files, LocalStates, EStateCacheUsage::Use);
			for(const auto& State : LocalStates)
			{
				if(State->IsDeleted())
				{
					Provider.RemoveFileFromCache(State->GetFilename());
				}
			}

			Operation->SetSuccessMessage(ParseCommitResults(InCommand.InfoMessages));
			const FString Message = (InCommand.InfoMessages.Num() > 0) ? InCommand.InfoMessages[0] : TEXT("");
			UE_LOG(LogSourceControl, Log, TEXT("commit successful: %s"), *Message);

			// The committed files stay claimed until pushed
			if (!HasGitalongHook(InCommand.PathToRepositoryRoot, TEXT("post-commit")))
			{
				UpdateGitalong(InCommand);
			}
		}
	}

	// now update the status of our files
	GitSourceControlUtils::RunUpdateStatus(InCommand, InCommand.Files, InCommand.ErrorMessages, States);
	GitSourceControlUtils::GetCommitInfo(InCommand.PathToGitBinary, InCommand.PathToRepositoryRoot, InCommand.CommitId, InCommand.CommitSummary);

	return InCommand.bCommandSuccessful;
}

bool FGitCheckInWorker::UpdateStates() const
{
	return GitSourceControlUtils::UpdateCachedStates(States);
}

FName FGitMarkForAddWorker::GetName() const
{
	return "MarkForAdd";
}

bool FGitMarkForAddWorker::Execute(FGitSourceControlCommand& InCommand)
{
	check(InCommand.Operation->GetName() == GetName());

	InCommand.bCommandSuccessful = GitSourceControlUtils::RunCommand(TEXT("add"), InCommand.PathToGitBinary, InCommand.PathToRepositoryRoot, TArray<FString>(), InCommand.Files, InCommand.InfoMessages, InCommand.ErrorMessages);

	// now update the status of our files
	GitSourceControlUtils::RunUpdateStatus(InCommand, InCommand.Files, InCommand.ErrorMessages, States);

	return InCommand.bCommandSuccessful;
}

bool FGitMarkForAddWorker::UpdateStates() const
{
	return GitSourceControlUtils::UpdateCachedStates(States);
}

FName FGitDeleteWorker::GetName() const
{
	return "Delete";
}

bool FGitDeleteWorker::Execute(FGitSourceControlCommand& InCommand)
{
	check(InCommand.Operation->GetName() == GetName());

	InCommand.bCommandSuccessful = GitSourceControlUtils::RunCommand(TEXT("rm"), InCommand.PathToGitBinary, InCommand.PathToRepositoryRoot, TArray<FString>(), InCommand.Files, InCommand.InfoMessages, InCommand.ErrorMessages);
	if (InCommand.bCommandSuccessful)
	{
		UpdateGitalong(InCommand);
	}

	// now update the status of our files
	GitSourceControlUtils::RunUpdateStatus(InCommand, InCommand.Files, InCommand.ErrorMessages, States);

	return InCommand.bCommandSuccessful;
}

bool FGitDeleteWorker::UpdateStates() const
{
	return GitSourceControlUtils::UpdateCachedStates(States);
}


// Get lists of Missing files (ie "deleted"), Modified files, and "other than Added" Existing files
void GetMissingVsExistingFiles(const TArray<FString>& InFiles, TArray<FString>& OutMissingFiles, TArray<FString>& OutAllExistingFiles, TArray<FString>& OutOtherThanAddedExistingFiles)
{
	FGitSourceControlModule& GitSourceControl = FModuleManager::GetModuleChecked<FGitSourceControlModule>("GitSourceControl");
	FGitSourceControlProvider& Provider = GitSourceControl.GetProvider();

	const TArray<FString> Files = (InFiles.Num() > 0) ? (InFiles) : (Provider.GetFilesInCache());

	TArray<TSharedRef<ISourceControlState, ESPMode::ThreadSafe>> LocalStates;
	Provider.GetState(Files, LocalStates, EStateCacheUsage::Use);
	for(const auto& State : LocalStates)
	{
		if(FPaths::FileExists(State->GetFilename()))
		{
			if(State->IsAdded())
			{
				OutAllExistingFiles.Add(State->GetFilename());
			}
			else if(State->IsModified())
			{
				OutOtherThanAddedExistingFiles.Add(State->GetFilename());
				OutAllExistingFiles.Add(State->GetFilename());
			}
			else if(State->CanRevert()) // for claimed but unmodified files
			{
				OutOtherThanAddedExistingFiles.Add(State->GetFilename());
			}
		}
		else
		{
			if (State->IsSourceControlled())
			{
				OutMissingFiles.Add(State->GetFilename());
			}
		}
	}
}

FName FGitRevertWorker::GetName() const
{
	return "Revert";
}

bool FGitRevertWorker::Execute(FGitSourceControlCommand& InCommand)
{
	// Filter files by status to use the right "revert" commands on them
	TArray<FString> MissingFiles;
	TArray<FString> AllExistingFiles;
	TArray<FString> OtherThanAddedExistingFiles;
	GetMissingVsExistingFiles(InCommand.Files, MissingFiles, AllExistingFiles, OtherThanAddedExistingFiles);

	InCommand.bCommandSuccessful = true;
	if(MissingFiles.Num() > 0)
	{
		// "Added" files that have been deleted needs to be removed from source control
		InCommand.bCommandSuccessful &= GitSourceControlUtils::RunCommand(TEXT("rm"), InCommand.PathToGitBinary, InCommand.PathToRepositoryRoot, TArray<FString>(), MissingFiles, InCommand.InfoMessages, InCommand.ErrorMessages);
	}
	if(AllExistingFiles.Num() > 0)
	{
		// reset any changes already added to the index
		InCommand.bCommandSuccessful &= GitSourceControlUtils::RunCommand(TEXT("reset"), InCommand.PathToGitBinary, InCommand.PathToRepositoryRoot, TArray<FString>(), AllExistingFiles, InCommand.InfoMessages, InCommand.ErrorMessages);
	}
	if(OtherThanAddedExistingFiles.Num() > 0)
	{
		// revert any changes in working copy (this would fails if the asset was in "Added" state, since after "reset" it is now "untracked")
		InCommand.bCommandSuccessful &= GitSourceControlUtils::RunCommand(TEXT("checkout"), InCommand.PathToGitBinary, InCommand.PathToRepositoryRoot, TArray<FString>(), OtherThanAddedExistingFiles, InCommand.InfoMessages, InCommand.ErrorMessages);
	}

	// Release the claims, reverted files are no longer changed by this clone
	UpdateGitalong(InCommand);

	// If no files were specified (full revert), refresh all relevant files instead of the specified files (which is an empty list in full revert)
	// This is required so that files that were "Marked for add" have their status updated after a full revert.
	TArray<FString> FilesToUpdate = InCommand.Files;
	if (InCommand.Files.Num() <= 0)
	{
		for (const auto& File : MissingFiles) FilesToUpdate.Add(File);
		for (const auto& File : AllExistingFiles) FilesToUpdate.Add(File);
		for (const auto& File : OtherThanAddedExistingFiles) FilesToUpdate.Add(File);
	}

	// now update the status of our files
	GitSourceControlUtils::RunUpdateStatus(InCommand, FilesToUpdate, InCommand.ErrorMessages, States);

	return InCommand.bCommandSuccessful;
}

bool FGitRevertWorker::UpdateStates() const
{
	return GitSourceControlUtils::UpdateCachedStates(States);
}

FName FGitSyncWorker::GetName() const
{
	return "Sync";
}

bool FGitSyncWorker::Execute(FGitSourceControlCommand& InCommand)
{
	// pull the branch to get remote changes by rebasing any local commits (not merging them to avoid complex graphs)
	TArray<FString> Parameters;
	Parameters.Add(TEXT("--rebase"));
	Parameters.Add(TEXT("--autostash"));
	// TODO Configure origin
	Parameters.Add(TEXT("origin"));
	Parameters.Add(TEXT("HEAD"));
	InCommand.bCommandSuccessful = GitSourceControlUtils::RunCommand(TEXT("pull"), InCommand.PathToGitBinary, InCommand.PathToRepositoryRoot, Parameters, TArray<FString>(), InCommand.InfoMessages, InCommand.ErrorMessages);

	// Rebased local commits got new identifiers, publish them
	UpdateGitalong(InCommand);

	// now update the status of our files
	GitSourceControlUtils::RunUpdateStatus(InCommand, InCommand.Files, InCommand.ErrorMessages, States);
	GitSourceControlUtils::GetCommitInfo(InCommand.PathToGitBinary, InCommand.PathToRepositoryRoot, InCommand.CommitId, InCommand.CommitSummary);

	return InCommand.bCommandSuccessful;
}

bool FGitSyncWorker::UpdateStates() const
{
	return GitSourceControlUtils::UpdateCachedStates(States);
}


FName FGitPushWorker::GetName() const
{
	return "Push";
}

bool FGitPushWorker::Execute(FGitSourceControlCommand& InCommand)
{
	// push the branch to its default remote
	// (works only if the default remote "origin" is set and does not require authentication)
	TArray<FString> Parameters;
	Parameters.Add(TEXT("--set-upstream"));
	// TODO Configure origin
	Parameters.Add(TEXT("origin"));
	Parameters.Add(TEXT("HEAD"));
	InCommand.bCommandSuccessful = GitSourceControlUtils::RunCommand(TEXT("push"), InCommand.PathToGitBinary, InCommand.PathToRepositoryRoot, Parameters, TArray<FString>(), InCommand.InfoMessages, InCommand.ErrorMessages);

	if (InCommand.bCommandSuccessful)
	{
		// Pushed commits are no longer local to this clone, which releases their files for the rest of the team
		UpdateGitalong(InCommand);

		// Refresh the files we knew were claimed by this clone
		FGitSourceControlModule& GitSourceControl = FModuleManager::GetModuleChecked<FGitSourceControlModule>("GitSourceControl");
		TArray<FSourceControlStateRef> ClaimedStates = GitSourceControl.GetProvider().GetCachedStateByPredicate([](const FSourceControlStateRef& InState)
		{
			return static_cast<const FGitSourceControlState&>(InState.Get()).Gitalong.IsClaimedByMe();
		});
		TArray<FString> ClaimedFiles;
		for (const FSourceControlStateRef& State : ClaimedStates)
		{
			ClaimedFiles.Add(State->GetFilename());
		}
		if (ClaimedFiles.Num() > 0)
		{
			GitSourceControlUtils::RunUpdateStatus(InCommand, ClaimedFiles, InCommand.ErrorMessages, States);
		}
	}

	return InCommand.bCommandSuccessful;
}

bool FGitPushWorker::UpdateStates() const
{
	return GitSourceControlUtils::UpdateCachedStates(States);
}

FName FGitUpdateStatusWorker::GetName() const
{
	return "UpdateStatus";
}

bool FGitUpdateStatusWorker::Execute(FGitSourceControlCommand& InCommand)
{
	check(InCommand.Operation->GetName() == GetName());

	TSharedRef<FUpdateStatus, ESPMode::ThreadSafe> Operation = StaticCastSharedRef<FUpdateStatus>(InCommand.Operation);

	if (Operation->ShouldCheckAllFiles())
	{
		FetchRemote(InCommand);
	}

	if(InCommand.Files.Num() > 0)
	{
		InCommand.bCommandSuccessful = GitSourceControlUtils::RunUpdateStatus(InCommand, InCommand.Files, InCommand.ErrorMessages, States);
		GitSourceControlUtils::RemoveRedundantErrors(InCommand, TEXT("' is outside repository"));

		if(Operation->ShouldUpdateHistory())
		{
			for(const FString& File : InCommand.Files)
			{
				TGitSourceControlHistory History;

				const FGitSourceControlState* State = States.FindByPredicate([&File](const FGitSourceControlState& InState) { return InState.LocalFilename == File; });
				if(State && State->IsConflicted())
				{
					// In case of a merge conflict, we first need to get the tip of the "remote branch" (MERGE_HEAD)
					GitSourceControlUtils::RunGetHistory(InCommand.PathToGitBinary, InCommand.PathToRepositoryRoot, File, true, InCommand.ErrorMessages, History);
				}
				// Get the history of the file in the current branch
				InCommand.bCommandSuccessful &= GitSourceControlUtils::RunGetHistory(InCommand.PathToGitBinary, InCommand.PathToRepositoryRoot, File, false, InCommand.ErrorMessages, History);
				Histories.Add(*File, History);
			}
		}
	}
	else
	{
		// no path provided: only update the status of assets in Content/ directory and also Config files
		TArray<FString> ProjectDirs;
		ProjectDirs.Add(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()));
		ProjectDirs.Add(FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir()));
		InCommand.bCommandSuccessful = GitSourceControlUtils::RunUpdateStatus(InCommand, ProjectDirs, InCommand.ErrorMessages, States);
	}

	GitSourceControlUtils::GetCommitInfo(InCommand.PathToGitBinary, InCommand.PathToRepositoryRoot, InCommand.CommitId, InCommand.CommitSummary);

	// don't use the ShouldUpdateModifiedState() hint here as it is specific to Perforce: the above normal Git status has already told us this information (like Git and Mercurial)

	return InCommand.bCommandSuccessful;
}

bool FGitUpdateStatusWorker::UpdateStates() const
{
	bool bUpdated = GitSourceControlUtils::UpdateCachedStates(States);

	FGitSourceControlModule& GitSourceControl = FModuleManager::GetModuleChecked<FGitSourceControlModule>( "GitSourceControl" );
	FGitSourceControlProvider& Provider = GitSourceControl.GetProvider();

	const FDateTime Now = FDateTime::Now();

	// add history, if any
	for(const auto& History : Histories)
	{
		TSharedRef<FGitSourceControlState, ESPMode::ThreadSafe> State = Provider.GetStateInternal(History.Key);
		State->History = History.Value;
		State->TimeStamp = Now;
		bUpdated = true;
	}

	return bUpdated;
}

FName FGitCopyWorker::GetName() const
{
	return "Copy";
}

bool FGitCopyWorker::Execute(FGitSourceControlCommand& InCommand)
{
	check(InCommand.Operation->GetName() == GetName());

	// Copy or Move operation on a single file : Git does not need an explicit copy nor move,
	// but after a Move the Editor create a redirector file with the old asset name that points to the new asset.
	// The redirector needs to be commited with the new asset to perform a real rename.
	// => the following is to "MarkForAdd" the redirector, but it still need to be committed by selecting the whole directory and "check-in"
	InCommand.bCommandSuccessful = GitSourceControlUtils::RunCommand(TEXT("add"), InCommand.PathToGitBinary, InCommand.PathToRepositoryRoot, TArray<FString>(), InCommand.Files, InCommand.InfoMessages, InCommand.ErrorMessages);
	if (InCommand.bCommandSuccessful)
	{
		UpdateGitalong(InCommand);
	}

	return InCommand.bCommandSuccessful;
}

bool FGitCopyWorker::UpdateStates() const
{
	return GitSourceControlUtils::UpdateCachedStates(States);
}

FName FGitResolveWorker::GetName() const
{
	return "Resolve";
}

bool FGitResolveWorker::Execute( class FGitSourceControlCommand& InCommand )
{
	check(InCommand.Operation->GetName() == GetName());

	// mark the conflicting files as resolved:
	TArray<FString> Results;
	InCommand.bCommandSuccessful = GitSourceControlUtils::RunCommand(TEXT("add"), InCommand.PathToGitBinary, InCommand.PathToRepositoryRoot, TArray<FString>(), InCommand.Files, Results, InCommand.ErrorMessages);

	// now update the status of our files
	GitSourceControlUtils::RunUpdateStatus(InCommand, InCommand.Files, InCommand.ErrorMessages, States);

	return InCommand.bCommandSuccessful;
}

bool FGitResolveWorker::UpdateStates() const
{
	return GitSourceControlUtils::UpdateCachedStates(States);
}

#undef LOCTEXT_NAMESPACE

// Copyright Epic Games, Inc. All Rights Reserved.

#include "GitSourceControlState.h"

#define LOCTEXT_NAMESPACE "GitSourceControl.State"

int32 FGitSourceControlState::GetHistorySize() const
{
	return History.Num();
}

TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FGitSourceControlState::GetHistoryItem( int32 HistoryIndex ) const
{
	check(History.IsValidIndex(HistoryIndex));
	return History[HistoryIndex];
}

TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FGitSourceControlState::FindHistoryRevision( int32 RevisionNumber ) const
{
	for(const auto& Revision : History)
	{
		if(Revision->GetRevisionNumber() == RevisionNumber)
		{
			return Revision;
		}
	}

	return nullptr;
}

TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FGitSourceControlState::FindHistoryRevision(const FString& InRevision) const
{
	for(const auto& Revision : History)
	{
		if(Revision->GetRevision() == InRevision)
		{
			return Revision;
		}
	}

	return nullptr;
}

TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FGitSourceControlState::GetBaseRevForMerge() const
{
	for(const auto& Revision : History)
	{
		// look for the the SHA1 id of the file, not the commit id (revision)
		if(Revision->FileHash == PendingMergeBaseFileHash)
		{
			return Revision;
		}
	}

	return nullptr;
}

// @todo add Slate icons for git specific states (NotAtHead vs Conflicted...)
FName FGitSourceControlState::GetIconName() const
{
	switch(WorkingCopyState)
	{
	case EWorkingCopyState::Modified:
		return FName("Perforce.CheckedOut");
	case EWorkingCopyState::Added:
		return FName("Perforce.OpenForAdd");
	case EWorkingCopyState::Renamed:
	case EWorkingCopyState::Copied:
		return FName("Perforce.Branched");
	case EWorkingCopyState::Deleted: // Deleted & Missing files does not show in Content Browser
	case EWorkingCopyState::Missing:
		return FName("Perforce.MarkedForDelete");
	case EWorkingCopyState::Conflicted:
		return FName("Perforce.NotAtHeadRevision");
	case EWorkingCopyState::NotControlled:
		return FName("Perforce.NotInDepot");
	case EWorkingCopyState::Unknown:
	case EWorkingCopyState::Unchanged: // Unchanged is the same as "Pristine" (not checked out) for Perforce, ie no icon
	case EWorkingCopyState::Ignored:
	default:
		if (IsCheckedOut())
		{
			return FName("Perforce.CheckedOut");
		}
		if (IsCheckedOutOther())
		{
			return FName("Perforce.CheckedOutByOtherUser");
		}
		if (!IsCurrent())
		{
			return FName("Perforce.NotAtHeadRevision");
		}
		return NAME_None;
	}
}

FName FGitSourceControlState::GetSmallIconName() const
{
	switch(WorkingCopyState)
	{
	case EWorkingCopyState::Modified:
		return FName("Perforce.CheckedOut_Small");
	case EWorkingCopyState::Added:
		return FName("Perforce.OpenForAdd_Small");
	case EWorkingCopyState::Renamed:
	case EWorkingCopyState::Copied:
		return FName("Perforce.Branched_Small");
	case EWorkingCopyState::Deleted: // Deleted & Missing files can appear in the Submit to Source Control window
	case EWorkingCopyState::Missing:
		return FName("Perforce.MarkedForDelete_Small");
	case EWorkingCopyState::Conflicted:
		return FName("Perforce.NotAtHeadRevision_Small");
	case EWorkingCopyState::NotControlled:
		return FName("Perforce.NotInDepot_Small");
	case EWorkingCopyState::Unknown:
	case EWorkingCopyState::Unchanged: // Unchanged is the same as "Pristine" (not checked out) for Perforce, ie no icon
	case EWorkingCopyState::Ignored:
	default:
		if (IsCheckedOut())
		{
			return FName("Perforce.CheckedOut_Small");
		}
		if (IsCheckedOutOther())
		{
			return FName("Perforce.CheckedOutByOtherUser_Small");
		}
		if (!IsCurrent())
		{
			return FName("Perforce.NotAtHeadRevision_Small");
		}
		return NAME_None;
	}
}

FText FGitSourceControlState::GetDisplayName() const
{
	switch(WorkingCopyState)
	{
	case EWorkingCopyState::Added:
		return LOCTEXT("Added", "Added");
	case EWorkingCopyState::Deleted:
		return LOCTEXT("Deleted", "Deleted");
	case EWorkingCopyState::Modified:
		return LOCTEXT("Modified", "Modified");
	case EWorkingCopyState::Renamed:
		return LOCTEXT("Renamed", "Renamed");
	case EWorkingCopyState::Copied:
		return LOCTEXT("Copied", "Copied");
	case EWorkingCopyState::Conflicted:
		return LOCTEXT("ContentsConflict", "Contents conflict");
	case EWorkingCopyState::Ignored:
		return LOCTEXT("Ignored", "Ignored");
	case EWorkingCopyState::NotControlled:
		return LOCTEXT("NotControlled", "Not under source control");
	case EWorkingCopyState::Missing:
		return LOCTEXT("Missing", "Missing");
	case EWorkingCopyState::Unknown:
	case EWorkingCopyState::Unchanged: // Unchanged is the same as "Pristine" (not checked out) for Perforce, ie no icon
	default:
		if (IsCheckedOut())
		{
			return LOCTEXT("CheckedOut", "Checked out");
		}
		if (IsCheckedOutOther())
		{
			return FText::Format(LOCTEXT("CheckedOutOther", "Checked out by: {0}"), FText::FromString(LastCommitAuthor));
		}
		if (!IsCurrent())
		{
			return LOCTEXT("NotAtRevision", "Not at revision");
		}
		return FText();
		
	}
}

FText FGitSourceControlState::GetDisplayTooltip() const
{
	switch(WorkingCopyState)
	{
	case EWorkingCopyState::Added:
		return LOCTEXT("Added_Tooltip", "Item is scheduled for addition");
	case EWorkingCopyState::Deleted:
		return LOCTEXT("Deleted_Tooltip", "Item is scheduled for deletion");
	case EWorkingCopyState::Modified:
		return LOCTEXT("Modified_Tooltip", "Item has been modified");
	case EWorkingCopyState::Renamed:
		return LOCTEXT("Renamed_Tooltip", "Item has been renamed");
	case EWorkingCopyState::Copied:
		return LOCTEXT("Copied_Tooltip", "Item has been copied");
	case EWorkingCopyState::Conflicted:
		return LOCTEXT("ContentsConflict_Tooltip", "The contents of the item conflict with updates received from the repository.");
	case EWorkingCopyState::Ignored:
		return LOCTEXT("Ignored_Tooltip", "Item is being ignored.");
	case EWorkingCopyState::NotControlled:
		return LOCTEXT("NotControlled_Tooltip", "Item is not under version control.");
	case EWorkingCopyState::Missing:
		return LOCTEXT("Missing_Tooltip", "Item is missing (e.g., you moved or deleted it without using Git). This also indicates that a directory is incomplete (a checkout or update was interrupted).");
	case EWorkingCopyState::Unknown:
	case EWorkingCopyState::Unchanged: // Unchanged is the same as "Pristine" (not checked out) for Perforce, ie no icon
	default:
		if (IsCheckedOut())
		{
			return LOCTEXT("CheckedOut", "Checked out");
		}
		if (IsCheckedOutOther())
		{
			return FText::Format(LOCTEXT("CheckedOutOther", "Checked out by: {0}"), FText::FromString(LastCommitAuthor));
		}
		if (!IsCurrent())
		{
			return LOCTEXT("NotAtRevision", "Not at revision");
		}
		return FText();
	}
}

const FString& FGitSourceControlState::GetFilename() const
{
	return LocalFilename;
}

const FDateTime& FGitSourceControlState::GetTimeStamp() const
{
	return TimeStamp;
}

// Deleted and Missing assets cannot appear in the Content Browser, but the do in the Submit files to Source Control window!
bool FGitSourceControlState::CanCheckIn() const
{
	return WorkingCopyState == EWorkingCopyState::Added
		|| WorkingCopyState == EWorkingCopyState::Deleted
		|| WorkingCopyState == EWorkingCopyState::Missing
		|| WorkingCopyState == EWorkingCopyState::Modified
		|| WorkingCopyState == EWorkingCopyState::Renamed;
}

bool FGitSourceControlState::CanCheckout() const
{
	if (LastCommitSpread == ECommitSpread::Unknown) {
		return false;
	}
	return IsCurrent();
}

bool FGitSourceControlState::IsCheckedOut() const
{
	if (LastCommitSpread == ECommitSpread::Unknown) {
		return false; // With Git all tracked files in the working copy are always checked-out (as opposed to Perforce)
	}
	if ((LastCommitSpread & ECommitSpread::LocalUncommitted) == ECommitSpread::LocalUncommitted)
	{
		return true;
	}
	const bool LocalActiveBranch = (LastCommitSpread & ECommitSpread::LocalActiveBranch) == ECommitSpread::LocalActiveBranch;
	const bool RemoteMatchingBranch = (LastCommitSpread & ECommitSpread::RemoteMatchingBranch) ==  ECommitSpread::RemoteMatchingBranch;
	return LocalActiveBranch && !RemoteMatchingBranch;
}

bool FGitSourceControlState::IsCheckedOutOther(FString* Who) const
{
	if (LastCommitSpread == ECommitSpread::Unknown) {
		return false; // Git does not lock checked-out files as Perforce does
	}
	const bool LocalUncommitted = (LastCommitSpread & ECommitSpread::LocalUncommitted) == ECommitSpread::LocalUncommitted;
	const bool LocalActiveBranch = (LastCommitSpread & ECommitSpread::LocalActiveBranch) == ECommitSpread::LocalActiveBranch;
	const bool RemoteMatchingBranch = (LastCommitSpread & ECommitSpread::RemoteMatchingBranch) ==  ECommitSpread::RemoteMatchingBranch;
	return (!LocalUncommitted && !LocalActiveBranch && !RemoteMatchingBranch);
}

bool FGitSourceControlState::IsCurrent() const
{
	if (LastCommitSpread == ECommitSpread::Unknown) {
		return true;
	}
	const bool LocalUncommitted = (LastCommitSpread & ECommitSpread::LocalUncommitted) == ECommitSpread::LocalUncommitted;
	const bool LocalActiveBranch = (LastCommitSpread & ECommitSpread::LocalActiveBranch) == ECommitSpread::LocalActiveBranch;
	return (LocalUncommitted || LocalActiveBranch);
}

bool FGitSourceControlState::IsSourceControlled() const
{
	return WorkingCopyState != EWorkingCopyState::NotControlled && WorkingCopyState != EWorkingCopyState::Ignored && WorkingCopyState != EWorkingCopyState::Unknown;
}

bool FGitSourceControlState::IsAdded() const
{
	return WorkingCopyState == EWorkingCopyState::Added;
}

bool FGitSourceControlState::IsDeleted() const
{
	return WorkingCopyState == EWorkingCopyState::Deleted || WorkingCopyState == EWorkingCopyState::Missing;
}

bool FGitSourceControlState::IsIgnored() const
{
	return WorkingCopyState == EWorkingCopyState::Ignored;
}

bool FGitSourceControlState::CanEdit() const
{
	if (LastCommitSpread == ECommitSpread::Unknown) {
		return true; // With Git all files in the working copy are always editable (as opposed to Perforce)
	}
	return IsCurrent();
}

bool FGitSourceControlState::CanDelete() const
{
	return IsSourceControlled() && IsCurrent();
}

bool FGitSourceControlState::IsUnknown() const
{
	return WorkingCopyState == EWorkingCopyState::Unknown;
}

bool FGitSourceControlState::IsModified() const
{
	// Warning: for Perforce, a checked-out file is locked for modification (whereas with Git all tracked files are checked-out),
	// so for a clean "check-in" (commit) checked-out files unmodified should be removed from the changeset (the index)
	// http://stackoverflow.com/questions/12357971/what-does-revert-unchanged-files-mean-in-perforce
	//
	// Thus, before check-in UE4 Editor call RevertUnchangedFiles() in PromptForCheckin() and CheckinFiles().
	//
	// So here we must take care to enumerate all states that need to be committed,
	// all other will be discarded :
	//  - Unknown
	//  - Unchanged
	//  - NotControlled
	//  - Ignored
	return WorkingCopyState == EWorkingCopyState::Added
		|| WorkingCopyState == EWorkingCopyState::Deleted
		|| WorkingCopyState == EWorkingCopyState::Modified
		|| WorkingCopyState == EWorkingCopyState::Renamed
		|| WorkingCopyState == EWorkingCopyState::Copied
		|| WorkingCopyState == EWorkingCopyState::Conflicted
		|| WorkingCopyState == EWorkingCopyState::Missing;
}


bool FGitSourceControlState::CanAdd() const
{
	return LastCommitSha.IsEmpty() || (LastCommitSpread & ECommitSpread::CloneUncommitted) != ECommitSpread::CloneUncommitted;
}

bool FGitSourceControlState::IsConflicted() const
{
	return WorkingCopyState == EWorkingCopyState::Conflicted;
}

bool FGitSourceControlState::CanRevert() const
{
	return CanCheckIn();
}

#undef LOCTEXT_NAMESPACE

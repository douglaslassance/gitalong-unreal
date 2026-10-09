// Copyright (c) 2014-2022 Sebastien Rombauts (sebastien.rombauts@gmail.com)
// Copyright (c) 2021-2026 Douglas Lassance
//
// Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
// or copy at http://opensource.org/licenses/MIT)

#include "GitSourceControlState.h"
#include "RevisionControlStyle/RevisionControlStyle.h"
#include "Textures/SlateIcon.h"

#define LOCTEXT_NAMESPACE "GitSourceControl.State"

bool FGitalongStatus::IsClaimedByMe() const
{
	// Same rule Gitalong applies to make a file writable
	return Spread == EGitalongSpread::MineUncommitted || Spread == EGitalongSpread::MineActiveBranch;
}

bool FGitalongStatus::IsBlocked() const
{
	// Same rule `gitalong claim` applies to refuse a claim
	return Spread != EGitalongSpread::None && !EnumHasAnyFlags(Spread, EGitalongSpread::MineUncommitted | EGitalongSpread::MineActiveBranch);
}

bool FGitalongStatus::IsBehind() const
{
	return IsBlocked() && EnumHasAnyFlags(Spread, EGitalongSpread::RemoteMatchingBranch);
}

bool FGitalongStatus::IsOnOtherBranch() const
{
	const EGitalongSpread OtherBranch = EGitalongSpread::MineOtherBranch | EGitalongSpread::RemoteOtherBranch | EGitalongSpread::TheirOtherBranch;
	return IsBlocked() && EnumHasAnyFlags(Spread, OtherBranch) && !EnumHasAnyFlags(Spread, ~OtherBranch);
}

FString FGitalongStatus::GetHolder() const
{
	FString Holder = Author.IsEmpty() ? User : Author;
	if (!Host.IsEmpty())
	{
		Holder = Holder.IsEmpty() ? Host : FString::Printf(TEXT("%s (%s)"), *Holder, *Host);
	}
	return Holder;
}

FString FGitalongStatus::GetBranch() const
{
	if (RemoteBranches.Num() > 0)
	{
		return RemoteBranches[0];
	}
	if (LocalBranches.Num() > 0)
	{
		return LocalBranches[0];
	}
	return FString();
}

int32 FGitSourceControlState::GetHistorySize() const
{
	return History.Num();
}

TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FGitSourceControlState::GetHistoryItem(int32 HistoryIndex) const
{
	check(History.IsValidIndex(HistoryIndex));
	return History[HistoryIndex];
}

TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FGitSourceControlState::FindHistoryRevision(int32 RevisionNumber) const
{
	for (const auto& Revision : History)
	{
		if (Revision->GetRevisionNumber() == RevisionNumber)
		{
			return Revision;
		}
	}

	return nullptr;
}

TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FGitSourceControlState::FindHistoryRevision(const FString& InRevision) const
{
	for (const auto& Revision : History)
	{
		if (Revision->GetRevision() == InRevision)
		{
			return Revision;
		}
	}

	return nullptr;
}

ISourceControlState::FResolveInfo FGitSourceControlState::GetResolveInfo() const
{
	return PendingResolveInfo;
}

FSlateIcon FGitSourceControlState::GetIcon() const
{
	const FName StyleSetName = FRevisionControlStyleManager::GetStyleSetName();
	if (IsConflicted())
	{
		return FSlateIcon(StyleSetName, "RevisionControl.Conflicted");
	}
	if (IsCheckedOutOther())
	{
		return FSlateIcon(StyleSetName, Gitalong.IsOnOtherBranch() ? "RevisionControl.CheckedOutByOtherUserOtherBranch" : "RevisionControl.CheckedOutByOtherUser");
	}
	if (!IsCurrent())
	{
		return FSlateIcon(StyleSetName, "RevisionControl.NotAtHeadRevision");
	}

	switch (WorkingCopyState)
	{
	case EWorkingCopyState::Added:
		return FSlateIcon(StyleSetName, "RevisionControl.OpenForAdd");
	case EWorkingCopyState::Renamed:
	case EWorkingCopyState::Copied:
		return FSlateIcon(StyleSetName, "RevisionControl.Branched");
	case EWorkingCopyState::Deleted: // Deleted & Missing files does not show in Content Browser
	case EWorkingCopyState::Missing:
		return FSlateIcon(StyleSetName, "RevisionControl.MarkedForDelete");
	case EWorkingCopyState::NotControlled:
		return FSlateIcon(StyleSetName, "RevisionControl.NotInDepot");
	case EWorkingCopyState::Modified:
		return FSlateIcon(StyleSetName, "RevisionControl.CheckedOut");
	default:
		break;
	}

	if (bUsesCheckout && Gitalong.IsClaimedByMe())
	{
		return FSlateIcon(StyleSetName, "RevisionControl.CheckedOut");
	}
	return FSlateIcon();
}

FText FGitSourceControlState::GetDisplayName() const
{
	if (IsConflicted())
	{
		return LOCTEXT("ContentsConflict", "Contents Conflict");
	}
	if (IsCheckedOutOther())
	{
		if (Gitalong.Sha.IsEmpty())
		{
			return FText::Format(LOCTEXT("ClaimedOther", "Claimed by {0}"), FText::FromString(Gitalong.GetHolder()));
		}
		return FText::Format(LOCTEXT("CommittedOther", "Changed by {0} in {1}"), FText::FromString(Gitalong.GetHolder()), FText::FromString(Gitalong.GetBranch()));
	}
	if (!IsCurrent())
	{
		return LOCTEXT("NotCurrent", "Not Current");
	}

	switch (WorkingCopyState)
	{
	case EWorkingCopyState::Unknown:
		return LOCTEXT("Unknown", "Unknown");
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
	case EWorkingCopyState::Ignored:
		return LOCTEXT("Ignored", "Ignored");
	case EWorkingCopyState::NotControlled:
		return LOCTEXT("NotControlled", "Not Under Revision Control");
	case EWorkingCopyState::Missing:
		return LOCTEXT("Missing", "Missing");
	default:
		break;
	}

	if (bUsesCheckout && Gitalong.IsClaimedByMe())
	{
		return Gitalong.Sha.IsEmpty() ? LOCTEXT("Claimed", "Claimed") : LOCTEXT("CommittedLocally", "Committed Locally");
	}
	return LOCTEXT("Unchanged", "Unchanged");
}

FText FGitSourceControlState::GetDisplayTooltip() const
{
	if (IsConflicted())
	{
		return LOCTEXT("ContentsConflict_Tooltip", "The contents of the item conflict with updates received from the repository.");
	}
	if (IsCheckedOutOther())
	{
		if (Gitalong.Sha.IsEmpty())
		{
			return FText::Format(LOCTEXT("ClaimedOther_Tooltip", "Claimed by {0} who has uncommitted changes."), FText::FromString(Gitalong.GetHolder()));
		}
		return FText::Format(LOCTEXT("CommittedOther_Tooltip", "Changed by {0} in commit {1} on branch {2}, which is missing from the active branch."), FText::FromString(Gitalong.GetHolder()), FText::FromString(Gitalong.Sha.Left(8)), FText::FromString(Gitalong.GetBranch()));
	}
	if (!IsCurrent())
	{
		return FText::Format(LOCTEXT("NotCurrent_Tooltip", "A newer version from commit {0} must be synced before editing."), FText::FromString(Gitalong.Sha.Left(8)));
	}

	switch (WorkingCopyState)
	{
	case EWorkingCopyState::Unknown:
		return LOCTEXT("Unknown_Tooltip", "Unknown revision control state");
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
	case EWorkingCopyState::Ignored:
		return LOCTEXT("Ignored_Tooltip", "Item is being ignored.");
	case EWorkingCopyState::NotControlled:
		return LOCTEXT("NotControlled_Tooltip", "Item is not under version control.");
	case EWorkingCopyState::Missing:
		return LOCTEXT("Missing_Tooltip", "Item is missing (e.g., you moved or deleted it without using Git). This also indicates that a directory is incomplete (a checkout or update was interrupted).");
	default:
		break;
	}

	if (bUsesCheckout && Gitalong.IsClaimedByMe())
	{
		if (Gitalong.Sha.IsEmpty())
		{
			return LOCTEXT("Claimed_Tooltip", "Claimed for editing by this clone");
		}
		return FText::Format(LOCTEXT("CommittedLocally_Tooltip", "Changed in local commit {0} that is not pushed yet"), FText::FromString(Gitalong.Sha.Left(8)));
	}
	return LOCTEXT("Pristine_Tooltip", "There are no modifications");
}

const FString& FGitSourceControlState::GetFilename() const
{
	return LocalFilename;
}

const FDateTime& FGitSourceControlState::GetTimeStamp() const
{
	return TimeStamp;
}

bool FGitSourceControlState::HasLocalChanges() const
{
	return WorkingCopyState == EWorkingCopyState::Added
		|| WorkingCopyState == EWorkingCopyState::Deleted
		|| WorkingCopyState == EWorkingCopyState::Missing
		|| WorkingCopyState == EWorkingCopyState::Modified
		|| WorkingCopyState == EWorkingCopyState::Renamed;
}

// Deleted and Missing assets cannot appear in the Content Browser, but the do in the Submit files to Source Control window!
bool FGitSourceControlState::CanCheckIn() const
{
	return HasLocalChanges() && IsCurrent() && !IsCheckedOutOther();
}

bool FGitSourceControlState::CanCheckout() const
{
	if (!bUsesCheckout)
	{
		return false; // Without shared claims, all tracked files in the working copy are always already checked-out
	}
	return IsSourceControlled() && !IsAdded() && !IsCheckedOut() && !Gitalong.IsBlocked();
}

bool FGitSourceControlState::IsCheckedOut() const
{
	if (!bUsesCheckout)
	{
		return IsSourceControlled(); // Without shared claims, all tracked files in the working copy are always checked-out
	}
	// Files changed outside of the editor count too, Gitalong publishes them on its next update
	return Gitalong.IsClaimedByMe()
		|| WorkingCopyState == EWorkingCopyState::Modified
		|| WorkingCopyState == EWorkingCopyState::Renamed
		|| WorkingCopyState == EWorkingCopyState::Copied
		|| WorkingCopyState == EWorkingCopyState::Conflicted;
}

bool FGitSourceControlState::IsCheckedOutOther(FString* Who) const
{
	const bool bCheckedOutOther = Gitalong.IsBlocked() && !Gitalong.IsBehind();
	if (bCheckedOutOther && Who != nullptr)
	{
		*Who = Gitalong.GetHolder();
	}
	return bCheckedOutOther;
}

bool FGitSourceControlState::IsCurrent() const
{
	return !Gitalong.IsBehind();
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
	return IsCurrent() && !IsCheckedOutOther();
}

bool FGitSourceControlState::CanDelete() const
{
	return !IsCheckedOutOther() && IsSourceControlled() && IsCurrent();
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
	// Thus, before check-in UE Editor call RevertUnchangedFiles() in PromptForCheckin() and CheckinFiles().
	//
	// So here we must take care to enumerate all states that need to be commited,
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
		|| WorkingCopyState == EWorkingCopyState::Missing
		|| WorkingCopyState == EWorkingCopyState::Conflicted;
}


bool FGitSourceControlState::CanAdd() const
{
	return WorkingCopyState == EWorkingCopyState::NotControlled;
}

bool FGitSourceControlState::IsConflicted() const
{
	return WorkingCopyState == EWorkingCopyState::Conflicted;
}

bool FGitSourceControlState::CanRevert() const
{
	// Reverting an unmodified claim releases it
	return IsModified() || (bUsesCheckout && Gitalong.Spread == EGitalongSpread::MineUncommitted);
}

TSharedPtr<ISourceControlRevision, ESPMode::ThreadSafe> FGitSourceControlState::GetCurrentRevision() const
{
	return nullptr;
}

#undef LOCTEXT_NAMESPACE

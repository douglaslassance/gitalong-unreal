// Copyright (c) 2014-2022 Sebastien Rombauts (sebastien.rombauts@gmail.com)
// Copyright (c) 2021-2026 Douglas Lassance
//
// Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
// or copy at http://opensource.org/licenses/MIT)

#pragma once

#include "CoreMinimal.h"
#include "ISourceControlState.h"
#include "ISourceControlRevision.h"
#include "GitSourceControlRevision.h"

namespace EWorkingCopyState
{
	enum Type
	{
		Unknown,
		Unchanged,
		// called "clean" in SVN, "Pristine" in Perforce
		Added,
		Deleted,
		Modified,
		Renamed,
		Copied,
		Missing,
		Conflicted,
		NotControlled,
		Ignored,
	};
}

/** Where the latest change to a file lives, in the bit order of the `gitalong status` spread string */
enum class EGitalongSpread : uint8
{
	None = 0,
	MineUncommitted = 1 << 0,
	MineActiveBranch = 1 << 1,
	MineOtherBranch = 1 << 2,
	RemoteMatchingBranch = 1 << 3,
	RemoteOtherBranch = 1 << 4,
	TheirOtherBranch = 1 << 5,
	TheirMatchingBranch = 1 << 6,
	TheirUncommitted = 1 << 7,
};
ENUM_CLASS_FLAGS(EGitalongSpread);

/** Latest change to a file as reported by `gitalong status --json` */
struct FGitalongStatus
{
	/** False until Gitalong has been queried for this file */
	bool bKnown = false;

	EGitalongSpread Spread = EGitalongSpread::None;

	/** Empty for uncommitted changes */
	FString Sha;

	/** Git identity of the commit author */
	FString Author;

	/** OS identity of whoever published the record */
	FString User;

	FString Host;

	TArray<FString> LocalBranches;

	TArray<FString> RemoteBranches;

	/** The change is this clone's alone, so the file is claimed (and writable) here */
	bool IsClaimedByMe() const;

	/** Somebody else holds a change that is not on this clone's active branch, so a claim would be blocked */
	bool IsBlocked() const;

	/** Blocked by a change already pushed to the remote branch matching the active one: a sync is required */
	bool IsBehind() const;

	/** Blocked by a change living on another branch than the active one */
	bool IsOnOtherBranch() const;

	/** Readable name of whoever holds the blocking change */
	FString GetHolder() const;

	/** First branch holding the change, remote branches first */
	FString GetBranch() const;
};

class FGitSourceControlState : public ISourceControlState
{
public:
	FGitSourceControlState(const FString& InLocalFilename, const bool bInUsesCheckout)
		: LocalFilename(InLocalFilename)
		, WorkingCopyState(EWorkingCopyState::Unknown)
		, bUsesCheckout(bInUsesCheckout)
		, TimeStamp(0)
	{
	}

	/** ISourceControlState interface */
	virtual int32 GetHistorySize() const override;
	virtual TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> GetHistoryItem(int32 HistoryIndex) const override;
	virtual TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FindHistoryRevision(int32 RevisionNumber) const override;
	virtual TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FindHistoryRevision(const FString& InRevision) const override;
	virtual FResolveInfo GetResolveInfo() const override;
	virtual TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> GetCurrentRevision() const override;
	virtual FSlateIcon GetIcon() const override;
	virtual FText GetDisplayName() const override;
	virtual FText GetDisplayTooltip() const override;
	virtual const FString& GetFilename() const override;
	virtual const FDateTime& GetTimeStamp() const override;
	virtual bool CanCheckIn() const override;
	virtual bool CanCheckout() const override;
	virtual bool IsCheckedOut() const override;
	virtual bool IsCheckedOutOther(FString* Who = nullptr) const override;
	virtual bool IsCheckedOutInOtherBranch(const FString& CurrentBranch = FString()) const override { return false; }
	virtual bool IsModifiedInOtherBranch(const FString& CurrentBranch = FString()) const override { return false; }
	virtual bool IsCheckedOutOrModifiedInOtherBranch(const FString& CurrentBranch = FString()) const override { return IsCheckedOutInOtherBranch(CurrentBranch) || IsModifiedInOtherBranch(CurrentBranch); }
	virtual TArray<FString> GetCheckedOutBranches() const override { return TArray<FString>(); }
	virtual FString GetOtherUserBranchCheckedOuts() const override { return FString(); }
	virtual bool GetOtherBranchHeadModification(FString& HeadBranchOut, FString& ActionOut, int32& HeadChangeListOut) const override { return false; }
	virtual bool IsCurrent() const override;
	virtual bool IsSourceControlled() const override;
	virtual bool IsAdded() const override;
	virtual bool IsDeleted() const override;
	virtual bool IsIgnored() const override;
	virtual bool CanEdit() const override;
	virtual bool IsUnknown() const override;
	virtual bool IsModified() const override;
	virtual bool CanAdd() const override;
	virtual bool CanDelete() const override;
	virtual bool IsConflicted() const override;
	virtual bool CanRevert() const override;

	/** The working copy holds changes that Git can commit */
	bool HasLocalChanges() const;

public:
	/** History of the item, if any */
	TGitSourceControlHistory History;

	/** Filename on disk */
	FString LocalFilename;

	/** Pending rev info with which a file must be resolved, invalid if no resolve pending */
	FResolveInfo PendingResolveInfo;

	/** State of the working copy */
	EWorkingCopyState::Type WorkingCopyState;

	/** Latest change to the file across the team, as reported by Gitalong */
	FGitalongStatus Gitalong;

	/** Tells if files must be claimed (checked out) before being edited */
	bool bUsesCheckout;

	/** The timestamp of the last update */
	FDateTime TimeStamp;
};
